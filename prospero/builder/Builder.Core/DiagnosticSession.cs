using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Threading.Channels;

namespace Simpsons.Builder;

/// <summary>
/// One local JSON-lines log for the whole application session, including
/// bootstrap and input inspection. File writes and periodic flushes run on a
/// dedicated asynchronous consumer; UI producers never wait for disk writes.
/// The UI/clipboard excerpt is bounded, while the file retains every event.
/// </summary>
public sealed class DiagnosticSession : IDisposable, IAsyncDisposable
{
    private sealed record Item(string? Line = null, TaskCompletionSource<long?>? Barrier = null);
    private static readonly JsonSerializerOptions Compact = new(JsonFiles.Options) { WriteIndented = false };
    private readonly Channel<Item> queue = Channel.CreateUnbounded<Item>(new UnboundedChannelOptions { SingleReader = true, AllowSynchronousContinuations = false });
    private readonly object gate = new();
    private readonly Queue<string> recent = new();
    private readonly Stopwatch clock = Stopwatch.StartNew();
    private readonly Task consumer;
    private long sequence;
    private long lastFlushedBytes = -1;
    private int recentCharacters;
    private bool completed;
    private string? storageProblem;
    public string? LogPath { get; }
    public string SessionId { get; } = Guid.NewGuid().ToString("N");
    public string? StorageProblem => Volatile.Read(ref storageProblem);
    public long LastSequence => Interlocked.Read(ref sequence);
    public string LocationDescription => LogPath is null ? "File logging is unavailable; use Copy diagnostics to save the recent session details."
        : LogPath + (StorageProblem is null ? "" : "\nLog writing stopped: " + StorageProblem);

    private DiagnosticSession(StreamWriter? writer, string? path, string? initialProblem)
    {
        LogPath = path; storageProblem = writer is null ? initialProblem : null;
        consumer = Task.Run(() => Consume(writer));
        Record("session.start", new
        {
            schemaVersion = 1, sessionId = SessionId, logPath = path,
            builderVersion = Assembly.GetEntryAssembly()?.GetName().Version?.ToString(),
            os = RuntimeInformation.OSDescription, osVersion = Environment.OSVersion.ToString(),
            osArchitecture = RuntimeInformation.OSArchitecture.ToString(),
            processArchitecture = RuntimeInformation.ProcessArchitecture.ToString(),
            is64BitProcess = Environment.Is64BitProcess, processId = Environment.ProcessId,
            framework = RuntimeInformation.FrameworkDescription, clrVersion = Environment.Version.ToString(),
            processors = Environment.ProcessorCount, processPath = Environment.ProcessPath,
            workingDirectory = Environment.CurrentDirectory, runtimeIncluded = true,
            contentPolicy = "Local file names, paths, sizes, settings and integrity hashes only; no game/DLL file contents, network requests or environment-variable dumps."
        });
        if (initialProblem is not null) Record("logging.storage-fallback", new { details = initialProblem, selectedPath = path }, "warning");
    }

    public static DiagnosticSession Start(string? primaryDirectory = null, string? fallbackDirectory = null)
    {
        primaryDirectory ??= Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "TheSimpsonsGame-PS5-Builder", "logs");
        fallbackDirectory ??= Path.Combine(Path.GetTempPath(), "TheSimpsonsGame-PS5-Builder", "logs");
        string filename = "builder-" + DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss-fff") + "-" + Guid.NewGuid().ToString("N")[..8] + ".log";
        var failures = new List<string>();
        foreach (string directory in new[] { primaryDirectory, fallbackDirectory }.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            try
            {
                string root = Path.GetFullPath(directory); Directory.CreateDirectory(root);
                string path = Path.Combine(root, filename);
                var stream = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read | FileShare.Delete,
                    64 * 1024, FileOptions.Asynchronous | FileOptions.SequentialScan);
                var writer = new StreamWriter(stream, new UTF8Encoding(false), 64 * 1024, leaveOpen: false);
                return new(writer, path, failures.Count == 0 ? null : string.Join("\n", failures));
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException or NotSupportedException)
            { failures.Add(directory + ": " + e); }
        }
        return new(null, null, string.Join("\n", failures));
    }

    public static string FormatEvent(string name, object? data = null) => data is null ? name : name + " " + JsonSerializer.Serialize(data, Compact);
    public void Write(string message) => Record("build", message);
    public void Error(string name, Exception error, object? context = null) => Record(name,
        new { type = error.GetType().FullName, error.Message, error.HResult, exception = error.ToString(), context }, "error");

    public void Record(string name, object? data = null, string level = "info")
    {
        lock (gate)
        {
            if (completed) return;
            string line = JsonSerializer.Serialize(new
            {
                utc = DateTimeOffset.UtcNow, elapsedMs = clock.Elapsed.TotalMilliseconds,
                sequence = ++sequence, threadId = Environment.CurrentManagedThreadId,
                level, @event = name, data
            }, Compact);
            string excerpt = line.Length > 64 * 1024 ? line[..(64 * 1024)] + " [excerpt truncated; full event is in the log file]" : line;
            recent.Enqueue(excerpt); recentCharacters += excerpt.Length;
            while (recent.Count > 300 || (recentCharacters > 256 * 1024 && recent.Count > 1)) recentCharacters -= recent.Dequeue().Length;
            queue.Writer.TryWrite(new(Line: line));
        }
    }

    public string RecentText(int maximumLines = 300)
    {
        lock (gate) return string.Join(Environment.NewLine, recent.TakeLast(Math.Clamp(maximumLines, 1, 300)));
    }

    public Task FlushAsync() => FlushPositionAsync();

    private Task<long?> FlushPositionAsync()
    {
        lock (gate)
        {
            if (completed) return CompletedPosition();
            var barrier = new TaskCompletionSource<long?>(TaskCreationOptions.RunContinuationsAsynchronously);
            queue.Writer.TryWrite(new(Barrier: barrier));
            return barrier.Task;
        }
    }

    private async Task<long?> CompletedPosition()
    {
        await consumer.ConfigureAwait(false);
        long bytes = Volatile.Read(ref lastFlushedBytes);
        return bytes < 0 ? null : bytes;
    }

    public async Task<string> DiagnosticTextAsync()
    {
        await FlushAsync().ConfigureAwait(false);
        return "The Simpsons Game PS5 Builder diagnostics\nSession: " + SessionId + "\nLog: " + LocationDescription +
            "\n\nRecent events (bounded excerpt; export the log for the complete session):\n" + RecentText();
    }

    public async Task ExportAsync(string destination, CancellationToken token = default)
    {
        string target = Path.GetFullPath(destination);
        if (LogPath is not null && target.Equals(Path.GetFullPath(LogPath), StringComparison.OrdinalIgnoreCase))
            throw new BuilderException("Choose a different name for the exported log.");
        long? checkpoint = await FlushPositionAsync().ConfigureAwait(false);
        await using var output = new FileStream(target, FileMode.CreateNew, FileAccess.Write, FileShare.None, 64 * 1024, FileOptions.Asynchronous);
        if (LogPath is not null && checkpoint.HasValue)
        {
            await using var input = new FileStream(LogPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete, 64 * 1024, FileOptions.Asynchronous);
            // The consumer captured this byte boundary immediately after a
            // flush, before it started the next record. Sampling File.Length
            // here could race a partial write of that next JSON/UTF-8 line.
            long remaining = checkpoint.Value;
            byte[] buffer = new byte[64 * 1024];
            while (remaining > 0)
            {
                int count = await input.ReadAsync(buffer.AsMemory(0, (int)Math.Min(remaining, buffer.Length)), token).ConfigureAwait(false);
                if (count == 0) throw new IOException("The session log ended while exporting.");
                await output.WriteAsync(buffer.AsMemory(0, count), token).ConfigureAwait(false); remaining -= count;
            }
        }
        else
        {
            byte[] text = Encoding.UTF8.GetBytes("PARTIAL DIAGNOSTICS: file logging was unavailable or stopped.\n" + await DiagnosticTextAsync().ConfigureAwait(false));
            await output.WriteAsync(text, token).ConfigureAwait(false);
        }
        await output.FlushAsync(token).ConfigureAwait(false);
    }

    private async Task Consume(StreamWriter? writer)
    {
        long lastFlush = 0;
        async Task Flush()
        {
            if (writer is null) return;
            try
            {
                await writer.FlushAsync().ConfigureAwait(false);
                Volatile.Write(ref lastFlushedBytes, writer.BaseStream.Position);
                lastFlush = clock.ElapsedMilliseconds;
            }
            catch (Exception e) { Volatile.Write(ref storageProblem, e.ToString()); Volatile.Write(ref lastFlushedBytes, -1); try { writer.Dispose(); } catch { } writer = null; }
        }
        try
        {
            while (true)
            {
                Task<bool> ready = queue.Reader.WaitToReadAsync().AsTask();
                if (await Task.WhenAny(ready, Task.Delay(500)).ConfigureAwait(false) != ready)
                { await Flush().ConfigureAwait(false); if (!await ready.ConfigureAwait(false)) break; }
                else if (!await ready.ConfigureAwait(false)) break;
                int batch = 0;
                while (batch++ < 256 && queue.Reader.TryRead(out Item? item))
                {
                    if (item.Line is not null && writer is not null)
                    {
                        try { await writer.WriteLineAsync(item.Line).ConfigureAwait(false); }
                        catch (Exception e) { Volatile.Write(ref storageProblem, e.ToString()); Volatile.Write(ref lastFlushedBytes, -1); try { writer.Dispose(); } catch { } writer = null; }
                    }
                    if (item.Barrier is not null)
                    {
                        await Flush().ConfigureAwait(false);
                        item.Barrier.TrySetResult(writer is null ? null : Volatile.Read(ref lastFlushedBytes));
                    }
                }
                if (batch >= 256 || clock.ElapsedMilliseconds - lastFlush >= 500) await Flush().ConfigureAwait(false);
            }
            await Flush().ConfigureAwait(false);
        }
        finally
        {
            if (writer is not null) { try { await writer.DisposeAsync().ConfigureAwait(false); } catch (Exception e) { Volatile.Write(ref storageProblem, e.ToString()); } }
            while (queue.Reader.TryRead(out Item? item)) item.Barrier?.TrySetResult(null);
        }
    }

    public async ValueTask DisposeAsync()
    {
        lock (gate)
        {
            if (!completed) { Record("session.end"); completed = true; queue.Writer.TryComplete(); }
        }
        await consumer.ConfigureAwait(false);
    }
    public void Dispose() => DisposeAsync().AsTask().GetAwaiter().GetResult();
}

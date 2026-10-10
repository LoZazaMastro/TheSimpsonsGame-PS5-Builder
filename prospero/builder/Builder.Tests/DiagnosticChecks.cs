using System.Text;
using System.Text.Json;

namespace Simpsons.Builder.Tests;

internal static class DiagnosticChecks
{
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static string[] Messages(string path) => File.ReadAllLines(path).Select(line =>
    {
        using JsonDocument document = JsonDocument.Parse(line);
        JsonElement root = document.RootElement;
        return root.GetProperty("event").GetString() + " " + (root.GetProperty("data").ValueKind == JsonValueKind.String
            ? root.GetProperty("data").GetString() : root.GetProperty("data").GetRawText());
    }).ToArray();
    private sealed class Progress(Action<BuildProgress> action) : IProgress<BuildProgress> { public void Report(BuildProgress progress) => action(progress); }

    internal static async Task RunAllAsync(Func<string, Func<Task>, Task> run, BundlePackage bundle,
        Capabilities capabilities, Func<string, string> newPath)
    {
        await run("diagnostic session starts before work; inaccessible primary falls back and memory-only is explicit", async () =>
        {
            string blocked = newPath("log-path-is-file"); File.WriteAllText(blocked, "KEEP");
            await using (DiagnosticSession log = DiagnosticSession.Start(blocked, newPath("log-fallback")))
            {
                Check(log.LogPath is not null, "Fallback log was not created.");
                log.Error("bootstrap.failed", new BuilderException("Synthetic startup failure", new IOException("inner startup cause")));
                await log.FlushAsync();
                string text = string.Join('\n', Messages(log.LogPath!));
                Check(text.Contains("session.start") && text.Contains("logging.storage-fallback") && text.Contains("inner startup cause"), "Startup or fallback diagnostic is missing.");
                Check(text.Contains("framework") && text.Contains("processArchitecture") && text.Contains("is64BitProcess"), "Environment identity missing.");
            }
            await using (DiagnosticSession log = DiagnosticSession.Start(blocked, blocked))
            {
                Check(log.LogPath is null && log.StorageProblem is not null, "Both failed log roots were hidden.");
                log.Record("test.memory-fallback", new { result = "recorded" });
                string export = newPath("partial-diagnostics.log"); await log.ExportAsync(export);
                Check(File.ReadAllText(export).StartsWith("PARTIAL DIAGNOSTICS") && File.ReadAllText(export).Contains("test.memory-fallback"), "Memory-only export pretended to be complete or lost events.");
            }
            Check(File.ReadAllText(blocked) == "KEEP", "Log bootstrap replaced an existing file.");
        });
        await run("async diagnostic writer preserves concurrent event order, bounded excerpts, flush and export", async () =>
        {
            await using DiagnosticSession log = DiagnosticSession.Start(newPath("parallel-logs"));
            await Task.WhenAll(Enumerable.Range(0, 4).Select(worker => Task.Run(() =>
            {
                for (int item = 0; item < 1000; item++) log.Record("test.parallel", new { worker, item, escaped = "one\ntwo\rthree" });
            })));
            await log.FlushAsync();
            string[] lines = File.ReadAllLines(log.LogPath!); Check(lines.Length == 4001, "A concurrent event was dropped or split across lines.");
            long previous = 0; double previousTime = -1;
            foreach (string line in lines)
            {
                using JsonDocument json = JsonDocument.Parse(line); JsonElement root = json.RootElement;
                long sequence = root.GetProperty("sequence").GetInt64(); double elapsed = root.GetProperty("elapsedMs").GetDouble();
                Check(sequence == previous + 1 && elapsed >= previousTime && root.GetProperty("utc").GetDateTimeOffset() != default, "Sequence/timestamp ordering was lost.");
                previous = sequence; previousTime = elapsed;
            }
            Check(log.RecentText().Split(Environment.NewLine).Length <= 300, "UI excerpt was not bounded.");
            string export = newPath("export.log"); await log.ExportAsync(export);
            Check(File.ReadAllBytes(export).SequenceEqual(File.ReadAllBytes(log.LogPath!)), "Export did not contain the complete flushed session.");
            try { await log.ExportAsync(export); throw new Exception("An existing export was overwritten."); } catch (IOException) { }
            Check(File.ReadAllBytes(export).SequenceEqual(File.ReadAllBytes(log.LogPath!)), "Refused export changed the existing file.");
            Check((await log.DiagnosticTextAsync()).Contains("bounded excerpt"), "Clipboard diagnostics do not identify their scope.");

            // Each large record spans multiple writer buffers. Export while a
            // producer is active, then require an exact prefix of complete
            // JSON records, never a transient File.Length inside the next one.
            string large = string.Concat(Enumerable.Repeat("À🙂-synthetic-diagnostic-", 8000));
            var producing = Task.Run(() =>
            {
                for (int index = 0; index < 32; index++) { log.Record("test.large-concurrent", new { index, large }); Thread.Yield(); }
            });
            Task<string>[] exporting = Enumerable.Range(0, 16).Select(async index =>
            {
                string path = newPath("concurrent-export-" + index + ".log"); await log.ExportAsync(path); return path;
            }).ToArray();
            await producing; string[] exports = await Task.WhenAll(exporting);
            foreach (string path in exports)
            {
                byte[] bytes = File.ReadAllBytes(path);
                Check(bytes.Length > 0 && bytes[^1] == '\n', "Concurrent export ended inside a log record.");
                string text = new UTF8Encoding(false, true).GetString(bytes);
                long last = 0;
                foreach (string line in text.Split('\n', StringSplitOptions.RemoveEmptyEntries))
                {
                    using JsonDocument json = JsonDocument.Parse(line);
                    Check(json.RootElement.GetProperty("sequence").GetInt64() == ++last, "Concurrent export is not a complete event prefix.");
                }
            }
        });
        await run("successful build log contains source probes, exact settings, every copy/hash verification and publication", async () =>
        {
            var files = Fixtures.GameFiles(); const string marker = "PRIVATE-FILE-CONTENT-NOT-TO-BE-LOGGED-712efb";
            files["archive.bin"] = Encoding.UTF8.GetBytes(marker);
            IsoFixture fixture = Fixtures.Iso(newPath("logged.iso"), 0x0FD90000, files);
            await using DiagnosticSession log = DiagnosticSession.Start(newPath("success-logs"));
            BuildResult result = await new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), newPath("logged-output"), capabilities.Defaults(losslessAvailable: false)), null, log.Write, CancellationToken.None);
            await log.FlushAsync(); string[] messages = Messages(log.LogPath!); string text = string.Join('\n', messages);
            foreach (string expected in new[] { "build.request", "payload.manifest", "source.partition.probe", "source.partition.selected", "rootByteOffset", "source.xex.header", "source.identity",
                         "space.check", "staging.created", "phase.start", "phase.end", "configuration.written", "output.published", "build.finished", "\"outcome\":\"success\"",
                         "\"frame_rate\":60", "\"menu_frame_rate\":30", "\"frame_gen\":false", "\"frame_gen_target_rate\":120", "\"frame_gen_multiplier\":2", "\"prospero_output_hz\":120",
                         "\"vsync\":false", "\"present_fsr_sharpness_reduction\":0.0", "\"present_effect\":\"fsr\"", "\"swap_post_effect\":\"fxaa\"", "\"eye_shading\":\"clean\"" })
                Check(text.Contains(expected, StringComparison.Ordinal), "Missing successful diagnostic: " + expected);
            using JsonDocument info = JsonDocument.Parse(File.ReadAllText(Path.Combine(result.OutputDirectory, "build-info.json")));
            foreach (JsonProperty file in info.RootElement.GetProperty("files").EnumerateObject())
                Check(messages.Any(line => line.Contains("file.verify.complete") && line.Contains(file.Name) && line.Contains(file.Value.GetString()!)), "Read-back record missing for " + file.Name);
            Check(messages.Count(line => line.Contains("file.copy.complete")) == files.Count, "Game file copy records are incomplete.");
            Check(!text.Contains(marker, StringComparison.Ordinal), "Game file contents entered the log.");
        });
        await run("failed build logs full nested exception stack and terminal outcome before staging", async () =>
        {
            IsoFixture fixture = Fixtures.Iso(newPath("error-log.iso"));
            await using DiagnosticSession log = DiagnosticSession.Start(newPath("error-logs"));
            try
            {
                await new BuildPipeline(bundle, _ => throw new IOException("synthetic disk failure", new InvalidOperationException("nested cause sentinel")))
                    .BuildAsync(new(new(InputKind.Iso, fixture.Path), newPath("error-output"), capabilities.Defaults(losslessAvailable: false)), null, log.Write, CancellationToken.None);
                throw new Exception("Injected failure was ignored.");
            }
            catch (BuilderException) { }
            await log.FlushAsync(); string text = string.Join('\n', Messages(log.LogPath!));
            foreach (string expected in new[] { "build.io-error", "nested cause sentinel", "InvalidOperationException", "IOException", "BuildPipeline", "cleanup.not-needed", "\"outcome\":\"failed\"" })
                Check(text.Contains(expected), "Nested error diagnostic missing: " + expected);
        });
        await run("failed read-back log records expected/actual hashes, affected file and completed cleanup", async () =>
        {
            IsoFixture fixture = Fixtures.Iso(newPath("corrupt-log.iso")); string output = newPath("corrupt-log-output"); bool changed = false;
            var progress = new Progress(p =>
            {
                if (p.Phase != "Verifying" || changed) return; changed = true;
                string stage = Directory.GetDirectories(output, ".*.building-*").Single();
                Fixtures.WriteAt(Path.Combine(stage, "gamedata", "archive.bin"), 0, [0xBE]);
            });
            await using DiagnosticSession log = DiagnosticSession.Start(newPath("corruption-logs"));
            try
            {
                await new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), output, capabilities.Defaults(losslessAvailable: false)), progress, log.Write, CancellationToken.None);
                throw new Exception("Corruption was ignored.");
            }
            catch (BuilderException) { }
            await log.FlushAsync(); string text = string.Join('\n', Messages(log.LogPath!));
            foreach (string expected in new[] { "file.verify.failed", "expectedSha256", "actualSha256", "gamedata/archive.bin", "cleanup.start", "cleanup.complete", "\"outcome\":\"failed\"" })
                Check(text.Contains(expected), "Corruption diagnostic missing: " + expected);
            Check(!Directory.EnumerateFileSystemEntries(output).Any(), "Failed output was retained.");
        });
        await run("cancelled build logs both pre-cancel and in-copy cancellation, stage cleanup and final outcome", async () =>
        {
            foreach (bool before in new[] { true, false })
            {
                IsoFixture fixture = Fixtures.Iso(newPath("cancel-log.iso")); string output = newPath("cancel-log-output");
                await using DiagnosticSession log = DiagnosticSession.Start(newPath("cancel-logs"));
                using var cancellation = new CancellationTokenSource(); if (before) cancellation.Cancel();
                var progress = new Progress(p => { if (p.Phase == "Extracting") cancellation.Cancel(); });
                try
                {
                    await new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), output, capabilities.Defaults(losslessAvailable: false)), progress, log.Write, cancellation.Token);
                    throw new Exception("Cancellation was ignored.");
                }
                catch (OperationCanceledException) { }
                await log.FlushAsync(); string text = string.Join('\n', Messages(log.LogPath!));
                Check(text.Contains("build.cancelled") && text.Contains("\"outcome\":\"cancelled\"") && text.Contains(before ? "cleanup.not-needed" : "cleanup.complete"), "Cancellation log was incomplete.");
                Check(!Directory.Exists(output) || !Directory.EnumerateFileSystemEntries(output).Any(), "Cancelled output was retained.");
            }
        });
    }
}

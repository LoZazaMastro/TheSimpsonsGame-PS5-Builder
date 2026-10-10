using System.Buffers;
using System.Diagnostics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Simpsons.Builder;

public sealed class BuildPipeline(BundlePackage bundle, Func<string, long>? availableSpace = null)
{
    private readonly Func<string, long> freeSpace = availableSpace ?? GetAvailableSpace;
    private const long ReserveBytes = 256L * 1024 * 1024;

    public Task<BuildResult> BuildAsync(BuildRequest request, IProgress<BuildProgress>? progress,
        Action<string>? log, CancellationToken token) => Task.Run(() => Build(request, progress, log, token));

    private BuildResult Build(BuildRequest request, IProgress<BuildProgress>? progress, Action<string>? log, CancellationToken token)
    {
        var clock = Stopwatch.StartNew();
        string? stage = null;
        bool committed = false;
        string phaseName = "Starting", outcome = "failed";
        string? activeFile = null;
        long phaseStart = 0;
        void WriteLog(string message) => log?.Invoke($"elapsedMs={clock.ElapsedMilliseconds} " + message);
        void Event(string name, object? data = null) { if (log is not null) WriteLog(DiagnosticSession.FormatEvent(name, data)); }
        void Phase(string name)
        {
            Event("phase.end", new { name = phaseName, durationMs = clock.ElapsedMilliseconds - phaseStart });
            phaseName = name; phaseStart = clock.ElapsedMilliseconds; activeFile = null;
            Event("phase.start", new { name });
        }
        try
        {
            Event("build.request", new { request.Input, request.OutputParent, request.LosslessDllPath, settings = request.Settings,
                lockedSettings = bundle.Capabilities.LockedSettings, gameOptions = request.PatchOptions ?? new GamePatchOptions() });
            Event("payload.manifest", bundle.Manifest);
            token.ThrowIfCancellationRequested();
            Phase("Validate input and options");
            bundle.Validate();
            bundle.Capabilities.ValidateValues(request.Settings);
            using OptionalLibrary? lossless = string.IsNullOrWhiteSpace(request.LosslessDllPath) ? null : OptionalLibrary.Open(request.LosslessDllPath);
            Event("optional-library.presence", new { selected = lossless is not null, path = lossless?.FullPath, bytes = lossless?.Length,
                compatibilityCheck = "none in builder", destination = OptionalLibrary.Destination });
            bundle.Capabilities.ValidateOptionalFileChoices(request.Settings, lossless is not null);
            string parent = Path.GetFullPath(request.OutputParent);
            SafePaths.NoReparsePoints(parent);
            string destination = Path.Combine(parent, bundle.Manifest.TitleId);
            if (Path.GetFileName(Path.TrimEndingDirectorySeparator(parent)).Equals(bundle.Manifest.TitleId, StringComparison.OrdinalIgnoreCase))
                throw new BuilderException("Choose the folder that will contain PPSA74207, rather than PPSA74207 itself.");
            if (File.Exists(destination) || Directory.Exists(destination))
                throw new BuilderException("PPSA74207 already exists in this location. Choose another output folder to keep your existing game and saves intact.");
            if (request.Input.Kind == InputKind.GameDataFolder && SafePaths.IsWithin(parent, request.Input.Path))
                throw new BuilderException("Choose an output folder outside your source game data folder.");
            WriteLog($"Starting builder {bundle.Manifest.BuildVersion}; input kind={request.Input.Kind}; input={request.Input.Path}; output={destination}");
            Phase("Inspect source");
            using GameSource source = GameSource.Open(request.Input, bundle.Manifest.XboxTitleId, token, progress, WriteLog);
            int sourceFileCount = source.FileCount;
            WriteLog($"Source verified: title={source.Identity.TitleId}; media={source.Identity.MediaId}; version={source.Identity.Version}; files={source.FileCount}; bytes={source.TotalBytes}; partition=0x{source.PartitionOffset:x}");
            long needed = checked(source.TotalBytes + bundle.Manifest.TotalBytes + (lossless?.Length ?? 0) + ReserveBytes);
            string spacePath = parent;
            while (!Directory.Exists(spacePath))
                spacePath = Path.GetDirectoryName(spacePath) ?? throw new BuilderException("The output drive is not available.");
            long available = freeSpace(spacePath);
            Event("space.check", new { path = spacePath, requiredBytes = needed, availableBytes = available, reserveBytes = ReserveBytes,
                gameBytes = source.TotalBytes, payloadBytes = bundle.Manifest.TotalBytes, optionalLibraryBytes = lossless?.Length ?? 0 });
            if (available < needed)
                throw new BuilderException($"There is not enough free space on the output drive. This build needs at least {Format.Bytes(needed)}; {Format.Bytes(available)} is available. Choose another drive or free up space.");
            WriteLog($"Space check: required={needed}; available={available}");
            token.ThrowIfCancellationRequested();
            Directory.CreateDirectory(parent);
            SafePaths.NoReparsePoints(parent);
            stage = Path.Combine(parent, "." + bundle.Manifest.TitleId + ".building-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(stage);
            Event("staging.created", new { stage, destination, sameParentPublication = true });
            foreach (string directory in new[] { "logs", "cache", "userdata", "gamedata" }) Directory.CreateDirectory(Path.Combine(stage, directory));
            var checksums = new Dictionary<string, string>(StringComparer.Ordinal);
            progress?.Report(new("Preparing", "Preparing the PS5 title…", 7));
            Phase("Extract PS5 payload");
            ExtractPayload(stage, checksums, token, WriteLog);
            bundle.ValidateTitleMetadata(stage);
            WriteLog("Bundled PS5 files verified.");
            if (lossless is not null)
            {
                Phase("Copy optional library"); activeFile = OptionalLibrary.Destination;
                long fileStart = clock.ElapsedMilliseconds;
                Event("file.copy.start", new { file = activeFile, bytes = lossless.Length, sourcePath = lossless.FullPath });
                progress?.Report(new("Preparing", "Copying your Lossless.dll file…", 9, OptionalLibrary.Destination));
                string libraryOutput = SafePaths.Under(stage, OptionalLibrary.Destination);
                Directory.CreateDirectory(Path.GetDirectoryName(libraryOutput)!);
                using var target = new FileStream(libraryOutput, FileMode.CreateNew, FileAccess.Write, FileShare.None, 1024 * 1024, FileOptions.SequentialScan);
                long libraryCopied = 0;
                checksums.Add(OptionalLibrary.Destination, CopyChecked(lossless.Stream, target, lossless.Length, token, bytes =>
                {
                    libraryCopied += bytes;
                    progress?.Report(new("Preparing", "Copying your Lossless.dll file…", 9,
                        OptionalLibrary.Destination, libraryCopied, lossless.Length));
                }));
                target.Flush(flushToDisk: true);
                Event("file.copy.complete", new { file = activeFile, bytes = libraryCopied, sha256 = checksums[OptionalLibrary.Destination], durationMs = clock.ElapsedMilliseconds - fileStart });
                WriteLog($"User-provided Lossless.dll copied without execution or compatibility gating: bytes={lossless.Length}");
            }

            long copied = 0;
            int filesDone = 0;
            long lastReport = -100;
            string phase = request.Input.Kind == InputKind.Iso ? "Extracting" : "Copying";
            Phase(phase + " game data");
            foreach (GameEntry entry in source.Entries)
            {
                token.ThrowIfCancellationRequested();
                string relative = "gamedata/" + entry.RelativePath;
                activeFile = relative;
                string output = SafePaths.Under(stage, relative);
                if (entry.IsDirectory) { Directory.CreateDirectory(output); continue; }
                Directory.CreateDirectory(Path.GetDirectoryName(output)!);
                long fileStart = clock.ElapsedMilliseconds;
                Event("file.copy.start", new { file = relative, bytes = entry.Length, sourceOffset = entry.Offset, sourcePath = entry.SourcePath });
                using Stream input = source.OpenFile(entry);
                using (var target = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None, 1024 * 1024, FileOptions.SequentialScan))
                {
                    checksums.Add(relative, CopyChecked(input, target, entry.Length, token, bytes =>
                    {
                        copied += bytes;
                        if (clock.ElapsedMilliseconds - lastReport >= 100)
                        {
                            lastReport = clock.ElapsedMilliseconds;
                            progress?.Report(new(phase, "Preparing your game data…", 10 + 64.0 * copied / Math.Max(1, source.TotalBytes),
                                entry.RelativePath, copied, source.TotalBytes, filesDone, source.FileCount));
                        }
                    }));
                    target.Flush(flushToDisk: true);
                }
                filesDone++;
                Event("file.copy.complete", new { file = relative, bytes = entry.Length, sha256 = checksums[relative], durationMs = clock.ElapsedMilliseconds - fileStart,
                    filesDone, filesTotal = sourceFileCount, bytesDone = copied, bytesTotal = source.TotalBytes });
            }
            WriteLog($"Game data copied: files={filesDone}; bytes={copied}");
            string config = bundle.Capabilities.WriteToml(request.Settings);
            Phase("Write configuration");
            string configPath = Path.Combine(stage, "userdata", "simpsons.toml");
            File.WriteAllText(configPath, config, new UTF8Encoding(false));
            checksums.Add("userdata/simpsons.toml", HashFile(configPath, token));
            Event("configuration.written", new { path = "userdata/simpsons.toml", bytes = new FileInfo(configPath).Length, sha256 = checksums["userdata/simpsons.toml"], settings = request.Settings, lockedSettings = bundle.Capabilities.LockedSettings });
            token.ThrowIfCancellationRequested();

            // Read back every produced file before publishing the final folder.
            // This catches incomplete writes and errors in the copy path.
            long verified = 0;
            long verifyTotal = checked(source.TotalBytes + bundle.Manifest.TotalBytes + (lossless?.Length ?? 0) + new FileInfo(configPath).Length);
            int verifyCount = 0;
            progress?.Report(new("Verifying", "Checking the files written to your output drive…", 75));
            Phase("Read-back verification");
            foreach ((string relative, string expected) in checksums)
            {
                token.ThrowIfCancellationRequested();
                string path = SafePaths.Under(stage, relative);
                activeFile = relative; long verifyStart = clock.ElapsedMilliseconds; long fileBytesBefore = verified;
                Event("file.verify.start", new { file = relative, expectedSha256 = expected });
                string actual = HashFile(path, token, bytes =>
                {
                    verified += bytes;
                    if (clock.ElapsedMilliseconds - lastReport >= 100)
                    {
                        lastReport = clock.ElapsedMilliseconds;
                        progress?.Report(new("Verifying", "Checking every completed file…", 75 + 22.0 * verified / Math.Max(1, verifyTotal),
                            relative, verified, verifyTotal, verifyCount, checksums.Count));
                    }
                });
                if (!actual.Equals(expected, StringComparison.OrdinalIgnoreCase))
                {
                    Event("file.verify.failed", new { file = relative, expectedSha256 = expected, actualSha256 = actual, bytes = verified - fileBytesBefore });
                    throw new BuilderException("A written file failed verification: " + relative + ". Try another output drive.");
                }
                verifyCount++;
                Event("file.verify.complete", new { file = relative, bytes = verified - fileBytesBefore, sha256 = actual, durationMs = clock.ElapsedMilliseconds - verifyStart,
                    filesDone = verifyCount, filesTotal = checksums.Count });
            }
            string xexHash = checksums["gamedata/default.xex"];
            WriteLog($"Read-back verification complete: files={verifyCount}; default.xex SHA256={xexHash}");
            GamePatchOptions patchOptions = request.PatchOptions ?? new GamePatchOptions();
            if (patchOptions.SkipIntroLogos || patchOptions.StartEpisode is not null)
            {
                Phase("Reversible game options");
                progress?.Report(new("Customizing", "Applying your game options…", 98));
                GamePatchResult changes = GamePatches.Apply(Path.Combine(stage, "gamedata"), patchOptions, token, WriteLog);
                foreach (string relative in changes.DeletedRelativePaths)
                {
                    checksums.Remove("gamedata/" + SafePaths.ValidateRelative(relative)); Event("patch.path-renamed", new { file = relative });
                }
                foreach (string relative in changes.ChangedRelativePaths)
                {
                    checksums["gamedata/" + SafePaths.ValidateRelative(relative)] = HashFile(SafePaths.Under(stage, "gamedata/" + relative), token);
                    Event("patch.file-final-hash", new { file = relative, sha256 = checksums["gamedata/" + relative] });
                }
                WriteLog($"Reversible game options applied: updated={changes.ChangedRelativePaths.Count}; renamed/deleted={changes.DeletedRelativePaths.Count}");
            }
            var info = new
            {
                schemaVersion = 1, gameTitle = bundle.Manifest.GameTitle, titleId = bundle.Manifest.TitleId,
                buildVersion = bundle.Manifest.BuildVersion, createdUtc = DateTimeOffset.UtcNow,
                xbox = source.Identity, defaultXexSha256 = xexHash, source.FileCount, source.TotalBytes,
                verification = "All copied game data, bundled title files, configuration and optional user DLL were read back and SHA256 verified before optional reversible game patches. The recorded hashes describe the final files.",
                settings = request.Settings, gameOptions = patchOptions, losslessDllIncluded = lossless is not null, files = checksums
            };
            File.WriteAllText(Path.Combine(stage, "build-info.json"), JsonSerializer.Serialize(info, JsonFiles.Options), new UTF8Encoding(false));
            File.WriteAllText(Path.Combine(stage, "README.txt"), InstallationReadme(), new UTF8Encoding(false));
            Event("output.reports.written", new { files = new[] { "build-info.json", "README.txt" }, verifiedFileCount = checksums.Count });
            token.ThrowIfCancellationRequested();
            progress?.Report(new("Finishing", "Making your PS5 folder ready…", 99));
            Phase("Publish output");
            // Same-parent Directory.Move is an atomic publication on the local
            // filesystem and refuses to replace an existing directory.
            SafePaths.NoReparsePoints(parent);
            token.ThrowIfCancellationRequested();
            Directory.Move(stage, destination);
            committed = true; outcome = "success";
            Event("output.published", new { destination, source.FileCount, source.TotalBytes, defaultXexSha256 = xexHash, durationMs = clock.ElapsedMilliseconds });
            WriteLog($"Build complete: {destination}; elapsed={clock.Elapsed}");
            progress?.Report(new("Complete", "Your PS5 folder is ready!", 100, FilesDone: source.FileCount, FilesTotal: source.FileCount));
            return new(destination, source.FileCount, source.TotalBytes, source.Identity.TitleId, source.Identity.MediaId, xexHash, clock.Elapsed);
        }
        catch (OperationCanceledException e) { outcome = "cancelled"; Event("build.cancelled", new { phase = phaseName, activeFile, stage, exception = e.ToString() }); throw; }
        catch (BuilderException e) { Event("build.error", new { phase = phaseName, activeFile, stage, exception = e.ToString() }); throw; }
        catch (UnauthorizedAccessException e)
        {
            Event("build.access-error", new { phase = phaseName, activeFile, stage, exception = e.ToString() });
            throw new BuilderException("The builder could not access a file or folder. Close applications using the files and choose an output folder you can write to.", e);
        }
        catch (IOException e)
        {
            Event("build.io-error", new { phase = phaseName, activeFile, stage, exception = e.ToString() });
            throw new BuilderException("A file could not be read or written. Check that the source and output drives are connected and writable. Your existing files have been kept. Details: " + e.Message, e);
        }
        catch (Exception e) { Event("build.unexpected-error", new { phase = phaseName, activeFile, stage, exception = e.ToString() }); throw new BuilderException("The build could not be completed. Open the builder log for details: " + e.Message, e); }
        finally
        {
            if (!committed && stage is not null && Directory.Exists(stage))
            {
                long cleanupStart = clock.ElapsedMilliseconds;
                Event("cleanup.start", new { stage });
                try { Directory.Delete(stage, recursive: true); Event("cleanup.complete", new { stage, durationMs = clock.ElapsedMilliseconds - cleanupStart }); }
                catch (Exception e) when (e is IOException or UnauthorizedAccessException)
                {
                    Event("cleanup.failed", new { stage, exception = e.ToString() });
                    progress?.Report(new("Cleanup", "An incomplete temporary folder remains and may be removed: " + stage));
                }
            }
            else Event("cleanup.not-needed", new { committed, stage });
            Event("phase.end", new { name = phaseName, durationMs = clock.ElapsedMilliseconds - phaseStart, outcome });
            Event("build.finished", new { outcome, durationMs = clock.ElapsedMilliseconds, committed, stage });
        }
    }

    private void ExtractPayload(string stage, Dictionary<string, string> checksums, CancellationToken token, Action<string> log)
    {
        using ZipArchive archive = bundle.OpenZip();
        var expected = bundle.Manifest.Files.ToDictionary(f => f.Path, StringComparer.Ordinal);
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (ZipArchiveEntry entry in archive.Entries)
        {
            var fileClock = Stopwatch.StartNew();
            token.ThrowIfCancellationRequested();
            string relative = SafePaths.ValidateRelative(entry.FullName);
            if (!seen.Add(relative) || !expected.TryGetValue(relative, out PayloadFile? file) || entry.Length != file.Bytes)
                throw new BuilderException("The bundled PS5 archive does not match its file list. Download a fresh copy of the builder.");
            string path = SafePaths.Under(stage, relative);
            log(DiagnosticSession.FormatEvent("payload.file.start", new { file = relative, bytes = entry.Length, compressedBytes = entry.CompressedLength, expectedSha256 = file.Sha256 }));
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            using Stream input = entry.Open();
            using var output = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None, 1024 * 1024, FileOptions.SequentialScan);
            string actual = CopyChecked(input, output, file.Bytes, token);
            output.Flush(flushToDisk: true);
            if (!actual.Equals(file.Sha256, StringComparison.OrdinalIgnoreCase))
            {
                log(DiagnosticSession.FormatEvent("payload.file.failed", new { file = relative, expectedSha256 = file.Sha256, actualSha256 = actual }));
                throw new BuilderException("A bundled PS5 file failed verification. Download a fresh copy of the builder.");
            }
            checksums.Add(relative, actual);
            log(DiagnosticSession.FormatEvent("payload.file.complete", new { file = relative, bytes = file.Bytes, sha256 = actual, durationMs = fileClock.ElapsedMilliseconds }));
        }
        if (seen.Count != expected.Count) throw new BuilderException("The bundled PS5 archive is incomplete.");
    }

    internal static string CopyChecked(Stream input, Stream output, long length, CancellationToken token, Action<int>? progressed = null)
    {
        byte[] buffer = ArrayPool<byte>.Shared.Rent(1024 * 1024);
        using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
        try
        {
            long remaining = length;
            while (remaining > 0)
            {
                token.ThrowIfCancellationRequested();
                int count = input.Read(buffer, 0, (int)Math.Min(buffer.Length, remaining));
                if (count == 0) throw new BuilderException("A source file ended unexpectedly. Choose a complete game dump and try again.");
                output.Write(buffer, 0, count);
                hash.AppendData(buffer, 0, count);
                remaining -= count;
                progressed?.Invoke(count);
            }
            token.ThrowIfCancellationRequested();
            if (input.ReadByte() != -1) throw new BuilderException("A source file changed size or does not match its declared size.");
            return Convert.ToHexString(hash.GetHashAndReset()).ToLowerInvariant();
        }
        finally { ArrayPool<byte>.Shared.Return(buffer); }
    }

    public static string HashFile(string path, CancellationToken token, Action<int>? progressed = null)
    {
        using var input = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1024 * 1024, FileOptions.SequentialScan);
        return CopyChecked(input, Stream.Null, input.Length, token, progressed);
    }

    private static long GetAvailableSpace(string path)
    {
        string root = Path.GetPathRoot(Path.GetFullPath(path)) ?? throw new BuilderException("The output drive could not be identified.");
        return new DriveInfo(root).AvailableFreeSpace;
    }

    private string InstallationReadme() => $"""
        THE SIMPSONS GAME
        PS5 · {bundle.Manifest.TitleId} · Build {bundle.Manifest.BuildVersion}

        Your folder includes the PS5 title, your extracted game data and the
        settings chosen in the builder.

        1. Copy the complete {bundle.Manifest.TitleId} folder to your PS5:
           /data/homebrew/{bundle.Manifest.TitleId}
        2. Use ShadowMountPlus to mount the title, as you do for your other titles.
        3. Launch The Simpsons Game from the PS5 home screen.

        Keep the folder structure intact. The gamedata folder belongs directly
        inside {bundle.Manifest.TitleId}. Keep userdata when updating: it contains
        your settings and saved progress. Game logs are written inside logs.

        On the first launch from a read-only mounted image, the game imports the
        builder settings into its writable title folder. An existing installation
        keeps its previously saved settings. To apply new builder choices to an
        existing installation, copy only the generated userdata/simpsons.toml to
        that title's writable userdata folder, preserving its other files and saves.

        If you selected your own Lossless.dll, its copy is included at:
        userdata/lossless/Lossless.dll
        Frame generation choices are separate from the native gameplay FPS target.
        The default is off. A 120 Hz output requires a compatible display and
        connection; the game falls back to 60 Hz if 120 Hz is unavailable.

        The builder checked the Xbox 360 title identity and verified every copied
        game file. It did not send your ISO or game data online.

        build-info.json records the selected settings and SHA256 verification.
        """ + "\n";
}

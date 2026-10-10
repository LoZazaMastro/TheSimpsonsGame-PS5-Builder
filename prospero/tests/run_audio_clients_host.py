#!/usr/bin/env python3
"""Compile exact AudioSystem lifecycle/control-flow methods with host fixtures.

Native device calls, wait handles, guest heap and execution are modeled. The
registration, worker dispatch, lease, retirement and shutdown logic are copied
byte-for-byte from production source. No game/PS5 execution is claimed.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

from run_pipeline_cache_host import block_end, function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--baseline-commit")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    relative = "tools/rexglue-sdk/src/audio/audio_system.cpp"
    header_relative = "tools/rexglue-sdk/include/rex/audio/audio_system.h"
    def read(path):
        if args.baseline_commit:
            return subprocess.check_output(["git", "show", args.baseline_commit + ":" + path], cwd=root)
        return (root / path).read_bytes()
    source_bytes, header_bytes = read(relative), read(header_relative)
    source, header = source_bytes.decode(), header_bytes.decode()
    print("Audio source SHA256: " + hashlib.sha256(source_bytes).hexdigest(), flush=True)
    print("Audio header SHA256: " + hashlib.sha256(header_bytes).hexdigest(), flush=True)
    print("Boundary: exact production methods; host executor/device/heap fixtures; no PS5 run", flush=True)
    declarations = header[header.index("  rex::thread::global_critical_region global_critical_region_"):
                          header.index("  int FindFreeClient();")]
    signatures = [
        "AudioSystem::AudioSystem(", "AudioSystem::~AudioSystem()",
        "void AudioSystem::WorkerThreadMain()", "int AudioSystem::FindFreeClient()",
        "void AudioSystem::Initialize()", "void AudioSystem::Shutdown()",
        "X_STATUS AudioSystem::RegisterClient(", "void AudioSystem::SubmitFrame(",
        "void AudioSystem::UnregisterClient(", "void AudioSystem::Pause()",
        "void AudioSystem::Resume()", "bool AudioSystem::Restore(",
    ]
    if not args.baseline_commit:
        signatures.extend([
            "AudioSystem::ClientLease::ClientLease(", "AudioSystem::ClientLease::~ClientLease()",
            "AudioSystem::ClientLease AudioSystem::RetainClient(", "void AudioSystem::ReleaseClient(",
            "void AudioSystem::CleanupRetiredClient(",
        ])
    methods = "\n\n".join(function(source, signature) for signature in signatures)
    cases = ["unregister-before-execute", "unregister-during-callback", "self-unregister",
             "reregister-pending", "submit-after-retire", "submit-retire-concurrent",
             "stale-wait-credit", "registration-failure", "registration-shutdown",
             "callback-exception", "shutdown-active", "shutdown-paused",
             "restore-retiring", "restore-invalid", "restore-idle", "restore-shutdown",
             "cleanup-notify-shutdown"]
    expected_failure_messages = {
        "unregister-before-execute": "retired paced callback never executes",
        "unregister-during-callback": "callback wrapper stays live until Execute returns",
        "self-unregister": "self-unregister executes once with live wrapper",
        "stale-wait-credit": "old wait result cannot synthesize an extra new-client frame credit",
    }
    if args.baseline_commit:
        cases = ["unregister-before-execute", "unregister-during-callback", "self-unregister",
                 "stale-wait-credit"]
    with tempfile.TemporaryDirectory(prefix="simpsons-audio-clients-host-") as directory:
        work = Path(directory)
        (work / "client_state.inc").write_text(declarations + "\n")
        (work / "audio_methods.inc").write_text(methods + "\n")
        binary = work / "audio_clients_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-O2", "-g", "-DNDEBUG", "-pthread",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-DREX_PLATFORM_PROSPERO=1", "-I" + str(root / "tools/rexglue-sdk/include"),
            "-I" + str(work), str(Path(__file__).with_name("audio_clients_host.cpp")),
            str(root / "tools/rexglue-sdk/src/core/byte_stream.cpp"),
            "-Wl,--wrap=nanosleep", "-Wl,--wrap=_ZNSt18condition_variable10notify_allEv",
            "-o", str(binary),
        ], check=True)
        for case in cases:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=15)
            output = (result.stdout + result.stderr).strip()
            print(output, flush=True)
            if args.baseline_commit:
                if result.returncode != 1 or "FAIL " + expected_failure_messages[case] not in output:
                    raise RuntimeError(f"baseline {case} did not reproduce the ownership failure: {result.returncode}")
            elif result.returncode:
                raise RuntimeError(f"{case}: exit={result.returncode}")
        print(f"PASS {len(cases)} production audio scenarios" +
              (" (baseline failures reproduced)" if args.baseline_commit else ""), flush=True)


if __name__ == "__main__":
    main()

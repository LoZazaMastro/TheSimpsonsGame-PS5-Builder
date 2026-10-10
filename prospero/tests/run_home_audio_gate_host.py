#!/usr/bin/env python3
"""Positive and rejection checks for the native gate's explicit audio exception."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile

from home_audio_validation import read_atrac9, validate_home_audio_update


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[2]
    source_report = root / "prospero/tests/fixtures/home_audio/expected.json"
    source = json.loads(source_report.read_text())
    baseline = source["original_sha256"]
    production = root / source["production_asset"]
    watched = [Path(__file__).resolve(), Path(__file__).with_name("home_audio_validation.py"),
               source_report, production]
    hashes = {str(p.relative_to(root)): sha(p) for p in watched}
    checks = []

    def rejected(name, operation):
        try:
            operation()
        except ValueError as error:
            assert str(error).startswith("Home audio update rejected:")
            checks.append({"name": name, "status": "PASS", "rejection": str(error)})
        else:
            raise AssertionError(name + " was incorrectly accepted")

    with tempfile.TemporaryDirectory(prefix="audio-gate-host-", dir=root / "prospero/validation") as work:
        work = Path(work)
        stage = work / "stage"
        (stage / "sce_sys").mkdir(parents=True)
        staged = stage / "sce_sys/snd0.at9"
        shutil.copy2(production, staged)
        accepted = validate_home_audio_update(source_report, root, stage, baseline)
        assert accepted["status"] == "PASS" and accepted["final_sha256"] == sha(production)
        checks.append({"name": "actual baseline, measured replacement, recipe and staged file agree", "status": "PASS"})
        rejected("wrong baseline hash", lambda: validate_home_audio_update(source_report, root, stage, "0" * 64))
        shutil.copy2(root / source["original_asset_backup"], staged)
        rejected("old audio staged with a new report", lambda: validate_home_audio_update(source_report, root, stage, baseline))
        shutil.copy2(production, staged)

        def mutation(name, edit):
            changed = deepcopy(source)
            edit(changed)
            altered = work / "altered-report.json"
            altered.write_text(json.dumps(changed))
            rejected(name, lambda: validate_home_audio_update(altered, root, stage, baseline))

        mutation("failed report", lambda r: r.update(status="FAIL"))
        mutation("exception redirected to artwork", lambda r: r.update(production_asset="prospero/sce_sys/icon0.png"))
        mutation("different gain", lambda r: r.update(gain_db=-0.5))
        mutation("wrong final hash", lambda r: r.update(final_sha256="0" * 64))
        mutation("invented sample count", lambda r: r["format_validation"]["after"].update(samples=1))
        mutation("baseline reconstruction uses the new file", lambda r: r["master_match"].update(
            original_reencoded_asset=r["candidate_asset"], original_reencoded_sha256=r["final_sha256"]))
        mutation("unrequested normalization", lambda r: r["pcm_gain"].update(additional_normalization_limiter_or_fades=True))
        mutation("decoder errors", lambda r: r["decoded_measurement"]["after_validation"].update(errors=1))
        mutation("relaxed decoder gain tolerance", lambda r: r["decoded_measurement"].update(rms_gain_tolerance_db=0.1))
        mutation("recipe drift", lambda r: r["current_recipe_validation"].update(sha256="0" * 64))
        mutation("proof path escapes repository", lambda r: r.update(original_asset_backup="../snd0.at9"))

        damaged = work / "damaged.at9"
        damaged.write_bytes(production.read_bytes()[:-1])
        rejected("truncated actual RIFF file", lambda: read_atrac9(damaged))
        data = bytearray(production.read_bytes())
        position = 12
        while data[position:position + 4] != b"smpl":
            size = struct.unpack_from("<I", data, position + 4)[0]
            position += 8 + size + size % 2
        struct.pack_into("<I", data, position + 8 + 44, 0)
        damaged.write_bytes(data)
        rejected("changed loop position in actual RIFF metadata", lambda: read_atrac9(damaged))

    assert len(checks) == 16
    assert all(sha(root / name) == value for name, value in hashes.items())
    report = root / "prospero/validation/home-audio-gate-v3.10.5-ui-menu.json"
    report.write_text(json.dumps({"status": "PASS", "checks": checks,
        "source_sha256": hashes, "accepted_audio": accepted,
        "scope": "Host verification of the native gate's strict Home-audio exception using the real original and replacement assets. No audio playback or console launch."}, indent=2) + "\n")
    print(json.dumps({"status": "PASS", "checks": len(checks), "report": str(report)}, indent=2))


if __name__ == "__main__":
    main()

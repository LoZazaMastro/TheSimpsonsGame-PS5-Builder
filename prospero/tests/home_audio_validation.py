"""Validate Home audio gain, sample count and ATRAC9 loop metadata.

Measured PCM and decoded ATRAC9 reference data are checked against the
original, reencoded, production and staged files.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import struct


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError("Home audio update rejected: " + reason)


def checked_path(repo: Path, relative: str) -> Path:
    supplied = Path(relative)
    require(not supplied.is_absolute() and ".." not in supplied.parts,
            "proof paths must be relative to the repository")
    path = (repo / supplied).resolve()
    require(path.is_relative_to(repo.resolve()) and path.is_file(),
            "missing or external proof file: " + relative)
    return path


def close(actual: float, expected: float, tolerance: float = 1e-10) -> bool:
    return (isinstance(actual, (int, float)) and not isinstance(actual, bool)
            and math.isfinite(actual) and abs(actual - expected) <= tolerance)


def read_atrac9(path: Path) -> tuple[dict, list[tuple[bytes, bytes]]]:
    data = path.read_bytes()
    require(128 <= len(data) <= 2_097_152 and data[:4] == b"RIFF"
            and data[8:12] == b"WAVE", "unsupported RIFF/WAVE container")
    require(struct.unpack_from("<I", data, 4)[0] + 8 == len(data),
            "incomplete RIFF file")
    chunks, non_audio = {}, []
    position = 12
    while position < len(data):
        require(position + 8 <= len(data), "truncated RIFF chunk header")
        name = data[position:position + 4]
        size = struct.unpack_from("<I", data, position + 4)[0]
        start, end = position + 8, position + 8 + size
        require(name not in chunks and end + size % 2 <= len(data),
                "duplicate or truncated RIFF chunk")
        chunks[name] = data[start:end]
        if name != b"data":
            non_audio.append((name, data[position:end + size % 2]))
        position = end + size % 2
    require(all(name in chunks for name in (b"fmt ", b"fact", b"smpl", b"data")),
            "required ATRAC9 chunks are missing")
    fmt, fact, smpl = (chunks[name] for name in (b"fmt ", b"fact", b"smpl"))
    require(len(fmt) >= 40 and len(fact) >= 12 and len(smpl) >= 60,
            "undersized format, sample count or loop metadata")
    tag, channels, rate, byte_rate = struct.unpack_from("<HHII", fmt)
    samples, delay = struct.unpack_from("<II", fact)
    loop_count = struct.unpack_from("<I", smpl, 28)[0]
    loop_type, loop_start, loop_end = struct.unpack_from("<III", smpl, 40)
    require(tag == 0xFFFE and channels == 2 and rate == 48_000
            and 0 < byte_rate <= channels * 12_000
            and fmt[24:40] == bytes.fromhex("D242E147BA368D4D88FC61654F8C836C"),
            "audio must remain stereo 48 kHz ATRAC9 within PS5 bitrate limits")
    require(samples > 0 and loop_count == 1 and loop_type == 0
            and loop_start == delay and loop_end - loop_start + 1 == samples,
            "sample count, encoder delay or loop boundaries are inconsistent")
    profile = {"bytes": len(data), "samples": samples, "rate": rate,
               "seconds": samples / rate, "bitrate": byte_rate * 8,
               "loop_start": loop_start, "loop_end_inclusive": loop_end}
    return profile, non_audio


def validate_home_audio_update(report_path: Path, repo: Path, stage: Path,
                               baseline_sha256: str) -> dict:
    repo, report_path = repo.resolve(), report_path.resolve()
    require(report_path.is_relative_to(repo) and report_path.is_file(),
            "audio report must exist within this repository")
    report = json.loads(report_path.read_text())
    require(report["status"] == "PASS", "audio report did not pass")
    require(report["production_asset"] == "prospero/sce_sys/snd0.at9",
            "only sce_sys/snd0.at9 may change")
    require(close(report["gain_db"], -1.0)
            and close(report["gain_linear"], 10 ** (-1.0 / 20.0), 1e-12),
            "the authorized change is exactly an additional -1 dB")
    original = checked_path(repo, report["original_asset_backup"])
    production = checked_path(repo, report["production_asset"])
    candidate = checked_path(repo, report["candidate_asset"])
    staged = stage / "sce_sys/snd0.at9"
    require(report["original_sha256"] == baseline_sha256 == sha(original),
            "original audio does not match the delivered v3.8 baseline")
    final_sha = sha(production)
    require(staged.is_file() and report["final_sha256"] == final_sha
            == sha(candidate) == sha(staged) and final_sha != baseline_sha256,
            "report, encoded candidate, production and staged audio disagree")

    before, before_chunks = read_atrac9(original)
    after, after_chunks = read_atrac9(staged)
    require(before == after and before_chunks == after_chunks,
            "audio container format, length or non-audio chunks changed")
    fmt = report["format_validation"]
    require(fmt["status"] == "PASS" and fmt["before"] == before
            and fmt["after"] == after and fmt["codec"] == "ATRAC9"
            and fmt["channels"] == 2 and fmt["sample_rate"] == 48_000
            and fmt["loop_preserved"] is True
            and fmt["all_non_audio_riff_chunks_byte_identical"] is True
            and fmt["sonic_validate_assets"] == "PASS",
            "format evidence does not match the actual files")

    master = report["master_match"]
    reencoded = checked_path(repo, master["original_reencoded_asset"])
    require(master["status"] == "PASS"
            and master["original_reencoded_byte_identical"] is True
            and master["no_atrac9_to_atrac9_generation"] is True
            and master["original_reencoded_sha256"] == baseline_sha256 == sha(reencoded)
            and master["master_sha256"] == sha(checked_path(repo, master["master"]))
            and close(master["prior_gain_linear"], 0.5)
            and close(master["total_gain_linear_from_master"],
                      0.5 * report["gain_linear"], 1e-12),
            "the previous audio was not reproduced exactly from the verified master")

    pcm, decoded = report["pcm_gain"], report["decoded_measurement"]
    require(pcm["status"] == "PASS" and pcm["all_float_samples_exact_gain"] is True
            and pcm["samples_per_channel"] == after["samples"] and pcm["channels"] == 2
            and close(pcm["float_rms_gain_db"], -1.0)
            and pcm["saturation"] is False
            and pcm["additional_normalization_limiter_or_fades"] is False,
            "PCM gain evidence does not establish the requested change")
    pcm_ratio = 20 * math.log10(pcm["after_float"]["rms"] / pcm["before_float"]["rms"])
    require(close(pcm_ratio, -1.0), "PCM RMS measurements do not support -1 dB")
    require(decoded["status"] == "PASS"
            and decoded["samples_per_channel"] == after["samples"]
            and decoded["channels"] == 2 and decoded["saturation"] is False
            and close(decoded["rms_gain_tolerance_db"], 0.03)
            and close(decoded["rms_gain_db"], -1.0, 0.03)
            and math.isfinite(decoded["peak_gain_db"]) and decoded["peak_gain_db"] < 0,
            "decoded ATRAC9 measurements exceed the agreed tolerance")
    decoded_ratio = 20 * math.log10(decoded["after"]["rms"] / decoded["before"]["rms"])
    require(close(decoded_ratio, decoded["rms_gain_db"]),
            "decoded RMS measurements disagree with the reported gain")
    for label in ("before_validation", "after_validation"):
        proof = decoded[label]
        require(proof["status"] == "PASS" and proof["errors"] == 0
                and proof["channels"] == 2 and proof["sample_rate"] == 48_000
                and proof["samples_per_channel"] == after["samples"]
                and proof["encoder_delay_removed_samples"] == after["loop_start"]
                and proof["frames"] > 0 and proof["frames"] == proof["superframes"] * 4,
                "ATRAC9 decode was incomplete or misaligned")
    require(decoded["before_validation"]["frames"] == decoded["after_validation"]["frames"],
            "decoded frame counts differ")
    recipe = report["current_recipe_validation"]
    require(recipe["status"] == "PASS"
            and recipe["sha256"] == sha(checked_path(repo, recipe["path"]))
            and close(recipe["direct_master_gain_linear"], 0.5 * report["gain_linear"], 1e-12)
            and recipe["pcm16_sha256"] == master["new_pcm_sha256"]
            and recipe["pcm16_byte_identical_to_encoded_input"] is True
            and recipe["gain_verification_sha256"]
                == sha(repo / "prospero/assets/audio-gain-verification.json")
            and recipe["assets_manifest_sha256"] == sha(repo / "prospero/assets/manifest.json"),
            "the current audio recipe or asset manifest does not match the measured result")
    return {
        "status": "PASS", "authorized_asset": "sce_sys/snd0.at9",
        "report": {"path": str(report_path.relative_to(repo)),
                   "bytes": report_path.stat().st_size, "sha256": sha(report_path)},
        "original_sha256": baseline_sha256, "final_sha256": final_sha,
        "gain_db": -1.0, "decoded_rms_gain_db": decoded["rms_gain_db"],
        "unchanged_format": after, "non_audio_riff_chunks_byte_identical": True,
        "baseline_reencoded_byte_identical": True,
        "scope": "Only the measured -1 dB Home-hover sound replacement is authorized; all other presentation assets retain their v3.8 hashes."
    }

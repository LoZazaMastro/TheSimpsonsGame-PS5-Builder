#!/usr/bin/env python3
"""Rebuild Andrea's presentation assets with the pinned Sonic converters.

Requires Python 3.9+, native GCC/Clang, patch, FFmpeg and the Sonic reference checkout.
Only this project's prospero/sce_sys outputs are written. The reference checkout
is read-only. Audio keeps the earlier linear gain 0.5 and adds -1 dB before
PCM16 conversion, with no normalization, added fades or limiter; see ASSETS.md.
"""

import argparse
import array
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import wave


SONIC_COMMIT = "a63abf2db139b04da90989024da5bc804d11323a"
CONVERTER_HASHES = {
    "tools/png_to_bc7_dds/png_to_bc7_dds.cpp":
        "7d0a88d6876824d71d925309667f659e0f7ee39d632304e47b30b7f31e989e25",
    "tools/wav_to_at9/wav_to_at9.cpp":
        "0b0b2daa75417e8371167436de6079f20754c8da6ba9f0fed49e7838d573ff3f",
}
SOURCE_HASHES = {
    "home-background.png":
        "8435649ffeb20dc063c60b2172d2a7b7aa6c448c41705e91e1166d7961b5cde9",
    "launch-background.png":
        "3b51d79b8e9901d1518fed39fb1f48424952ce4e001c3fd6ab4a64823b00cce5",
    "menu-music.mp3":
        "4b751f7abe3b90653fd79ae1eb4c5d459374475c44f2648057ada77ee1adc6b8",
}
PREVIOUS_AUDIO_GAIN = 0.5
ADDITIONAL_AUDIO_GAIN_DB = -1.0
AUDIO_GAIN = PREVIOUS_AUDIO_GAIN * 10 ** (ADDITIONAL_AUDIO_GAIN_DB / 20.0)


def prepare_audio_pcm(source, work):
    """Apply the requested gain exactly once, before integer PCM conversion."""
    original = work / "menu-original.f64"
    reduced = work / "menu-prepared.f64"
    pcm = work / "menu-prepared-pcm16.wav"
    run(["ffmpeg", "-v", "error", "-i", source, "-vn", "-sn", "-dn",
         "-ar", "48000", "-ac", "2", "-c:a", "pcm_f64le", "-f", "f64le", original])
    run(["ffmpeg", "-v", "error", "-f", "f64le", "-ar", "48000", "-ac", "2",
         "-i", original, "-af", f"volume={AUDIO_GAIN}:precision=double",
         "-c:a", "pcm_f64le", "-f", "f64le", reduced])
    before = array.array("d")
    after = array.array("d")
    before.frombytes(original.read_bytes())
    after.frombytes(reduced.read_bytes())
    if sys.byteorder != "little":
        before.byteswap()
        after.byteswap()
    if not before or len(before) != len(after) or len(before) % 2:
        raise ValueError("Gain conversion changed the audio sample count")
    peak_before = peak_after = energy_before = energy_after = 0.0
    for source_sample, output_sample in zip(before, after):
        if not math.isfinite(source_sample) or output_sample != source_sample * AUDIO_GAIN:
            raise ValueError("PCM sample does not have exactly the requested linear gain")
        peak_before = max(peak_before, abs(source_sample))
        peak_after = max(peak_after, abs(output_sample))
        energy_before += source_sample * source_sample
        energy_after += output_sample * output_sample
    if peak_after >= 1.0:
        raise ValueError("Reduced PCM unexpectedly exceeds integer full scale")
    run(["ffmpeg", "-v", "error", "-f", "f64le", "-ar", "48000", "-ac", "2",
         "-i", reduced, "-c:a", "pcm_s16le", pcm])
    with wave.open(str(pcm), "rb") as wav:
        if (wav.getnchannels(), wav.getframerate(), wav.getsampwidth(), wav.getnframes()) != (
                2, 48_000, 2, len(before) // 2):
            raise ValueError("Unexpected PCM16 format or duration")
        quantized = array.array("h")
        quantized.frombytes(wav.readframes(wav.getnframes()))
    if sys.byteorder != "little":
        quantized.byteswap()
    max_quantization_error = max(
        abs(integer / 32768.0 - floating) for integer, floating in zip(quantized, after))
    if len(quantized) != len(after) or max_quantization_error > 0.50001 / 32768.0:
        raise ValueError("PCM16 conversion added more than rounding error")
    result = {
        "gain_linear": AUDIO_GAIN,
        "gain_db": 20 * math.log10(AUDIO_GAIN),
        "previous_gain_linear": PREVIOUS_AUDIO_GAIN,
        "additional_gain_db": ADDITIONAL_AUDIO_GAIN_DB,
        "samples_per_channel": len(before) // 2,
        "channels": 2,
        "rate": 48_000,
        "all_float_samples_exact_gain": True,
        "rms_gain_ratio": math.sqrt(energy_after / energy_before),
        "source_float_peak": peak_before,
        "reduced_float_peak": peak_after,
        "pcm16_peak": max(abs(value) for value in quantized) / 32768.0,
        "pcm16_max_quantization_error_lsb": max_quantization_error * 32768.0,
        "pcm16_saturation": False,
    }
    print(f"PCM gain verification: {result}", flush=True)
    return pcm, result


def run(args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def validate_dds(path):
    data = path.read_bytes()
    if len(data) != 8_294_548 or data[:4] != b"DDS ":
        raise ValueError(f"Invalid DDS length or signature: {path}")
    actual = (
        struct.unpack_from("<I", data, 4)[0],
        struct.unpack_from("<II", data, 12),
        struct.unpack_from("<I", data, 28)[0], data[84:88],
        struct.unpack_from("<II", data, 128),
        struct.unpack_from("<I", data, 140)[0],
    )
    if actual != (124, (2160, 3840), 1, b"DX10", (98, 3), 1):
        raise ValueError(f"Invalid 3840x2160 BC7_UNORM DX10 profile: {path}")


def validate_at9(path):
    data = path.read_bytes()
    if (not 128 <= len(data) <= 2_097_152 or data[:4] != b"RIFF"
            or data[8:12] != b"WAVE"
            or struct.unpack_from("<I", data, 4)[0] + 8 != len(data)):
        raise ValueError("Invalid or oversized ATRAC9 RIFF container")
    chunks = {}
    offset = 12
    while offset + 8 <= len(data):
        name = data[offset:offset + 4]
        length = struct.unpack_from("<I", data, offset + 4)[0]
        offset += 8
        if offset + length > len(data) or name in chunks:
            raise ValueError("Truncated or duplicate RIFF chunk")
        chunks[name] = data[offset:offset + length]
        offset += length + (length & 1)
    if offset != len(data) or not all(
            name in chunks for name in (b"fmt ", b"fact", b"smpl", b"data")):
        raise ValueError("Incomplete ATRAC9 RIFF container")
    fmt, fact, smpl = chunks[b"fmt "], chunks[b"fact"], chunks[b"smpl"]
    if len(fmt) < 52 or len(fact) < 12 or len(smpl) < 60:
        raise ValueError("Incomplete ATRAC9 metadata")
    tag, channels, rate, byte_rate = struct.unpack_from("<HHII", fmt)
    if ((tag, channels, rate) != (0xFFFE, 2, 48_000)
            or not 0 < byte_rate <= 24_000
            or fmt[24:40] != bytes.fromhex("D242E147BA368D4D88FC61654F8C836C")
            or struct.unpack_from("<I", smpl, 28)[0] != 1):
        raise ValueError("Unsupported ATRAC9 format or loop count")
    samples, delay, _ = struct.unpack_from("<III", fact)
    loop_start, loop_end = struct.unpack_from("<II", smpl, 44)
    if loop_start != delay or loop_end != delay + samples - 1:
        raise ValueError("ATRAC9 loop does not cover the complete audio")
    return {"bytes": len(data), "samples": samples, "rate": rate,
            "seconds": samples / rate, "bitrate": byte_rate * 8,
            "loop_start": loop_start, "loop_end_inclusive": loop_end}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sonic-root", type=Path, required=True)
    parser.add_argument("--cc", default=os.environ.get("HOST_CC", "cc"))
    parser.add_argument("--cxx", default=os.environ.get("HOST_CXX", "c++"))
    parser.add_argument("--audio-only", action="store_true",
                        help="regenerate only the Home music, keeping existing artwork")
    args = parser.parse_args()
    sonic = args.sonic_root.resolve(strict=True)
    root = Path(__file__).resolve().parent
    sources = root / "assets" / "source"
    output = root / "sce_sys"
    revision = subprocess.check_output(
        ["git", "-C", str(sonic), "rev-parse", "HEAD"], text=True).strip()
    if revision != SONIC_COMMIT:
        parser.error(f"Sonic checkout must be at {SONIC_COMMIT}")
    run(["git", "-C", sonic, "diff", "--quiet", "HEAD", "--",
         "thirdparty/stb", "thirdparty/libatrac9"])
    for relative, expected in CONVERTER_HASHES.items():
        if hashlib.sha256((sonic / relative).read_bytes()).hexdigest() != expected:
            parser.error(f"Unexpected converter source: {relative}")
    for name, expected in SOURCE_HASHES.items():
        if hashlib.sha256((sources / name).read_bytes()).hexdigest() != expected:
            parser.error(f"Supplied asset changed; review the conversion recipe: {name}")

    # Build an isolated tree so the encoder's original relative includes work.
    with tempfile.TemporaryDirectory(prefix="simpsons-presentation-") as temp:
        work = Path(temp)
        (work / "thirdparty").symlink_to(sonic / "thirdparty", target_is_directory=True)
        encoder_source = work / "tools/wav_to_at9/wav_to_at9.cpp"
        encoder_source.parent.mkdir(parents=True)
        shutil.copy2(sonic / "tools/wav_to_at9/wav_to_at9.cpp", encoder_source)
        run(["patch", "--batch", "--fuzz=0", "-p1", "-i",
             root / "tools/wav_to_at9_unity.patch"], cwd=work)
        if not args.audio_only:
            converter = work / "png_to_bc7_dds"
            print("Building the BC7 image converter...", flush=True)
            run([args.cxx, "-std=c++20", "-O3", "-I", sonic / "thirdparty/stb",
                 sonic / "tools/png_to_bc7_dds/png_to_bc7_dds.cpp", "-o", converter])
        print("Building the ATRAC9 encoder without extra audio processing...", flush=True)
        objects = []
        for source in sorted((sonic / "thirdparty/libatrac9/src").glob("*.c")):
            obj = work / (source.stem + ".o")
            run([args.cc, "-O3", "-c", source, "-o", obj])
            objects.append(obj)
        encoder = work / "wav_to_at9_unity"
        run([args.cxx, "-std=c++20", "-O3", encoder_source, *objects,
             "-lm", "-o", encoder])

        if not args.audio_only:
            print("Converting the supplied images without cropping...", flush=True)
            for name, source in (("pic0", "home-background.png"),
                                 ("pic1", "launch-background.png")):
                run([converter, sources / source, work / (name + ".png"), 3840, 2160])
                run([converter, sources / source, work / (name + ".dds")])
                validate_dds(work / (name + ".dds"))
        print(f"Preparing the complete MP3 at linear gain {AUDIO_GAIN:.17g} "
              f"({ADDITIONAL_AUDIO_GAIN_DB:.1f} dB relative to the previous Home music)...", flush=True)
        pcm_input, gain_verification = prepare_audio_pcm(sources / "menu-music.mp3", work)
        # This source is ~100.2 seconds. The encoder's 174.7-second upper bound
        # does not trim it, and automatic 144 kbps selection fits within 2 MiB.
        run([encoder, pcm_input, work / "snd0.at9", "174.7"])
        audio = validate_at9(work / "snd0.at9")
        if audio["samples"] != gain_verification["samples_per_channel"]:
            raise ValueError("ATRAC9 encoding did not preserve the full audio duration")
        output.mkdir(parents=True, exist_ok=True)
        names = ("snd0.at9",) if args.audio_only else (
            "pic0.png", "pic1.png", "pic0.dds", "pic1.dds", "snd0.at9")
        for name in names:
            shutil.copyfile(work / name, output / (name + ".tmp"))
            os.replace(output / (name + ".tmp"), output / name)
        gain_verification["snd0_sha256"] = hashlib.sha256((output / "snd0.at9").read_bytes()).hexdigest()
        (root / "assets/audio-gain-verification.json").write_text(
            json.dumps(gain_verification, indent=2) + "\n")
        print(f"Presentation assets ready: {output}")
        print(f"Audio container: {audio}")


if __name__ == "__main__":
    main()

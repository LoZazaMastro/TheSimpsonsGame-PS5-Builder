#!/usr/bin/env python3
"""Compiles the native replacement shaders into an ahead-of-time shader set.

Each <hash>_<vs|ps>.frag/.vert here replaces the translation of one Xenos
shader. It is compiled with glslangValidator, given the float controls every
translated module declares (DenormFlushToZero, SignedZeroInfNanPreserve and
RoundingModeRTE for 32-bit floats, which GLSL cannot express), and written as
<out>/<hash>_<stage>_<modification>.spv next to the translation's .bind
sidecar, so aot_shader_path serves it instead of the runtime translation.
Pixel shaders that support them (XE_TEXTURES_PLAIN, XE_TEXTURES_LEVEL0) are
also built for the translator's plain and level 0 texture variants, and both,
under the modifications with those bits set.

Translations depend on the draw resolution scale, so the set for each scale
is built separately (XE_RESOLUTION_SCALE), scale 1 in <out_dir> and the others
in <out_dir>/scale<X>x<Y>, where the runtime looks for them. Every set records
the translator version and the render target path it was made for (the native
shaders write host render targets, so they're for the "host" path only).

usage: build_native_shaders.py <out_dir> <modification> [bind_source_dir]

The binding sidecars (<hash>_<stage>.bind, from the translation of each shader)
are next to the sources unless bind_source_dir says otherwise.

<modification> is for shaders that don't name theirs (// xe_modification <hex>).
"""
import os
import shutil
import struct
import subprocess
import sys

OP_CAPABILITY = 17
OP_EXECUTION_MODE = 16
OP_ENTRY_POINT = 15
CAP_DENORM_FLUSH_TO_ZERO = 4465
CAP_SIGNED_ZERO_INF_NAN_PRESERVE = 4466
CAP_ROUNDING_MODE_RTE = 4467
MODE_DENORM_FLUSH_TO_ZERO = 4460
MODE_SIGNED_ZERO_INF_NAN_PRESERVE = 4461
MODE_ROUNDING_MODE_RTE = 4462
FLOAT_CONTROL_CAPABILITIES = (CAP_DENORM_FLUSH_TO_ZERO, CAP_SIGNED_ZERO_INF_NAN_PRESERVE,
                              CAP_ROUNDING_MODE_RTE)
FLOAT_CONTROL_MODES = (MODE_DENORM_FLUSH_TO_ZERO, MODE_SIGNED_ZERO_INF_NAN_PRESERVE,
                       MODE_ROUNDING_MODE_RTE)
# SpirvShaderTranslator::Modification::pixel.textures_plain (word 1, bit 17).
MODIFICATION_TEXTURES_PLAIN = 1 << 49
# SpirvShaderTranslator::Modification::pixel.textures_level0 (word 1, bit 18).
MODIFICATION_TEXTURES_LEVEL0 = 1 << 50


def add_float_controls(words):
    header, body = words[:5], words[5:]
    out, i = [], 0
    entry_id = None
    capabilities_done = modes_done = False
    instructions = []
    while i < len(body):
        n = body[i] >> 16
        instructions.append(body[i:i + n])
        i += n
    for inst in instructions:
        op = inst[0] & 0xFFFF
        if op == OP_ENTRY_POINT:
            entry_id = inst[2]
        # Capabilities go with the other capabilities, execution modes right
        # before the first existing one (both sections come in this order).
        if op != OP_CAPABILITY and not capabilities_done:
            for cap in FLOAT_CONTROL_CAPABILITIES:
                out.append([(2 << 16) | OP_CAPABILITY, cap])
            capabilities_done = True
        if op == OP_EXECUTION_MODE and not modes_done:
            for mode in FLOAT_CONTROL_MODES:
                out.append([(4 << 16) | OP_EXECUTION_MODE, entry_id, mode, 32])
            modes_done = True
        out.append(inst)
    assert capabilities_done and modes_done, "no execution mode section"
    return header + [w for inst in out for w in inst]


# Draw resolution scales to build sets for. Only powers of two: the modules
# must compute 1/scale exactly like the translator's float constant.
RESOLUTION_SCALES = (1, 2)


def build_set(set_dir, modification, bind_dir, scale):
    os.makedirs(set_dir, exist_ok=True)
    with open(os.path.join(set_dir, "translator_version.txt"), "w") as f:
        f.write("1\n")
    with open(os.path.join(set_dir, "render_target_path.txt"), "w") as f:
        f.write("host\n")
    here = os.path.dirname(os.path.abspath(__file__))
    for name in sorted(os.listdir(here)):
        base, ext = os.path.splitext(name)
        if ext not in (".frag", ".vert"):
            continue
        shader_hash, stage = base.split("_")
        source = os.path.join(here, name)
        text = open(source).read()
        if scale != 1 and "XE_RESOLUTION_SCALE" not in text:
            # Written for scale 1 only.
            continue
        scale_defines = [f"-DXE_RESOLUTION_SCALE={scale}"] if scale != 1 else []
        # A shader may name the modification it's for (// xe_modification <hex>).
        shader_modification = int(modification, 16)
        for line in text.splitlines():
            if line.startswith("// xe_modification "):
                shader_modification = int(line.split()[2], 16)
        variants = [(shader_modification, scale_defines)]
        if stage == "ps" and "XE_TEXTURES_PLAIN" in text:
            variants.append((shader_modification | MODIFICATION_TEXTURES_PLAIN,
                             scale_defines + ["-DXE_TEXTURES_PLAIN=1"]))
        if stage == "ps" and "XE_TEXTURES_LEVEL0" in text:
            variants += [(modification_value | MODIFICATION_TEXTURES_LEVEL0,
                          defines + ["-DXE_TEXTURES_LEVEL0=1"])
                         for modification_value, defines in list(variants)]
        for variant_modification, defines in variants:
            spv = os.path.join(set_dir, f"{shader_hash}_{stage}_{variant_modification:016X}.spv")
            subprocess.run(["glslangValidator", "-V", "--target-env", "vulkan1.2", *defines,
                            "-o", spv, source], check=True, stdout=subprocess.DEVNULL)
            data = open(spv, "rb").read()
            words = list(struct.unpack(f"<{len(data) // 4}I", data))
            words = add_float_controls(words)
            open(spv, "wb").write(struct.pack(f"<{len(words)}I", *words))
            print("built", spv)
        if bind_dir:
            shutil.copy(os.path.join(bind_dir, f"{shader_hash}_{stage}.bind"), set_dir)


def main():
    out_dir, modification = sys.argv[1], sys.argv[2]
    bind_dir = (sys.argv[3] if len(sys.argv) > 3 else
                os.path.dirname(os.path.abspath(__file__)))
    for scale in RESOLUTION_SCALES:
        assert scale & (scale - 1) == 0
        set_dir = out_dir if scale == 1 else os.path.join(out_dir, f"scale{scale}x{scale}")
        build_set(set_dir, modification, bind_dir, scale)


if __name__ == "__main__":
    main()

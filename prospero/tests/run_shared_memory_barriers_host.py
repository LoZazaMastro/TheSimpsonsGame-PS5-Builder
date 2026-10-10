#!/usr/bin/env python3
"""Check the actual Vulkan shared-memory barrier builder against RAW/WAW hazards.

Use() and GetUsageMasks() are extracted unchanged from each revision. A capture
sink replaces command submission; this checks emitted Vulkan barriers, not GPU
execution or PS5 performance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from run_pipeline_cache_host import function

FIXTURE = r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#define assert_true(...) ((void)0)
#define assert_unhandled_case(...) ((void)0)
#define REXGPU_ERROR(...) ((void)0)
struct Barrier {
  VkDeviceSize offset, size;
  VkPipelineStageFlags source_stage, destination_stage;
  VkAccessFlags source_access, destination_access;
  bool may_skip;
};
struct Commands {
  std::vector<Barrier> barriers;
  void PushBufferMemoryBarrier(VkBuffer, VkDeviceSize offset, VkDeviceSize size,
      VkPipelineStageFlags source_stage, VkPipelineStageFlags destination_stage,
      VkAccessFlags source_access, VkAccessFlags destination_access,
      uint32_t, uint32_t, bool may_skip) {
    barriers.push_back({offset, size, source_stage, destination_stage,
                        source_access, destination_access, may_skip});
  }
};
class VulkanSharedMemory {
 public:
  enum class Usage { kRead, kGuestDrawReadWrite, kComputeWrite, kTransferDestination };
  static constexpr uint32_t kBufferSize = 1 << 29;
  Commands command_processor_;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  Usage last_usage_ = Usage::kTransferDestination;
  std::pair<uint32_t,uint32_t> last_written_range_{0, 0};
  VkPipelineStageFlags guest_shader_pipeline_stages_ =
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  bool FlushGpuWrittenRange(uint32_t, uint32_t, bool) { return true; }
  void Use(Usage usage, std::pair<uint32_t,uint32_t> written_range = {},
           std::pair<uint32_t,uint32_t> read_range = {});
  void GetUsageMasks(Usage, VkPipelineStageFlags&, VkAccessFlags&) const;
};
#include "production.inc"
unsigned failures = 0;
void Check(bool ok, const char* text) {
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL %s\n", text); }
}
int main() {
  using Usage = VulkanSharedMemory::Usage;
  VulkanSharedMemory memory;
  memory.Use(Usage::kComputeWrite, {4096, 8192});
  auto b = memory.command_processor_.barriers.back();
  Check((b.source_access & VK_ACCESS_TRANSFER_WRITE_BIT) != 0, "upload writes available before compute");
  Check((b.destination_access & VK_ACCESS_SHADER_WRITE_BIT) != 0, "compute write is in destination access scope");
  Check(b.destination_stage == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, "compute destination stage");
  memory.Use(Usage::kRead, {}, {4096, 8192});
  b = memory.command_processor_.barriers.back();
  Check((b.source_access & VK_ACCESS_SHADER_WRITE_BIT) != 0, "resolve writes available before vertex/index/texture/copy reads");
  Check((b.destination_access & (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT)) ==
        (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT), "all consumer accesses visible");
  Check(b.offset == 0 && b.size == VK_WHOLE_SIZE, "usage transition covers entire buffer");
  auto n = memory.command_processor_.barriers.size();
  memory.Use(Usage::kRead, {}, {4096, 8192});
  Check(memory.command_processor_.barriers.size() == n, "read-after-read needs no redundant barrier");
  memory.Use(Usage::kComputeWrite, {4096, 8192});
  memory.Use(Usage::kComputeWrite, {16384, 4096});
  b = memory.command_processor_.barriers.back();
  Check((b.source_access & VK_ACCESS_SHADER_WRITE_BIT) && (b.destination_access & VK_ACCESS_SHADER_WRITE_BIT),
        "consecutive compute writes have write-after-write memory dependency");
  Check(b.offset == 4096 && b.size == 8192 && !b.may_skip,
        "same-usage barrier commits previous written range without elision");
  memory.Use(Usage::kGuestDrawReadWrite, {16384, 4096});
  memory.Use(Usage::kComputeWrite, {32768, 4096});
  b = memory.command_processor_.barriers.back();
  Check((b.source_access & VK_ACCESS_SHADER_WRITE_BIT) && (b.destination_access & VK_ACCESS_SHADER_WRITE_BIT),
        "guest memexport and compute resolve writes ordered");
  std::printf("%s actual Use/GetUsageMasks: upload->compute->read, compute->compute, draw->compute; failures=%u\n",
              failures ? "FAIL" : "PASS", failures);
  return failures ? 1 : 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="g++")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    path = "tools/rexglue-sdk/src/graphics/vulkan/shared_memory.cpp"
    baseline = "8a93bd1ca0da475716f953b39e1a19155b52af84"
    records = []
    with tempfile.TemporaryDirectory(prefix="simpsons-barrier-host-") as directory:
        work = Path(directory)
        (work / "fixture.cpp").write_text(FIXTURE)
        for variant in ("baseline", "current"):
            source = (subprocess.check_output(["git", "show", baseline + ":" + path], cwd=root).decode()
                      if variant == "baseline" else (root / path).read_text())
            selected = "\n".join(function(source, signature) for signature in (
                "void VulkanSharedMemory::Use(", "void VulkanSharedMemory::GetUsageMasks("))
            (work / "production.inc").write_text(selected + "\n")
            binary = work / variant
            subprocess.run([args.cxx, "-std=c++23", "-O2", "-DNDEBUG", "-Wall", "-Wextra", "-Werror",
                            "-I", str(work), "-I", str(root / "tools/rexglue-sdk/thirdparty/vulkan-headers/include"),
                            str(work / "fixture.cpp"), "-o", str(binary)], check=True, timeout=30)
            run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
            capture = output / (variant + ".txt")
            capture.write_text(run.stdout + run.stderr)
            ok = (run.returncode == 1 and "resolve writes available" in run.stderr) if variant == "baseline" else run.returncode == 0
            records.append({"variant": variant, "pass": ok, "exit_code": run.returncode,
                            "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                            "extracted_body_sha256": hashlib.sha256(selected.encode()).hexdigest(),
                            "capture": str(capture.relative_to(root)),
                            "capture_sha256": hashlib.sha256(capture.read_bytes()).hexdigest()})
            print(f"{'PASS' if ok else 'FAIL'} {variant}: exit={run.returncode}", flush=True)
    report = {"result": "PASS" if all(x["pass"] for x in records) else "FAIL",
              "baseline_revision": baseline, "tests": records,
              "boundary": "Exact production Use/GetUsageMasks methods; emitted barrier sink; actual Khronos Vulkan enum constants.",
              "limits": ["No GPU commands executed", "No PS5 run", "Bug reached by unscaled fallback resolve; no claim explains 3x console failure"],
              "primary_reference": "https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-dependencies"}
    (output.parent / "shared-memory-barriers-full-audit.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

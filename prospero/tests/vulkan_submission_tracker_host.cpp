// Compile and execute the actual production tracker header and .cpp. Only the
// Vulkan API boundary is simulated; no tracker logic is copied into this test.
#include <rex/ui/vulkan/submission_tracker.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using rex::ui::vulkan::VulkanDevice;
using rex::ui::vulkan::VulkanSubmissionTracker;

static void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct FakeGpu {
  struct Fence {
    VkResult status = VK_NOT_READY;
    VkResult wait = VK_SUCCESS;
    unsigned resets = 0;
    unsigned destroys = 0;
  };
  std::map<VkFence, Fence> fences;
  std::vector<VkFence> queries, waits, resets, destroys;
  uintptr_t next_fence = 1;
  bool fail_create = false, fail_reset = false;

  static FakeGpu& Get(VkDevice device) { return *reinterpret_cast<FakeGpu*>(device); }
  Fence& Alive(VkFence fence) {
    Check(fences.contains(fence), "unknown fence used");
    Check(fences.at(fence).destroys == 0, "destroyed fence used");
    return fences.at(fence);
  }
  static VKAPI_ATTR VkResult VKAPI_CALL Create(VkDevice device, const VkFenceCreateInfo* info,
                                               const VkAllocationCallbacks*, VkFence* fence) {
    auto& gpu = Get(device);
    Check(info->sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO && info->flags == 0,
          "tracker must create an unsignaled fence");
    if (gpu.fail_create) { gpu.fail_create = false; *fence = VK_NULL_HANDLE; return VK_ERROR_OUT_OF_HOST_MEMORY; }
    *fence = reinterpret_cast<VkFence>(gpu.next_fence++);
    gpu.fences.emplace(*fence, Fence{});
    return VK_SUCCESS;
  }
  static VKAPI_ATTR void VKAPI_CALL Destroy(VkDevice device, VkFence fence, const VkAllocationCallbacks*) {
    auto& gpu = Get(device);
    ++gpu.Alive(fence).destroys;
    gpu.destroys.push_back(fence);
  }
  static VKAPI_ATTR VkResult VKAPI_CALL Reset(VkDevice device, uint32_t count, const VkFence* fences) {
    auto& gpu = Get(device);
    Check(count == 1, "unexpected reset count");
    gpu.resets.push_back(fences[0]);
    if (gpu.fail_reset) { gpu.fail_reset = false; return VK_ERROR_DEVICE_LOST; }
    auto& fence = gpu.Alive(fences[0]);
    ++fence.resets;
    fence.status = VK_NOT_READY;
    fence.wait = VK_SUCCESS;
    return VK_SUCCESS;
  }
  static VKAPI_ATTR VkResult VKAPI_CALL Status(VkDevice device, VkFence fence) {
    auto& gpu = Get(device);
    gpu.queries.push_back(fence);
    return gpu.Alive(fence).status;
  }
  static VKAPI_ATTR VkResult VKAPI_CALL Wait(VkDevice device, uint32_t count, const VkFence* fences,
                                             VkBool32 all, uint64_t timeout) {
    auto& gpu = Get(device);
    Check(count == 1 && all == VK_TRUE && timeout == UINT64_MAX, "incorrect fence wait");
    gpu.waits.push_back(fences[0]);
    auto& fence = gpu.Alive(fences[0]);
    if (fence.wait == VK_SUCCESS) fence.status = VK_SUCCESS;
    return fence.wait;
  }
  VulkanDevice Device() {
    return VulkanDevice(reinterpret_cast<VkDevice>(this), {Create, Destroy, Reset, Status, Wait});
  }
  void ClearCalls() { queries.clear(); waits.clear(); resets.clear(); destroys.clear(); }
  void NoGpuCalls() const {
    Check(queries.empty() && waits.empty() && resets.empty() && destroys.empty(),
          "already-complete request touched unrelated GPU work");
  }
  void AllDestroyedOnce() const {
    for (const auto& [unused, fence] : fences) {
      (void)unused;
      Check(fence.destroys == 1, "fence was leaked or destroyed more than once");
    }
  }
};

struct Fixture {
  FakeGpu gpu;
  VulkanDevice device = gpu.Device();
  VulkanSubmissionTracker tracker{&device};
  std::pair<uint64_t, VkFence> Submit() {
    const uint64_t index = tracker.GetCurrentSubmission();
    VkFence fence;
    { auto acquired = tracker.AcquireFenceToAdvanceSubmission(); fence = acquired.fence(); }
    Check(tracker.GetCurrentSubmission() == index + 1, "successful submission did not advance");
    return {index, fence};
  }
  void CompleteTwo() {
    const auto first = Submit(), second = Submit();
    gpu.fences[first.second].status = gpu.fences[second.second].status = VK_SUCCESS;
    Check(tracker.UpdateAndGetCompletedSubmission() == 2, "failed to observe two real signals");
    gpu.ClearCalls();
  }
  void Finish() { tracker.Shutdown(); gpu.AllDestroyedOnce(); }
};

static void ZeroAfterTwoCompletions() {
  Fixture f; f.CompleteTwo();
  Check(f.tracker.AwaitSubmissionCompletion(0), "index 0 rejected after GPU reached 2");
  f.gpu.NoGpuCalls(); f.Finish();
}
static void EarlierThanCompleted() {
  Fixture f; f.CompleteTwo();
  Check(f.tracker.AwaitSubmissionCompletion(1), "GPU reached 2 but completion of 1 rejected");
  Check(f.tracker.AwaitSubmissionCompletion(2), "exact completion rejected");
  f.gpu.NoGpuCalls(); f.Finish();
}
static void ZeroWithPendingWork() {
  Fixture f; f.Submit(); f.Submit(); f.gpu.ClearCalls();
  Check(f.tracker.AwaitSubmissionCompletion(0), "never-used slot rejected");
  f.gpu.NoGpuCalls(); f.Finish();
}
static void NewerSignalCoversRequested() {
  Fixture f; const auto first = f.Submit(), second = f.Submit();
  f.gpu.fences[second.second].status = VK_SUCCESS; f.gpu.ClearCalls();
  Check(f.tracker.AwaitSubmissionCompletion(first.first), "newer signaled fence did not cover older work");
  Check(f.tracker.UpdateAndGetCompletedSubmission() == second.first, "timeline did not reach newer fence");
  Check(f.gpu.queries == std::vector<VkFence>{second.second} && f.gpu.waits.empty(),
        "later signal should avoid a blocking wait on earlier fences");
  f.Finish();
}
static void PendingWaitRetiresOnlyCoveredFences() {
  Fixture f; const auto first = f.Submit(), second = f.Submit(), third = f.Submit();
  f.gpu.ClearCalls();
  Check(f.tracker.AwaitSubmissionCompletion(second.first), "pending requested fence did not complete");
  Check(f.gpu.queries == std::vector<VkFence>{third.second} &&
        f.gpu.waits == std::vector<VkFence>{second.second}, "wait did not target the right submission");
  Check(f.tracker.UpdateAndGetCompletedSubmission() == 2, "unsignaled newer fence retired early");
  const auto fourth = f.Submit();
  Check(fourth.second == second.second, "covered fence was not reclaimed for reuse");
  Check(f.gpu.fences[first.second].resets == 0 && f.gpu.fences[third.second].resets == 0,
        "fence outside the selected reclaimed slot was reset");
  Check(f.gpu.fences[third.second].destroys == 0, "pending fence destroyed prematurely");
  f.Finish();
}
static void PendingTimeoutStaysIncomplete() {
  Fixture f; const auto first = f.Submit(); f.gpu.fences[first.second].wait = VK_TIMEOUT;
  Check(!f.tracker.AwaitSubmissionCompletion(first.first), "wait timeout reported as GPU completion");
  Check(f.tracker.UpdateAndGetCompletedSubmission() == 0 && f.gpu.resets.empty(),
        "pending fence retired or reused after timeout");
  f.gpu.fences[first.second].wait = VK_SUCCESS; f.Finish();
}
static void QueryNotReadyDoesNotRetire() {
  Fixture f; const auto first = f.Submit();
  Check(f.tracker.UpdateAndGetCompletedSubmission() == 0, "NOT_READY query advanced completion");
  const auto second = f.Submit();
  Check(first.second != second.second && f.gpu.resets.empty(), "pending fence reused before completion");
  f.Finish();
}
static void DeviceLostStaysIncomplete() {
  Fixture f; const auto first = f.Submit();
  f.gpu.fences[first.second].wait = f.gpu.fences[first.second].status = VK_ERROR_DEVICE_LOST;
  Check(!f.tracker.AwaitSubmissionCompletion(first.first), "device loss reported as GPU completion");
  Check(f.tracker.UpdateAndGetCompletedSubmission() == 0, "error query advanced completion");
  f.Finish();
}
static void EarlierFallbackDoesNotProveRequestedComplete() {
  Fixture f; const auto first = f.Submit(), second = f.Submit();
  f.gpu.fences[second.second].wait = f.gpu.fences[second.second].status = VK_ERROR_DEVICE_LOST;
  Check(!f.tracker.AwaitSubmissionCompletion(second.first), "older fallback accepted for failed target");
  Check(f.tracker.UpdateAndGetCompletedSubmission() == first.first, "older successful signal lost");
  Check(f.gpu.fences[second.second].resets == 0 && f.gpu.fences[second.second].destroys == 0,
        "failed target's fence retired prematurely");
  f.Finish();
}
static void FailedSignalNeedsNewerActualSignal() {
  Fixture f;
  { auto acquisition = f.tracker.AcquireFenceToAdvanceSubmission(); acquisition.SubmissionSucceededSignalFailed(); }
  Check(f.tracker.GetCurrentSubmission() == 2, "successful commands with failed signal did not advance");
  Check(!f.tracker.AwaitSubmissionCompletion(1), "failed signal falsely proved GPU completion");
  const auto second = f.Submit(); f.gpu.fences[second.second].status = VK_SUCCESS;
  Check(f.tracker.AwaitSubmissionCompletion(1), "later actual signal did not cover failed earlier signal");
  f.Finish();
}
static void DroppedSubmissionDoesNotAdvance() {
  Fixture f; VkFence dropped;
  { auto acquisition = f.tracker.AcquireFenceToAdvanceSubmission(); dropped = acquisition.fence(); acquisition.SubmissionFailedOrDropped(); }
  Check(f.tracker.GetCurrentSubmission() == 1, "dropped submission advanced the timeline");
  Check(!f.tracker.AwaitSubmissionCompletion(1), "dropped submission reported as completed work");
  Check(f.Submit().second == dropped, "unsubmitted fence not reused");
  f.Finish();
}
static void CreationFailureNeedsActualSignal() {
  Fixture f; f.gpu.fail_create = true;
  Check(f.Submit().second == VK_NULL_HANDLE, "injected fence creation failure missed");
  Check(!f.tracker.AwaitSubmissionCompletion(1), "null fence proved completion");
  const auto second = f.Submit(); f.gpu.fences[second.second].status = VK_SUCCESS;
  Check(f.tracker.AwaitSubmissionCompletion(1), "newer real signal did not cover unfenced submission");
  f.Finish();
}
static void ResetFailurePreservesOwnership() {
  Fixture f; const auto first = f.Submit();
  f.gpu.fences[first.second].status = VK_SUCCESS;
  Check(f.tracker.UpdateAndGetCompletedSubmission() == 1, "initial signal missed");
  f.gpu.fail_reset = true; const auto second = f.Submit();
  Check(first.second != second.second, "failed fence reset reused broken fence");
  Check(f.gpu.fences[first.second].destroys == 0, "reclaimed fence ownership was lost");
  f.Finish();
}
static void ShutdownPreservesTimeline() {
  Fixture f; f.Submit(); f.Submit(); f.Finish();
  Check(f.tracker.GetCurrentSubmission() == 3 && f.tracker.UpdateAndGetCompletedSubmission() == 2,
        "Shutdown reset the public timeline");
  Check(f.tracker.AwaitSubmissionCompletion(0) && f.tracker.AwaitSubmissionCompletion(1),
        "old client references invalidated by Shutdown");
  Check(f.Submit().first == 3, "reinitialization reused a submission index");
  f.Finish();
}

int main() {
  const std::pair<const char*, void (*)()> cases[] = {
    {"zero after two completed submissions", ZeroAfterTwoCompletions},
    {"earlier and exact completed indices", EarlierThanCompleted},
    {"never-used slot avoids pending GPU work", ZeroWithPendingWork},
    {"newer observed signal covers requested work", NewerSignalCoversRequested},
    {"pending wait retires only covered fences", PendingWaitRetiresOnlyCoveredFences},
    {"timeout and NOT_READY preserve pending ownership", PendingTimeoutStaysIncomplete},
    {"NOT_READY query cannot recycle a pending fence", QueryNotReadyDoesNotRetire},
    {"device loss cannot prove completion", DeviceLostStaysIncomplete},
    {"earlier fallback cannot prove requested completion", EarlierFallbackDoesNotProveRequestedComplete},
    {"failed signal needs a later actual GPU signal", FailedSignalNeedsNewerActualSignal},
    {"dropped submission does not advance", DroppedSubmissionDoesNotAdvance},
    {"fence creation failure needs a later actual signal", CreationFailureNeedsActualSignal},
    {"reset failure preserves fence ownership", ResetFailurePreservesOwnership},
    {"Shutdown preserves timeline and fence ownership", ShutdownPreservesTimeline},
  };
  unsigned failed = 0;
  for (const auto& [name, test] : cases) {
    try { test(); std::cout << "PASS " << name << '\n'; }
    catch (const std::exception& error) { ++failed; std::cout << "FAIL " << name << ": " << error.what() << '\n'; }
  }
  std::cout << "RESULT passed=" << std::size(cases) - failed << " failed=" << failed << '\n';
  return failed ? 1 : 0;
}

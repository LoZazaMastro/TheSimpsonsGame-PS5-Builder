// Actual PS5 ReX adapter + actual direct-stack wrapper + SDK host kernel.
// These probes do not execute on PS5; only names/affinity are host adapters.
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <signal.h>
#include <semaphore.h>
#include <thread>
#include <vector>
#include <rex/thread.h>
#include <rex/logging.h>
#include <ps5platform/libc.h>
extern "C" {
#include "host_kernel.h"
int __real_sigaction(int, const struct sigaction*, struct sigaction*);
int __real_sem_destroy(sem_t*);
int __real_pthread_create(pthread_t*, const pthread_attr_t*, void*(*)(void*), void*);
int __real_pthread_join(pthread_t, void**);
int __real_pthread_cancel(pthread_t);
int __real_pthread_mutex_lock(pthread_mutex_t*);
int __real_pthread_mutex_unlock(pthread_mutex_t*);
void ps5_thread_stacks(unsigned*, unsigned*);
int ps5___cxa_thread_atexit_impl(void(*)(void*), void*, void*);
}

namespace rex {
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId(0); }
spdlog::logger* GetLoggerRaw(LogCategoryId) { return spdlog::default_logger_raw(); }
}
extern "C" int scePthreadGetaffinity(pthread_t, uint64_t* mask) { *mask=0x1fff; return 0; }
extern "C" int scePthreadSetaffinity(pthread_t, uint64_t) { return 0; }
static std::atomic<bool> block_install, install_entered, allow_install, fail_install, fail_create, fail_join;
static std::atomic<unsigned> native_creations, semaphore_destroys;
static thread_local bool observe_state_lock;
static std::atomic<pthread_mutex_t*> observed_state_mutex{nullptr};
static std::atomic<bool> hold_state_on_cancel{false};
static pthread_mutex_t* held_state_mutex;
extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex) {
  const int result=__real_pthread_mutex_lock(mutex);
  if (result==0 && observe_state_lock) observed_state_mutex.store(mutex);
  return result;
}
extern "C" int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex) {
  return __real_pthread_mutex_unlock(mutex);
}
extern "C" int __wrap_pthread_cancel(pthread_t thread) {
  if (hold_state_on_cancel.exchange(false)) {
    // Terminate has published its state and released this mutex by the time
    // it reaches pthread_cancel. Hold it on this caller to prove native
    // cleanup is independent of its availability, without abandoning a lock
    // on a canceled thread or relying on C++ forced-unwind behavior.
    held_state_mutex=observed_state_mutex.load();
    if (!held_state_mutex || __real_pthread_mutex_lock(held_state_mutex)!=0) std::abort();
  }
  return __real_pthread_cancel(thread);
}
extern "C" int __wrap_sigaction(int sig, const struct sigaction* action, struct sigaction* old) {
  if (action && sig==SIGRTMIN) {
    if (fail_install.exchange(false)) { errno=EACCES; return -1; }
    if (block_install.exchange(false)) {
      install_entered.store(true);
      while (!allow_install.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  return __real_sigaction(sig,action,old);
}
extern "C" int __wrap_sem_destroy(sem_t* sem) {
  semaphore_destroys.fetch_add(1);
  return __real_sem_destroy(sem);
}
extern "C" int ownership_native_create(pthread_t* thread, const pthread_attr_t* attr,
                                       void*(*entry)(void*), void* arg) {
  native_creations.fetch_add(1);
  if (fail_create.exchange(false)) return EAGAIN;
  return __real_pthread_create(thread,attr,entry,arg);
}
extern "C" int ownership_native_join(pthread_t thread, void** value) {
  if (fail_join.exchange(false)) return EINVAL;
  return __real_pthread_join(thread,value);
}
using namespace std::chrono_literals;
static std::atomic<unsigned> checks;
static void Check(bool okay,const char* message) {
  ++checks;
  if (!okay) { std::fprintf(stderr,"FAIL %s\n",message); std::abort(); }
}
static bool Await(const std::function<bool()>& predicate,unsigned ms=3000) {
  auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);
  while (!predicate()) {
    if (std::chrono::steady_clock::now()>=end) return false;
    std::this_thread::sleep_for(1ms);
  }
  return true;
}
static unsigned LiveStacks() { unsigned live,cached; ps5_thread_stacks(&live,&cached); return live; }
static void Retired(unsigned before=0) { Check(Await([&]{return LiveStacks()==before;}),"direct stacks retired after native exit"); }
static void Wait(rex::thread::Thread* thread) {
  Check(rex::thread::Wait(thread,false,3000ms)==rex::thread::WaitResult::kSuccess,"thread wait completes");
}
static void InstallRace(bool baseline,bool real_signal) {
  std::atomic<bool> stop{false},second_ready{false};
  std::unique_ptr<rex::thread::Thread> first,second;
  block_install.store(true);
  std::thread a([&] { first=rex::thread::Thread::Create({},[&]{while(!stop.load()) std::this_thread::sleep_for(1ms);}); });
  Check(Await([]{return install_entered.load();}),"first installer entered");
  std::thread b([&] {
    second=rex::thread::Thread::Create({},[&]{while(!stop.load()) std::this_thread::sleep_for(1ms);});
    Check(bool(second),"competing creation"); second->set_name("install-race"); second_ready.store(true);
  });
  bool bypassed=Await([&]{return second_ready.load();},200);
  struct sigaction active{};
  Check(__real_sigaction(SIGRTMIN,nullptr,&active)==0,"query real signal disposition");
  bool is_default=active.sa_handler==SIG_DFL;
  std::printf("created_before_handler=%d disposition_default=%d\n",bypassed,is_default); std::fflush(stdout);
  Check(bypassed==baseline && is_default,"concurrent creator observes expected publication rule");
  if (real_signal && baseline) {
    second->Suspend();
    std::this_thread::sleep_for(500ms);
    Check(false,"default real-time signal must terminate baseline");
  }
  allow_install.store(true); a.join(); b.join();
  Check(second->Suspend(),"suspend after completed installation");
  Check(second->Resume(),"resume after completed installation");
  stop.store(true); Wait(first.get()); Wait(second.get()); first.reset(); second.reset(); Retired();
}
static void InstallFailure() {
  fail_install.store(true);
  auto rejected=rex::thread::Thread::Create({},[]{});
  Check(!rejected && native_creations.load()==0,"failed sigaction prevents thread start");
  auto retry=rex::thread::Thread::Create({},[]{}); Check(bool(retry),"failed handler installation is retryable");
  Wait(retry.get()); retry.reset(); Retired();
}
static void JoinFailure(bool baseline) {
  auto worker=rex::thread::Thread::Create({},[]{});
  Check(bool(worker),"create worker for native join failure");
  fail_join.store(true);
  const auto result=rex::thread::Wait(worker.get(),false,3000ms);
  const bool reported_success=result==rex::thread::WaitResult::kSuccess;
  std::printf("failed_join_reported_success=%d\n",reported_success);
  Check(baseline && reported_success && !fail_join.load(),
        "only baseline reports a successful wait after the native join failed");
  // Baseline kept the native thread unjoined. Reclaim it with a second wait
  // after the one-shot fault, without mistaking fixture leakage for the bug.
  Wait(worker.get()); worker.reset(); Retired();
}
static void CloseFinished(bool baseline) {
  std::vector<pthread_t> leaked;
  for (unsigned i=0;i<16;++i) {
    auto worker=rex::thread::Thread::Create({.stack_size=16*1024*1024},[]{});
    Check(bool(worker),"create completed worker");
    leaked.push_back(reinterpret_cast<pthread_t>(worker->native_handle()));
    Check(Await([&]{return worker->affinity_mask()==0;}),"worker reaches finished state");
    std::this_thread::sleep_for(5ms);
    worker.reset();
    if (!baseline) Retired();
  }
  unsigned live=LiveStacks(),cached=0,unused=0; ps5_thread_stacks(&unused,&cached);
  std::printf("closed=16 live_direct_stacks=%u cached_stacks=%u direct_allocations=%lld semaphore_destroys=%u\n",
              live,cached,host_direct_allocations(),semaphore_destroys.load());
  if (baseline) {
    Check(live==16,"baseline retains sixteen completed direct stacks");
    for (auto thread:leaked) Check(pthread_join(thread,nullptr)==0,"test-only baseline reclamation");
  } else {
    Check(live==0 && cached<=4 && host_direct_allocations()<=4,"completed close reaches a bounded stack cache");
    Check(semaphore_destroys.load()==16,"completed close destroys each semaphore once");
  }
}
struct TlsCheck {
  rex::thread::Thread* current;
  std::atomic<bool>* ran;
  std::weak_ptr<int> execution_owner;
};
static void TlsDestructor(void* raw) {
  auto* check=static_cast<TlsCheck*>(raw);
  Check(!check->execution_owner.expired(),"TLS destructor retains closure-owned execution objects");
  Check(rex::thread::Thread::GetCurrentThread()==check->current,"TLS destructor retains running facade");
  (void)check->current->priority();
  check->ran->store(true);
  delete check;
}
static void CloseActive(const char* mode) {
  std::atomic<bool> entered{false},proceed{false},tls_ran{false},after_close{false};
  auto capture=std::make_shared<int>(42); std::weak_ptr<int> weak=capture;
  std::unique_ptr<rex::thread::Thread> worker;
  worker=rex::thread::Thread::Create({.stack_size=16*1024*1024},[&,held=std::move(capture)]{
    auto* current=rex::thread::Thread::GetCurrentThread();
    Check(ps5___cxa_thread_atexit_impl(TlsDestructor,new TlsCheck{current,&tls_ran,weak},nullptr)==0,
          "register native TLS destructor");
    entered.store(true);
    while(!proceed.load()) std::this_thread::sleep_for(1ms);
    if (!std::strcmp(mode,"self-close")) worker.reset();
    Check(rex::thread::Thread::GetCurrentThread()==current,"running facade survives public close");
    Check(current->priority()==rex::thread::ThreadPriority::kNormal,"live condition survives public close");
    after_close.store(true);
    if (!std::strcmp(mode,"close-exit")) rex::thread::Thread::Exit(23);
  });
  Check(bool(worker)&&Await([&]{return entered.load();}),"active worker entered");
  if (std::strcmp(mode,"self-close")) {
    auto begin=std::chrono::steady_clock::now(); worker.reset();
    auto elapsed=std::chrono::steady_clock::now()-begin;
    Check(elapsed<100ms,"CloseHandle does not wait for active worker");
    Check(!weak.expired() && LiveStacks()==1,"closure and stack retained while worker blocked");
  }
  proceed.store(true);
  Check(Await([&]{return weak.expired();}),"closure released by native cleanup");
  Retired();
  Check(after_close.load()&&tls_ran.load(),"work and TLS cleanup finish after public close");
  Check(semaphore_destroys.load()==1,"active close destroys semaphore once");
}
static void TerminateClose(bool wait_first) {
  for (unsigned i=0;i<32;++i) {
    std::atomic<bool> unexpected{false};
    auto capture=std::make_shared<int>(42); std::weak_ptr<int> weak=capture;
    auto worker=rex::thread::Thread::Create({.stack_size=16*1024*1024,.create_suspended=true},
        [&,held=std::move(capture)]{unexpected.store(true);});
    Check(bool(worker),"create canceled worker");
    worker->Terminate(13);
    if (wait_first) { Wait(worker.get()); Wait(worker.get()); }
    worker.reset();
    Check(Await([&]{return weak.expired();}),"cancellation releases captured closure");
    Retired(); Check(!unexpected.load(),"canceled suspended worker never executes");
  }
  Check(semaphore_destroys.load()==32,"cancellation destroys each semaphore once");
}
static void CreationFailure() {
  auto capture=std::make_shared<int>(42); std::weak_ptr<int> weak=capture;
  fail_create.store(true);
  auto worker=rex::thread::Thread::Create({},[held=std::move(capture)]{});
  Check(!worker&&weak.expired(),"rejected native creation releases both facades and closure");
  Retired(); Check(semaphore_destroys.load()==1,"creation failure destroys semaphore once");
}
static void NativeFinished(void* value) { static_cast<std::atomic<bool>*>(value)->store(true); }
static void TerminateLockedState() {
  pthread_key_t key;
  Check(pthread_key_create(&key,NativeFinished)==0,"create native finish key");
  std::atomic<bool> entered{false},native_finished{false};
  auto worker=rex::thread::Thread::Create({},[&]{
    auto* current=rex::thread::Thread::GetCurrentThread();
    observe_state_lock=true;
    (void)current->priority();
    observe_state_lock=false;
    Check(pthread_setspecific(key,&native_finished)==0,"publish native finish flag");
    entered.store(true);
    for (;;) std::this_thread::sleep_for(1ms);
  });
  Check(bool(worker) && Await([&]{return entered.load();}),"worker publishes its actual state mutex");
  hold_state_on_cancel.store(true);
  worker->Terminate(17);
  const bool finished_while_locked=Await([&]{return native_finished.load();},500);
  Check(held_state_mutex && __real_pthread_mutex_unlock(held_state_mutex)==0,"release owned probe lock");
  Check(finished_while_locked,"native cancellation cleanup never waits for the terminated state mutex");
  Wait(worker.get()); worker.reset(); Retired();
  Check(pthread_key_delete(key)==0,"release native finish key");
}
int main(int argc,char**argv) {
  Check(argc>=2,"case argument"); bool baseline=argc>2 && !std::strcmp(argv[2],"baseline");
  const char* mode=argv[1];
  if (!std::strcmp(mode,"install")||!std::strcmp(mode,"install-signal")) InstallRace(baseline,!std::strcmp(mode,"install-signal"));
  else if (!std::strcmp(mode,"install-failure")) InstallFailure();
  else if (!std::strcmp(mode,"join-failure")) JoinFailure(baseline);
  else if (!std::strcmp(mode,"close-finished")) CloseFinished(baseline);
  else if (!std::strcmp(mode,"close-running")||!std::strcmp(mode,"self-close")||!std::strcmp(mode,"close-exit")) CloseActive(mode);
  else if (!std::strcmp(mode,"terminate-close")) TerminateClose(false);
  else if (!std::strcmp(mode,"terminate-wait")) TerminateClose(true);
  else if (!std::strcmp(mode,"create-failure")) CreationFailure();
  else if (!std::strcmp(mode,"terminate-locked-state")) TerminateLockedState();
  else if (!std::strcmp(mode,"native-fallback")) {
    host_fail(HOST_CALL_ALLOCATE,-1);
    // The native fallback has no direct stacks, but all ownership still applies.
    auto worker=rex::thread::Thread::Create({},[]{}); Check(bool(worker),"native fallback create");
    Wait(worker.get()); worker.reset(); Retired();
  } else return 2;
  std::printf("PASS %s checks=%u hardware=not_run\n",mode,checks.load());
}

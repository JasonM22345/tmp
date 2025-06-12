#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <x86intrin.h>

constexpr int kRSBSize = 4;
constexpr int kNumGadgets = 8;
constexpr int kPageSize = 4096;
constexpr int CACHE_HIT_THRESHOLD = 80;

// 256 * 4KB reload buffer, aligned to page size
uint8_t reloadbuffer[256 * kPageSize] __attribute__((aligned(4096)));

// Touches a byte to bring its cache line into L1
__attribute__((noinline)) void Touch(uint8_t *addr) {
  volatile uint8_t val = *addr;
}

// Gadget functions: Touch a reloadbuffer index
__attribute__((noinline)) void Gadget0() { Touch(&reloadbuffer[0 * kPageSize]); }
__attribute__((noinline)) void Gadget1() { Touch(&reloadbuffer[1 * kPageSize]); }
__attribute__((noinline)) void Gadget2() { Touch(&reloadbuffer[2 * kPageSize]); }
__attribute__((noinline)) void Gadget3() { Touch(&reloadbuffer[3 * kPageSize]); }
__attribute__((noinline)) void Gadget4() { Touch(&reloadbuffer[4 * kPageSize]); }
__attribute__((noinline)) void Gadget5() { Touch(&reloadbuffer[5 * kPageSize]); }
__attribute__((noinline)) void Gadget6() { Touch(&reloadbuffer[6 * kPageSize]); }
__attribute__((noinline)) void Gadget7() { Touch(&reloadbuffer[7 * kPageSize]); }

using GadgetFn = void(*)();
std::array<GadgetFn, kNumGadgets> gadgets = {{
  Gadget0, Gadget1, Gadget2, Gadget3,
  Gadget4, Gadget5, Gadget6, Gadget7
}};

// Evicts memory from cache
void Flush(void *addr) {
  _mm_clflush(addr);
}

// Measures access time to addr
bool ReloadAccess(uint8_t *addr) {
  unsigned int junk;
  uint64_t start = __rdtscp(&junk);
  volatile uint8_t val = *addr;
  uint64_t end = __rdtscp(&junk);
  return (end - start) < CACHE_HIT_THRESHOLD;
}

// Fills the RSB with calls to 4 gadgets
void FillRSB() {
  gadgets[0](); // ret@Gadget0
  gadgets[1](); // ret@Gadget1
  gadgets[2](); // ret@Gadget2
  gadgets[3](); // ret@Gadget3
}

// Overwrites return address on stack and executes return (no real call)
__attribute__((noinline)) void TriggerRSBA(int fake_return_target) {
  uint64_t *ret_addr = (uint64_t *)__builtin_frame_address(0) + 1;
  *ret_addr = (uint64_t)&&skip;

  // Force RSB underflow with extra return
  asm volatile("ret");

skip:
  // Execution resumes here architecturally
  asm volatile("" ::: "memory");
}

// Flush all reloadbuffer lines
void FlushReloadBuffer() {
  for (int i = 0; i < kNumGadgets; ++i)
    Flush(&reloadbuffer[i * kPageSize]);
}

int main() {
  for (int i = 0; i < kNumGadgets; ++i) {
    printf("Testing RSBA speculation to Gadget%d\n", i);

    for (int trial = 0; trial < 10000; ++trial) {
      FlushReloadBuffer();   // Step 0: Flush
      FillRSB();             // Step 1: Fill RSB with 4 gadgets
      TriggerRSBA(i);        // Step 2: Underflow & trigger speculation

      if (ReloadAccess(&reloadbuffer[i * kPageSize])) {
        printf("RSBA predicted Gadget%d (trial %d)\n", i, trial);
        break;
      }
    }
  }

  return 0;
}

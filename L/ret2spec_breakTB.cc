#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <x86intrin.h>

constexpr size_t kNumRSBEntries = 32;
constexpr size_t kPageSize = 4096;
constexpr int CACHE_HIT_THRESHOLD = 80;

uint8_t probe_array[kNumRSBEntries * kPageSize] __attribute__((aligned(4096)));

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
  volatile uint8_t dummy = probe_array[i * kPageSize];
}

// Fills the RSB with distinct return addresses pointing to different gadgets
void FillRSBWithGadgets() {
  for (size_t i = 0; i < kNumRSBEntries; ++i) {
    RSBGadget(i);  // Each call pushes a return address
  }
}

// Flush the probe_array from the cache
void FlushProbeArray() {
  for (size_t i = 0; i < kNumRSBEntries; ++i) {
    _mm_clflush(&probe_array[i * kPageSize]);
  }
}

// Detects which RSB entry was speculatively used
int DetectUsedRSBEntry() {
  for (size_t i = 0; i < kNumRSBEntries; ++i) {
    if (ReloadAccess(&probe_array[i * kPageSize])) {
      return i;
    }
  }
  return -1;  // Nothing hit
}

// Dummy misprediction trigger: cause speculative return to a gadget
__attribute__((noinline)) void TriggerFalseReturn(int depth) {
  if (depth > 0) {
    TriggerFalseReturn(depth - 1);
  } else {
    asm volatile("lfence; ret");
  }
}

// Wrapper that mispredicts by corrupting RSB
__attribute__((noinline)) void TriggerTrueReturn(int depth) {
  if (depth > 0) {
    TriggerTrueReturn(depth - 1);
  } else {
    TriggerFalseReturn(kNumRSBEntries);  // Fills and pollutes RSB
  }
}

int main() {
  std::cout << "[+] Starting RSB tracking experiment...\n";

  FillRSBWithGadgets();            // Step 1: Fill RSB with gadgets
  FlushProbeArray();               // Step 2: Flush probe_array
  TriggerTrueReturn(kNumRSBEntries); // Step 3: Trigger mispredicted return

  int index = DetectUsedRSBEntry();
  if (index >= 0) {
    std::cout << "[+] Speculative execution used RSB entry: " << index << "\n";
  } else {
    std::cout << "[-] No RSB entry hit detected.\n";
  }

  return 0;
}

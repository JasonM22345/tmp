/*
 * Copyright 2019 Google LLC
 *
 * Licensed under both the 3-Clause BSD License and the GPLv2, found in the
 * LICENSE and LICENSE.GPL-2.0 files, respectively, in the root directory.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
 */

// See note in `README.md` about underscores.
#ifdef __ELF__
#  define DECORATE(f) f
#elif __APPLE__
#  define DECORATE(f) _##f
#endif

.intel_syntax noprefix

.global MeasureReadLatency
// uint64_t MeasureReadLatency(const void* address);
MeasureReadLatency:
  // rdi = address

  // Full memory and speculation barrier
  mfence
  lfence

  // edx:eax = <time-stamp counter>
  rdtsc

  // rax = edx:eax
  shl rdx, 32
  or rax, rdx

  // r8 = rax
  mov r8, rax

  // Finish reading the timestamp before starting the read.
  lfence

  // Read *rdi
  mov al, byte ptr [rdi]

  // Finish the read before reading timestamp again
  lfence

  // edx:eax = <time-stamp counter>
  rdtsc

  // rax = edx:eax
  shl rdx, 32
  or rax, rdx

  // rax -= r8
  sub rax, r8

  ret

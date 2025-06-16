/*
 * Copyright 2019 Google LLC
 *
 * Licensed under both the 3-Clause BSD License and the GPLv2, found in the
 * LICENSE and LICENSE.GPL-2.0 files, respectively, in the root directory.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
 */

#ifndef DEMOS_ASM_MEASUREREADLATENCY_H_
#define DEMOS_ASM_MEASUREREADLATENCY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reads a byte from *address and returns the memory read latency in cycles.
uint64_t MeasureReadLatency(const void* address);

#ifdef __cplusplus
}
#endif

#endif  // DEMOS_ASM_MEASUREREADLATENCY_H_

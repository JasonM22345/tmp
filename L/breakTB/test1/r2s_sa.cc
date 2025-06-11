#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "cache_sidechannel.h"
#include "instr.h"
#include "local_content.h"
#include "ret2spec_common.h"
#include "utils.h"

int main() {
  std::cout << "Tracking RSB entries used during speculative returns:\n";

  for (int trial = 0; trial < 100; ++trial) {
    FlushProbeArray();
    FillRSB();              // Push return addresses (aka gadgets)
    MispredictReturnSite(); // Return with poisoned stack

    int hit = DetectRSBHit();
    if (hit != -1) {
      std::cout << "Trial " << trial << ": RSB Entry " << hit << " executed speculatively.\n";
    } else {
      std::cout << "Trial " << trial << ": No speculative hit detected.\n";
    }
  }

  std::cout << "\nDone!\n";
  return 0;
}

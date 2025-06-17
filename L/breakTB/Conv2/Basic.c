// main.c
#include <stdio.h>
#include "cache_sidechannel_wrapper.h"

int main() {
    CSCHandle channel = CSC_Create();

    CSC_FlushOracle(channel);

    char result;
    if (CSC_RecomputeScores(channel, 'A', &result)) {
        printf("Speculative access detected char: %c\n", result);
    } else {
        printf("Insufficient confidence. Top guess: %c\n", result);
    }

    if (CSC_AddHitAndRecomputeScores(channel, &result)) {
        printf("After adding artificial hit, top guess: %c\n", result);
    }

    CSC_Destroy(channel);
    return 0;
}

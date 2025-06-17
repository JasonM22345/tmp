// cache_sidechannel_wrapper.cc
#include "cache_sidechannel.h"

extern "C" {

// Opaque type for C
typedef void* CSCHandle;

// Create and destroy the CacheSideChannel object
CSCHandle CSC_Create() {
    return new CacheSideChannel();
}

void CSC_Destroy(CSCHandle obj) {
    delete static_cast<CacheSideChannel*>(obj);
}

// Call FlushOracle
void CSC_FlushOracle(CSCHandle obj) {
    static_cast<CacheSideChannel*>(obj)->FlushOracle();
}

// Call RecomputeScores
bool CSC_RecomputeScores(CSCHandle obj, char safe_offset_char, char *result) {
    auto [success, value] = static_cast<CacheSideChannel*>(obj)->RecomputeScores(safe_offset_char);
    *result = value;
    return success;
}

// Call AddHitAndRecomputeScores
bool CSC_AddHitAndRecomputeScores(CSCHandle obj, char *result) {
    auto [success, value] = static_cast<CacheSideChannel*>(obj)->AddHitAndRecomputeScores();
    *result = value;
    return success;
}

}

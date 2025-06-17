// cache_sidechannel_wrapper.h
#ifndef CACHE_SIDEBAND_WRAPPER_H
#define CACHE_SIDEBAND_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* CSCHandle;

CSCHandle CSC_Create();
void CSC_Destroy(CSCHandle obj);

void CSC_FlushOracle(CSCHandle obj);
bool CSC_RecomputeScores(CSCHandle obj, char safe_offset_char, char *result);
bool CSC_AddHitAndRecomputeScores(CSCHandle obj, char *result);

#ifdef __cplusplus
}
#endif

#endif  // CACHE_SIDEBAND_WRAPPER_H

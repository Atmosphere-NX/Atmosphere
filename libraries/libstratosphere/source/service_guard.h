#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch/types.h>
#include <switch/result.h>
#include <switch/kernel/mutex.h>
#include <switch/sf/service.h>
#include <switch/services/sm.h>

typedef struct ServiceGuard {
    Mutex mutex;
    u32 refCount;
} ServiceGuard;

NX_INLINE bool serviceGuardBeginInit(ServiceGuard* g)
{
    mutexLock(&g->mutex);
    return (g->refCount++) == 0;
}

NX_INLINE Result serviceGuardEndInit(ServiceGuard* g, Result rc, void (*cleanupFunc)(void))
{
    if (R_FAILED(rc)) {
        cleanupFunc();
        --g->refCount;
    }
    mutexUnlock(&g->mutex);
    return rc;
}

NX_INLINE void serviceGuardExit(ServiceGuard* g, void (*cleanupFunc)(void))
{
    mutexLock(&g->mutex);
    if (g->refCount && (--g->refCount) == 0)
        cleanupFunc();
    mutexUnlock(&g->mutex);
}

#define NX_GENERATE_SERVICE_GUARD_PARAMS(name, _paramdecl, _parampass) \
\
static ServiceGuard g_##name##Guard; \
NX_INLINE Result _##name##Initialize _paramdecl; \
static void _##name##Cleanup(void); \
\
Result name##Initialize _paramdecl \
{ \
    Result rc = 0; \
    if (serviceGuardBeginInit(&g_##name##Guard)) \
        rc = _##name##Initialize _parampass; \
    return serviceGuardEndInit(&g_##name##Guard, rc, _##name##Cleanup); \
} \
\
void name##Exit(void) \
{ \
    serviceGuardExit(&g_##name##Guard, _##name##Cleanup); \
}

#define NX_GENERATE_SERVICE_GUARD(name) NX_GENERATE_SERVICE_GUARD_PARAMS(name, (void), ())

#ifdef __cplusplus
}
#endif

#include <switch.h>
#include <stratosphere.hpp>
#include <switch.h>
#include <stratosphere.hpp>
#include <stdio.h>
#include "ldr_rei.hpp"
u64 HbOverrideTid = 0x010000000000100D;
Result RNXService::GetReiNXVersion(Out<u32> maj, Out<u32> min) {
    // PASS IN 2 and 4 to trick reinx appls. This is to trick apps into working with ams
    *maj.GetPointer() = 2;
    *min.GetPointer() = 4;
    return 0;
}
// For deltalaunch
Result RNXService::SetHbTidForDelta(u64 tid) {
    // Hardcode the tid
    HbOverrideTid = tid;
    return 0;
}

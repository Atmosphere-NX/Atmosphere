#include <switch.h>
#include <stratosphere.hpp>
#include <stdio.h>
#include "ldr_rei.hpp"
#include "ldr_content_management.hpp"
#include <strings.h>
#include <vector>
#include <algorithm>
#include <map>
#include "ldr_registration.hpp"
#include "ldr_hid.hpp"
#include "ldr_npdm.hpp"

#include "ini.h"


u64 HbOverrideTid = 0x010000000000100D;
Result RNXService::GetReiNXVersion(Out<u32> maj, Out<u32> min) {
    // PASS IN 2 and 4 to trick reinx applications. This is to trick apps into working with ams
    *maj.GetPointer() = 2;
    *min.GetPointer() = 4;
    return 0;
}
// For deltalaunch
Result RNXService::SetHbTidForDelta(u64 tid) {
    // Hardcode the tid
    HbOverrideTid = tid;
    // Then mount the nsp
    ContentManagement::TryMountHblNspOnSd();
    ContentManagement::ShouldOverrideContentsWithSD(tid);
    return tid;
}

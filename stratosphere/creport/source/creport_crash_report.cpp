#include <switch.h>
#include "creport_crash_report.hpp"

void CrashReport::BuildReport(u64 pid, bool has_extra_info) {
    this->has_extra_info = has_extra_info;
    if (OpenProcess(pid)) {
        /* TODO: Actually generate report... */
        Close();
    }
}
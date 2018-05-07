#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class ResourceLimitUtils {
    public:
        enum ResourceLimitCategory {
            ResourceLimitCategory_System = 0,
            ResourceLimitCategory_Application = 1,
            ResourceLimitCategory_Applet = 2
        };
        static void InitializeLimits();
        static void EnsureApplicationResourcesAvailable();
        static Handle GetResourceLimitHandle(u16 application_type);
        static Result BoostSystemMemoryResourceLimit(u64 boost_size);
};
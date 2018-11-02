#pragma once

#include <mesosphere/core/types.hpp>

namespace mesosphere
{

struct Result {
    enum class Module : uint {
        None    = 0,
        Kernel  = 1,
        /* Other modules not included. */
    };

    enum class Description : uint {
        None                        = 0,

        InvalidCapabilityDescriptor = 14,

        NotImplemented              = 33,

        ThreadTerminated            = 59,

        OutOfDebugEvents            = 70,

        InvalidSize                 = 101,
        InvalidAddress              = 102,
        AllocatorDepleted           = 103,
        OutOfMemory                 = 104,
        OutOfHandles                = 105,
        InvalidMemoryState          = 106,
        InvalidMemoryPermissions    = 108,
        InvalidMemoryRange          = 110,
        InvalidPriority             = 112,
        InvalidCoreId               = 113,
        InvalidHandle               = 114,
        InvalidUserBuffer           = 115,
        InvalidCombination          = 116,
        TimedOut                    = 117,
        Cancelled                   = 118,
        OutOfRange                  = 119,
        InvalidEnumValue            = 120,
        NotFound                    = 121,
        AlreadyExists               = 122,
        ConnectionClosed            = 123,
        UnhandledUserInterrupt      = 124,
        NotPermitted                = 125,
        ReservedValue               = 126,
        InvalidHwBreakpoint         = 127,
        FatalUserException          = 128,
        OwnedByAnotherProcess       = 129,
        ConnectionRefused           = 131,
        OutOfResource               = 132,

        IpcMapFailed                = 259,
        IpcCmdbufTooSmall           = 260,

        NotDebugged                 = 520,
    };

    Module module           : 9;
    Description description : 23;

    Result() : module{Module::None}, description{Description::None} {}
    Result(Description description, Module module = Module::Kernel) : description{description}, module{module} {}

    constexpr bool IsSuccess() const { return module == Module::None && description == Description::None; }
    constexpr bool operator==(const Result &other) const { return module == other.module && description == other.description; }
    constexpr bool operator!=(const Result &other) const { return !(*this == other); }
};

}

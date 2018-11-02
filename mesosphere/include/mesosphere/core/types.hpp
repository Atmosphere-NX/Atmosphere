#pragma once

#include <cstddef>
#include <cstdint>
#include <climits>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#define MAX_CORES 4

namespace mesosphere
{

using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

using std::size_t;

using uiptr = std::uintptr_t;
using iptr = std::intptr_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using vu8  = volatile uint8_t;
using vu16 = volatile uint16_t;
using vu32 = volatile uint32_t;
using vu64 = volatile uint64_t;

using vs8  = volatile int8_t;
using vs16 = volatile int16_t;
using vs32 = volatile int32_t;
using vs64 = volatile int64_t;

template <typename T>
using SharedPtr = boost::intrusive_ptr<T>;

struct Handle {
    u16 index       : 15;
    s16 id          : 16;
    bool isAlias    : 1;

    constexpr bool IsAliasOrFree() const { return isAlias || id < 0; }

    constexpr bool operator==(const Handle &other) const
    {
        return index == other.index && id == other.id && isAlias == other.isAlias;
    }

    constexpr bool operator!=(const Handle &other) const { return !(*this == other); }
};

}

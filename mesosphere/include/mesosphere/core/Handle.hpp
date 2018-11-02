#pragma once

#include <mesosphere/core/types.hpp>

namespace mesosphere
{

class Handle final {
    public:
    constexpr bool IsAliasOrFree() const { return isAlias || id < 0; }

    constexpr bool operator==(const Handle &other) const
    {
        return index == other.index && id == other.id && isAlias == other.isAlias;
    }

    constexpr bool operator!=(const Handle &other) const { return !(*this == other); }

    constexpr Handle() : index{0}, id{0}, isAlias{false} {}

    private:
    friend class KHandleTable;
    constexpr Handle(u16 index, s16 id, bool isAlias = false) : index{index}, id{id}, isAlias{isAlias} {}
    u32 index       : 15;
    s32 id          : 16;
    u32 isAlias     : 1;
};

}

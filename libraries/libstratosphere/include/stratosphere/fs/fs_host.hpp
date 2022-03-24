/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/fs/fs_code_verification_data.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */
    enum MountHostOptionFlag : u32 {
        MountHostOptionFlag_None                = (0 << 0),
        MountHostOptionFlag_PseudoCaseSensitive = (1 << 0),
    };

    struct MountHostOption {
        u32 _value;

        constexpr inline bool HasPseudoCaseSensitiveFlag() const {
            return _value & MountHostOptionFlag_PseudoCaseSensitive;
        }

        static const MountHostOption None;
        static const MountHostOption PseudoCaseSensitive;
    };

    inline constexpr const MountHostOption MountHostOption::None                = {MountHostOptionFlag_None};
    inline constexpr const MountHostOption MountHostOption::PseudoCaseSensitive = {MountHostOptionFlag_PseudoCaseSensitive};

    inline constexpr bool operator==(const MountHostOption &lhs, const MountHostOption &rhs) {
        return lhs._value == rhs._value;
    }

    inline constexpr bool operator!=(const MountHostOption &lhs, const MountHostOption &rhs) {
        return !(lhs == rhs);
    }

    Result MountHost(const char *name, const char *root_path);
    Result MountHost(const char *name, const char *root_path, const MountHostOption &option);

    Result MountHostRoot();
    Result MountHostRoot(const MountHostOption &option);

    void UnmountHostRoot();

}

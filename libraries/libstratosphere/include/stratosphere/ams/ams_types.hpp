/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include "../sf/sf_buffer_tags.hpp"
#include "../hos.hpp"

namespace ams::exosphere {

    using TargetFirmware = ams::TargetFirmware;

    constexpr ALWAYS_INLINE u32 GetVersion(u32 major, u32 minor, u32 micro) {
        return (major << 16) | (minor << 8) | (micro);
    }

    struct ApiInfo {
        using TargetFirmwareVersion = util::BitPack64::Field<0,                           32, TargetFirmware>;
        using MasterKeyRevision     = util::BitPack64::Field<TargetFirmwareVersion::Next,  8, u32>;
        using MicroVersion          = util::BitPack64::Field<MasterKeyRevision::Next,      8, u32>;
        using MinorVersion          = util::BitPack64::Field<MicroVersion::Next,           8, u32>;
        using MajorVersion          = util::BitPack64::Field<MinorVersion::Next,           8, u32>;

        util::BitPack64 value;

        constexpr ALWAYS_INLINE u32 GetVersion() const {
            return ::ams::exosphere::GetVersion(this->GetMajorVersion(), this->GetMinorVersion(), this->GetMicroVersion());
        }

        constexpr ALWAYS_INLINE u32 GetMajorVersion() const {
            return this->value.Get<MajorVersion>();
        }

        constexpr ALWAYS_INLINE u32 GetMinorVersion() const {
            return this->value.Get<MinorVersion>();
        }

        constexpr ALWAYS_INLINE u32 GetMicroVersion() const {
            return this->value.Get<MicroVersion>();
        }

        constexpr ALWAYS_INLINE TargetFirmware GetTargetFirmware() const {
            return this->value.Get<TargetFirmwareVersion>();
        }

        constexpr ALWAYS_INLINE u32 GetMasterKeyRevision() const {
            return this->value.Get<MasterKeyRevision>();
        }
    };

}

namespace ams {

    struct FatalErrorContext : ::ams::impl::FatalErrorContext, sf::LargeData, sf::PrefersMapAliasTransferMode {};

    static_assert(sizeof(FatalErrorContext) == sizeof(::ams::impl::FatalErrorContext));

#ifdef ATMOSPHERE_GIT_BRANCH
    NX_CONSTEXPR const char *GetGitBranch() {
        return ATMOSPHERE_GIT_BRANCH;
    }
#endif

#ifdef ATMOSPHERE_GIT_REVISION
    NX_CONSTEXPR const char *GetGitRevision() {
        return ATMOSPHERE_GIT_REVISION;
    }
#endif

}

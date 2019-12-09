/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

    #define AMS_DEFINE_TARGET_FIRMWARE_ENUM(n) TargetFirmware_##n = ATMOSPHERE_TARGET_FIRMWARE_##n
    enum TargetFirmware : u32 {
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(100),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(200),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(300),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(400),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(500),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(600),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(620),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(700),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(800),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(810),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(900),
        AMS_DEFINE_TARGET_FIRMWARE_ENUM(910),
    };
    #undef AMS_DEFINE_TARGET_FIRMWARE_ENUM

    constexpr inline u32 GetVersion(u32 major, u32 minor, u32 micro) {
        return (major << 16) | (minor << 8) | (micro);
    }

    struct ApiInfo {
        u32 major_version;
        u32 minor_version;
        u32 micro_version;
        TargetFirmware target_firmware;
        u32 master_key_revision;

        constexpr u32 GetVersion() const {
            return ::ams::exosphere::GetVersion(this->major_version, this->minor_version, this->micro_version);
        }

        constexpr TargetFirmware GetTargetFirmware() const {
            return this->target_firmware;
        }

        constexpr u32 GetMasterKeyRevision() const {
            return this->master_key_revision;
        }
    };

}

namespace ams {

    struct FatalErrorContext : sf::LargeData, sf::PrefersMapAliasTransferMode {
        static constexpr size_t MaxStackTrace = 0x20;
        static constexpr size_t MaxStackDumpSize = 0x100;
        static constexpr size_t NumGprs = 29;
        static constexpr uintptr_t StdAbortMagicAddress = 0x8;
        static constexpr u64       StdAbortMagicValue = 0xA55AF00DDEADCAFEul;
        static constexpr u32       StdAbortErrorDesc = 0xFFE;
        static constexpr u32       DataAbortErrorDesc = 0x101;
        static constexpr u32       Magic = 0x31454641;

        u32 magic;
        u32 error_desc;
        u64 program_id;
        union {
            u64 gprs[32];
            struct {
                u64 _gprs[29];
                u64 fp;
                u64 lr;
                u64 sp;
            };
        };
        u64 pc;
        u64 module_base;
        u32 pstate;
        u32 afsr0;
        u32 afsr1;
        u32 esr;
        u64 far;
        u64 report_identifier; /* Normally just system tick. */
        u64 stack_trace_size;
        u64 stack_dump_size;
        u64 stack_trace[MaxStackTrace];
        u8 stack_dump[MaxStackDumpSize];
    };

    static_assert(sizeof(FatalErrorContext) == 0x350, "sizeof(FatalErrorContext)");
    static_assert(std::is_pod<FatalErrorContext>::value, "FatalErrorContext");

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

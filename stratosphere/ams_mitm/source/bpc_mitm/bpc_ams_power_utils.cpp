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
#include <stratosphere.hpp>
#include "bpc_ams_power_utils.hpp"
#include "../amsmitm_fs_utils.hpp"

namespace ams::mitm::bpc {

    namespace {

        /* Convenience definitions. */
        constexpr uintptr_t IramBase = 0x40000000ull;
        constexpr uintptr_t IramPayloadBase = 0x40010000ull;
        constexpr size_t IramSize = 0x40000;
        constexpr size_t IramPayloadMaxSize = 0x24000;
        constexpr size_t IramFatalErrorContextOffset = 0x2E000;

        /* Helper enum. */
        enum class RebootType : u32 {
            Standard,
            ToRcm,
            ToPayload,
        };

        /* Globals. */
        alignas(os::MemoryPageSize) u8 g_work_page[os::MemoryPageSize];
        alignas(os::MemoryPageSize) u8 g_reboot_payload[IramPayloadMaxSize];
        RebootType g_reboot_type = RebootType::ToRcm;

        /* Helpers. */
        void ClearIram() {
            /* Make page CCs. */
            std::memset(g_work_page, 0xCC, sizeof(g_work_page));

            /* Overwrite all of IRAM with CCs. */
            for (size_t ofs = 0; ofs < IramSize; ofs += sizeof(g_work_page)) {
                exosphere::CopyToIram(IramBase + ofs, g_work_page, sizeof(g_work_page));
            }
        }

        void DoRebootToPayload() {
            /* Ensure clean IRAM state. */
            ClearIram();

            /* Copy in payload. */
            for (size_t ofs = 0; ofs < sizeof(g_reboot_payload); ofs += sizeof(g_work_page)) {
                std::memcpy(g_work_page, &g_reboot_payload[ofs], std::min(sizeof(g_reboot_payload) - ofs, sizeof(g_work_page)));
                exosphere::CopyToIram(IramPayloadBase + ofs, g_work_page, sizeof(g_work_page));
            }

            exosphere::ForceRebootToIramPayload();
        }

        void DoRebootToFatalError(const ams::FatalErrorContext *ctx) {
            /* Ensure clean IRAM state. */
            ClearIram();

            /* Copy in payload. */
            for (size_t ofs = 0; ofs < sizeof(g_reboot_payload); ofs += sizeof(g_work_page)) {
                std::memcpy(g_work_page, &g_reboot_payload[ofs], std::min(sizeof(g_reboot_payload) - ofs, sizeof(g_work_page)));
                exosphere::CopyToIram(IramPayloadBase + ofs, g_work_page, sizeof(g_work_page));
            }

            /* Copy in fatal error context, if relevant. */
            if (ctx != nullptr) {
                std::memset(g_work_page, 0xCC, sizeof(g_work_page));
                std::memcpy(g_work_page, ctx, sizeof(*ctx));
                exosphere::CopyToIram(IramPayloadBase + IramFatalErrorContextOffset, g_work_page, sizeof(g_work_page));
            }

            exosphere::ForceRebootToFatalError();
        }

    }

    /* Power utilities. */
    bool IsRebootManaged() {
        return g_reboot_type != RebootType::Standard;
    }

    void RebootSystem() {
        switch (g_reboot_type) {
            case RebootType::ToRcm:
                exosphere::ForceRebootToRcm();
                break;
            case RebootType::ToPayload:
            default: /* This should never be called with ::Standard */
                DoRebootToPayload();
                break;
        }
    }

    void ShutdownSystem() {
        exosphere::ForceShutdown();
    }

    /* Atmosphere power utilities. */
    void RebootForFatalError(const ams::FatalErrorContext *ctx) {
        DoRebootToFatalError(ctx);
    }

    void SetRebootPayload(const void *payload, size_t payload_size) {
        /* Clear payload buffer */
        std::memset(g_reboot_payload, 0xCC, sizeof(g_reboot_payload));

        /* Ensure valid. */
        AMS_ABORT_UNLESS(payload != nullptr && payload_size <= sizeof(g_reboot_payload));

        /* Copy in payload. */
        std::memcpy(g_reboot_payload, payload, payload_size);

        /* Note to the secure monitor that we have a payload. */
        spl::smc::SetConfig(spl::ConfigItem::ExospherePayloadAddress, g_reboot_payload, nullptr, 0);

        /* NOTE: Preferred reboot type may be overrwritten when parsed from settings during boot. */
        g_reboot_type = RebootType::ToPayload;
    }

    Result LoadRebootPayload() {
        /* Clear payload buffer */
        std::memset(g_reboot_payload, 0xCC, sizeof(g_reboot_payload));

        /* Open payload file. */
        FsFile payload_file;
        R_TRY(fs::OpenAtmosphereSdFile(&payload_file, "/reboot_payload.bin", ams::fs::OpenMode_Read));
        ON_SCOPE_EXIT { fsFileClose(&payload_file); };

        /* Read payload file. Discard result. */
        {
            size_t actual_size;
            fsFileRead(&payload_file, 0, g_reboot_payload, sizeof(g_reboot_payload), FsReadOption_None, &actual_size);
        }

        /* NOTE: Preferred reboot type will be parsed from settings later on. */
        g_reboot_type = RebootType::ToPayload;

        return ResultSuccess();
    }

    Result DetectPreferredRebootFunctionality() {
        char reboot_type[0x40] = {};
        settings::fwdbg::GetSettingsItemValue(reboot_type, sizeof(reboot_type) - 1, "atmosphere", "power_menu_reboot_function");

        if (strcasecmp(reboot_type, "stock") == 0 || strcasecmp(reboot_type, "normal") == 0 || strcasecmp(reboot_type, "standard") == 0) {
            g_reboot_type = RebootType::Standard;
        } else if (strcasecmp(reboot_type, "rcm") == 0) {
            g_reboot_type = RebootType::ToRcm;
        } else if (strcasecmp(reboot_type, "payload") == 0) {
            g_reboot_type = RebootType::ToPayload;
        }

        return ResultSuccess();
    }

}

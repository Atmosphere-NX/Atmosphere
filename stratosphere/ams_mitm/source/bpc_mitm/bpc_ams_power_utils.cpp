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
#include "bpc_ams_power_utils.hpp"
#include "../amsmitm_fs_utils.hpp"

namespace ams::mitm::bpc {

    namespace {

        /* Convenience definitions. */
        constexpr uintptr_t IramBase = 0x40000000ull;
        constexpr uintptr_t IramPayloadBase = 0x40010000ull;
        constexpr size_t IramSize = 0x40000;
        constexpr size_t IramPayloadMaxSize = 0x2E000;

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

        void DoRebootToPayload(const ams::FatalErrorContext *ctx) {
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
                exosphere::CopyToIram(IramPayloadBase + IramPayloadMaxSize, g_work_page, sizeof(g_work_page));
            }

            exosphere::ForceRebootToIramPayload();
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
                DoRebootToPayload(nullptr);
                break;
        }
    }

    void ShutdownSystem() {
        exosphere::ForceShutdown();
    }

    /* Atmosphere power utilities. */
    void RebootForFatalError(const ams::FatalErrorContext *ctx) {
        DoRebootToPayload(ctx);
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

        /* TODO: Parse reboot type from settings. */
        g_reboot_type = RebootType::ToPayload;

        return ResultSuccess();
    }

}

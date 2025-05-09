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
#include <stratosphere.hpp>
#include "boot_power_utils.hpp"
#include "boot_pmic_driver.hpp"

namespace ams::boot {

    namespace {

        /* Convenience definitions. */
        constexpr uintptr_t IramBase = 0x40000000ull;
        constexpr uintptr_t IramPayloadBase = 0x40010000ull;
        constexpr size_t IramSize = 0x40000;
        constexpr size_t IramPayloadMaxSize = 0x24000;
        constexpr size_t IramFatalErrorContextOffset = 0x2E000;

        /* Globals. */
        alignas(os::MemoryPageSize) u8 g_work_page[os::MemoryPageSize];

        constexpr const u8 FuseeBin[] = {
            #embed <fusee.bin>
        };

        /* Helpers. */
        void ClearIram() {
            /* Make page FFs. */
            std::memset(g_work_page, 0xFF, sizeof(g_work_page));

            /* Overwrite all of IRAM with FFs. */
            for (size_t ofs = 0; ofs < IramSize; ofs += sizeof(g_work_page)) {
                exosphere::CopyToIram(IramBase + ofs, g_work_page, sizeof(g_work_page));
            }
        }

        void DoRebootToPayload() {
            /* Ensure clean IRAM state. */
            ClearIram();

            /* Copy in payload. */
            for (size_t ofs = 0; ofs < sizeof(FuseeBin); ofs += sizeof(g_work_page)) {
                std::memcpy(g_work_page, FuseeBin + ofs, std::min(static_cast<size_t>(sizeof(FuseeBin) - ofs), sizeof(g_work_page)));
                exosphere::CopyToIram(IramPayloadBase + ofs, g_work_page, sizeof(g_work_page));
            }

            exosphere::ForceRebootToIramPayload();
        }

        void DoRebootToFatalError(const ams::FatalErrorContext *ctx) {
            /* Ensure clean IRAM state. */
            ClearIram();

            /* Copy in payload. */
            for (size_t ofs = 0; ofs < sizeof(FuseeBin); ofs += sizeof(g_work_page)) {
                std::memcpy(g_work_page, FuseeBin + ofs, std::min(static_cast<size_t>(sizeof(FuseeBin) - ofs), sizeof(g_work_page)));
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

    void RebootSystem() {
        if (spl::GetSocType() == spl::SocType_Erista) {
            DoRebootToPayload();
        } else {
            /* On Mariko, we can't reboot to payload, so we should just do a reboot. */
            PmicDriver().RebootSystem();
        }
    }

    void ShutdownSystem() {
        PmicDriver().ShutdownSystem();
    }

    void SetInitialRebootPayload() {
        ::ams::SetInitialRebootPayload(FuseeBin, sizeof(FuseeBin));
    }

    void RebootForFatalError(ams::FatalErrorContext *ctx) {
        DoRebootToFatalError(ctx);
    }

}

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
#include <exosphere.hpp>

namespace ams::nxboot {

    void DoMemoryTrainingErista(int index, void *mtc_tables_buffer);
    void DoMemoryTrainingMariko(bool *out_did_training, int index, void *mtc_tables_buffer);

    void RestoreMemoryClockRateMariko(void *mtc_tables_buffer);

    namespace {

        alignas(4) constinit u8 g_mtc_tables_buffer[0x26C0];

        constinit bool g_did_training_mariko = false;

        constexpr const u8 MemoryTrainingTableIndex_Invalid = std::numeric_limits<u8>::max();

        constexpr const u8 MemoryTrainingTableIndices[] = {
            /* DramId_EristaIcosaSamsung4gb    */ 0x00,
            /* DramId_EristaIcosaHynix4gb      */ 0x02,
            /* DramId_EristaIcosaMicron4gb     */ 0x03,
            /* DramId_MarikoIowaHynix1y4gb     */ 0x10,
            /* DramId_EristaIcosaSamsung6gb    */ 0x01,
            /* DramId_MarikoHoagHynix1y4gb     */ 0x10,
            /* DramId_MarikoAulaHynix1y4gb     */ 0x10,
            /* DramId_MarikoIowax1x2Samsung4gb */ 0x00,
            /* DramId_MarikoIowaSamsung4gb     */ 0x05,
            /* DramId_MarikoIowaSamsung8gb     */ 0x06,
            /* DramId_MarikoIowaHynix4gb       */ 0x07,
            /* DramId_MarikoIowaMicron4gb      */ 0x08,
            /* DramId_MarikoHoagSamsung4gb     */ 0x05,
            /* DramId_MarikoHoagSamsung8gb     */ 0x06,
            /* DramId_MarikoHoagHynix4gb       */ 0x07,
            /* DramId_MarikoHoagMicron4gb      */ 0x08,
            /* DramId_MarikoIowaSamsung4gbY    */ 0x09,
            /* DramId_MarikoIowaSamsung1y4gbX  */ 0x0C,
            /* DramId_MarikoIowaSamsung1y8gbX  */ 0x0D,
            /* DramId_MarikoHoagSamsung1y4gbX  */ 0x0C,
            /* DramId_MarikoIowaSamsung1y4gbY  */ 0x0A,
            /* DramId_MarikoIowaSamsung1y8gbY  */ 0x0B,
            /* DramId_MarikoAulaSamsung1y4gb   */ 0x0E,
            /* DramId_MarikoHoagSamsung1y8gbX  */ 0x0D,
            /* DramId_MarikoAulaSamsung1y4gbX  */ 0x0C,
            /* DramId_MarikoIowaMicron1y4gb    */ 0x0F,
            /* DramId_MarikoHoagMicron1y4gb    */ 0x0F,
            /* DramId_MarikoAulaMicron1y4gb    */ 0x0F,
            /* DramId_MarikoAulaSamsung1y8gbX  */ 0x0D,
        };

        int GetMemoryTrainingTableIndex() {
            if (const auto dram_id = fuse::GetDramId(); dram_id < util::size(MemoryTrainingTableIndices) && MemoryTrainingTableIndices[dram_id] != MemoryTrainingTableIndex_Invalid) {
                return static_cast<int>(MemoryTrainingTableIndices[dram_id]);
            } else {
                return -1;
            }
        }

    }

    void DoMemoryTraining() {
        const auto index = GetMemoryTrainingTableIndex();

        if (fuse::GetSocType() == fuse::SocType_Erista) {
            DoMemoryTrainingErista(index, g_mtc_tables_buffer);
        } else {
            DoMemoryTrainingMariko(std::addressof(g_did_training_mariko), index, g_mtc_tables_buffer);
        }
    }

    void RestoreMemoryClockRate() {
        /* NOTE: This resolves an off-by-one issue in PCV's detection of memory clock rate on Mariko. */
        if (fuse::GetSocType() == fuse::SocType_Mariko && g_did_training_mariko) {
            RestoreMemoryClockRateMariko(g_mtc_tables_buffer);
        }
    }

}

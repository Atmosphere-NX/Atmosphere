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
#include <exosphere.hpp>
#include "../fusee_fatal.hpp"
#include "fusee_mtc.hpp"
#include "fusee_mtc_timing_table_erista.hpp"

namespace ams::nxboot {

    namespace {

        #include "fusee_mtc_tables_erista.inc"

        using EmcDvfsTimingTable = erista::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables() {
            const auto index = GetMemoryTrainingTableIndex();
            switch (index) {
                case 0:
                case 3:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableS4gb01);
                case 1:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableS6gb01);
                case 2:
                    return reinterpret_cast<EmcDvfsTimingTable *>(T210SdevEmcDvfsTableH4gb01);
                default:
                    ShowFatalError("Unknown EmcDvfsTimingTableIndex: %d\n", index);
            }
        }

    }

    void DoMemoryTrainingErista() {
        /* Get timing tables. */
        auto *timing_tables     = GetEmcDvfsTimingTables();
        auto *src_timing_tables = timing_tables + 0;
        auto *dst_timing_tables = timing_tables + 1;

        /* Check timing tables. */
        if (src_timing_tables->rate_khz != 204000 || dst_timing_tables->rate_khz != 1600000) {
            ShowFatalError("EmcDvfsTimingTables seem corrupted %" PRIu32 " %" PRIu32 "?\n", src_timing_tables->rate_khz, dst_timing_tables->rate_khz);
        }

        /* TODO */
    }

}

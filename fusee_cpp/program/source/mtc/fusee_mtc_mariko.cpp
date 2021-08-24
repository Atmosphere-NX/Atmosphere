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
#include "../fusee_uncompress.hpp"
#include "fusee_mtc.hpp"
#include "fusee_mtc_timing_table_mariko.hpp"

namespace ams::nxboot {

    namespace {

        #include "fusee_mtc_tables_mariko.inc"

        using EmcDvfsTimingTable = mariko::EmcDvfsTimingTable;

        EmcDvfsTimingTable *GetEmcDvfsTimingTables() {
            const auto index = GetMemoryTrainingTableIndex();

            /* Get the compressed table. */
            u8 *cmp_table;
            size_t cmp_table_size;
            switch (index) {
                #define HANDLE_CASE(N, TABLE)           \
                    case N:                             \
                        cmp_table = TABLE;              \
                        cmp_table_size = sizeof(TABLE); \
                        break;
                HANDLE_CASE(0x00, T210b01SdevEmcDvfsTableS4gb01)
                HANDLE_CASE(0x05, T210b01SdevEmcDvfsTableS4gb03)
                HANDLE_CASE(0x06, T210b01SdevEmcDvfsTableS8gb03)
                HANDLE_CASE(0x07, T210b01SdevEmcDvfsTableH4gb03)
                HANDLE_CASE(0x08, T210b01SdevEmcDvfsTableM4gb03)
                HANDLE_CASE(0x09, T210b01SdevEmcDvfsTableS4gbY01)
                HANDLE_CASE(0x0A, T210b01SdevEmcDvfsTableS1y4gbY01)
                HANDLE_CASE(0x0B, T210b01SdevEmcDvfsTableS1y8gbY01)
                HANDLE_CASE(0x0C, T210b01SdevEmcDvfsTableS1y4gbX03)
                HANDLE_CASE(0x0D, T210b01SdevEmcDvfsTableS1y8gbX03)
                HANDLE_CASE(0x0E, T210b01SdevEmcDvfsTableS1y4gb01)
                HANDLE_CASE(0x0F, T210b01SdevEmcDvfsTableM1y4gb01)
                HANDLE_CASE(0x10, T210b01SdevEmcDvfsTableH1y4gb01)
                default:
                    ShowFatalError("Unknown EmcDvfsTimingTableIndex: %d\n", index);
            }

            /* Uncompress the table. */
            EmcDvfsTimingTable *out_tables = reinterpret_cast<EmcDvfsTimingTable *>(0x40040000 - 2 * sizeof(EmcDvfsTimingTable));
            Uncompress(out_tables, 2 * sizeof(EmcDvfsTimingTable), cmp_table, cmp_table_size);

            return out_tables;
        }

    }

    void DoMemoryTrainingMariko() {
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

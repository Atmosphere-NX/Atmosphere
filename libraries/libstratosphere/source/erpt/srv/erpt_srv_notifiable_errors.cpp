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
#include "erpt_srv_notifiable_errors.hpp"

namespace ams::erpt::srv {

    namespace {

        constexpr size_t MaxEntriesPerType = 25;

        struct NotifiableErrorCodeReportState {
            u32 report_counts[ReportType_Count];
            NotifiableErrorCodeReportEntry report_entries[ReportType_Count][MaxEntriesPerType];
            os::Tick last_tick;
            u32 consecutive_count;
        };

        constinit NotifiableErrorCodeReportState g_state = {
            .report_counts     = {},
            .report_entries    = {},
            .last_tick         = os::Tick{},
            .consecutive_count = 0,
        };

    }

    void NotifiableErrorCodeReport::PushEntry(const char *error_code, const char *program_id, ReportType type, bool is_system_abort, bool is_application_abort) {
        u32 &count = g_state.report_counts[type];
        NotifiableErrorCodeReportEntry *entries = g_state.report_entries[type];

        /* If we're full, shift the oldest entry out. */
        if (count >= MaxEntriesPerType) {
            std::memmove(entries, entries + 1, sizeof(NotifiableErrorCodeReportEntry) * (MaxEntriesPerType - 1));
            count = MaxEntriesPerType - 1;
        }

        /* Fill the new entry. */
        NotifiableErrorCodeReportEntry &entry = entries[count];
        util::Strlcpy(entry.error_code, error_code, sizeof(entry.error_code));
        util::Strlcpy(entry.program_id, program_id, sizeof(entry.program_id));
        entry.is_visible = (type == ReportType_Visible);
        entry.is_system_abort = is_system_abort;
        entry.is_application_abort = is_application_abort;

        count++;
    }

    void NotifiableErrorCodeReport::PopNotifiableErrorCodes(NotifiableErrorCodesData *out) {
        /* Fill basic info from lazily-initialized system info. */
        const auto &sys_info = srv::GetSystemInfo();
        util::Strlcpy(out->firmware_display_version, sys_info.os_version, sizeof(out->firmware_display_version));
        util::Strlcpy(out->private_os_version, sys_info.private_os_version, sizeof(out->private_os_version));
        util::Strlcpy(out->product_model, sys_info.product_model, sizeof(out->product_model));
        util::Strlcpy(out->region_code, sys_info.region, sizeof(out->region_code));

        u32 total_count = 0;

        /* Copy entries. */
        for (u32 i = 0; i < ReportType_Count; i++) {
            if (g_state.report_counts[i] == 0) {
                continue;
            }
            std::memcpy(out->entries + total_count, g_state.report_entries[i], sizeof(NotifiableErrorCodeReportEntry) * g_state.report_counts[i]);
            total_count += g_state.report_counts[i];

            /* Reset count (destructive read). */
            g_state.report_counts[i] = 0; 
        }

        out->entry_count = total_count;
    }

    void NotifiableErrorCodeReport::Clear() {
        for (u32 i = 0; i < ReportType_Count; i++) {
            g_state.report_counts[i] = 0;
        }
    }

}
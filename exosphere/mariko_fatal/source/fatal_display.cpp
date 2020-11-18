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
#include "fatal_device_page_table.hpp"

namespace ams::secmon::fatal {

    void InitializeDisplay() {
        /* TODO */
        AMS_SECMON_LOG("InitializeDisplay not yet implemented\n");
    }

    void ShowDisplay(const ams::impl::FatalErrorContext *f_ctx, const Result save_result) {
        /* TODO */
        AMS_UNUSED(f_ctx, save_result);
        AMS_SECMON_LOG("ShowDisplay not yet implemented\n");
    }

    void FinalizeDisplay() {
        /* TODO */
        AMS_SECMON_LOG("FinalizeDisplay not yet implemented\n");
    }

}

/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_install_progress_state.hpp>

namespace ams::ncm {

    struct InstallProgress {
        InstallProgressState state;
        u8 pad[3];
        TYPED_STORAGE(Result) last_result;
        s64 installed_size;
        s64 total_size;

        Result GetLastResult() const {
            return util::GetReference(last_result);
        }

        void SetLastResult(Result result) {
            *util::GetPointer(last_result) = result;
        }
    };

}

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
#include <stratosphere/windows.hpp>
#include <ntstatus.h>
#include <bcrypt.h>

namespace ams::crypto {

    void GenerateCryptographicallyRandomBytes(void *dst, size_t dst_size) {
        const auto status = ::BCryptGenRandom(nullptr, static_cast<PUCHAR>(dst), dst_size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        AMS_ABORT_UNLESS(status == STATUS_SUCCESS);
    }

}

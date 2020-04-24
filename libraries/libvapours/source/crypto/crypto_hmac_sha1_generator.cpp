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
#include <vapours.hpp>

namespace ams::crypto {

    void GenerateHmacSha1Mac(void *dst, size_t dst_size, const void *data, size_t data_size, const void *key, size_t key_size) {
        HmacSha1Generator hmac;

        hmac.Initialize(key, key_size);
        hmac.Update(data, data_size);
        hmac.GetMac(dst, dst_size);
    }

}

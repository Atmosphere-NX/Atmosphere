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
#include <stratosphere.hpp>
#include "spl_api_impl.hpp"
#include "spl_manu_service.hpp"

namespace ams::spl {

    Result ManuService::ReEncryptRsaPrivateKey(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &src, AccessKey access_key_dec, KeySource source_dec, AccessKey access_key_enc, KeySource source_enc, u32 option) {
        return impl::ReEncryptRsaPrivateKey(out.GetPointer(), out.GetSize(), src.GetPointer(), src.GetSize(), access_key_dec, source_dec, access_key_enc, source_enc, option);
    }

}

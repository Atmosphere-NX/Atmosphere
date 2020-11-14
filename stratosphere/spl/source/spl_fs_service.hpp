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
#pragma once
#include "spl_crypto_service.hpp"

namespace ams::spl {

    class FsService : public CryptoService {
        public:
            /* Actual commands. */
            Result DecryptAndStoreGcKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            Result DecryptAndStoreGcKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source);
            Result DecryptGcMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            Result LoadPreparedAesKey(s32 keyslot, AccessKey access_key);
            Result GetPackage2Hash(const sf::OutPointerBuffer &dst);
    };
    static_assert(spl::impl::IsIFsInterface<FsService>);

}

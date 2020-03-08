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
#include <stratosphere/ncm/ncm_content_info.hpp>
#include <stratosphere/ncm/ncm_content_storage.hpp>
#include <stratosphere/ncm/ncm_storage_id.hpp>
#include <stratosphere/ncm/ncm_content_meta_key.hpp>

namespace ams::ncm {

    struct Digest {
        u8 data[crypto::Sha256Generator::HashSize];
    };

    struct PackagedContentInfo {
        Digest digest;
        ContentInfo info;

        constexpr const ContentId &GetId() const {
            return this->info.GetId();
        }

        constexpr const ContentType GetType() const {
            return this->info.GetType();
        }

        constexpr const u8 GetIdOffset() const {
            return this->info.GetIdOffset();
        }
    };

}

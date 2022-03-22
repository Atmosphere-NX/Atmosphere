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
#pragma once
#include <stratosphere/ncm/ncm_content_info.hpp>
#include <stratosphere/ncm/ncm_content_storage.hpp>
#include <stratosphere/ncm/ncm_storage_id.hpp>
#include <stratosphere/ncm/ncm_content_meta_key.hpp>

namespace ams::ncm {

    struct Digest {
        u8 data[crypto::Sha256Generator::HashSize];
    };

    enum class InstallState : u8 {
        NotPrepared,
        Prepared,
        Installed,
        AlreadyExists,
    };

    struct PackagedContentInfo {
        Digest digest;
        ContentInfo info;

        constexpr const ContentId &GetId() const {
            return this->info.GetId();
        }

        constexpr ContentType GetType() const {
            return this->info.GetType();
        }

        constexpr u8 GetIdOffset() const {
            return this->info.GetIdOffset();
        }
    };

    struct InstallContentInfo {
        Digest digest;
        crypto::Sha256Context context;
        u8 buffered_data[crypto::Sha256Generator::BlockSize];
        u64 buffered_data_size;
        ContentInfo info;
        PlaceHolderId placeholder_id;
        ContentMetaType meta_type;
        InstallState install_state;
        bool verify_digest;
        StorageId storage_id;
        bool is_temporary;
        bool is_sha256_calculated;
        s64 written;

        constexpr const ContentId &GetId() const {
            return this->info.GetId();
        }

        constexpr u64 GetSize() const {
            return this->info.GetSize();
        }

        constexpr ContentType GetType() const {
            return this->info.GetType();
        }

        constexpr u8 GetIdOffset() const {
            return this->info.GetIdOffset();
        }

        constexpr const PlaceHolderId &GetPlaceHolderId() const {
            return this->placeholder_id;
        }

        constexpr ContentMetaType GetContentMetaType() const {
            return this->meta_type;
        }

        constexpr InstallState GetInstallState() const {
            return this->install_state;
        }

        constexpr StorageId GetStorageId() const {
            return this->storage_id;
        }

        constexpr s64 GetSizeWritten() const {
            return this->written;
        }

        static constexpr InstallContentInfo Make(const ContentInfo &info, ContentMetaType meta_type) {
            return {
                .info          = info,
                .meta_type     = meta_type,
            };
        }

        static constexpr InstallContentInfo Make(const PackagedContentInfo &info, ContentMetaType meta_type) {
            return {
                .digest        = info.digest,
                .info          = info.info,
                .meta_type     = meta_type,
                .verify_digest = true,
            };
        }
    };

    static_assert(sizeof(InstallContentInfo) == 0xC8);

}

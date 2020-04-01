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
#include <vapours.hpp>
#include <stratosphere/ncm/ncm_content_meta_id.hpp>
#include <stratosphere/ncm/ncm_content_meta_type.hpp>
#include <stratosphere/ncm/ncm_system_content_meta_id.hpp>

namespace ams::ncm {

    enum class ContentInstallType : u8 {
        Full            = 0,
        FragmentOnly    = 1,
        Unknown         = 7,
    };

    struct ContentMetaKey {
        u64 id;
        u32 version;
        ContentMetaType type;
        ContentInstallType install_type;
        u8 padding[2];

        bool operator<(const ContentMetaKey &rhs) const {
            return std::tie(this->id, this->version, this->type, this->install_type) < std::tie(rhs.id, rhs.version, rhs.type, rhs.install_type);
        }

        constexpr bool operator==(const ContentMetaKey &rhs) const {
            return std::tie(this->id, this->version, this->type, this->install_type) == std::tie(rhs.id, rhs.version, rhs.type, rhs.install_type);
        }

        constexpr bool operator!=(const ContentMetaKey &rhs) const {
            return !(*this == rhs);
        }

        static constexpr ContentMetaKey MakeUnknownType(u64 id, u32 version) {
            return { .id = id, .version = version, .type = ContentMetaType::Unknown };
        }

        static constexpr ContentMetaKey Make(u64 id, u32 version, ContentMetaType type) {
            return { .id = id, .version = version, .type = type };
        }

        static constexpr ContentMetaKey Make(u64 id, u32 version, ContentMetaType type, ContentInstallType install_type) {
            return { .id = id, .version = version, .type = type, .install_type = install_type };
        }

        static constexpr ContentMetaKey Make(SystemProgramId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::SystemProgram };
        }

        static constexpr ContentMetaKey Make(SystemDataId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::SystemData };
        }

        static constexpr ContentMetaKey Make(SystemUpdateId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::SystemUpdate };
        }

        static constexpr ContentMetaKey Make(ApplicationId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::Application };
        }

        static constexpr ContentMetaKey Make(PatchId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::Patch };
        }

        static constexpr ContentMetaKey Make(PatchId id, u32 version, ContentInstallType install_type) {
            return { .id = id.value, .version = version, .type = ContentMetaType::Patch, .install_type = install_type };
        }

        static constexpr ContentMetaKey Make(DeltaId id, u32 version) {
            return { .id = id.value, .version = version, .type = ContentMetaType::Delta };
        }
    };

    static_assert(sizeof(ContentMetaKey) == 0x10);

    struct ApplicationContentMetaKey {
        ContentMetaKey key;
        ncm::ApplicationId application_id;
    };

    static_assert(sizeof(ApplicationContentMetaKey) == 0x18);

    struct StorageContentMetaKey {
        ContentMetaKey key;
        StorageId storage_id;
        u8 reserved[7];

        constexpr bool operator==(StorageContentMetaKey &rhs) const {
            return this->key == rhs.key && this->storage_id == rhs.storage_id;
        }

        constexpr bool operator<(StorageContentMetaKey &rhs) const {
            return this->key == rhs.key ? this->storage_id < rhs.storage_id : this->key < rhs.key;
        }
    };
    static_assert(sizeof(StorageContentMetaKey) == 0x18);

}

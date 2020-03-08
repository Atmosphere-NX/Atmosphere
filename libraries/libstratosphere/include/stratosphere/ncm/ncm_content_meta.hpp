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
#include <stratosphere/ncm/ncm_content_meta_id.hpp>
#include <stratosphere/ncm/ncm_content_meta_key.hpp>
#include <stratosphere/ncm/ncm_content_info.hpp>
#include <stratosphere/ncm/ncm_content_info_data.hpp>
#include <stratosphere/ncm/ncm_storage_id.hpp>

namespace ams::ncm {

    enum ContentMetaAttribute : u8 {
        ContentMetaAttribute_None                   = (0 << 0),
        ContentMetaAttribute_IncludesExFatDriver    = (1 << 0),
        ContentMetaAttribute_Rebootless             = (1 << 1),
    };

    struct ContentMetaInfo {
        u64 id;
        u32 version;
        ContentMetaType type;
        u8 attributes;
        u8 padding[2];

        static constexpr ContentMetaInfo Make(u64 id, u32 version, ContentMetaType type, u8 attributes) {
            return {
                .id         = id,
                .version    = version,
                .type       = type,
                .attributes = attributes,
            };
        }

        constexpr ContentMetaKey ToKey() {
            return ContentMetaKey::Make(this->id, this->version, this->type);
        }
    };

    static_assert(sizeof(ContentMetaInfo) == 0x10);

    struct ContentMetaHeader {
        u16 extended_header_size;
        u16 content_count;
        u16 content_meta_count;
        u8 attributes;
        StorageId storage_id;
    };

    static_assert(sizeof(ContentMetaHeader) == 0x8);

    struct PackagedContentMetaHeader {
        u64 id;
        u32 version;
        ContentMetaType type;
        u8 reserved_0D;
        u16 extended_header_size;
        u16 content_count;
        u16 content_meta_count;
        u8 attributes;
        u8 storage_id;
        ContentInstallType install_type;
        u8 reserved_17;
        u32 required_download_system_version;
        u8 reserved_1C[4];
    };
    static_assert(sizeof(PackagedContentMetaHeader) == 0x20);
    static_assert(OFFSETOF(PackagedContentMetaHeader, reserved_0D) == 0x0D);
    static_assert(OFFSETOF(PackagedContentMetaHeader, reserved_17) == 0x17);
    static_assert(OFFSETOF(PackagedContentMetaHeader, reserved_1C) == 0x1C);

    struct ApplicationMetaExtendedHeader {
        PatchId patch_id;
        u32 required_system_version;
        u32 required_application_version;
    };

    struct PatchMetaExtendedHeader {
        ApplicationId application_id;
        u32 required_system_version;
        u32 extended_data_size;
        u8 reserved[0x8];
    };

    struct AddOnContentMetaExtendedHeader {
        ApplicationId application_id;
        u32 required_application_version;
        u32 padding;
    };

    struct DeltaMetaExtendedHeader {
        ApplicationId application_id;
        u32 extended_data_size;
        u32 padding;
    };

    template<typename ContentMetaHeaderType, typename ContentInfoType>
    class ContentMetaAccessor {
        public:
            using HeaderType = ContentMetaHeaderType;
            using InfoType   = ContentInfoType;
        private:
            void *data;
            const size_t size;
            bool is_header_valid;
        private:
            static size_t GetExtendedHeaderSize(ContentMetaType type) {
                switch (type) {
                    case ContentMetaType::Application:  return sizeof(ApplicationMetaExtendedHeader);
                    case ContentMetaType::Patch:        return sizeof(PatchMetaExtendedHeader);
                    case ContentMetaType::AddOnContent: return sizeof(AddOnContentMetaExtendedHeader);
                    case ContentMetaType::Delta:        return sizeof(DeltaMetaExtendedHeader);
                    default:                            return 0;
                }
            }
        protected:
            constexpr ContentMetaAccessor(const void *d, size_t sz) : data(const_cast<void *>(d)), size(sz), is_header_valid(true) { /* ... */ }
            constexpr ContentMetaAccessor(void *d, size_t sz) : data(d), size(sz), is_header_valid(false) { /* ... */ }

            template<class NewHeaderType, class NewInfoType>
            static constexpr size_t CalculateSizeImpl(size_t ext_header_size, size_t content_count, size_t content_meta_count, size_t extended_data_size, bool has_digest) {
                return sizeof(NewHeaderType) + ext_header_size + content_count * sizeof(NewInfoType) + content_meta_count * sizeof(ContentMetaInfo) + extended_data_size + (has_digest ? sizeof(Digest) : 0);
            }

            static constexpr size_t CalculateSize(ContentMetaType type, size_t content_count, size_t content_meta_count, size_t extended_data_size, bool has_digest = false) {
                return CalculateSizeImpl<ContentMetaHeaderType, ContentInfoType>(GetExtendedHeaderSize(type), content_count, content_meta_count, extended_data_size, has_digest);
            }

            uintptr_t GetExtendedHeaderAddress() const {
                return reinterpret_cast<uintptr_t>(this->data) + sizeof(HeaderType);
            }

            uintptr_t GetContentInfoStartAddress() const {
                return this->GetExtendedHeaderAddress() + this->GetExtendedHeaderSize();
            }

            uintptr_t GetContentInfoAddress(size_t i) const {
                return this->GetContentInfoStartAddress() + i * sizeof(InfoType);
            }

            uintptr_t GetContentMetaInfoStartAddress() const {
                return this->GetContentInfoAddress(this->GetContentCount());
            }

            uintptr_t GetContentMetaInfoAddress(size_t i) const {
                return this->GetContentMetaInfoStartAddress() + i * sizeof(ContentMetaInfo);
            }

            uintptr_t GetExtendedDataAddress() const {
                return this->GetContentMetaInfoAddress(this->GetContentMetaCount());
            }

            uintptr_t GetDigestAddress() const {
                return this->GetExtendedDataAddress() + this->GetExtendedDataSize();
            }

            InfoType *GetWritableContentInfo(size_t i) const {
                AMS_ABORT_UNLESS(i < this->GetContentCount());

                return reinterpret_cast<InfoType *>(this->GetContentInfoAddress(i));
            }

            InfoType *GetWritableContentInfo(ContentType type) const {
                InfoType *found = nullptr;
                for (size_t i = 0; i < this->GetContentCount(); i++) {
                    /* We want to find the info with the lowest id offset and the correct type. */
                    InfoType *info = this->GetWritableContentInfo(i);
                    if (info->GetType() == type && (found == nullptr || info->GetIdOffset() < found->GetIdOffset())) {
                        found = info;
                    }
                }
                return found;
            }

            InfoType *GetWritableContentInfo(ContentType type, u8 id_ofs) const {
                for (size_t i = 0; i < this->GetContentCount(); i++) {
                    /* We want to find the info with the correct id offset and the correct type. */
                    if (InfoType *info = this->GetWritableContentInfo(i); info->GetType() == type && info->GetIdOffset() == id_ofs) {
                        return info;
                    }
                }
                return nullptr;
            }

        public:
            const void *GetData() const {
                return this->data;
            }

            size_t GetSize() const {
                return this->size;
            }

            const HeaderType *GetHeader() const {
                AMS_ABORT_UNLESS(this->is_header_valid);
                return static_cast<const HeaderType *>(this->data);
            }

            ContentMetaKey GetKey() const {
                auto header = this->GetHeader();
                return ContentMetaKey::Make(header->id, header->version, header->type, header->install_type);
            }

            size_t GetExtendedHeaderSize() const {
                return this->GetHeader()->extended_header_size;
            }

            template<typename ExtendedHeaderType>
            const ExtendedHeaderType *GetExtendedHeader() const {
                return reinterpret_cast<const ExtendedHeaderType *>(this->GetExtendedHeaderAddress());
            }

            size_t GetContentCount() const {
                return this->GetHeader()->content_count;
            }

            const InfoType *GetContentInfo(size_t i) const {
                AMS_ABORT_UNLESS(i < this->GetContentCount());

                return this->GetWritableContentInfo(i);
            }

            const InfoType *GetContentInfo(ContentType type) const {
                return this->GetWritableContentInfo(type);
            }

            const InfoType *GetContentInfo(ContentType type, u8 id_ofs) const {
                return this->GetWritableContentInfo(type, id_ofs);
            }

            size_t GetContentMetaCount() const {
                return this->GetHeader()->content_meta_count;
            }

            const ContentMetaInfo *GetContentMetaInfo(size_t i) const {
                AMS_ABORT_UNLESS(i < this->GetContentMetaCount());

                return reinterpret_cast<const ContentMetaInfo *>(this->GetContentMetaInfoAddress(i));
            }

            size_t GetExtendedDataSize() const {
                switch (this->GetHeader()->type) {
                    case ContentMetaType::Patch: return this->GetExtendedHeader<PatchMetaExtendedHeader>()->extended_data_size;
                    case ContentMetaType::Delta: return this->GetExtendedHeader<DeltaMetaExtendedHeader>()->extended_data_size;
                    default:                     return 0;
                }
            }

            const void *GetExtendedData() const {
                return reinterpret_cast<const void *>(this->GetExtendedDataAddress());
            }

            const Digest *GetDigest() const {
                return reinterpret_cast<Digest *>(this->GetDigestAddress());
            }

            bool HasContent(const ContentId &id) const {
                for (size_t i = 0; i < this->GetContentCount(); i++) {
                    if (id == this->GetContentInfo(i)->GetId()) {
                        return true;
                    }
                }
                return false;
            }

            std::optional<ApplicationId> GetApplicationId(const ContentMetaKey &key) const {
                switch (key.type) {
                    case ContentMetaType::Application:  return ApplicationId{ key.id };
                    case ContentMetaType::Patch:        return this->GetExtendedHeader<PatchMetaExtendedHeader>()->application_id;
                    case ContentMetaType::AddOnContent: return this->GetExtendedHeader<AddOnContentMetaExtendedHeader>()->application_id;
                    case ContentMetaType::Delta:        return this->GetExtendedHeader<DeltaMetaExtendedHeader>()->application_id;
                    default:                            return std::nullopt;
                }
            }

            std::optional<ApplicationId> GetApplicationId() const {
                return this->GetApplicationId(this->GetKey());
            }
    };

    class ContentMetaReader : public ContentMetaAccessor<ContentMetaHeader, ContentInfo> {
        public:
            constexpr ContentMetaReader(const void *data, size_t size) : ContentMetaAccessor(data, size) { /* ... */ }

            using ContentMetaAccessor::CalculateSize;
    };

    class PackagedContentMetaReader : public ContentMetaAccessor<PackagedContentMetaHeader, PackagedContentInfo> {
        public:
            constexpr PackagedContentMetaReader(const void *data, size_t size) : ContentMetaAccessor(data, size) { /* ... */ }

            size_t CalculateConvertContentMetaSize() const;

            void ConvertToContentMeta(void *dst, size_t size, const ContentInfo &meta);

            size_t CountDeltaFragments() const;

            static constexpr size_t CalculateSize(ContentMetaType type, size_t content_count, size_t content_meta_count, size_t extended_data_size) {
                return ContentMetaAccessor::CalculateSize(type, content_count, content_meta_count, extended_data_size, true);
            }
    };

}

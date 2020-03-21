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
#include <stratosphere.hpp>

namespace ams::ncm {

    namespace {

        void ConvertPackageContentMetaHeaderToContentMetaHeader(ContentMetaHeader *dst, const PackagedContentMetaHeader &src) {
            /* Clear destination. */
            *dst = {};

            /* Set converted fields. */
            dst->extended_header_size = src.extended_header_size;
            dst->content_meta_count   = src.content_meta_count;
            dst->content_count        = src.content_meta_count;
            dst->attributes           = src.attributes;
        }

        void ConvertPackageContentMetaHeaderToInstallContentMetaHeader(InstallContentMetaHeader *dst, const PackagedContentMetaHeader &src) {
            /* Clear destination. */
            *dst = {};

            /* Set converted fields. */
            dst->id                               = src.id;
            dst->version                          = src.version;
            dst->type                             = src.type;
            dst->extended_header_size             = src.extended_header_size;
            dst->content_count                    = src.content_count;
            dst->content_meta_count               = src.content_meta_count;
            dst->attributes                       = src.attributes;
            dst->storage_id                       = src.storage_id;
            dst->install_type                     = src.install_type;
            dst->committed                        = src.committed;
            dst->required_download_system_version = src.required_download_system_version;
        }

        void ConvertInstallContentMetaHeaderToContentMetaHeader(ContentMetaHeader *dst, const InstallContentMetaHeader &src) {
            /* Clear destination. */
            *dst = {};

            /* Set converted fields. */
            dst->extended_header_size = src.extended_header_size;
            dst->content_meta_count   = src.content_meta_count;
            dst->content_count        = src.content_meta_count;
            dst->attributes           = src.attributes;
        }

    }

    size_t PackagedContentMetaReader::CalculateConvertInstallContentMetaSize() const {
        /* Prepare the header. */
        const auto *header = this->GetHeader();

        if ((header->type == ContentMetaType::SystemUpdate && this->GetExtendedHeaderSize() > 0) || header->type == ContentMetaType::Delta) {
            /* Newer SystemUpdates and Deltas contain extended data. */
            return this->CalculateSizeImpl<InstallContentMetaHeader, InstallContentInfo>(header->extended_header_size, header->content_count + 1, header->content_meta_count, this->GetExtendedDataSize(), false);
        } else if (header->type == ContentMetaType::Patch) {
            /* Subtract the number of delta fragments for patches, include extended data. */
            return this->CalculateSizeImpl<InstallContentMetaHeader, InstallContentInfo>(header->extended_header_size, header->content_count - this->CountDeltaFragments() + 1, header->content_meta_count, this->GetExtendedDataSize(), false);
        }
        
        /* No extended data or delta fragments by default. */
        return this->CalculateSizeImpl<InstallContentMetaHeader, InstallContentInfo>(header->extended_header_size, header->content_count + 1, header->content_meta_count, 0, false);
    }

    size_t PackagedContentMetaReader::CountDeltaFragments() const {
        size_t count = 0;
        for (size_t i = 0; i < this->GetContentCount(); i++) {
            if (this->GetContentInfo(i)->GetType() == ContentType::DeltaFragment) {
                count++;
            }
        }
        return count;
    }

    size_t PackagedContentMetaReader::CalculateConvertContentMetaSize() const {
        const auto *header = this->GetHeader();
        return this->CalculateSizeImpl<ContentMetaHeader, ContentInfo>(header->extended_header_size, header->content_count + 1, header->content_meta_count, 0, false);
    }

    void PackagedContentMetaReader::ConvertToInstallContentMeta(void *dst, size_t size, const InstallContentInfo &meta) {
        /* Ensure we have enough space to convert. */
        AMS_ABORT_UNLESS(size >= this->CalculateConvertInstallContentMetaSize());

        /* Prepare for conversion. */
        const auto *packaged_header = this->GetHeader();
        uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);

        /* Convert the header. */
        InstallContentMetaHeader header;
        ConvertPackageContentMetaHeaderToInstallContentMetaHeader(std::addressof(header), *packaged_header);
        header.content_count += 1;

        /* Don't include deltas. */
        if (packaged_header->type == ContentMetaType::Patch) {
            header.content_count -= this->CountDeltaFragments();
        }

        /* Copy the header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(header), sizeof(header));
        dst_addr += sizeof(header);

        /* Copy the extended header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), reinterpret_cast<void *>(this->GetExtendedHeaderAddress()), packaged_header->extended_header_size);
        dst_addr += packaged_header->extended_header_size;

        /* Copy the top level meta. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(meta), sizeof(meta));
        dst_addr += sizeof(meta);

        /* Copy content infos. */
        for (size_t i = 0; i < this->GetContentCount(); i++) {
            const auto *packaged_content_info = this->GetContentInfo(i);

            /* Don't copy any delta fragments. */
            if (packaged_header->type == ContentMetaType::Patch) {
                if (packaged_content_info->GetType() == ContentType::DeltaFragment) {
                    continue;
                }
            }

            /* Create the install content info. */
            InstallContentInfo install_content_info = InstallContentInfo::Make(*packaged_content_info, packaged_header->type);

            /* Copy the info. */
            std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(install_content_info), sizeof(InstallContentInfo));
            dst_addr += sizeof(InstallContentInfo);
        }

        /* Copy content meta infos. */
        for (size_t i = 0; i < this->GetContentMetaCount(); i++) {
            std::memcpy(reinterpret_cast<void *>(dst_addr), this->GetContentMetaInfo(i), sizeof(ContentMetaInfo));
            dst_addr += sizeof(ContentMetaInfo);
        }
    }

    void PackagedContentMetaReader::ConvertToContentMeta(void *dst, size_t size, const ContentInfo &meta) {
        /* Ensure we have enough space to convert. */
        AMS_ABORT_UNLESS(size >= this->CalculateConvertContentMetaSize());

        /* Prepare for conversion. */
        const auto *packaged_header = this->GetHeader();
        uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);

        /* Convert the header. */
        ContentMetaHeader header;
        ConvertPackageContentMetaHeaderToContentMetaHeader(std::addressof(header), *packaged_header);
        header.content_count += 1;

        /* Don't include deltas. */
        if (packaged_header->type == ContentMetaType::Patch) {
            header.content_count -= this->CountDeltaFragments();
        }

        /* Copy the header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(header), sizeof(header));
        dst_addr += sizeof(header);

        /* Copy the extended header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), reinterpret_cast<void *>(this->GetExtendedHeaderAddress()), packaged_header->extended_header_size);
        dst_addr += packaged_header->extended_header_size;

        /* Copy the top level meta. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(meta), sizeof(meta));
        dst_addr += sizeof(meta);

        /* Copy content infos. */
        for (size_t i = 0; i < this->GetContentCount(); i++) {
            /* Don't copy any delta fragments. */
            if (packaged_header->type == ContentMetaType::Patch) {
                if (this->GetContentInfo(i)->GetType() == ContentType::DeltaFragment) {
                    continue;
                }
            }

            /* Copy the current info. */
            std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(this->GetContentInfo(i)->info), sizeof(ContentInfo));
            dst_addr += sizeof(ContentInfo);
        }

        /* Copy content meta infos. */
        for (size_t i = 0; i < this->GetContentMetaCount(); i++) {
            std::memcpy(reinterpret_cast<void *>(dst_addr), this->GetContentMetaInfo(i), sizeof(ContentMetaInfo));
            dst_addr += sizeof(ContentMetaInfo);
        }
    }

    size_t InstallContentMetaReader::CalculateConvertSize() const {
        return CalculateSizeImpl<ContentMetaHeader, ContentInfo>(this->GetExtendedHeaderSize(), this->GetContentCount(), this->GetContentMetaCount(), this->GetExtendedDataSize(), false);
    }

    void InstallContentMetaReader::ConvertToContentMeta(void *dst, size_t size) const {
        /* Ensure we have enough space to convert. */
        AMS_ABORT_UNLESS(size >= this->CalculateConvertSize());

        /* Prepare for conversion. */
        const auto *install_header = this->GetHeader();
        uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);

        /* Convert the header. */
        ContentMetaHeader header;
        ConvertInstallContentMetaHeaderToContentMetaHeader(std::addressof(header), *install_header);

        /* Copy the header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(header), sizeof(header));
        dst_addr += sizeof(header);

        /* Copy the extended header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), reinterpret_cast<void *>(this->GetExtendedHeaderAddress()), install_header->extended_header_size);
        dst_addr += install_header->extended_header_size;

        /* Copy content infos. */
        for (size_t i = 0; i < this->GetContentCount(); i++) {
            /* Copy the current info. */
            std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(this->GetContentInfo(i)->info), sizeof(ContentInfo));
            dst_addr += sizeof(ContentInfo);
        }

        /* Copy content meta infos. */
        for (size_t i = 0; i < this->GetContentMetaCount(); i++) {
            std::memcpy(reinterpret_cast<void *>(dst_addr), this->GetContentMetaInfo(i), sizeof(ContentMetaInfo));
            dst_addr += sizeof(ContentMetaInfo);
        }
    }

}

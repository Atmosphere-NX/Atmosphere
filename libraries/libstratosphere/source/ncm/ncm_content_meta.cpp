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

namespace ams::ncm {

    namespace {

        void ConvertPackageContentMetaHeaderToContentMetaHeader(ContentMetaHeader *dst, const PackagedContentMetaHeader &src) {
            /* Set destination. */
            *dst = {
                .extended_header_size = src.extended_header_size,
                .content_count        = src.content_count,
                .content_meta_count   = src.content_meta_count,
                .attributes           = src.attributes,
            };
        }

        void ConvertPackageContentMetaHeaderToInstallContentMetaHeader(InstallContentMetaHeader *dst, const PackagedContentMetaHeader &src) {
            /* Set destination. */
            static_assert(std::is_same<InstallContentMetaHeader, PackagedContentMetaHeader>::value);
            std::memcpy(dst, std::addressof(src), sizeof(*dst));
        }

        void ConvertInstallContentMetaHeaderToContentMetaHeader(ContentMetaHeader *dst, const InstallContentMetaHeader &src) {
            /* Set destination. */
            *dst = {
                .extended_header_size = src.extended_header_size,
                .content_count        = src.content_count,
                .content_meta_count   = src.content_meta_count,
                .attributes           = src.attributes,
            };
        }

        Result FindDeltaIndex(s32 *out_index, const PatchMetaExtendedDataReader &reader, u32 src_version, u32 dst_version) {
            /* Iterate over all deltas. */
            auto header = reader.GetHeader();
            for (s32 i = 0; i < static_cast<s32>(header->delta_count); i++) {
                /* Check if the current delta matches the versions. */
                auto delta = reader.GetPatchDeltaHeader(i);
                if ((src_version == 0 || delta->delta.source_version == src_version) && delta->delta.destination_version == dst_version) {
                    *out_index = i;
                    return ResultSuccess();
                }
            }

            /* We didn't find the delta. */
            return ncm::ResultDeltaNotFound();
        }

        s32 CountContentExceptForMeta(const PatchMetaExtendedDataReader &reader, s32 delta_index) {
            /* Iterate over packaged content infos, checking for those which aren't metas. */
            s32 count = 0;
            auto delta = reader.GetPatchDeltaHeader(delta_index);
            for (s32 i = 0; i < static_cast<s32>(delta->content_count); i++) {
                if (reader.GetPatchDeltaPackagedContentInfo(delta_index, i)->GetType() != ContentType::Meta) {
                    count++;
                }
            }

            return count;
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

    Result PackagedContentMetaReader::ConvertToFragmentOnlyInstallContentMeta(void *dst, size_t size, const InstallContentInfo &meta, u32 source_version) {
        /* Ensure that we have enough space. */
        size_t required_size;
        R_TRY(this->CalculateConvertFragmentOnlyInstallContentMetaSize(std::addressof(required_size), source_version));
        AMS_ABORT_UNLESS(size >= required_size);

        /* Find the delta index. */
        PatchMetaExtendedDataReader reader(this->GetExtendedData(), this->GetExtendedDataSize());
        s32 index;
        R_TRY(FindDeltaIndex(std::addressof(index), reader, source_version, this->GetKey().version));
        auto delta = reader.GetPatchDeltaHeader(index);

        /* Prepare for conversion. */
        const auto *packaged_header = this->GetHeader();
        uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);

        /* Convert the header. */
        InstallContentMetaHeader header;
        ConvertPackageContentMetaHeaderToInstallContentMetaHeader(std::addressof(header), *packaged_header);
        header.install_type = ContentInstallType::FragmentOnly;

        /* Set the content count. */
        auto fragment_count = CountContentExceptForMeta(reader, index);
        header.content_count = static_cast<u16>(fragment_count) + 1;

        /* Copy the header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(header), sizeof(header));
        dst_addr += sizeof(header);

        /* Copy the extended header. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), reinterpret_cast<void *>(this->GetExtendedHeaderAddress()), packaged_header->extended_header_size);
        dst_addr += packaged_header->extended_header_size;

        /* Copy the top level meta. */
        std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(meta), sizeof(meta));
        dst_addr += sizeof(meta);

        s32 count = 0;
        for (s32 i = 0; i < static_cast<s32>(delta->content_count); i++) {
            auto packaged_content_info = reader.GetPatchDeltaPackagedContentInfo(index, i);
            if (packaged_content_info->GetType() != ContentType::Meta) {
                /* Create the install content info. */
                InstallContentInfo install_content_info = InstallContentInfo::Make(*packaged_content_info, packaged_header->type);

                /* Copy the info. */
                std::memcpy(reinterpret_cast<void *>(dst_addr), std::addressof(install_content_info), sizeof(InstallContentInfo));
                dst_addr += sizeof(InstallContentInfo);

                /* Increment the count. */
                count++;
            }
        }

        /* Assert that we copied the right number of infos. */
        AMS_ASSERT(count == fragment_count);

        return ResultSuccess();
    }

    Result PackagedContentMetaReader::CalculateConvertFragmentOnlyInstallContentMetaSize(size_t *out_size, u32 source_version) const {
        /* Find the delta index. */
        PatchMetaExtendedDataReader reader(this->GetExtendedData(), this->GetExtendedDataSize());
        s32 index;
        R_TRY(FindDeltaIndex(std::addressof(index), reader, source_version, this->GetKey().version));

        /* Get the fragment count. */
        auto fragment_count = CountContentExceptForMeta(reader, index);

        /* Recalculate. */
        *out_size = CalculateSizeImpl<InstallContentMetaHeader, InstallContentInfo>(this->GetExtendedHeaderSize(), fragment_count + 1, 0, this->GetExtendedDataSize(), false);
        return ResultSuccess();
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

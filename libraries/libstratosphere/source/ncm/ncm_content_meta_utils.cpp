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
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        using FilePathString = kvdb::BoundedString<64>;

        bool IsContentMetaFileName(const char *name) {
            return impl::PathView(name).HasSuffix(".cnmt");
        }

        Result MountContentMetaByRemoteFileSystemProxy(const char *mount_name, const char *path) {
            return fs::MountContent(mount_name, path, fs::ContentType_Meta);
        }

        constinit MountContentMetaFunction g_mount_content_meta_func = MountContentMetaByRemoteFileSystemProxy;

    }

    Result ReadContentMetaPath(AutoBuffer *out, const char *path) {
        /* Mount the content. */
        auto mount_name = impl::CreateUniqueMountName();
        R_TRY(g_mount_content_meta_func(mount_name.str, path));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.str); };

        /* Open the root directory. */
        auto root_path = impl::GetRootDirectoryPath(mount_name);
        fs::DirectoryHandle dir;
        R_TRY(fs::OpenDirectory(std::addressof(dir), root_path.str, fs::OpenDirectoryMode_File));
        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

        /* Loop directory reading until we find the entry we're looking for. */
        while (true) {
            /* Read one entry, and finish when we fail to read. */
            fs::DirectoryEntry entry;
            s64 num_read;
            R_TRY(fs::ReadDirectory(std::addressof(num_read), std::addressof(entry), dir, 1));
            if (num_read == 0) {
                break;
            }

            /* If this is the content meta file, parse it. */
            if (IsContentMetaFileName(entry.name)) {
                /* Create the file path. */
                FilePathString file_path(root_path.str);
                file_path.Append(entry.name);

                /* Open the content meta file. */
                fs::FileHandle file;
                R_TRY(fs::OpenFile(std::addressof(file), file_path, fs::OpenMode_Read));
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                /* Get the meta size. */
                s64 file_size;
                R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                const size_t meta_size = static_cast<size_t>(file_size);

                /* Create a buffer for the meta. */
                R_TRY(out->Initialize(meta_size));

                /* Read the meta into the buffer. */
                return fs::ReadFile(file, 0, out->Get(), meta_size);
            }
        }

        return ncm::ResultContentMetaNotFound();
    }

    Result ReadVariationContentMetaInfoList(s32 *out_count, std::unique_ptr<ContentMetaInfo[]> *out_meta_infos, const Path &path, FirmwareVariationId firmware_variation_id) {
        AutoBuffer meta;
        {
            fs::ScopedAutoAbortDisabler aad;
            R_TRY(ReadContentMetaPath(std::addressof(meta), path.str));
        }

        /* Create a reader for the content meta. */
        PackagedContentMetaReader reader(meta.Get(), meta.GetSize());

        /* Define a helper to output the base meta infos. */
        const auto ReadMetaInfoListFromBase = [&] ALWAYS_INLINE_LAMBDA () -> Result {
            /* Output the base content meta info count. */
            *out_count = reader.GetContentMetaCount();

            /* Create a buffer to hold the infos. NOTE: N does not check for nullptr before accessing. */
            std::unique_ptr<ContentMetaInfo[]> buffer(new (std::nothrow) ContentMetaInfo[reader.GetContentMetaCount()]);
            AMS_ABORT_UNLESS(buffer != nullptr);

            /* Copy all base meta infos to output */
            for (size_t i = 0; i < reader.GetContentMetaCount(); i++) {
                buffer[i] = *reader.GetContentMetaInfo(i);
            }

            /* Write out the buffer we've populated. */
            *out_meta_infos = std::move(buffer);
            return ResultSuccess();
        };

        /* If there are no firmware variations to list, read meta infos from base. */
        R_UNLESS(reader.GetExtendedDataSize() != 0, ReadMetaInfoListFromBase());

        SystemUpdateMetaExtendedDataReader extended_data_reader(reader.GetExtendedData(), reader.GetExtendedDataSize());
        std::optional<s32> firmware_variation_index = std::nullopt;

        /* NOTE: Atmosphere extension to support downgrading. */
        /* If all firmware variations refer to base, don't require the current variation be present. */
        bool force_refer_to_base = true;

        /* Find the input firmware variation id. */
        for (size_t i = 0; i < extended_data_reader.GetFirmwareVariationCount(); i++) {
            if (*extended_data_reader.GetFirmwareVariationId(i) == firmware_variation_id) {
                firmware_variation_index = i;
                break;
            } else {
                /* Check if the current variation refers to base. */
                const FirmwareVariationInfo *cur_variation_info = extended_data_reader.GetFirmwareVariationInfo(i);
                const bool cur_refers_to_base = extended_data_reader.GetHeader()->version == 1 || cur_variation_info->refer_to_base;

                /* We force referral to base on unsupported variation only if all supported variations refer to base. */
                force_refer_to_base &= cur_refers_to_base;
            }
        }

        /* We couldn't find the input firmware variation id. */
        if (!firmware_variation_index) {
            /* Unless we can force a referral to base, the firmware isn't supported. */
            R_UNLESS(force_refer_to_base, ncm::ResultInvalidFirmwareVariation());

            /* Force a referral to base. */
            return ReadMetaInfoListFromBase();
        }

        /* Obtain the variation info. */
        const FirmwareVariationInfo *variation_info = extended_data_reader.GetFirmwareVariationInfo(*firmware_variation_index);

        /* Refer to base if variation info says we should, or if version is 1. */
        const bool refer_to_base = extended_data_reader.GetHeader()->version == 1 || variation_info->refer_to_base;
        R_UNLESS(!refer_to_base, ReadMetaInfoListFromBase());

        /* Output the content meta count. */
        const u32 content_meta_count = variation_info->content_meta_count;
        *out_count = content_meta_count;

        /* We're done if there are no content metas to list. */
        R_SUCCEED_IF(content_meta_count == 0);

        /* Allocate a buffer for the content meta infos. */
        std::unique_ptr<ContentMetaInfo[]> buffer(new (std::nothrow) ContentMetaInfo[content_meta_count]);
        AMS_ABORT_UNLESS(buffer != nullptr);

        /* Get the content meta infos. */
        Span<const ContentMetaInfo> meta_infos;
        extended_data_reader.GetContentMetaInfoList(std::addressof(meta_infos), content_meta_count);

        /* Copy the meta infos to the buffer. */
        for (size_t i = 0; i < content_meta_count; i++) {
            buffer[i] = meta_infos[i];
        }

        /* Output the content meta info buffer. */
        *out_meta_infos = std::move(buffer);
        return ResultSuccess();
    }

    void SetMountContentMetaFunction(MountContentMetaFunction func) {
        g_mount_content_meta_func = func;
    }

}

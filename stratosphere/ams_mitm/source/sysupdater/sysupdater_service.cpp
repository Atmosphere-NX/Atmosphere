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
#include "sysupdater_service.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        /* ExFat NCAs prior to 2.0.0 do not actually include the exfat driver, and don't boot. */
        constexpr inline u32 MinimumVersionForExFatDriver = 65536;

        Result ActivateSystemUpdateContentMetaDatabase() {
            /* TODO: Don't use gamecard db. */
            return ncm::ActivateContentMetaDatabase(ncm::StorageId::GameCard);
        }

        void InactivateSystemUpdateContentMetaDatabase() {
            /* TODO: Don't use gamecard db. */
            ncm::InactivateContentMetaDatabase(ncm::StorageId::GameCard);
        }

        Result OpenSystemUpdateContentMetaDatabase(ncm::ContentMetaDatabase *out) {
            /* TODO: Don't use gamecard db. */
            return ncm::OpenContentMetaDatabase(out, ncm::StorageId::GameCard);
        }

        Result GetContentInfoOfContentMeta(ncm::ContentInfo *out, ncm::ContentMetaDatabase &db, const ncm::ContentMetaKey &key) {
            s32 ofs = 0;
            while (true) {
                /* List content infos. */
                s32 count;
                ncm::ContentInfo info;
                R_TRY(db.ListContentInfo(std::addressof(count), std::addressof(info), 1, key, ofs++));

                /* No content infos left to list. */
                if (count == 0) {
                    break;
                }

                /* Check if the info is for meta content. */
                if (info.GetType() == ncm::ContentType::Meta) {
                    *out = info;
                    return ResultSuccess();
                }
            }

            /* Not found. */
            return ncm::ResultContentInfoNotFound();
        }

        Result ReadContentMetaPath(ncm::AutoBuffer *out, const char *package_root, const ncm::ContentInfo &content_info) {
            /* Get the content id string for the info. */
            auto content_id_str = ncm::GetContentIdString(content_info.GetId());

            /* Create a new path. */
            ncm::Path content_path;
            std::snprintf(content_path.str, sizeof(content_path.str), "%s/%s.cnmt.nca", package_root, content_id_str.data);

            /* Read the content meta path. */
            return ncm::ReadContentMetaPath(out, content_path.str);
        }

        bool IsExFatDriverSupported(const ncm::ContentMetaInfo &info) {
            return info.version >= MinimumVersionForExFatDriver && ((info.attributes & ncm::ContentMetaAttribute_IncludesExFatDriver) != 0);
        }

        Result FormatUserPackagePath(ncm::Path *out, const ncm::Path &user_path) {
            /* Ensure that the user path is valid. */
            R_UNLESS(user_path.str[0] == '/', fs::ResultInvalidPath());

            /* Print as @Sdcard:<user_path>/ */
            std::snprintf(out->str, sizeof(out->str), "%s:%s/", ams::fs::impl::SdCardFileSystemMountName, user_path.str);

            /* Normalize, if the user provided an ending / */
            const size_t len = std::strlen(out->str);
            if (out->str[len - 1] == '/' && out->str[len - 2] == '/') {
                out->str[len - 1] = '\x00';
            }

            return ResultSuccess();
        }

    }

    Result SystemUpdateService::GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path) {
        /* Adjust the path. */
        ncm::Path package_root;
        R_TRY(FormatUserPackagePath(std::addressof(package_root), path));

        /* Create a new update information. */
        UpdateInformation update_info = {};

        /* Parse the update. */
        {
            /* Activate the package database. */
            R_TRY(ActivateSystemUpdateContentMetaDatabase());
            ON_SCOPE_EXIT { InactivateSystemUpdateContentMetaDatabase(); };

            /* Open the package database. */
            ncm::ContentMetaDatabase package_db;
            R_TRY(OpenSystemUpdateContentMetaDatabase(std::addressof(package_db)));

            /* Cleanup and build the content meta database. */
            ncm::ContentMetaDatabaseBuilder builder(std::addressof(package_db));
            R_TRY(builder.Cleanup());
            R_TRY(builder.BuildFromPackage(package_root.str));

            /* Get the key for the SystemUpdate. */
            ncm::ContentMetaKey key;
            auto list_count = package_db.ListContentMeta(std::addressof(key), 1, ncm::ContentMetaType::SystemUpdate);
            R_UNLESS(list_count.written > 0, ncm::ResultSystemUpdateNotFoundInPackage());

            /* Get the content info for the key. */
            ncm::ContentInfo content_info;
            R_TRY(GetContentInfoOfContentMeta(std::addressof(content_info), package_db, key));

            /* Read the content meta. */
            ncm::AutoBuffer content_meta_buffer;
            R_TRY(ReadContentMetaPath(std::addressof(content_meta_buffer), package_root.str, content_info));

            /* Create a reader. */
            const auto reader = ncm::PackagedContentMetaReader(content_meta_buffer.Get(), content_meta_buffer.GetSize());

            /* Get the version from the header. */
            update_info.version = reader.GetHeader()->version;

            /* Iterate over infos to find the system update info. */
            for (size_t i = 0; i < reader.GetContentMetaCount(); ++i) {
                const auto &meta_info = *reader.GetContentMetaInfo(i);

                switch (meta_info.type) {
                    case ncm::ContentMetaType::BootImagePackage:
                        /* Detect exFAT support. */
                        update_info.exfat_supported |= IsExFatDriverSupported(meta_info);
                        break;
                    default:
                        break;
                }
            }

            /* Default to no firmware variations. */
            update_info.firmware_variation_count = 0;

            /* Parse firmware variations if relevant. */
            if (reader.GetExtendedDataSize() != 0) {
                /* Get the actual firmware variation count. */
                ncm::SystemUpdateMetaExtendedDataReader extended_data_reader(reader.GetExtendedData(), reader.GetExtendedDataSize());
                update_info.firmware_variation_count = extended_data_reader.GetFirmwareVariationCount();

                /* NOTE: Update this if Nintendo ever actually releases an update with this many variations? */
                R_UNLESS(update_info.firmware_variation_count <= FirmwareVariationCountMax, ncm::ResultInvalidFirmwareVariation());

                for (size_t i = 0; i < update_info.firmware_variation_count; ++i) {
                    update_info.firmware_variation_ids[i] = *extended_data_reader.GetFirmwareVariationId(i);
                }
            }
        }

        /* Set the parsed update info. */
        out.SetValue(update_info);
        return ResultSuccess();
    }

}

/*
 * Copyright (c) 2019 Adubbz
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

#include "ncm_icontentmetadatabase.hpp"

namespace sts::ncm {

    Result IContentMetaDatabase::EnsureEnabled() {
        if (this->disabled) {
            return ResultNcmInvalidContentMetaDatabase;
        }

        return ResultSuccess;
    }

    Result IContentMetaDatabase::Set(ContentMetaKey key, InBuffer<u8> value) {
        std::abort();
    }

    Result IContentMetaDatabase::Get(Out<u64> out_size, ContentMetaKey key, OutBuffer<u8> out_value) {
        std::abort();
    }

    Result IContentMetaDatabase::Remove(ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::GetContentIdByType(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type) {
        std::abort();
    }

    Result IContentMetaDatabase::ListContentInfo(Out<u32> out_entries_written, OutBuffer<ContentInfo> out_info, ContentMetaKey key, u32 start_index) {
        std::abort();
    }

    Result IContentMetaDatabase::List(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ContentMetaKey> out_info, ContentMetaType type, TitleId application_title_id, TitleId title_id_min, TitleId title_id_max, ContentInstallType install_type) {
        std::abort();
    }

    Result IContentMetaDatabase::GetLatestContentMetaKey(Out<ContentMetaKey> out_key, TitleId title_id) {        
        std::abort();
    }

    Result IContentMetaDatabase::ListApplication(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ApplicationContentMetaKey> out_keys, ContentMetaType type) {
        std::abort();
    }

    Result IContentMetaDatabase::Has(Out<bool> out, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::HasAll(Out<bool> out, InBuffer<ContentMetaKey> keys) {
        std::abort();
    }

    Result IContentMetaDatabase::GetSize(Out<u64> out_size, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::GetRequiredSystemVersion(Out<u32> out_version, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::GetPatchId(Out<TitleId> out_patch_id, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::DisableForcibly() {
        std::abort();
    }

    Result IContentMetaDatabase::LookupOrphanContent(OutBuffer<bool> out_orphaned, InBuffer<ContentId> content_ids) {
        std::abort();
    }

    Result IContentMetaDatabase::Commit() {
        std::abort();
    }

    Result IContentMetaDatabase::HasContent(Out<bool> out, ContentMetaKey key, ContentId content_id) {
        std::abort();
    }

    Result IContentMetaDatabase::ListContentMetaInfo(Out<u32> out_entries_written, OutBuffer<ContentMetaInfo> out_meta_info, ContentMetaKey key, u32 start_index) {
        std::abort();
    }

    Result IContentMetaDatabase::GetAttributes(Out<ContentMetaAttribute> out_attributes, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::GetRequiredApplicationVersion(Out<u32> out_version, ContentMetaKey key) {
        std::abort();
    }

    Result IContentMetaDatabase::GetContentIdByTypeAndIdOffset(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset) {
        std::abort();
    }

    Result IContentMetaDatabase::GetLatestProgram(ContentId* out_content_id, TitleId title_id) {
        std::abort();
    }

    Result IContentMetaDatabase::GetLatestData(ContentId* out_content_id, TitleId title_id) {
        std::abort();
    }

}
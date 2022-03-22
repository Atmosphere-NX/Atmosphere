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
#include <stratosphere.hpp>

#include "ncm_content_storage_impl_base.hpp"
#include "ncm_placeholder_accessor.hpp"

namespace ams::ncm {

    class ContentStorageImpl : public ContentStorageImplBase {
        private:
            class ContentIterator {
                NON_COPYABLE(ContentIterator);
                NON_MOVEABLE(ContentIterator);

                static constexpr size_t MaxDirectoryHandles = 0x8;
                static constexpr size_t MaxDirectoryEntries = 0x10;

                public:
                    fs::DirectoryHandle m_handles[MaxDirectoryHandles]{};
                    size_t m_depth{};
                    size_t m_max_depth{};
                    PathString m_path{};
                    fs::DirectoryEntry m_entries[MaxDirectoryEntries]{};
                    s64 m_entry_count{};
                public:
                   constexpr ContentIterator() = default;
                    ~ContentIterator();

                    Result Initialize(const char *root_path, size_t max_depth);
                    Result GetNext(util::optional<fs::DirectoryEntry> *out);
                private:
                    Result OpenCurrentDirectory();
                    Result OpenDirectory(const char *dir);
                    Result LoadEntries();
            };
        protected:
            PlaceHolderAccessor m_placeholder_accessor;
            ContentId m_cached_content_id;
            fs::FileHandle m_cached_file_handle;
            RightsIdCache *m_rights_id_cache;
            util::optional<ContentIterator> m_content_iterator;
            util::optional<s32> m_last_content_offset;
        public:
            static Result InitializeBase(const char *root_path);
            static Result CleanupBase(const char *root_path);
            static Result VerifyBase(const char *root_path);
        public:
            ContentStorageImpl() : m_placeholder_accessor(), m_cached_content_id(InvalidContentId), m_cached_file_handle(), m_rights_id_cache(nullptr), m_content_iterator(util::nullopt), m_last_content_offset(util::nullopt) { /* ... */ }
            ~ContentStorageImpl();

            Result Initialize(const char *root_path, MakeContentPathFunction content_path_func, MakePlaceHolderPathFunction placeholder_path_func, bool delay_flush, RightsIdCache *rights_id_cache);
        private:
            /* Helpers. */
            Result OpenContentIdFile(ContentId content_id);
            void InvalidateFileCache();
        public:
            /* Actual commands. */
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) override;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) override;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) override;
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) override;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) override;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) override;
            virtual Result Delete(ContentId content_id) override;
            virtual Result Has(sf::Out<bool> out, ContentId content_id) override;
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id) override;
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) override;
            virtual Result CleanupAllPlaceHolder() override;
            virtual Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) override;
            virtual Result GetContentCount(sf::Out<s32> out_count) override;
            virtual Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset) override;
            virtual Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) override;
            virtual Result DisableForcibly() override;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) override;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) override;
            virtual Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) override;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) override;
            virtual Result GetFreeSpaceSize(sf::Out<s64> out_size) override;
            virtual Result GetTotalSpaceSize(sf::Out<s64> out_size) override;
            virtual Result FlushPlaceHolder() override;
            virtual Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) override;
            virtual Result RepairInvalidFileAttribute() override;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) override;
            virtual Result RegisterPath(const ContentId &content_id, const Path &path) override;
            virtual Result ClearRegisteredPath() override;
    };

}

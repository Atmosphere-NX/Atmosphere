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
#include "ncm_content_storage_impl.hpp"
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        constexpr inline const char * const BaseContentDirectory = "/registered";

        void MakeBaseContentDirectoryPath(PathString *out, const char *root_path) {
            out->AssignFormat("%s%s", root_path, BaseContentDirectory);
        }

        void MakeContentPath(PathString *out, ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeBaseContentDirectoryPath(std::addressof(path), root_path);
            func(out, id, path);
        }

        Result EnsureContentDirectory(ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeContentPath(std::addressof(path), id, func, root_path);
            R_RETURN(fs::EnsureParentDirectory(path));
        }

        Result DeleteContentFile(ContentId id, MakeContentPathFunction func, const char *root_path) {
            /* Create the content path. */
            PathString path;
            MakeContentPath(std::addressof(path), id, func, root_path);

            /* Delete the content. */
            R_TRY_CATCH(fs::DeleteFile(path)) {
                R_CONVERT(fs::ResultPathNotFound, ncm::ResultContentNotFound())
            } R_END_TRY_CATCH;

            R_SUCCEED();
        }

        template<typename F>
        Result TraverseDirectory(bool *out_should_continue, const char *root_path, int max_level, F f) {
            /* If the level is zero, we're done. */
            R_SUCCEED_IF(max_level <= 0);

            /* On 1.0.0, NotRequireFileSize was not a valid open mode. */
            const auto open_dir_mode = hos::GetVersion() >= hos::Version_2_0_0 ? (fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize) : (fs::OpenDirectoryMode_All);

            /* Retry traversal upon request. */
            bool retry_dir_read = true;
            while (retry_dir_read) {
                retry_dir_read = false;

                /* Open the directory at the given path. All entry types are allowed. */
                fs::DirectoryHandle dir;
                R_TRY(fs::OpenDirectory(std::addressof(dir), root_path, open_dir_mode));
                ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                while (true) {
                    /* Read a single directory entry. */
                    fs::DirectoryEntry entry;
                    s64 entry_count;
                    R_TRY(fs::ReadDirectory(std::addressof(entry_count), std::addressof(entry), dir, 1));

                    /* Directory has no entries to process. */
                    if (entry_count == 0) {
                        break;
                    }

                    /* Path of the current entry. */
                    PathString current_path;
                    current_path.AssignFormat("%s/%s", root_path, entry.name);

                    /* Call the process function. */
                    bool should_continue = true;
                    bool should_retry_dir_read = false;
                    R_TRY(f(std::addressof(should_continue), std::addressof(should_retry_dir_read), current_path, entry));

                    /* If the provided function wishes to terminate immediately, we should respect it. */
                    if (!should_continue) {
                        *out_should_continue = false;
                        R_SUCCEED();
                    }

                    /* Mark for retry. */
                    if (should_retry_dir_read) {
                        retry_dir_read = true;
                        break;
                    }

                    /* If the entry is a directory, recurse. */
                    if (entry.type == fs::DirectoryEntryType_Directory) {
                        R_TRY(TraverseDirectory(std::addressof(should_continue), current_path, max_level - 1, f));

                        if (!should_continue) {
                            *out_should_continue = false;
                            R_SUCCEED();
                        }
                    }
                }
            }

            R_SUCCEED();
        }


        template<typename F>
        Result TraverseDirectory(const char *root_path, int max_level, F f) {
            bool should_continue = false;
            R_RETURN(TraverseDirectory(std::addressof(should_continue), root_path, max_level, f));
        }

        bool IsContentPath(const char *path) {
            impl::PathView view(path);

            /* Ensure nca suffix. */
            if (!view.HasSuffix(".nca")) {
                return false;
            }

            /* File name should be the size of a content id plus the nca file extension. */
            auto file_name = view.GetFileName();
            if (file_name.length() != ContentIdStringLength + 4) {
                return false;
            }

            /* Ensure file name is comprised of hex characters. */
            for (size_t i = 0; i < ContentIdStringLength; i++) {
                if (!std::isxdigit(static_cast<unsigned char>(file_name[i]))) {
                    return false;
                }
            }

            return true;
        }

        bool IsPlaceHolderPath(const char *path) {
            return IsContentPath(path);
        }

        Result CleanDirectoryRecursively(const PathString &path) {
            if (hos::GetVersion() >= hos::Version_3_0_0) {
                R_TRY(fs::CleanDirectoryRecursively(path));
            } else {
                /* CleanDirectoryRecursively didn't exist on < 3.0.0, so we will polyfill it. */
                /* We'll delete the directory, then recreate it. */
                R_TRY(fs::DeleteDirectoryRecursively(path));
                R_TRY(fs::CreateDirectory(path));
            }
            R_SUCCEED();
        }

    }

    ContentStorageImpl::ContentIterator::~ContentIterator() {
        for (size_t i = 0; i < m_depth; i++) {
            fs::CloseDirectory(m_handles[i]);
        }
    }

    Result ContentStorageImpl::ContentIterator::Initialize(const char *root_path, size_t max_depth) {
        /* Initialize tracking variables. */
        m_depth       = 0;
        m_max_depth   = max_depth;
        m_entry_count = 0;

        /* Create the base content directory path. */
        MakeBaseContentDirectoryPath(std::addressof(m_path), root_path);

        /* Open the base directory. */
        R_TRY(this->OpenCurrentDirectory());

        R_SUCCEED();
    }

    Result ContentStorageImpl::ContentIterator::OpenCurrentDirectory() {
        /* Determine valid directory mode (prior to 2.0.0, NotRequireFileSize was not valid). */
        const auto open_mode = hos::GetVersion() >= hos::Version_2_0_0 ? (fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize) : (fs::OpenDirectoryMode_All);

        /* Open the directory for our current path. */
        R_TRY(fs::OpenDirectory(std::addressof(m_handles[m_depth]), m_path, open_mode));

        /* Increase our depth. */
        ++m_depth;

        R_SUCCEED();
    }

    Result ContentStorageImpl::ContentIterator::OpenDirectory(const char *dir) {
        /* Set our current path. */
        m_path.Assign(dir);

        /* Open the directory. */
        R_RETURN(this->OpenCurrentDirectory());
    }

    Result ContentStorageImpl::ContentIterator::GetNext(util::optional<fs::DirectoryEntry> *out) {
        /* Iterate until we get the next entry. */
        while (true) {
            /* Ensure that we have entries loaded. */
            R_TRY(this->LoadEntries());

            /* If we failed to load any entries, there's nothing to get. */
            if (m_entry_count <= 0) {
                *out = util::nullopt;
                R_SUCCEED();
            }

            /* Get the next entry. */
            const auto &entry = m_entries[--m_entry_count];

            /* Process the current entry. */
            switch (entry.type) {
                case fs::DirectoryEntryType_Directory:
                    /* If the entry if a directory, we want to recurse into it if we can. */
                    if (m_depth < m_max_depth) {
                        /* Construct the full path for the subdirectory. */
                        PathString entry_path;
                        entry_path.AssignFormat("%s/%s", m_path.Get(), entry.name);

                        /* Open the subdirectory. */
                        R_TRY(this->OpenDirectory(entry_path.Get()));

                    }
                    break;
                case fs::DirectoryEntryType_File:
                    /* Otherwise, if the entry is a file, return it. */
                    *out = entry;
                    R_SUCCEED();
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        R_SUCCEED();
    }

    Result ContentStorageImpl::ContentIterator::LoadEntries() {
        /* If we already have entries loaded, we don't need to do anything. */
        R_SUCCEED_IF(m_entry_count != 0);

        /* If we have no directories open, there's nothing for us to load. */
        if (m_depth == 0) {
            m_entry_count = 0;
            R_SUCCEED();
        }

        /* Determine the maximum entries that we can load. */
        const s64 max_entries = m_depth == m_max_depth ? MaxDirectoryEntries : 1;

        /* Read entries from the current directory. */
        s64 num_entries;
        R_TRY(fs::ReadDirectory(std::addressof(num_entries), m_entries, m_handles[m_depth - 1], max_entries));

        /* If we successfully read entries, load them. */
        if (num_entries > 0) {
            /* Reverse the order of the loaded entries, for our future convenience. */
            for (fs::DirectoryEntry *start_entry = m_entries, *end_entry = m_entries + num_entries - 1; start_entry < end_entry; ++start_entry, --end_entry) {
                std::swap(*start_entry, *end_entry);
            }

            /* Set our entry count. */
            m_entry_count = num_entries;

            R_SUCCEED();
        }

        /* We didn't read any entries, so we need to advance to the next directory. */
        fs::CloseDirectory(m_handles[--m_depth]);

        /* Find the index of the parent directory's substring. */
        size_t i = m_path.GetLength() - 1;
        while (m_path.Get()[i--] != '/') {
            AMS_ABORT_UNLESS(i > 0);
        }

        /* Set the path to the parent directory. */
        m_path = m_path.MakeSubString(0, i + 1);

        /* Try to load again from the parent directory. */
        R_RETURN(this->LoadEntries());
    }

    ContentStorageImpl::~ContentStorageImpl() {
        this->InvalidateFileCache();
    }

    Result ContentStorageImpl::InitializeBase(const char *root_path) {
        PathString path;

        /* Create the content directory. */
        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(fs::EnsureDirectory(path));

        /* Create the placeholder directory. */
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_RETURN(fs::EnsureDirectory(path));
    }

    Result ContentStorageImpl::CleanupBase(const char *root_path) {
        PathString path;

        /* Create the content directory. */
        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(CleanDirectoryRecursively(path));

        /* Create the placeholder directory. */
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_RETURN(CleanDirectoryRecursively(path));
    }

    Result ContentStorageImpl::VerifyBase(const char *root_path) {
        PathString path;

        /* Check if root directory exists. */
        bool has_dir;
        R_TRY(fs::HasDirectory(std::addressof(has_dir), root_path));
        R_UNLESS(has_dir, ncm::ResultContentStorageBaseNotFound());

        /* Check if content directory exists. */
        bool has_registered;
        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(fs::HasDirectory(std::addressof(has_registered), path));

        /* Check if placeholder directory exists. */
        bool has_placeholder;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_TRY(fs::HasDirectory(std::addressof(has_placeholder), path));

        /* Convert findings to results. */
        R_UNLESS(has_registered || has_placeholder, ncm::ResultContentStorageBaseNotFound());
        R_UNLESS(has_registered,                    ncm::ResultInvalidContentStorageBase());
        R_UNLESS(has_placeholder,                   ncm::ResultInvalidContentStorageBase());

        R_SUCCEED();
    }

    void ContentStorageImpl::InvalidateFileCache() {
        if (m_cached_content_id != InvalidContentId) {
            fs::CloseFile(m_cached_file_handle);
            m_cached_content_id = InvalidContentId;
        }
        m_content_iterator = util::nullopt;
    }

    Result ContentStorageImpl::OpenContentIdFile(ContentId content_id) {
        /* If the file is the currently cached one, we've nothing to do. */
        R_SUCCEED_IF(m_cached_content_id == content_id);

        /* Close any cached file. */
        this->InvalidateFileCache();

        /* Create the content path. */
        PathString path;
        MakeContentPath(std::addressof(path), content_id, m_make_content_path_func, m_root_path);

        /* Open the content file and store to the cache. */
        R_TRY_CATCH(fs::OpenFile(std::addressof(m_cached_file_handle), path, fs::OpenMode_Read)) {
            R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultContentNotFound())
        } R_END_TRY_CATCH;

        m_cached_content_id = content_id;
        R_SUCCEED();
    }

    Result ContentStorageImpl::Initialize(const char *path, MakeContentPathFunction content_path_func, MakePlaceHolderPathFunction placeholder_path_func, bool delay_flush, RightsIdCache *rights_id_cache) {
        R_TRY(this->EnsureEnabled());

        /* Check paths exists for this content storage. */
        R_TRY(VerifyBase(path));

        /* Initialize members. */
        m_root_path = PathString(path);
        m_make_content_path_func = content_path_func;
        m_placeholder_accessor.Initialize(std::addressof(m_root_path), placeholder_path_func, delay_flush);
        m_rights_id_cache = rights_id_cache;
        R_SUCCEED();
    }

    Result ContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        R_TRY(this->EnsureEnabled());
        out.SetValue({util::GenerateUuid()});
        R_SUCCEED();
    }

    Result ContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
        R_TRY(this->EnsureEnabled());
        R_TRY(EnsureContentDirectory(content_id, m_make_content_path_func, m_root_path));
        R_RETURN(m_placeholder_accessor.CreatePlaceHolderFile(placeholder_id, size));
    }

    Result ContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());
        R_RETURN(m_placeholder_accessor.DeletePlaceHolderFile(placeholder_id));
    }

    Result ContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        /* Create the placeholder path. */
        PathString placeholder_path;
        m_placeholder_accessor.MakePath(std::addressof(placeholder_path), placeholder_id);

        /* Check if placeholder file exists. */
        bool has = false;
        R_TRY(fs::HasFile(std::addressof(has), placeholder_path));
        out.SetValue(has);
        R_SUCCEED();
    }

    Result ContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) {
        /* Ensure offset is valid. */
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());
        R_RETURN(m_placeholder_accessor.WritePlaceHolderFile(placeholder_id, offset, data.GetPointer(), data.GetSize()));
    }

    Result ContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        this->InvalidateFileCache();
        R_TRY(this->EnsureEnabled());

        /* Create the placeholder path. */
        PathString placeholder_path;
        m_placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Create the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, m_make_content_path_func, m_root_path);

        /* Move the placeholder to the content path. */
        R_TRY_CATCH(fs::RenameFile(placeholder_path, content_path)) {
            R_CONVERT(fs::ResultPathNotFound,      ncm::ResultPlaceHolderNotFound())
            R_CONVERT(fs::ResultPathAlreadyExists, ncm::ResultContentAlreadyExists())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result ContentStorageImpl::Delete(ContentId content_id) {
        R_TRY(this->EnsureEnabled());
        this->InvalidateFileCache();
        R_RETURN(DeleteContentFile(content_id, m_make_content_path_func, m_root_path));
    }

    Result ContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Create the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, m_make_content_path_func, m_root_path);

        /* Check if the content file exists. */
        bool has = false;
        R_TRY(fs::HasFile(std::addressof(has), content_path));
        out.SetValue(has);
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetPath(sf::Out<Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Create the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, m_make_content_path_func, m_root_path);

        /* Substitute our mount name for the common mount name. */
        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), content_path));

        out.SetValue(common_path);
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the placeholder path. */
        PathString placeholder_path;
        m_placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Substitute our mount name for the common mount name. */
        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), placeholder_path));

        out.SetValue(common_path);
        R_SUCCEED();
    }

    Result ContentStorageImpl::CleanupAllPlaceHolder() {
        R_TRY(this->EnsureEnabled());

        /* Clear the cache. */
        m_placeholder_accessor.InvalidateAll();

        /* Obtain the placeholder base directory path. */
        PathString placeholder_dir;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(placeholder_dir), m_root_path);

        /* Cleanup the placeholder base directory. */
        CleanDirectoryRecursively(placeholder_dir);
        R_SUCCEED();
    }

    Result ContentStorageImpl::ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the placeholder base directory path. */
        PathString placeholder_dir;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(placeholder_dir), m_root_path);

        const size_t max_entries = out_buf.GetSize();
        size_t entry_count = 0;

        /* Traverse the placeholder base directory finding valid placeholder files. */
        R_TRY(TraverseDirectory(placeholder_dir, m_placeholder_accessor.GetHierarchicalDirectoryDepth(), [&](bool *should_continue, bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) -> Result {
            AMS_UNUSED(current_path);

            *should_continue = true;
            *should_retry_dir_read = false;

            /* We are only looking for files. */
            if (entry.type == fs::DirectoryEntryType_File) {
                R_UNLESS(entry_count <= max_entries, ncm::ResultBufferInsufficient());

                /* Get the placeholder id from the filename. */
                PlaceHolderId placeholder_id;
                R_TRY(PlaceHolderAccessor::GetPlaceHolderIdFromFileName(std::addressof(placeholder_id), entry.name));

                out_buf[entry_count++] = placeholder_id;
            }

            R_SUCCEED();
        }));

        out_count.SetValue(static_cast<s32>(entry_count));
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetContentCount(sf::Out<s32> out_count) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the content base directory path. */
        PathString path;
        MakeBaseContentDirectoryPath(std::addressof(path), m_root_path);

        const auto depth = GetHierarchicalContentDirectoryDepth(m_make_content_path_func);
        size_t count = 0;

        /* Traverse the content base directory finding all files. */
        R_TRY(TraverseDirectory(path, depth, [&](bool *should_continue, bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) -> Result {
            AMS_UNUSED(current_path);

            *should_continue = true;
            *should_retry_dir_read = false;

            /* Increment the count for each file found. */
            if (entry.type == fs::DirectoryEntryType_File) {
                count++;
            }

            R_SUCCEED();
        }));

        out_count.SetValue(static_cast<s32>(count));
        R_SUCCEED();
    }

    Result ContentStorageImpl::ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out, s32 offset) {
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        if (!m_content_iterator.has_value() || !m_last_content_offset.has_value() || m_last_content_offset != offset) {
            /* Create and initialize the content cache. */
            m_content_iterator.emplace();
            R_TRY(m_content_iterator->Initialize(m_root_path, GetHierarchicalContentDirectoryDepth(m_make_content_path_func)));

            /* Advance to the desired offset. */
            for (auto current_offset = 0; current_offset < offset; /* ... */) {
                /* Get the next directory entry. */
                util::optional<fs::DirectoryEntry> dir_entry;
                R_TRY(m_content_iterator->GetNext(std::addressof(dir_entry)));

                /* If we run out of entries before reaching the desired offset, we're done. */
                if (!dir_entry) {
                    out_count.SetValue(0);
                    R_SUCCEED();
                }

                /* If the current entry is a valid content id, advance. */
                if (GetContentIdFromString(dir_entry->name, std::strlen(dir_entry->name))) {
                    ++current_offset;
                }
            }
        }

        /* Iterate, reading as many entries as we can. */
        s32 count = 0;
        while (count < static_cast<s32>(out.GetSize())) {
            /* Get the next directory entry. */
            util::optional<fs::DirectoryEntry> dir_entry;
            R_TRY(m_content_iterator->GetNext(std::addressof(dir_entry)));

            /* Don't continue if the directory entry is absent. */
            if (!dir_entry) {
                break;
            }

            /* Process the entry, if it's a valid content id. */
            if (auto content_id = GetContentIdFromString(dir_entry->name, std::strlen(dir_entry->name)); content_id.has_value()) {
                /* Output the content id. */
                out[count++] = *content_id;

                /* Update our last content offset. */
                m_last_content_offset = offset + count;
            }
        }

        /* Set the output count. */
        *out_count = count;

        R_SUCCEED();
    }

    Result ContentStorageImpl::GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Create the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, m_make_content_path_func, m_root_path);

        /* Open the content file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), content_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Obtain the size of the content. */
        s64 file_size;
        R_TRY(fs::GetFileSize(std::addressof(file_size), file));

        out_size.SetValue(file_size);
        R_SUCCEED();
    }

    Result ContentStorageImpl::DisableForcibly() {
        m_disabled = true;
        this->InvalidateFileCache();
        m_placeholder_accessor.InvalidateAll();
        R_SUCCEED();
    }

    Result ContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        R_TRY(this->EnsureEnabled());

        /* Close any cached file. */
        this->InvalidateFileCache();

        /* Ensure the future content directory exists. */
        R_TRY(EnsureContentDirectory(new_content_id, m_make_content_path_func, m_root_path));

        /* Ensure the destination placeholder directory exists. */
        R_TRY(m_placeholder_accessor.EnsurePlaceHolderDirectory(placeholder_id));

        /* Obtain the placeholder path. */
        PathString placeholder_path;
        m_placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Make the old content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), old_content_id, m_make_content_path_func, m_root_path);

        /* Move the content to the placeholder path. */
        R_TRY_CATCH(fs::RenameFile(content_path, placeholder_path)) {
            R_CONVERT(fs::ResultPathNotFound,      ncm::ResultPlaceHolderNotFound())
            R_CONVERT(fs::ResultPathAlreadyExists, ncm::ResultContentAlreadyExists())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result ContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
        R_TRY(this->EnsureEnabled());
        R_RETURN(m_placeholder_accessor.SetPlaceHolderFileSize(placeholder_id, size));
    }

    Result ContentStorageImpl::ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) {
        /* Ensure offset is valid. */
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        /* Create the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, m_make_content_path_func, m_root_path);

        /* Open the content file. */
        R_TRY(this->OpenContentIdFile(content_id));

        /* Read from the requested offset up to the requested size. */
        R_RETURN(fs::ReadFile(m_cached_file_handle, offset, buf.GetPointer(), buf.GetSize()));
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        /* Obtain the regular rights id for the placeholder id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromPlaceHolderIdDeprecated2(std::addressof(rights_id), placeholder_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        R_RETURN(this->GetRightsIdFromPlaceHolderId(out_rights_id, placeholder_id, fs::ContentAttributes_None));
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr) {
        R_TRY(this->EnsureEnabled());

        /* Get the placeholder path. */
        Path path;
        R_TRY(this->GetPlaceHolderPath(std::addressof(path), placeholder_id));

        /* Get the rights id for the placeholder id. */
        R_RETURN(GetRightsId(out_rights_id.GetPointer(), path, attr));
    }

    Result ContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        /* Obtain the regular rights id for the content id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentIdDeprecated2(std::addressof(rights_id), content_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
        R_RETURN(this->GetRightsIdFromContentId(out_rights_id, content_id, fs::ContentAttributes_None));
    }

    Result ContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr) {
        R_TRY(this->EnsureEnabled());

        /* Attempt to obtain the rights id from the cache. */
        if (m_rights_id_cache->Find(out_rights_id.GetPointer(), content_id)) {
            R_SUCCEED();
        }

        /* Get the path of the content. */
        Path path;
        R_TRY(this->GetPath(std::addressof(path), content_id));

        /* Obtain the rights id for the content. */
        ncm::RightsId rights_id;
        R_TRY(GetRightsId(std::addressof(rights_id), path, attr));

        /* Store the rights id to the cache. */
        m_rights_id_cache->Store(content_id, rights_id);

        out_rights_id.SetValue(rights_id);
        R_SUCCEED();
    }

    Result ContentStorageImpl::WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) {
        /* Ensure offset is valid. */
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        /* This command is for development hardware only. */
        AMS_ABORT_UNLESS(spl::IsDevelopment());

        /* Close any cached file. */
        this->InvalidateFileCache();

        /* Make the content path. */
        PathString path;
        MakeContentPath(std::addressof(path), content_id, m_make_content_path_func, m_root_path);

        /* Open the content file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path.Get(), fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Write the provided data to the file. */
        R_RETURN(fs::WriteFile(file, offset, data.GetPointer(), data.GetSize(), fs::WriteOption::Flush));
    }

    Result ContentStorageImpl::GetFreeSpaceSize(sf::Out<s64> out_size) {
        R_RETURN(fs::GetFreeSpaceSize(out_size.GetPointer(), m_root_path));
    }

    Result ContentStorageImpl::GetTotalSpaceSize(sf::Out<s64> out_size) {
        R_RETURN(fs::GetTotalSpaceSize(out_size.GetPointer(), m_root_path));
    }

    Result ContentStorageImpl::FlushPlaceHolder() {
        m_placeholder_accessor.InvalidateAll();
        R_SUCCEED();
    }

    Result ContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<s64> out_size, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        /* Attempt to get the placeholder file size. */
        bool found = false;
        s64 file_size = 0;
        R_TRY(m_placeholder_accessor.TryGetPlaceHolderFileSize(std::addressof(found), std::addressof(file_size), placeholder_id));

        /* Set the output if placeholder file is found. */
        if (found) {
            out_size.SetValue(file_size);
            R_SUCCEED();
        }

        /* Get the path of the placeholder. */
        PathString placeholder_path;
        m_placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Open the placeholder file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), placeholder_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Get the size of the placeholder file. */
        R_TRY(fs::GetFileSize(std::addressof(file_size), file));

        out_size.SetValue(file_size);
        R_SUCCEED();
    }

    Result ContentStorageImpl::RepairInvalidFileAttribute() {
        /* Callback for TraverseDirectory */
        using PathChecker = bool (*)(const char *);
        PathChecker path_checker = nullptr;

        /* Set the archive bit appropriately for content/placeholders. */
        auto fix_file_attributes = [&](bool *should_continue, bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) {
            *should_retry_dir_read = false;
            *should_continue = true;

            if (entry.type == fs::DirectoryEntryType_Directory) {
                if (path_checker(current_path)) {
                    if (R_SUCCEEDED(fs::SetConcatenationFileAttribute(current_path))) {
                        *should_retry_dir_read = true;
                    }
                }
            }

            R_SUCCEED();
        };

        /* Fix content. */
        {
            path_checker = IsContentPath;
            PathString path;
            MakeBaseContentDirectoryPath(std::addressof(path), m_root_path);

            R_TRY(TraverseDirectory(path, GetHierarchicalContentDirectoryDepth(m_make_content_path_func), fix_file_attributes));
        }

        /* Fix placeholders. */
        m_placeholder_accessor.InvalidateAll();
        {
            path_checker = IsPlaceHolderPath;
            PathString path;
            PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), m_root_path);

            R_TRY(TraverseDirectory(path, GetHierarchicalContentDirectoryDepth(m_make_content_path_func), fix_file_attributes));
        }

        R_SUCCEED();
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr) {
        R_TRY(this->EnsureEnabled());

        /* Attempt to find the rights id in the cache. */
        if (m_rights_id_cache->Find(out_rights_id.GetPointer(), cache_content_id)) {
            R_SUCCEED();
        }

        /* Get the placeholder path. */
        PathString placeholder_path;
        m_placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        /* Substitute mount name with the common mount name. */
        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), placeholder_path));

        /* Get the rights id. */
        ncm::RightsId rights_id;
        R_TRY(GetRightsId(std::addressof(rights_id), common_path, attr));
        m_rights_id_cache->Store(cache_content_id, rights_id);

        /* Set output. */
        out_rights_id.SetValue(rights_id);
        R_SUCCEED();
    }

    Result ContentStorageImpl::RegisterPath(const ContentId &content_id, const Path &path) {
        AMS_UNUSED(content_id, path);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result ContentStorageImpl::ClearRegisteredPath() {
        R_THROW(ncm::ResultInvalidOperation());
    }

}

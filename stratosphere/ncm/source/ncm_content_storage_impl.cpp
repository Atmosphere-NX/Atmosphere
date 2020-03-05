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

#include "ncm_content_storage_impl.hpp"
#include "ncm_fs_utils.hpp"
#include "ncm_make_path.hpp"
#include "ncm_content_id_utils.hpp"

namespace ams::ncm {

    namespace {

        constexpr inline const char * const BaseContentDirectory = "/registered";

        void MakeBaseContentDirectoryPath(PathString *out, const char *root_path) {
            out->SetFormat("%s%s", root_path, BaseContentDirectory);
        }

        void MakeContentPath(PathString *out, ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeBaseContentDirectoryPath(std::addressof(path), root_path);
            func(out, id, path);
        }

        Result EnsureContentDirectory(ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeContentPath(std::addressof(path), id, func, root_path);
            return impl::EnsureParentDirectoryRecursively(path);
        }

        Result DeleteContentFile(ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeContentPath(std::addressof(path), id, func, root_path);

            R_TRY_CATCH(fs::DeleteFile(path)) {
                R_CONVERT(fs::ResultPathNotFound, ncm::ResultContentNotFound())
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

        template<typename F>
        Result TraverseDirectory(bool *out_should_continue, const char *root_path, int max_level, F f) {
            R_UNLESS(max_level > 0, ResultSuccess());

            bool retry_dir_read = true;
            while (retry_dir_read) {
                retry_dir_read = false;

                fs::DirectoryHandle dir;
                R_TRY(fs::OpenDirectory(std::addressof(dir), root_path, fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize));
                ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                while (true) {
                    fs::DirectoryEntry entry;
                    s64 entry_count;
                    R_TRY(fs::ReadDirectory(std::addressof(entry_count), std::addressof(entry), dir, 1));
                    if (entry_count == 0) {
                        break;
                    }

                    PathString current_path;
                    current_path.SetFormat("%s/%s", root_path, entry.name);

                    bool should_continue = true;
                    bool should_retry_dir_read = false;
                    R_TRY(f(&should_continue, &should_retry_dir_read, current_path, entry));

                    /* If the provided function wishes to terminate immediately, we should respect it. */
                    if (!should_continue) {
                        *out_should_continue = false;
                        return ResultSuccess();
                    }

                    if (should_retry_dir_read) {
                        retry_dir_read = true;
                        break;
                    }

                    /* If the entry is a directory, recurse. */
                    if (entry.type == fs::DirectoryEntryType_Directory) {
                        R_TRY(TraverseDirectory(std::addressof(should_continue), current_path, max_level - 1, f));

                        if (!should_continue) {
                            *out_should_continue = false;
                            return ResultSuccess();
                        }
                    }
                }
            }

            return ResultSuccess();
        }


        template<typename F>
        Result TraverseDirectory(const char *root_path, int max_level, F f) {
            bool should_continue = false;
            return TraverseDirectory(std::addressof(should_continue), root_path, max_level, f);
        }

        bool IsContentPath(const char *path) {
            impl::PathView view(path);
            if (!view.HasSuffix(".nca")) {
                return false;
            }

            auto file_name = view.GetFileName();
            if (file_name.length() != ContentIdStringLength + 4) {
                return false;
            }

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
            if (hos::GetVersion() >= hos::Version_300) {
                R_TRY(fs::CleanDirectoryRecursively(path));
            } else {
                /* CleanDirectoryRecursively didn't exist on < 3.0.0, so we will polyfill it. */
                /* We'll delete the directory, then recreate it. */
                R_TRY(fs::DeleteDirectoryRecursively(path));
                R_TRY(fs::CreateDirectory(path));
            }
            return ResultSuccess();
        }

    }

    ContentStorageImpl::~ContentStorageImpl() {
        this->InvalidateFileCache();
    }

    Result ContentStorageImpl::InitializeBase(const char *root_path) {
        PathString path;

        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(impl::EnsureDirectoryRecursively(path));

        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_TRY(impl::EnsureDirectoryRecursively(path));

        return ResultSuccess();
    }

    Result ContentStorageImpl::CleanupBase(const char *root_path) {
        PathString path;

        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(CleanDirectoryRecursively(path));

        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_TRY(CleanDirectoryRecursively(path));

        return ResultSuccess();
    }

    Result ContentStorageImpl::VerifyBase(const char *root_path) {
        PathString path;

        bool has_dir;
        R_TRY(impl::HasDirectory(std::addressof(has_dir), root_path));
        R_UNLESS(has_dir, ncm::ResultContentStorageBaseNotFound());

        bool has_registered;
        MakeBaseContentDirectoryPath(std::addressof(path), root_path);
        R_TRY(impl::HasDirectory(std::addressof(has_registered), path));

        bool has_placeholder;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), root_path);
        R_TRY(impl::HasDirectory(std::addressof(has_placeholder), path));

        R_UNLESS(has_registered || has_placeholder, ncm::ResultContentStorageBaseNotFound());
        R_UNLESS(has_registered,                    ncm::ResultInvalidContentStorageBase());
        R_UNLESS(has_placeholder,                   ncm::ResultInvalidContentStorageBase());

        return ResultSuccess();
    }

    void ContentStorageImpl::InvalidateFileCache() {
        if (this->cached_content_id != InvalidContentId) {
            fs::CloseFile(this->cached_file_handle);
            this->cached_content_id = InvalidContentId;
        }
    }

    Result ContentStorageImpl::OpenContentIdFile(ContentId content_id) {
        R_UNLESS(this->cached_content_id != content_id, ResultSuccess());

        this->InvalidateFileCache();

        PathString path;
        MakeContentPath(std::addressof(path), content_id, this->make_content_path_func, this->root_path);

        R_TRY_CATCH(fs::OpenFile(&this->cached_file_handle, path, fs::OpenMode_Read)) {
            R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultContentNotFound())
        } R_END_TRY_CATCH;

        this->cached_content_id = content_id;
        return ResultSuccess();
    }

    Result ContentStorageImpl::Initialize(const char *path, MakeContentPathFunction content_path_func, MakePlaceHolderPathFunction placeholder_path_func, bool delay_flush, RightsIdCache *rights_id_cache) {
        R_TRY(this->EnsureEnabled());

        R_TRY(VerifyBase(path));

        this->root_path = PathString(path);
        this->make_content_path_func = content_path_func;
        this->placeholder_accessor.Initialize(std::addressof(this->root_path), placeholder_path_func, delay_flush);
        this->rights_id_cache = rights_id_cache;

        return ResultSuccess();
    }

    Result ContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        R_TRY(this->EnsureEnabled());
        out.SetValue({util::GenerateUuid()});
        return ResultSuccess();
    }

    Result ContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        R_TRY(this->EnsureEnabled());

        R_TRY(EnsureContentDirectory(content_id, this->make_content_path_func, this->root_path));
        R_TRY(this->placeholder_accessor.CreatePlaceHolderFile(placeholder_id, size));

        return ResultSuccess();
    }

    Result ContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());
        return this->placeholder_accessor.DeletePlaceHolderFile(placeholder_id);
    }

    Result ContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        PathString placeholder_path;
        this->placeholder_accessor.MakePath(std::addressof(placeholder_path), placeholder_id);

        bool has = false;
        R_TRY(impl::HasFile(&has, placeholder_path));
        out.SetValue(has);

        return ResultSuccess();
    }

    Result ContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data) {
        /* Offset is too large */
        R_UNLESS(offset <= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());
        return this->placeholder_accessor.WritePlaceHolderFile(placeholder_id, offset, data.GetPointer(), data.GetSize());
        return ResultSuccess();
    }

    Result ContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        this->InvalidateFileCache();
        R_TRY(this->EnsureEnabled());

        PathString placeholder_path;
        PathString content_path;
        this->placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        R_TRY_CATCH(fs::RenameFile(placeholder_path, content_path)) {
            R_CONVERT(fs::ResultPathNotFound,      ncm::ResultPlaceHolderNotFound())
            R_CONVERT(fs::ResultPathAlreadyExists, ncm::ResultContentAlreadyExists())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentStorageImpl::Delete(ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        this->InvalidateFileCache();

        return DeleteContentFile(content_id, this->make_content_path_func, this->root_path);
    }

    Result ContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        bool has = false;
        R_TRY(impl::HasFile(&has, content_path));
        out.SetValue(has);

        return ResultSuccess();
    }

    Result ContentStorageImpl::GetPath(sf::Out<Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), content_path));

        out.SetValue(common_path);
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        PathString placeholder_path;
        this->placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), placeholder_path));

        out.SetValue(common_path);
        return ResultSuccess();
    }

    Result ContentStorageImpl::CleanupAllPlaceHolder() {
        R_TRY(this->EnsureEnabled());

        this->placeholder_accessor.InvalidateAll();

        PathString placeholder_dir;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(placeholder_dir), this->root_path);

        CleanDirectoryRecursively(placeholder_dir);

        return ResultSuccess();
    }

    Result ContentStorageImpl::ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        R_TRY(this->EnsureEnabled());

        PathString placeholder_dir;
        PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(placeholder_dir), this->root_path);
        const size_t max_entries = out_buf.GetSize();
        size_t entry_count = 0;

        R_TRY(TraverseDirectory(placeholder_dir, placeholder_accessor.GetHierarchicalDirectoryDepth(), [&](bool *should_continue, bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) -> Result {
            *should_continue = true;
            *should_retry_dir_read = false;

            if (entry.type == fs::DirectoryEntryType_File) {
                R_UNLESS(entry_count <= max_entries, ncm::ResultBufferInsufficient());

                PlaceHolderId placeholder_id;
                R_TRY(PlaceHolderAccessor::GetPlaceHolderIdFromFileName(std::addressof(placeholder_id), entry.name));

                out_buf[entry_count++] = placeholder_id;
            }

            return ResultSuccess();
        }));

        out_count.SetValue(static_cast<u32>(entry_count));
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetContentCount(sf::Out<u32> out_count) {
        R_TRY(this->EnsureEnabled());

        PathString path;
        MakeBaseContentDirectoryPath(std::addressof(path), this->root_path);
        const auto depth = GetHierarchicalContentDirectoryDepth(this->make_content_path_func);
        size_t count = 0;

        R_TRY(TraverseDirectory(path, depth, [&](bool *should_continue, bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) -> Result {
            *should_continue = true;
            *should_retry_dir_read = false;

            if (entry.type == fs::DirectoryEntryType_File) {
                count++;
            }

            return ResultSuccess();
        }));

        out_count.SetValue(static_cast<u32>(count));
        return ResultSuccess();
    }

    Result ContentStorageImpl::ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 offset) {
        R_UNLESS(offset <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        PathString path;
        MakeBaseContentDirectoryPath(std::addressof(path), this->root_path);
        const auto depth = GetHierarchicalContentDirectoryDepth(this->make_content_path_func);
        size_t entry_count = 0;

        R_TRY(TraverseDirectory(path, depth, [&](bool *should_continue,  bool *should_retry_dir_read, const char *current_path, const fs::DirectoryEntry &entry) {
            *should_retry_dir_read = false;
            *should_continue = true;

            /* We have nothing to do if not working with a file. */
            if (entry.type != fs::DirectoryEntryType_File) {
                return ResultSuccess();
            }

            /* Skip entries until we reach the start offset. */
            if (offset > 0) {
                --offset;
                return ResultSuccess();
            }

            /* We don't necessarily expect to be able to completely fill the output buffer. */
            if (entry_count >= out_buf.GetSize()) {
                *should_continue = false;
                return ResultSuccess();
            }

            auto content_id = GetContentIdFromString(entry.name, std::strlen(entry.name));
            if (content_id) {
                out_buf[entry_count++] = *content_id;
            }

            return ResultSuccess();
        }));

        out_count.SetValue(static_cast<u32>(entry_count));
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), content_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        s64 file_size;
        R_TRY(fs::GetFileSize(std::addressof(file_size), file));

        out_size.SetValue(file_size);
        return ResultSuccess();
    }

    Result ContentStorageImpl::DisableForcibly() {
        this->disabled = true;
        this->InvalidateFileCache();
        this->placeholder_accessor.InvalidateAll();
        return ResultSuccess();
    }

    Result ContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        R_TRY(this->EnsureEnabled());

        this->InvalidateFileCache();

        R_TRY(EnsureContentDirectory(new_content_id, this->make_content_path_func, this->root_path));
        R_TRY(this->placeholder_accessor.EnsurePlaceHolderDirectory(placeholder_id));

        PathString placeholder_path;
        PathString content_path;
        this->placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);
        MakeContentPath(std::addressof(content_path), old_content_id, this->make_content_path_func, this->root_path);

        R_TRY_CATCH(fs::RenameFile(content_path, placeholder_path)) {
            R_CONVERT(fs::ResultPathNotFound,      ncm::ResultPlaceHolderNotFound())
            R_CONVERT(fs::ResultPathAlreadyExists, ncm::ResultContentAlreadyExists())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->placeholder_accessor.SetPlaceHolderFileSize(placeholder_id, size));
        return ResultSuccess();
    }

    Result ContentStorageImpl::ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset) {
        /* Offset is too large */
        R_UNLESS(offset <= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        R_TRY(this->OpenContentIdFile(content_id));

        R_TRY(fs::ReadFile(this->cached_file_handle, offset, buf.GetPointer(), buf.GetSize()));

        return ResultSuccess();
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromPlaceHolderId(&rights_id, placeholder_id));
        out_rights_id.SetValue(rights_id.id);
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        Path path;
        R_TRY(this->GetPlaceHolderPath(std::addressof(path), placeholder_id));

        return GetRightsId(out_rights_id.GetPointer(), path);
    }

    Result ContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentId(&rights_id, content_id));
        out_rights_id.SetValue(rights_id.id);
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        if (this->rights_id_cache->Find(out_rights_id.GetPointer(), content_id)) {
            return ResultSuccess();
        }

        Path path;
        R_TRY(this->GetPath(std::addressof(path), content_id));

        ncm::RightsId rights_id;
        R_TRY(GetRightsId(std::addressof(rights_id), path));
        this->rights_id_cache->Store(content_id, rights_id);

        out_rights_id.SetValue(rights_id);
        return ResultSuccess();
    }

    Result ContentStorageImpl::WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data) {
        /* Offset is too large */
        R_UNLESS(offset <= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        AMS_ABORT_UNLESS(spl::IsDevelopmentHardware());

        this->InvalidateFileCache();

        PathString path;
        MakeContentPath(std::addressof(path), content_id, this->make_content_path_func, this->root_path);

        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path.Get(), fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        R_TRY(fs::WriteFile(file, offset, data.GetPointer(), data.GetSize(), fs::WriteOption::Flush));

        return ResultSuccess();
    }

    Result ContentStorageImpl::GetFreeSpaceSize(sf::Out<u64> out_size) {
        s64 size;
        R_TRY(fs::GetFreeSpaceSize(std::addressof(size), this->root_path));
        out_size.SetValue(size);
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetTotalSpaceSize(sf::Out<u64> out_size) {
        s64 size;
        R_TRY(fs::GetTotalSpaceSize(std::addressof(size), this->root_path));
        out_size.SetValue(size);
        return ResultSuccess();
    }

    Result ContentStorageImpl::FlushPlaceHolder() {
        this->placeholder_accessor.InvalidateAll();
        return ResultSuccess();
    }

    Result ContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<u64> out_size, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        bool found = false;
        s64 file_size = 0;
        R_TRY(this->placeholder_accessor.TryGetPlaceHolderFileSize(std::addressof(found), std::addressof(file_size), placeholder_id));
        if (found) {
            out_size.SetValue(file_size);
            return ResultSuccess();
        }

        PathString placeholder_path;
        this->placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), placeholder_path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        R_TRY(fs::GetFileSize(std::addressof(file_size), file));

        out_size.SetValue(file_size);
        return ResultSuccess();
    }

    Result ContentStorageImpl::RepairInvalidFileAttribute() {
        /* Callback for TraverseDirectory */
        using PathChecker = bool (*)(const char *);
        PathChecker path_checker = nullptr;

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

            return ResultSuccess();
        };

        /* Fix Content */
        {
            path_checker = IsContentPath;
            PathString path;
            MakeBaseContentDirectoryPath(std::addressof(path), this->root_path);

            R_TRY(TraverseDirectory(path, GetHierarchicalContentDirectoryDepth(this->make_content_path_func), fix_file_attributes));
        }

        /* Fix placeholder. */
        this->placeholder_accessor.InvalidateAll();
        {
            path_checker = IsPlaceHolderPath;
            PathString path;
            PlaceHolderAccessor::MakeBaseDirectoryPath(std::addressof(path), this->root_path);

            R_TRY(TraverseDirectory(path, GetHierarchicalContentDirectoryDepth(this->make_content_path_func), fix_file_attributes));
        }

        return ResultSuccess();
    }

    Result ContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        R_TRY(this->EnsureEnabled());

        if (this->rights_id_cache->Find(out_rights_id.GetPointer(), cache_content_id)) {
            return ResultSuccess();
        }

        PathString placeholder_path;
        this->placeholder_accessor.GetPath(std::addressof(placeholder_path), placeholder_id);

        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), placeholder_path));

        ncm::RightsId rights_id;
        R_TRY(GetRightsId(&rights_id, common_path));
        this->rights_id_cache->Store(cache_content_id, rights_id);

        /* Set output. */
        out_rights_id.SetValue(rights_id);
        return ResultSuccess();
    }

}

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

namespace ams::fssystem {

    namespace {

        Result EnsureDirectoryImpl(fs::fsa::IFileSystem *fs, const fs::Path &path) {
            /* Create work path. */
            fs::Path work_path;
            R_TRY(work_path.Initialize(path));

            /* Create a directory path parser. */
            fs::DirectoryPathParser parser;
            R_TRY(parser.Initialize(std::addressof(work_path)));

            bool is_finished;
            do {
                /* Get the current path. */
                const fs::Path &cur_path = parser.GetCurrentPath();

                /* Get the entry type for the current path. */
                fs::DirectoryEntryType type;
                R_TRY_CATCH(fs->GetEntryType(std::addressof(type), cur_path)) {
                    R_CATCH(fs::ResultPathNotFound) {
                        /* The path doesn't exist. We should create it. */
                        R_TRY(fs->CreateDirectory(cur_path));

                        /* Get the updated entry type. */
                        R_TRY(fs->GetEntryType(std::addressof(type), cur_path));
                    }
                } R_END_TRY_CATCH;

                /* Verify that the current entry isn't a file. */
                R_UNLESS(type != fs::DirectoryEntryType_File, fs::ResultPathAlreadyExists());

                /* Advance to the next part of the path. */
                R_TRY(parser.ReadNext(std::addressof(is_finished)));
            } while (!is_finished);

            R_SUCCEED();
        }

        Result HasEntry(bool *out, fs::fsa::IFileSystem *fsa, const fs::Path &path, fs::DirectoryEntryType type) {
            /* Set out to false initially. */
            *out = false;

            /* Try to get the entry type. */
            fs::DirectoryEntryType entry_type;
            R_TRY_CATCH(fsa->GetEntryType(std::addressof(entry_type), path)) {
                /* If the path doesn't exist, nothing has gone wrong. */
                R_CONVERT(fs::ResultPathNotFound, ResultSuccess());
            } R_END_TRY_CATCH;

            /* We succeeded. */
            *out = entry_type == type;
            R_SUCCEED();
        }

    }

    Result CopyFile(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const fs::Path &dst_path, const fs::Path &src_path, void *work_buf, size_t work_buf_size) {
        /* Open source file. */
        std::unique_ptr<fs::fsa::IFile> src_file;
        R_TRY(src_fs->OpenFile(std::addressof(src_file), src_path, fs::OpenMode_Read));

        /* Get the file size. */
        s64 file_size;
        R_TRY(src_file->GetSize(std::addressof(file_size)));

        /* Open dst file. */
        std::unique_ptr<fs::fsa::IFile> dst_file;
        R_TRY(dst_fs->CreateFile(dst_path, file_size));
        R_TRY(dst_fs->OpenFile(std::addressof(dst_file), dst_path, fs::OpenMode_Write));

        /* Read/Write file in work buffer sized chunks. */
        s64 remaining = file_size;
        s64 offset = 0;
        while (remaining > 0) {
            size_t read_size;
            R_TRY(src_file->Read(std::addressof(read_size), offset, work_buf, work_buf_size, fs::ReadOption()));
            R_TRY(dst_file->Write(offset, work_buf, read_size, fs::WriteOption()));

            remaining -= read_size;
            offset    += read_size;
        }

        R_SUCCEED();
    }

    Result CopyDirectoryRecursively(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const fs::Path &dst_path, const fs::Path &src_path, fs::DirectoryEntry *entry, void *work_buf, size_t work_buf_size) {
        /* Set up the destination work path to point at the target directory. */
        fs::Path dst_work_path;
        R_TRY(dst_work_path.Initialize(dst_path));

        /* Iterate, copying files. */
        R_RETURN(IterateDirectoryRecursively(src_fs, src_path, entry,
            [&](const fs::Path &path, const fs::DirectoryEntry &entry) -> Result { /* On Enter Directory */
                AMS_UNUSED(path, entry);

                /* Append the current entry to the dst work path. */
                R_TRY(dst_work_path.AppendChild(entry.name));

                /* Create the directory. */
                R_RETURN(dst_fs->CreateDirectory(dst_work_path));
            },
            [&](const fs::Path &path, const fs::DirectoryEntry &entry) -> Result { /* On Exit Directory */
                AMS_UNUSED(path, entry);

                /* Remove the directory we're leaving from the dst work path. */
                R_RETURN(dst_work_path.RemoveChild());
            },
            [&](const fs::Path &path, const fs::DirectoryEntry &entry) -> Result { /* On File */
                /* Append the current entry to the dst work path. */
                R_TRY(dst_work_path.AppendChild(entry.name));

                /* Copy the file. */
                R_TRY(fssystem::CopyFile(dst_fs, src_fs, dst_work_path, path, work_buf, work_buf_size));

                /* Remove the current entry from the dst work path. */
                R_RETURN(dst_work_path.RemoveChild());
            }
        ));
    }

    Result HasFile(bool *out, fs::fsa::IFileSystem *fs, const fs::Path &path) {
        R_RETURN(HasEntry(out, fs, path, fs::DirectoryEntryType_File));
    }

    Result HasDirectory(bool *out, fs::fsa::IFileSystem *fs, const fs::Path &path) {
        R_RETURN(HasEntry(out, fs, path, fs::DirectoryEntryType_Directory));
    }

    Result EnsureDirectory(fs::fsa::IFileSystem *fs, const fs::Path &path) {
        /* First, check if the directory already exists. If it does, we're good to go. */
        fs::DirectoryEntryType type;
        R_TRY_CATCH(fs->GetEntryType(std::addressof(type), path)) {
            /* If the directory doesn't already exist, we should create it. */
            R_CATCH(fs::ResultPathNotFound) {
                R_TRY(EnsureDirectoryImpl(fs, path));
            }
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result TryAcquireCountSemaphore(util::unique_lock<SemaphoreAdaptor> *out, SemaphoreAdaptor *adaptor) {
        /* Create deferred unique lock. */
        util::unique_lock<SemaphoreAdaptor> lock(*adaptor, std::defer_lock);

        /* Try to lock. */
        R_UNLESS(lock.try_lock(), fs::ResultOpenCountLimit());

        /* Set the output lock. */
        *out = std::move(lock);
        R_SUCCEED();
    }

    void AddCounter(void *_counter, size_t counter_size, u64 value) {
        u8 *counter   = static_cast<u8 *>(_counter);
        u64 remaining = value;
        u8 carry = 0;

        for (size_t i = 0; i < counter_size; i++) {
            auto sum  = counter[counter_size - 1 - i] + (remaining & 0xFF) + carry;
            carry     = static_cast<u8>(sum >> BITSIZEOF(u8));
            auto sum8 = static_cast<u8>(sum & 0xFF);

            counter[counter_size - 1 - i] = sum8;

            remaining >>= BITSIZEOF(u8);
            if (carry == 0 && remaining == 0) {
                break;
            }
        }
    }

}

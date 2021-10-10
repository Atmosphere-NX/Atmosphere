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

namespace ams::fs::impl {

    struct FilePathHash;
    class FileSystemAccessor;

    enum class WriteState {
        None,
        NeedsFlush,
        Failed,
    };

    class FileAccessor : public util::IntrusiveListBaseNode<FileAccessor>, public Newable {
        NON_COPYABLE(FileAccessor);
        private:
            std::unique_ptr<fsa::IFile> m_impl;
            FileSystemAccessor * const m_parent;
            WriteState m_write_state;
            Result m_write_result;
            const OpenMode m_open_mode;
            std::unique_ptr<FilePathHash> m_file_path_hash;
            s32 m_path_hash_index;
        public:
            FileAccessor(std::unique_ptr<fsa::IFile>&& f, FileSystemAccessor *p, OpenMode mode);
            ~FileAccessor();

            Result Read(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option);
            Result Write(s64 offset, const void *buf, size_t size, const WriteOption &option);
            Result Flush();
            Result SetSize(s64 size);
            Result GetSize(s64 *out);
            Result OperateRange(void *dst, size_t dst_size, OperationId operation, s64 offset, s64 size, const void *src, size_t src_size);

            OpenMode GetOpenMode() const { return m_open_mode; }
            WriteState GetWriteState() const { return m_write_state; }
            FileSystemAccessor *GetParent() const { return m_parent; }

            void SetFilePathHash(std::unique_ptr<FilePathHash>&& file_path_hash, s32 index);
            Result ReadWithoutCacheAccessLog(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option);
        private:
            Result ReadWithCacheAccessLog(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option, bool use_path_cache, bool use_data_cache);

            ALWAYS_INLINE Result UpdateLastResult(Result r) {
                if (!fs::ResultNotEnoughFreeSpace::Includes(r)) {
                    m_write_result = r;
                }
                return r;
            }
    };

}

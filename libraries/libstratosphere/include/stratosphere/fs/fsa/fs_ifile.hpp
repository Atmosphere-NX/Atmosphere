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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_operate_range.hpp>

namespace ams::fs::fsa {

    class IFile {
        public:
            virtual ~IFile() { /* ... */ }

            Result Read(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
                /* Check that we have an output pointer. */
                R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

                /* If we have nothing to read, just succeed. */
                if (size == 0) {
                    *out = 0;
                    R_SUCCEED();
                }

                /* Check that the read is valid. */
                R_UNLESS(buffer != nullptr,                              fs::ResultNullptrArgument());
                R_UNLESS(offset >= 0,                                    fs::ResultOutOfRange());
                R_UNLESS(util::IsIntValueRepresentable<s64>(size),       fs::ResultOutOfRange());
                R_UNLESS(util::CanAddWithoutOverflow<s64>(offset, size), fs::ResultOutOfRange());

                /* Do the read. */
                R_RETURN(this->DoRead(out, offset, buffer, size, option));
            }

            ALWAYS_INLINE Result Read(size_t *out, s64 offset, void *buffer, size_t size) { R_RETURN(this->Read(out, offset, buffer, size, ReadOption::None)); }

            Result GetSize(s64 *out) {
                R_UNLESS(out != nullptr, fs::ResultNullptrArgument());
                R_RETURN(this->DoGetSize(out));
            }

            Result Flush() {
                R_RETURN(this->DoFlush());
            }

            Result Write(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
                /* Handle the zero-size case. */
                if (size == 0) {
                    if (option.HasFlushFlag()) {
                        R_TRY(this->Flush());
                    }
                    R_SUCCEED();
                }

                /* Check the write is valid. */
                R_UNLESS(buffer != nullptr,                              fs::ResultNullptrArgument());
                R_UNLESS(offset >= 0,                                    fs::ResultOutOfRange());
                R_UNLESS(util::IsIntValueRepresentable<s64>(size),       fs::ResultOutOfRange());
                R_UNLESS(util::CanAddWithoutOverflow<s64>(offset, size), fs::ResultOutOfRange());

                R_RETURN(this->DoWrite(offset, buffer, size, option));
            }

            Result SetSize(s64 size) {
                R_UNLESS(size >= 0, fs::ResultOutOfRange());
                R_RETURN(this->DoSetSize(size));
            }

            Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
                R_RETURN(this->DoOperateRange(dst, dst_size, op_id, offset, size, src, src_size));
            }

            Result OperateRange(fs::OperationId op_id, s64 offset, s64 size) {
                R_RETURN(this->DoOperateRange(nullptr, 0, op_id, offset, size, nullptr, 0));
            }
        public:
            /* TODO: This is a hack to allow the mitm API to work. Find a better way? */
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const = 0;
        protected:
            Result DryRead(size_t *out, s64 offset, size_t size, const fs::ReadOption &option, OpenMode open_mode) {
                AMS_UNUSED(option);

                /* Check that we can read. */
                R_UNLESS((open_mode & OpenMode_Read) != 0, fs::ResultReadNotPermitted());

                /* Get the file size, and validate our offset. */
                s64 file_size = 0;
                R_TRY(this->DoGetSize(std::addressof(file_size)));
                R_UNLESS(offset <= file_size, fs::ResultOutOfRange());

                *out = static_cast<size_t>(std::min(file_size - offset, static_cast<s64>(size)));
                R_SUCCEED();
            }

            Result DrySetSize(s64 size, fs::OpenMode open_mode) {
                AMS_UNUSED(size);

                /* Check that we can write. */
                R_UNLESS((open_mode & OpenMode_Write) != 0, fs::ResultWriteNotPermitted());
                R_SUCCEED();
            }

            Result DryWrite(bool *out_append, s64 offset, size_t size, const fs::WriteOption &option, fs::OpenMode open_mode) {
                AMS_UNUSED(option);

                /* Check that we can write. */
                R_UNLESS((open_mode & OpenMode_Write) != 0, fs::ResultWriteNotPermitted());

                /* Get the file size. */
                s64 file_size = 0;
                R_TRY(this->DoGetSize(&file_size));

                /* Determine if we need to append. */
                *out_append = false;
                if (file_size < offset + static_cast<s64>(size)) {
                    R_UNLESS((open_mode & OpenMode_AllowAppend) != 0, fs::ResultFileExtensionWithoutOpenModeAllowAppend());
                    *out_append = true;
                }

                R_SUCCEED();
            }
        private:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) = 0;
            virtual Result DoGetSize(s64 *out) = 0;
            virtual Result DoFlush() = 0;
            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) = 0;
            virtual Result DoSetSize(s64 size) = 0;
            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) = 0;
    };

}

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
                R_UNLESS(out != nullptr, fs::ResultNullptrArgument());
                if (size == 0) {
                    *out = 0;
                    return ResultSuccess();
                }
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());
                R_UNLESS(offset >= 0, fs::ResultOutOfRange());
                const s64 signed_size = static_cast<s64>(size);
                R_UNLESS(signed_size >= 0, fs::ResultOutOfRange());
                R_UNLESS((std::numeric_limits<s64>::max() - offset) >= signed_size, fs::ResultOutOfRange());
                return this->DoRead(out, offset, buffer, size, option);
            }

            Result Read(size_t *out, s64 offset, void *buffer, size_t size) {
                return this->Read(out, offset, buffer, size, ReadOption::None);
            }

            Result GetSize(s64 *out) {
                R_UNLESS(out != nullptr, fs::ResultNullptrArgument());
                return this->DoGetSize(out);
            }

            Result Flush() {
                return this->DoFlush();
            }

            Result Write(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
                if (size == 0) {
                    if (option.HasFlushFlag()) {
                        R_TRY(this->Flush());
                    }
                    return ResultSuccess();
                }
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());
                R_UNLESS(offset >= 0, fs::ResultOutOfRange());
                const s64 signed_size = static_cast<s64>(size);
                R_UNLESS(signed_size >= 0, fs::ResultOutOfRange());
                R_UNLESS((std::numeric_limits<s64>::max() - offset) >= signed_size, fs::ResultOutOfRange());
                return this->DoWrite(offset, buffer, size, option);
            }

            Result SetSize(s64 size) {
                R_UNLESS(size >= 0, fs::ResultOutOfRange());
                return this->DoSetSize(size);
            }

            Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
                return this->DoOperateRange(dst, dst_size, op_id, offset, size, src, src_size);
            }

            Result OperateRange(fs::OperationId op_id, s64 offset, s64 size) {
                return this->DoOperateRange(nullptr, 0, op_id, offset, size, nullptr, 0);
            }
        public:
            /* TODO: This is a hack to allow the mitm API to work. Find a better way? */
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const = 0;
        protected:
            Result DryRead(size_t *out, s64 offset, size_t size, const fs::ReadOption &option, OpenMode open_mode) {
                /* Check that we can read. */
                R_UNLESS((open_mode & OpenMode_Read) != 0, fs::ResultReadNotPermitted());

                /* Get the file size, and validate our offset. */
                s64 file_size = 0;
                R_TRY(this->GetSize(&file_size));
                R_UNLESS(offset <= file_size, fs::ResultOutOfRange());

                *out = static_cast<size_t>(std::min(file_size - offset, static_cast<s64>(size)));
                return ResultSuccess();
            }

            Result DrySetSize(s64 size, fs::OpenMode open_mode) {
                /* Check that we can write. */
                R_UNLESS((open_mode & OpenMode_Write) != 0, fs::ResultWriteNotPermitted());

                AMS_ASSERT(size >= 0);

                return ResultSuccess();
            }

            Result DryWrite(bool *out_append, s64 offset, size_t size, const fs::WriteOption &option, fs::OpenMode open_mode) {
                /* Check that we can write. */
                R_UNLESS((open_mode & OpenMode_Write) != 0, fs::ResultWriteNotPermitted());

                /* Get the file size. */
                s64 file_size = 0;
                R_TRY(this->GetSize(&file_size));

                /* Determine if we need to append. */
                if (file_size < offset +  static_cast<s64>(size)) {
                    R_UNLESS((open_mode & OpenMode_AllowAppend) != 0, fs::ResultFileExtensionWithoutOpenModeAllowAppend());
                    *out_append = true;
                } else {
                    *out_append = false;
                }

                return ResultSuccess();
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

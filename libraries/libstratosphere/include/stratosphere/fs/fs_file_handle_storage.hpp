/*
 * Copyright (c) 2018-2020 Adubbz, Atmosphère-NX
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
#include "fs_common.hpp"
#include "fs_istorage.hpp"

namespace ams::fs {

    class FileHandleStorage : public IStorage {
        private:
            static constexpr s64 InvalidSize = -1;
        private:
            FileHandle handle;
            bool close_file;
            s64 size;
            os::Mutex mutex;
        public:
            FileHandleStorage(FileHandle handle) : FileHandleStorage(handle, false) { /* ... */ }

            FileHandleStorage(FileHandle handle, bool close_file) : handle(handle), close_file(close_file), size(InvalidSize) {
                /* ... */
            }

            virtual ~FileHandleStorage() { 
                if (this->close_file) {
                    CloseFile(this->handle);
                }
             }
        protected:
            Result UpdateSize();
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result Flush() override;
            virtual Result GetSize(s64 *out_size) override;
            virtual Result SetSize(s64 size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
    };

}

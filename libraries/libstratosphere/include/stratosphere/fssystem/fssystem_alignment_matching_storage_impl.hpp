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
#include <vapours.hpp>
#include <stratosphere/fs/fs_istorage.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: Unknown */
    class AlignmentMatchingStorageImpl {
        public:
            static Result Read(fs::IStorage *base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, char *buffer, size_t size);
            static Result Write(fs::IStorage *base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, const char *buffer, size_t size);

            static Result Read(std::shared_ptr<fs::IStorage> &base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, char *buffer, size_t size) {
                R_RETURN(Read(base_storage.get(), work_buf, work_buf_size, data_alignment, buffer_alignment, offset, buffer, size));
            }

            static Result Write(std::shared_ptr<fs::IStorage> &base_storage, char *work_buf, size_t work_buf_size, size_t data_alignment, size_t buffer_alignment, s64 offset, const char *buffer, size_t size) {
                R_RETURN(Write(base_storage.get(), work_buf, work_buf_size, data_alignment, buffer_alignment, offset, buffer, size));
            }
    };

}

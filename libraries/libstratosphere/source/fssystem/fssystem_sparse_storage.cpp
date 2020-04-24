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
#include <stratosphere.hpp>

namespace ams::fssystem {

    Result SparseStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Allow zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        if (this->GetEntryTable().IsEmpty()) {
            R_UNLESS(this->GetEntryTable().Includes(offset, size), fs::ResultOutOfRange());
            std::memset(buffer, 0, size);
        } else {
            R_TRY(this->OperatePerEntry<false>(offset, size, [=](fs::IStorage *storage, s64 data_offset, s64 cur_offset, s64 cur_size) -> Result {
                R_TRY(storage->Read(data_offset, reinterpret_cast<u8 *>(buffer) + (cur_offset - offset), static_cast<size_t>(cur_size)));
                return ResultSuccess();
            }));
        }

        return ResultSuccess();
    }

}

/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fsmitm_istorage_interface.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    Result IStorageInterface::Read(s64 offset, const sf::OutNonSecureBuffer &buffer, s64 size) {
        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());
        return this->base_storage->Read(offset, buffer.GetPointer(), size);
    }

    Result IStorageInterface::Write(s64 offset, const sf::InNonSecureBuffer &buffer, s64 size)  {
        /* TODO: N increases thread priority temporarily when writing. We may want to eventually. */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());
        return this->base_storage->Write(offset, buffer.GetPointer(), size);
    }

    Result IStorageInterface::Flush()  {
        return this->base_storage->Flush();
    }

    Result IStorageInterface::SetSize(s64 size)  {
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        return this->base_storage->SetSize(size);
    }

    Result IStorageInterface::GetSize(sf::Out<s64> out)  {
        return this->base_storage->GetSize(out.GetPointer());
    }

    Result IStorageInterface::OperateRange(sf::Out<StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
        /* N includes this redundant check, so we will too. */
        R_UNLESS(out.GetPointer() != nullptr, fs::ResultNullptrArgument());

        out->Clear();
        if (op_id == static_cast<s32>(fs::OperationId::QueryRange)) {
            fs::StorageQueryRangeInfo info;
            R_TRY(this->base_storage->OperateRange(&info, sizeof(info), fs::OperationId::QueryRange, offset, size, nullptr, 0));
            out->Merge(info);
        }

        return ResultSuccess();
    }

}

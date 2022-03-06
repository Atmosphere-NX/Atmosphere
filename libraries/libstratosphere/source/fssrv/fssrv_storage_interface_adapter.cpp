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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "fssrv_retry_utility.hpp"

namespace ams::fssrv::impl {

    Result StorageInterfaceAdapter::Read(s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size) {
        /* Check pre-conditions. */
        R_UNLESS(0 <= offset,                                fs::ResultInvalidOffset());
        R_UNLESS(0 <= size,                                  fs::ResultInvalidSize());
        R_UNLESS(size <= static_cast<s64>(buffer.GetSize()), fs::ResultInvalidSize());

        R_RETURN(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_storage->Read(offset, buffer.GetPointer(), size));
        }));
    }

    Result StorageInterfaceAdapter::Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size)  {
        /* Check pre-conditions. */
        R_UNLESS(0 <= offset,                                fs::ResultInvalidOffset());
        R_UNLESS(0 <= size,                                  fs::ResultInvalidSize());
        R_UNLESS(size <= static_cast<s64>(buffer.GetSize()), fs::ResultInvalidSize());

        /* Temporarily increase our thread's priority. */
        fssystem::ScopedThreadPriorityChangerByAccessPriority cp(fssystem::ScopedThreadPriorityChangerByAccessPriority::AccessMode::Write);

        R_RETURN(m_base_storage->Write(offset, buffer.GetPointer(), size));
    }

    Result StorageInterfaceAdapter::Flush()  {
        R_RETURN(m_base_storage->Flush());
    }

    Result StorageInterfaceAdapter::SetSize(s64 size)  {
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        R_RETURN(m_base_storage->SetSize(size));
    }

    Result StorageInterfaceAdapter::GetSize(ams::sf::Out<s64> out)  {
        R_RETURN(m_base_storage->GetSize(out.GetPointer()));
    }

    Result StorageInterfaceAdapter::OperateRange(ams::sf::Out<fs::StorageQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
        /* N includes this redundant check, so we will too. */
        R_UNLESS(out.GetPointer() != nullptr, fs::ResultNullptrArgument());

        /* Clear the range info. */
        out->Clear();

        if (op_id == static_cast<s32>(fs::OperationId::QueryRange)) {
            fs::FileQueryRangeInfo info;
            R_TRY(m_base_storage->OperateRange(std::addressof(info), sizeof(info), fs::OperationId::QueryRange, offset, size, nullptr, 0));
            out->Merge(info);
        } else if (op_id == static_cast<s32>(fs::OperationId::Invalidate)) {
            R_TRY(m_base_storage->OperateRange(nullptr, 0, fs::OperationId::Invalidate, offset, size, nullptr, 0));
        }

        R_SUCCEED();
    }

}

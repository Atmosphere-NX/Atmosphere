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
#include "../fs_scoped_setter.hpp"
#include "../fs_file_path_hash.hpp"
#include "fs_file_accessor.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs::impl {

    FileAccessor::FileAccessor(std::unique_ptr<fsa::IFile>&& f, FileSystemAccessor *p, OpenMode mode)
        : impl(std::move(f)), parent(p), write_state(WriteState::None), write_result(ResultSuccess()), open_mode(mode)
    {
        /* ... */
    }

    FileAccessor::~FileAccessor() {
        /* Ensure that all files are flushed. */
        if (R_SUCCEEDED(this->write_result)) {
            AMS_FS_ABORT_UNLESS_WITH_RESULT(this->write_state != WriteState::NeedsFlush, fs::ResultNeedFlush());
        }
        this->impl.reset();

        if (this->parent != nullptr) {
            this->parent->NotifyCloseFile(this);
        }
    }

    Result FileAccessor::ReadWithCacheAccessLog(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option, bool use_path_cache, bool use_data_cache) {
        /* TODO */
        AMS_ABORT();
    }

    Result FileAccessor::ReadWithoutCacheAccessLog(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option) {
        return this->impl->Read(out, offset, buf, size, option);
    }

    Result FileAccessor::Read(size_t *out, s64 offset, void *buf, size_t size, const ReadOption &option) {
        /* Get a handle to this file for use in logging. */
        FileHandle handle = { this };

        /* Fail after a write fails. */
        R_UNLESS(R_SUCCEEDED(this->write_result), AMS_FS_IMPL_ACCESS_LOG_WITH_NAME(this->write_result, handle, "ReadFile", AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_FILE(out, offset, size)));

        /* TODO: Support cache. */
        const bool use_path_cache = this->parent != nullptr && this->file_path_hash != nullptr;
        const bool use_data_cache = /* TODO */false && this->parent != nullptr && this->parent->IsFileDataCacheAttachable();

        if (use_path_cache && use_data_cache && false) {
            /* TODO */
            return this->ReadWithCacheAccessLog(out, offset, buf, size, option, use_path_cache, use_data_cache);
        } else {
            return AMS_FS_IMPL_ACCESS_LOG_WITH_NAME(this->ReadWithoutCacheAccessLog(out, offset, buf, size, option), handle, "ReadFile", AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_FILE(out, offset, size));
        }
    }

    Result FileAccessor::Write(s64 offset, const void *buf, size_t size, const WriteOption &option) {
        /* Fail after a write fails. */
        R_TRY(this->write_result);

        auto setter = MakeScopedSetter(this->write_state, WriteState::Failed);
        if (this->file_path_hash != nullptr && /* TODO */ false) {
            /* TODO */
            AMS_ABORT();
        } else {
            R_TRY(this->UpdateLastResult(this->impl->Write(offset, buf, size, option)));
        }

        setter.Set(option.HasFlushFlag() ? WriteState::None : WriteState::NeedsFlush);

        return ResultSuccess();
    }

    Result FileAccessor::Flush() {
        /* Fail after a write fails. */
        R_TRY(this->write_result);

        auto setter = MakeScopedSetter(this->write_state, WriteState::Failed);
        R_TRY(this->UpdateLastResult(this->impl->Flush()));
        setter.Set(WriteState::None);

        return ResultSuccess();
    }

    Result FileAccessor::SetSize(s64 size) {
        /* Fail after a write fails. */
        R_TRY(this->write_result);

        const WriteState old_write_state = this->write_state;
        auto setter = MakeScopedSetter(this->write_state, WriteState::Failed);

        R_TRY(this->UpdateLastResult(this->impl->SetSize(size)));

        if (this->file_path_hash != nullptr) {
            /* TODO: invalidate path cache */
        }

        setter.Set(old_write_state);
        return ResultSuccess();
    }

    Result FileAccessor::GetSize(s64 *out) {
        /* Fail after a write fails. */
        R_TRY(this->write_result);

        return this->impl->GetSize(out);
    }

    Result FileAccessor::OperateRange(void *dst, size_t dst_size, OperationId operation, s64 offset, s64 size, const void *src, size_t src_size) {
        return this->impl->OperateRange(dst, dst_size, operation, offset, size, src, src_size);
    }

}
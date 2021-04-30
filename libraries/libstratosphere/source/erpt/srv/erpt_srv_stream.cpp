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
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_stream.hpp"

namespace ams::erpt::srv {

    constinit bool Stream::s_can_access_fs = true;
    constinit os::SdkMutex Stream::s_fs_commit_mutex;

    void Stream::EnableFsAccess(bool en) {
        s_can_access_fs = en;
    }

    Result Stream::DeleteStream(const char *path) {
        R_UNLESS(s_can_access_fs, erpt::ResultInvalidPowerState());
        return fs::DeleteFile(path);
    }

    Result Stream::CommitStream() {
        R_UNLESS(s_can_access_fs, erpt::ResultInvalidPowerState());

        std::scoped_lock lk(s_fs_commit_mutex);

        fs::CommitSaveData(ReportStoragePath);
        return ResultSuccess();
    }

    Result Stream::GetStreamSize(s64 *out, const char *path) {
        R_UNLESS(s_can_access_fs, erpt::ResultInvalidPowerState());

        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        return fs::GetFileSize(out, file);
    }

    Stream::Stream() : buffer_size(0), file_position(0), buffer_count(0), buffer(nullptr), stream_mode(StreamMode_Invalid), initialized(false) {
        /* ... */
    }

    Stream::~Stream() {
        this->CloseStream();
        AMS_ASSERT(!s_fs_commit_mutex.IsLockedByCurrentThread());
    }

    Result Stream::OpenStream(const char *path, StreamMode mode, u32 buffer_size) {
        R_UNLESS(s_can_access_fs,                       erpt::ResultInvalidPowerState());
        R_UNLESS(!this->initialized,                    erpt::ResultAlreadyInitialized());

        auto lock_guard = SCOPE_GUARD {
            if (s_fs_commit_mutex.IsLockedByCurrentThread()) {
                s_fs_commit_mutex.Unlock();
            }
        };

        if (mode == StreamMode_Write) {
            s_fs_commit_mutex.Lock();

            while (true) {
                R_TRY_CATCH(fs::OpenFile(std::addressof(this->file_handle), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend)) {
                    R_CATCH(fs::ResultPathNotFound) {
                        R_TRY(fs::CreateFile(path, 0));
                        continue;
                    }
                } R_END_TRY_CATCH;
                break;
            }
            fs::SetFileSize(this->file_handle, 0);
        } else {
            R_UNLESS(mode == StreamMode_Read, erpt::ResultInvalidArgument());

            fs::OpenFile(std::addressof(this->file_handle), path, fs::OpenMode_Read);
        }
        auto file_guard = SCOPE_GUARD { fs::CloseFile(this->file_handle); };

        std::strncpy(this->file_name, path, sizeof(this->file_name));
        this->file_name[sizeof(this->file_name) - 1] = '\x00';

        if (buffer_size > 0) {
            this->buffer = reinterpret_cast<u8 *>(Allocate(buffer_size));
            AMS_ASSERT(this->buffer != nullptr);
        } else {
            this->buffer = nullptr;
        }


        this->buffer_size     = this->buffer != nullptr ? buffer_size : 0;
        this->buffer_count    = 0;
        this->buffer_position = 0;
        this->file_position   = 0;
        this->stream_mode     = mode;
        this->initialized     = true;

        file_guard.Cancel();
        lock_guard.Cancel();
        return ResultSuccess();
    }

    Result Stream::ReadStream(u32 *out, u8 *dst, u32 dst_size) {
        R_UNLESS(s_can_access_fs,                       erpt::ResultInvalidPowerState());
        R_UNLESS(this->initialized,                     erpt::ResultNotInitialized());
        R_UNLESS(this->stream_mode == StreamMode_Read,  erpt::ResultNotInitialized());
        R_UNLESS(out != nullptr,                        erpt::ResultInvalidArgument());
        R_UNLESS(dst != nullptr,                        erpt::ResultInvalidArgument());

        size_t fs_read_size;
        u32 read_count = 0;

        ON_SCOPE_EXIT {
            *out = read_count;
        };

        if (this->buffer != nullptr) {
            while (dst_size > 0) {
                if (u32 cur = std::min<u32>(this->buffer_count - this->buffer_position, dst_size); cur > 0) {
                    std::memcpy(dst, this->buffer + this->buffer_position, cur);
                    this->buffer_position += cur;
                    dst                   += cur;
                    dst_size              -= cur;
                    read_count            += cur;
                } else {
                    R_TRY(fs::ReadFile(std::addressof(fs_read_size), this->file_handle, this->file_position, this->buffer, this->buffer_size));

                    this->buffer_position = 0;
                    this->file_position += static_cast<u32>(fs_read_size);
                    this->buffer_count = static_cast<u32>(fs_read_size);

                    if (this->buffer_count == 0) {
                        break;
                    }
                }
            }
        } else {
            R_TRY(fs::ReadFile(std::addressof(fs_read_size), this->file_handle, this->file_position, dst, dst_size));

            this->file_position += static_cast<u32>(fs_read_size);
            read_count = static_cast<u32>(fs_read_size);
        }

        return ResultSuccess();
    }

    Result Stream::WriteStream(const u8 *src, u32 src_size) {
        R_UNLESS(s_can_access_fs,                       erpt::ResultInvalidPowerState());
        R_UNLESS(this->initialized,                     erpt::ResultNotInitialized());
        R_UNLESS(this->stream_mode == StreamMode_Write, erpt::ResultNotInitialized());
        R_UNLESS(src != nullptr || src_size == 0,       erpt::ResultInvalidArgument());

        if (this->buffer != nullptr) {
            while (src_size > 0) {
                if (u32 cur = std::min<u32>(this->buffer_size - this->buffer_count, src_size); cur > 0) {
                    std::memcpy(this->buffer + this->buffer_count, src, cur);
                    this->buffer_count += cur;
                    src                += cur;
                    src_size           -= cur;
                }

                if (this->buffer_count == this->buffer_size) {
                    R_TRY(this->Flush());
                }
            }
        } else {
            R_TRY(fs::WriteFile(this->file_handle, this->file_position, src, src_size, fs::WriteOption::None));
            this->file_position += src_size;
        }

        return ResultSuccess();
    }

    void Stream::CloseStream() {
        if (this->initialized) {
            if (s_can_access_fs) {
                if (this->stream_mode == StreamMode_Write) {
                    this->Flush();
                    fs::FlushFile(this->file_handle);
                }
                fs::CloseFile(this->file_handle);
            }

            if (this->buffer != nullptr) {
                Deallocate(this->buffer);
            }

            this->initialized = false;

            if (s_fs_commit_mutex.IsLockedByCurrentThread()) {
                s_fs_commit_mutex.Unlock();
            }
        }
    }

    Result Stream::GetStreamSize(s64 *out) const {
        return GetStreamSize(out, this->file_name);
    }

    Result Stream::Flush() {
        AMS_ASSERT(s_fs_commit_mutex.IsLockedByCurrentThread());

        R_SUCCEED_IF(this->buffer_count == 0);
        R_TRY(fs::WriteFile(this->file_handle, this->file_position, this->buffer, this->buffer_count, fs::WriteOption::None));

        this->file_position += this->buffer_count;
        this->buffer_count = 0;
        return ResultSuccess();
    }

}

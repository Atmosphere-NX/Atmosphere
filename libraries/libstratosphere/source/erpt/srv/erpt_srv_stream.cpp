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
        R_RETURN(fs::DeleteFile(path));
    }

    Result Stream::CommitStream() {
        R_UNLESS(s_can_access_fs, erpt::ResultInvalidPowerState());

        std::scoped_lock lk(s_fs_commit_mutex);

        const auto commit_res = fs::CommitSaveData(ReportStoragePath);
        R_ASSERT(commit_res);
        AMS_UNUSED(commit_res);

        R_SUCCEED();
    }

    Result Stream::GetStreamSize(s64 *out, const char *path) {
        R_UNLESS(s_can_access_fs, erpt::ResultInvalidPowerState());

        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        R_RETURN(fs::GetFileSize(out, file));
    }

    Stream::Stream() : m_buffer_size(0), m_file_position(0), m_buffer_count(0), m_buffer(nullptr), m_stream_mode(StreamMode_Invalid), m_initialized(false) {
        /* ... */
    }

    Stream::~Stream() {
        this->CloseStream();
        AMS_ASSERT(!s_fs_commit_mutex.IsLockedByCurrentThread());
    }

    Result Stream::OpenStream(const char *path, StreamMode mode, u32 buffer_size) {
        R_UNLESS(s_can_access_fs,                   erpt::ResultInvalidPowerState());
        R_UNLESS(!m_initialized,                    erpt::ResultAlreadyInitialized());

        auto lock_guard = SCOPE_GUARD {
            if (s_fs_commit_mutex.IsLockedByCurrentThread()) {
                s_fs_commit_mutex.Unlock();
            }
        };

        if (mode == StreamMode_Write) {
            s_fs_commit_mutex.Lock();

            while (true) {
                R_TRY_CATCH(fs::OpenFile(std::addressof(m_file_handle), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend)) {
                    R_CATCH(fs::ResultPathNotFound) {
                        R_TRY(fs::CreateFile(path, 0));
                        continue;
                    }
                } R_END_TRY_CATCH;
                break;
            }
            R_TRY(fs::SetFileSize(m_file_handle, 0));
        } else {
            R_UNLESS(mode == StreamMode_Read, erpt::ResultInvalidArgument());

            R_TRY(fs::OpenFile(std::addressof(m_file_handle), path, fs::OpenMode_Read));
        }
        auto file_guard = SCOPE_GUARD { fs::CloseFile(m_file_handle); };

        std::strncpy(m_file_name, path, sizeof(m_file_name));
        m_file_name[sizeof(m_file_name) - 1] = '\x00';

        if (buffer_size > 0) {
            m_buffer = reinterpret_cast<u8 *>(Allocate(buffer_size));
            AMS_ASSERT(m_buffer != nullptr);
        } else {
            m_buffer = nullptr;
        }


        m_buffer_size     = m_buffer != nullptr ? buffer_size : 0;
        m_buffer_count    = 0;
        m_buffer_position = 0;
        m_file_position   = 0;
        m_stream_mode     = mode;
        m_initialized     = true;

        file_guard.Cancel();
        lock_guard.Cancel();
        R_SUCCEED();
    }

    Result Stream::ReadStream(u32 *out, u8 *dst, u32 dst_size) {
        R_UNLESS(s_can_access_fs,                   erpt::ResultInvalidPowerState());
        R_UNLESS(m_initialized,                     erpt::ResultNotInitialized());
        R_UNLESS(m_stream_mode == StreamMode_Read,  erpt::ResultNotInitialized());
        R_UNLESS(out != nullptr,                    erpt::ResultInvalidArgument());
        R_UNLESS(dst != nullptr,                    erpt::ResultInvalidArgument());

        size_t fs_read_size;
        u32 read_count = 0;

        ON_SCOPE_EXIT {
            *out = read_count;
        };

        if (m_buffer != nullptr) {
            while (dst_size > 0) {
                if (u32 cur = std::min<u32>(m_buffer_count - m_buffer_position, dst_size); cur > 0) {
                    std::memcpy(dst, m_buffer + m_buffer_position, cur);
                    m_buffer_position += cur;
                    dst               += cur;
                    dst_size          -= cur;
                    read_count        += cur;
                } else {
                    R_TRY(fs::ReadFile(std::addressof(fs_read_size), m_file_handle, m_file_position, m_buffer, m_buffer_size));

                    m_buffer_position = 0;
                    m_file_position += static_cast<u32>(fs_read_size);
                    m_buffer_count = static_cast<u32>(fs_read_size);

                    if (m_buffer_count == 0) {
                        break;
                    }
                }
            }
        } else {
            R_TRY(fs::ReadFile(std::addressof(fs_read_size), m_file_handle, m_file_position, dst, dst_size));

            m_file_position += static_cast<u32>(fs_read_size);
            read_count = static_cast<u32>(fs_read_size);
        }

        R_SUCCEED();
    }

    Result Stream::WriteStream(const u8 *src, u32 src_size) {
        R_UNLESS(s_can_access_fs,                   erpt::ResultInvalidPowerState());
        R_UNLESS(m_initialized,                     erpt::ResultNotInitialized());
        R_UNLESS(m_stream_mode == StreamMode_Write, erpt::ResultNotInitialized());
        R_UNLESS(src != nullptr || src_size == 0,   erpt::ResultInvalidArgument());

        if (m_buffer != nullptr) {
            while (src_size > 0) {
                if (u32 cur = std::min<u32>(m_buffer_size - m_buffer_count, src_size); cur > 0) {
                    std::memcpy(m_buffer + m_buffer_count, src, cur);
                    m_buffer_count += cur;
                    src            += cur;
                    src_size       -= cur;
                }

                if (m_buffer_count == m_buffer_size) {
                    R_TRY(this->Flush());
                }
            }
        } else {
            R_TRY(fs::WriteFile(m_file_handle, m_file_position, src, src_size, fs::WriteOption::None));
            m_file_position += src_size;
        }

        R_SUCCEED();
    }

    void Stream::CloseStream() {
        if (m_initialized) {
            if (s_can_access_fs) {
                if (m_stream_mode == StreamMode_Write) {
                    const auto self_flush_res = this->Flush();
                    R_ASSERT(self_flush_res);
                    AMS_UNUSED(self_flush_res);

                    const auto file_flush_res = fs::FlushFile(m_file_handle);
                    R_ASSERT(file_flush_res);
                    AMS_UNUSED(file_flush_res);
                }
                fs::CloseFile(m_file_handle);
            }

            if (m_buffer != nullptr) {
                Deallocate(m_buffer);
            }

            m_initialized = false;

            if (s_fs_commit_mutex.IsLockedByCurrentThread()) {
                s_fs_commit_mutex.Unlock();
            }
        }
    }

    Result Stream::GetStreamSize(s64 *out) const {
        R_RETURN(GetStreamSize(out, m_file_name));
    }

    Result Stream::Flush() {
        AMS_ASSERT(s_fs_commit_mutex.IsLockedByCurrentThread());

        R_SUCCEED_IF(m_buffer_count == 0);
        R_TRY(fs::WriteFile(m_file_handle, m_file_position, m_buffer, m_buffer_count, fs::WriteOption::None));

        m_file_position += m_buffer_count;
        m_buffer_count = 0;
        R_SUCCEED();
    }

}

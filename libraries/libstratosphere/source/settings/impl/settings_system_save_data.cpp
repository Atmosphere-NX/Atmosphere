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
#include "settings_static_object.hpp"
#include "settings_system_save_data.hpp"

namespace ams::settings::impl {

    namespace {

        constexpr s64 LazyWriterDelayMilliSeconds = 500;

        constexpr inline const char *MountNameSeparator = ":";
        constexpr inline const char *DirectoryNameSeparator = "/";
        constexpr inline const char *SystemSaveDataFileName = "file";

        class LazyFileAccessor final {
            NON_COPYABLE(LazyFileAccessor);
            NON_MOVEABLE(LazyFileAccessor);
            private:
                static constexpr size_t FileNameLengthMax = 31;
            private:
                bool m_is_activated;
                bool m_is_busy;
                bool m_is_cached;
                bool m_is_modified;
                bool m_is_file_size_changed;
                char m_mount_name[fs::MountNameLengthMax + 1];
                char m_file_path[fs::MountNameLengthMax + 1 + 1 + FileNameLengthMax + 1];
                int m_open_mode;
                s64 m_file_size;
                s64 m_offset;
                size_t m_size;
                u8 m_buffer[512_KB];
                os::Mutex m_mutex;
                os::TimerEvent m_timer_event;
                os::ThreadType m_thread;
                alignas(os::ThreadStackAlignment) u8 m_thread_stack[4_KB];
            public:
                LazyFileAccessor() : m_is_activated(false), m_is_busy(false), m_is_cached(false), m_is_modified(false), m_is_file_size_changed(false), m_mount_name{}, m_file_path{}, m_open_mode(0), m_file_size(0), m_offset(0), m_size(0), m_mutex(false), m_timer_event(os::EventClearMode_AutoClear), m_thread{}  {
                    std::memset(m_buffer, 0, sizeof(m_buffer));
                    std::memset(m_thread_stack, 0, sizeof(m_thread_stack));
                }

                Result Activate();
                Result Commit(const char *name, bool synchronous);
                Result Create(const char *name, s64 size);
                Result Open(const char *name, int mode);
                void Close();
                Result Read(s64 offset, void *dst, size_t size);
                Result Write(s64 offset, const void *src, size_t size);
                Result SetFileSize(s64 size);
            private:
                static void ThreadFunc(void *arg);

                template<size_t N>
                static int CreateFilePath(char (&path)[N], const char *name);

                static bool AreEqual(const void *lhs, const void *rhs, size_t size);

                void SetMountName(const char *name);
                bool CompareMountName(const char *name) const;
                void InvokeWriteBackLoop();
                Result CommitSynchronously();
        };

        LazyFileAccessor &GetLazyFileAccessor() {
            return StaticObject<LazyFileAccessor, void>::Get();
        }

        Result LazyFileAccessor::Activate() {
            std::scoped_lock lk(m_mutex);

            if (!m_is_activated) {
                /* Create and start the lazy writer thread. */
                R_TRY(os::CreateThread(std::addressof(m_thread), LazyFileAccessor::ThreadFunc, this, m_thread_stack, sizeof(m_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(settings, LazyWriter)));
                os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(settings, LazyWriter));
                os::StartThread(std::addressof(m_thread));
                m_is_activated = true;
            }

            return ResultSuccess();
        }

        Result LazyFileAccessor::Commit(const char *name, bool synchronous) {
            AMS_ASSERT(name != nullptr);
            AMS_UNUSED(name);
            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(!m_is_busy);
            AMS_ASSERT(this->CompareMountName(name));

            if (synchronous) {
                /* Stop the timer and commit synchronously. */
                m_timer_event.Stop();
                R_TRY(this->CommitSynchronously());
            } else {
                /* Start the timer to write. */
                m_timer_event.StartOneShot(TimeSpan::FromMilliSeconds(LazyWriterDelayMilliSeconds));
            }

            return ResultSuccess();
        }

        Result LazyFileAccessor::Create(const char *name, s64 size) {
            AMS_ASSERT(name != nullptr);
            AMS_ASSERT(size >= 0);

            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(!m_is_busy);

            if (m_is_cached) {
                /* Stop the timer and commit synchronously. */
                m_timer_event.Stop();
                R_TRY(this->CommitSynchronously());

                /* Reset the current state. */
                m_is_cached = false;
                this->SetMountName("");
                m_open_mode = 0;
                m_file_size = 0;

                /* Clear the buffer. */
                std::memset(m_buffer, 0, sizeof(m_buffer));
            }

            /* Create the save file. */
            this->CreateFilePath(m_file_path, name);
            R_TRY(fs::CreateFile(m_file_path, size));

            /* Initialize the accessor. */
            m_is_cached   = true;
            m_is_modified = true;

            this->SetMountName(name);

            m_open_mode = fs::OpenMode_Write;
            m_file_size = size;
            m_offset    = 0;
            m_size      = size;

            /* Start the timer to write. */
            m_timer_event.StartOneShot(TimeSpan::FromMilliSeconds(LazyWriterDelayMilliSeconds));
            return ResultSuccess();
        }

       Result LazyFileAccessor::Open(const char *name, int mode) {
            AMS_ASSERT(name != nullptr);

            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(!m_is_busy);

            bool caches = true;

            if (m_is_cached) {
                /* Check if the current mount matches the requested mount. */
                if (this->CompareMountName(name)) {
                    /* Check if the mode matches, or the existing mode is read-only. */
                    if (m_open_mode == mode || m_open_mode == fs::OpenMode_Read) {
                        m_is_busy = true;
                        m_open_mode = mode;
                        return ResultSuccess();
                    }
                    caches = false;
                }

                /* Stop the timer and commit synchronously. */
                m_timer_event.Stop();
                R_TRY(this->CommitSynchronously());
            }

            if (caches) {
                /* Create the save file if needed. */
                this->CreateFilePath(m_file_path, name);

                /* Open the save file. */
                fs::FileHandle file = {};
                R_TRY(fs::OpenFile(std::addressof(file), m_file_path, fs::OpenMode_Read));
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                /* Get the save size. */
                s64 file_size = 0;
                R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                AMS_ASSERT(0 <= file_size && file_size <= static_cast<s64>(sizeof(m_buffer)));
                R_UNLESS(file_size <= static_cast<s64>(sizeof(m_buffer)), settings::ResultTooLargeSystemSaveData());

                /* Read the save file. */
                R_TRY(fs::ReadFile(file, 0, m_buffer, static_cast<size_t>(file_size)));

                m_is_cached = true;
                this->SetMountName(name);
                m_file_size = file_size;
            }

            m_is_busy   = true;
            m_open_mode = mode;
            return ResultSuccess();
        }

        void LazyFileAccessor::Close() {
            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(m_is_busy);

            m_is_busy = false;
        }

        Result LazyFileAccessor::Read(s64 offset, void *dst, size_t size) {
            AMS_ASSERT(offset >= 0);
            AMS_ASSERT(dst != nullptr);

            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(m_is_busy);
            AMS_ASSERT(m_is_cached);
            AMS_ASSERT((m_open_mode & fs::OpenMode_Read) != 0);
            AMS_ASSERT(offset + static_cast<s64>(size) <= m_file_size);

            std::memcpy(dst, m_buffer + offset, size);

            return ResultSuccess();
        }

        Result LazyFileAccessor::Write(s64 offset, const void *src, size_t size) {
            AMS_ASSERT(offset >= 0);
            AMS_ASSERT(src != nullptr);

            std::scoped_lock lk(m_mutex);

            AMS_ASSERT(m_is_activated);
            AMS_ASSERT(m_is_busy);
            AMS_ASSERT(m_is_cached);
            AMS_ASSERT((m_open_mode & ::ams::fs::OpenMode_Write) != 0);

            s64 end = offset + static_cast<s64>(size);
            AMS_ASSERT(end <= static_cast<s64>(m_size));

            /* Succeed if there's nothing to write. */
            R_SUCCEED_IF(this->AreEqual(m_buffer + offset, src, size));

            /* Copy to dst. */
            std::memcpy(m_buffer + offset, src, size);

            /* Update offset and size. */
            if (m_is_modified) {
                end      = std::max(end, m_offset + static_cast<s64>(m_size));
                m_offset = std::min(m_offset, offset);
                m_size   = static_cast<size_t>(end - m_offset);
            } else {
                m_is_modified = true;
                m_offset      = offset;
                m_size        = size;
            }

            return ResultSuccess();
        }

        Result LazyFileAccessor::SetFileSize(s64 size) {
            std::scoped_lock lk(m_mutex);

            const s64 prev_file_size = m_file_size;

            /* If the existing file size exceeds the new file size, reset the state or truncate. */
            if (m_file_size >= size) {
                if (m_is_modified) {
                    /* If the current offset exceeds the new file size, reset the state. */
                    if (m_offset >= size) {
                        m_is_modified = false;
                        m_offset      = 0;
                        m_size        = 0;
                    } else if (m_offset + static_cast<s64>(m_size) > size) {
                        /* Truncate the buffer for the new file size. */
                        m_size = size - m_offset;
                    }
                }
            } else {
                /* If unmodified, mark as modified and move the offset to the current end of the file. */
                if (!m_is_modified) {
                    m_is_modified = true;
                    m_offset      = m_file_size;
                }

                /* Zero-initialize the expanded file segment. */
                std::memset(m_buffer + m_file_size, 0, size - m_file_size);

                /* Update the current buffer size. */
                m_size = size - m_offset;
            }

            /* Update state. */
            m_is_file_size_changed = prev_file_size != size;
            m_file_size            = size;
            return ResultSuccess();
        }

        void LazyFileAccessor::ThreadFunc(void *arg) {
            AMS_ASSERT(arg != nullptr);
            reinterpret_cast<LazyFileAccessor *>(arg)->InvokeWriteBackLoop();
        }

        template<size_t N>
        int LazyFileAccessor::CreateFilePath(char (&path)[N], const char *name) {
            AMS_ASSERT(name != nullptr);

            const auto len = static_cast<int>(N);

            s32 pos = util::Strlcpy(path, name, len);
            pos += util::Strlcpy(path + pos, MountNameSeparator, len - pos);
            pos += util::Strlcpy(path + pos, DirectoryNameSeparator, len - pos);
            pos += util::Strlcpy(path + pos, SystemSaveDataFileName, len - pos);
            return pos;
        }

        bool LazyFileAccessor::AreEqual(const void *lhs, const void *rhs, size_t size) {
            AMS_ASSERT(lhs != nullptr);
            AMS_ASSERT(rhs != nullptr);

            auto lhs8 = static_cast<const u8 *>(lhs);
            auto rhs8 = static_cast<const u8 *>(rhs);
            for (size_t i = 0; i < size; i++) {
                if (*(lhs8++) != *(rhs8++)) {
                    return false;
                }
            }

            return true;
        }

        void LazyFileAccessor::SetMountName(const char *name) {
            AMS_ASSERT(name != nullptr);

            util::Strlcpy(m_mount_name, name, sizeof(m_mount_name));
        }

        bool LazyFileAccessor::CompareMountName(const char *name) const {
            return util::Strncmp(m_mount_name, name, sizeof(m_mount_name)) == 0;
        }

        void LazyFileAccessor::InvokeWriteBackLoop() {
            while (true) {
                m_timer_event.Wait();

                std::scoped_lock lk(m_mutex);
                if (!m_is_busy) {
                    R_ABORT_UNLESS(this->CommitSynchronously());
                }
            }
        }

        Result LazyFileAccessor::CommitSynchronously() {
            /* If we have data cached/modified, we need to commit. */
            if (m_is_cached && (m_is_file_size_changed || m_is_modified)) {
                /* Get the save file path. */
                this->CreateFilePath(m_file_path, m_mount_name);

                {
                    /* Open the save file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), m_file_path, fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* If we need to, update the file size. */
                    if (m_is_file_size_changed) {
                        R_TRY(fs::SetFileSize(file, m_file_size));
                        m_is_file_size_changed = false;
                    }

                    /* If we need to, write the file data. */
                    if (m_is_modified) {
                        /* Write to the file. */
                        R_TRY(fs::WriteFile(file, m_offset, m_buffer + m_offset, m_size, fs::WriteOption::None));
                        R_TRY(fs::FlushFile(file));

                        /* Reset state. */
                        m_is_modified = false;
                        m_offset      = 0;
                        m_size        = 0;
                    }
                }

                /* Commit the savedata. */
                R_TRY(fs::CommitSaveData(m_mount_name));
            }

            return ResultSuccess();
        }

    }

    void SystemSaveData::SetSystemSaveDataId(u64 id) {
        m_system_save_data_id = id;
    }

    void SystemSaveData::SetTotalSize(s64 size) {
        m_total_size = size;
    }

    void SystemSaveData::SetJournalSize(s64 size) {
        m_journal_size = size;
    }

    void SystemSaveData::SetFlags(u32 flags) {
        m_flags = flags;
    }

    void SystemSaveData::SetMountName(const char *name) {
        AMS_ASSERT(name != nullptr);

        util::Strlcpy(m_mount_name, name, sizeof(m_mount_name));
    }

    Result SystemSaveData::Mount(bool create_save) {
        /* Activate the file accessor. */
        R_TRY(GetLazyFileAccessor().Activate());

        /* Don't allow automatic save data creation. */
        fs::DisableAutoSaveDataCreation();

        /* Attempt to get the flags of existing save data. */
        u32 cur_flags = 0;
        const auto flags_result = fs::GetSaveDataFlags(std::addressof(cur_flags), m_save_data_space_id, m_system_save_data_id);

        /* If the save data exists, ensure flags are correct. */
        if (R_SUCCEEDED(flags_result)) {
            if (cur_flags != m_flags) {
                R_TRY(fs::SetSaveDataFlags(m_system_save_data_id, m_save_data_space_id, m_flags));
            }
        } else {
            /* Unless we can create save data, return the error we got when retrieving flags. */
            R_UNLESS(create_save, flags_result);

            /* Create the save data. */
            R_TRY(fs::CreateSystemSaveData(m_save_data_space_id, m_system_save_data_id, ncm::SystemProgramId::Settings.value, m_total_size, m_journal_size, m_flags));
        }

        /* Mount the save data. */
        return fs::MountSystemSaveData(m_mount_name, m_save_data_space_id, m_system_save_data_id);
    }

    Result SystemSaveData::Commit(bool synchronous) {
        return GetLazyFileAccessor().Commit(m_mount_name, synchronous);
    }

    Result SystemSaveData::Create(s64 size) {
        return GetLazyFileAccessor().Create(m_mount_name, size);
    }

    Result SystemSaveData::OpenToRead() {
        return GetLazyFileAccessor().Open(m_mount_name, fs::OpenMode_Read);
    }

    Result SystemSaveData::OpenToWrite() {
        return GetLazyFileAccessor().Open(m_mount_name, fs::OpenMode_Write);
    }

    void SystemSaveData::Close() {
        GetLazyFileAccessor().Close();
    }

    Result SystemSaveData::Read(s64 offset, void *buf, size_t size) {
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(buf != nullptr);

        return GetLazyFileAccessor().Read(offset, buf, size);
    }

    Result SystemSaveData::Write(s64 offset, const void *buf, size_t size) {
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(buf != nullptr);

        return GetLazyFileAccessor().Write(offset, buf, size);
    }

    Result SystemSaveData::Flush() {
        /* N doesn't do anything here. */
        return ResultSuccess();
    }

    Result SystemSaveData::SetFileSize(s64 size) {
        return GetLazyFileAccessor().SetFileSize(size);
    }

}

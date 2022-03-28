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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_path.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */

    namespace impl {

        template<typename F>
        concept IterateDirectoryHandler = requires (F f, const fs::Path &path, const fs::DirectoryEntry &entry) {
            { f(path, entry) } -> std::convertible_to<::ams::Result>;
        };

        /* Iteration. */
        template<IterateDirectoryHandler OnEnterDir, IterateDirectoryHandler OnExitDir, IterateDirectoryHandler OnFile>
        Result IterateDirectoryRecursivelyImpl(fs::fsa::IFileSystem *fs, fs::Path &work_path, fs::DirectoryEntry *dir_ent, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            /* Open the directory. */
            std::unique_ptr<fs::fsa::IDirectory> dir;
            R_TRY(fs->OpenDirectory(std::addressof(dir), work_path, fs::OpenDirectoryMode_All));

            /* Read and handle entries. */
            while (true) {
                /* Read a single entry. */
                s64 read_count = 0;
                R_TRY(dir->Read(std::addressof(read_count), dir_ent, 1));

                /* If we're out of entries, we're done. */
                if (read_count == 0) {
                    break;
                }

                /* Append child path. */
                R_TRY(work_path.AppendChild(dir_ent->name));
                {
                    if (dir_ent->type == fs::DirectoryEntryType_Directory) {
                        /* Enter directory. */
                        R_TRY(on_enter_dir(work_path, *dir_ent));

                        /* Recurse. */
                        R_TRY(IterateDirectoryRecursivelyImpl(fs, work_path, dir_ent, on_enter_dir, on_exit_dir, on_file));

                        /* Exit directory. */
                        R_TRY(on_exit_dir(work_path, *dir_ent));
                    } else {
                        /* Call file handler. */
                        R_TRY(on_file(work_path, *dir_ent));
                    }
                }
                R_TRY(work_path.RemoveChild());
            }

            R_SUCCEED();
        }

        /* TODO: Cleanup. */

    }

    /* Iteration API */
    template<impl::IterateDirectoryHandler OnEnterDir, impl::IterateDirectoryHandler OnExitDir, impl::IterateDirectoryHandler OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, const fs::Path &root_path, fs::DirectoryEntry *dir_ent_buf, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        /* Create work path from the root path. */
        fs::Path work_path;
        R_TRY(work_path.Initialize(root_path));

        R_RETURN(impl::IterateDirectoryRecursivelyImpl(fs, work_path, dir_ent_buf, on_enter_dir, on_exit_dir, on_file));
    }

    template<impl::IterateDirectoryHandler OnEnterDir, impl::IterateDirectoryHandler OnExitDir, impl::IterateDirectoryHandler OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, const fs::Path &root_path, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        fs::DirectoryEntry dir_entry = {};
        R_RETURN(IterateDirectoryRecursively(fs, root_path, std::addressof(dir_entry), on_enter_dir, on_exit_dir, on_file));
    }

    template<impl::IterateDirectoryHandler OnEnterDir, impl::IterateDirectoryHandler OnExitDir, impl::IterateDirectoryHandler OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        R_RETURN(IterateDirectoryRecursively(fs, fs::MakeConstantPath("/"), on_enter_dir, on_exit_dir, on_file));
    }

    /* TODO: Cleanup API */

    /* Copy API. */
    Result CopyFile(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const fs::Path &dst_path, const fs::Path &src_path, void *work_buf, size_t work_buf_size);

    ALWAYS_INLINE Result CopyFile(fs::fsa::IFileSystem *fs, const fs::Path &dst_path, const fs::Path &src_path, void *work_buf, size_t work_buf_size) {
        R_RETURN(CopyFile(fs, fs, dst_path, src_path, work_buf, work_buf_size));
    }

    Result CopyDirectoryRecursively(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const fs::Path &dst_path, const fs::Path &src_path, fs::DirectoryEntry *entry, void *work_buf, size_t work_buf_size);

    ALWAYS_INLINE Result CopyDirectoryRecursively(fs::fsa::IFileSystem *fs, const fs::Path &dst_path, const fs::Path &src_path, fs::DirectoryEntry *entry, void *work_buf, size_t work_buf_size) {
        R_RETURN(CopyDirectoryRecursively(fs, fs, dst_path, src_path, entry, work_buf, work_buf_size));
    }

    /* Locking utilities. */
    class SemaphoreAdaptor : public os::Semaphore {
        public:
            SemaphoreAdaptor(int c, int mc) : os::Semaphore(c, mc) { /* ... */ }

            bool TryLock(int *out_acquired, int count) {
                AMS_ASSERT(count > 0);

                for (auto i = 0; i < count; ++i) {
                    if (!this->TryAcquire()) {
                        *out_acquired = i;
                        return false;
                    }
                }

                *out_acquired = count;
                return true;
            }

            void Unlock(int count) {
                if (count > 0) {
                    this->Release(count);
                }
            }

            bool try_lock() {
                return this->TryAcquire();
            }

            void unlock() {
                this->Release();
            }
    };

    Result TryAcquireCountSemaphore(util::unique_lock<SemaphoreAdaptor> *out, SemaphoreAdaptor *adaptor);

    class IUniqueLock {
        NON_COPYABLE(IUniqueLock);
        NON_MOVEABLE(IUniqueLock);
        public:
            virtual ~IUniqueLock() { /* ... */ }
    };

    template<typename T>
    class UniqueLockWithPin final : public IUniqueLock, public ::ams::fs::impl::Newable {
        private:
            util::unique_lock<SemaphoreAdaptor> m_lock;
            T m_pinned_object;
        public:
            UniqueLockWithPin(util::unique_lock<SemaphoreAdaptor> lock, T obj) : m_lock(std::move(lock)), m_pinned_object(std::move(obj)) { /* ... */ }

            virtual ~UniqueLockWithPin() override {
                m_lock = {};
            }
    };

    template<typename T>
    class MultiLockWithPin final : public IUniqueLock, public ::ams::fs::impl::Newable {
        private:
            T m_pinned_object;
            SemaphoreAdaptor *m_semaphore_adaptor;
            int m_lock_count;
        public:
            MultiLockWithPin(T obj, SemaphoreAdaptor *adaptor) : m_pinned_object(std::move(obj)), m_semaphore_adaptor(adaptor), m_lock_count(0) {
                /* ... */
            }

            virtual ~MultiLockWithPin() override {
                if (m_lock_count > 0) {
                    m_semaphore_adaptor->Unlock(m_lock_count);
                }
            }

            Result Lock(int count) {
                AMS_ASSERT(m_lock_count == 0);

                R_UNLESS(m_semaphore_adaptor->TryLock(std::addressof(m_lock_count), count), fs::ResultOpenCountLimit());

                R_SUCCEED();
            }
    };

    template<typename T>
    Result MakeUniqueLockWithPin(std::unique_ptr<IUniqueLock> *out, SemaphoreAdaptor *adaptor, T obj) {
        /* Create the semaphore unique lock. */
        util::unique_lock<SemaphoreAdaptor> sema_lock;
        R_TRY(TryAcquireCountSemaphore(std::addressof(sema_lock), adaptor));

        /* Create the output unique lock. */
        auto result_lock = std::unique_ptr<UniqueLockWithPin<T>>(new UniqueLockWithPin<T>(std::move(sema_lock), std::move(obj)));
        R_UNLESS(result_lock != nullptr, fs::ResultAllocationMemoryFailedNew());

        /* Set the output. */
        *out = std::move(result_lock);
        R_SUCCEED();
    }

    template<typename T>
    Result MakeUniqueLockWithPin(std::unique_ptr<IUniqueLock> *out, SemaphoreAdaptor *adaptor, int count, T obj) {
        /* Create the output unique lock. */
        auto result_lock = std::unique_ptr<MultiLockWithPin<T>>(new MultiLockWithPin<T>(std::move(obj), adaptor));
        R_UNLESS(result_lock != nullptr, fs::ResultAllocationMemoryFailedNew());

        /* Acquire the output lock. */
        R_TRY(result_lock->Lock(count));

        /* Set the output. */
        *out = std::move(result_lock);
        R_SUCCEED();
    }

    /* Other utility. */
    Result HasFile(bool *out, fs::fsa::IFileSystem *fs, const fs::Path &path);
    Result HasDirectory(bool *out, fs::fsa::IFileSystem *fs, const fs::Path &path);

    Result EnsureDirectory(fs::fsa::IFileSystem *fs, const fs::Path &path);

    template<s64 RetryMilliSeconds = 100, s32 MaxTryCount = 10>
    ALWAYS_INLINE Result RetryFinitelyForTargetLocked(auto f) {
        /* Retry sleeping between retries. */
        constexpr TimeSpan RetryWaitTime = TimeSpan::FromMilliSeconds(RetryMilliSeconds);

        Result result = f();
        for (int i = 0; i < MaxTryCount && fs::ResultTargetLocked::Includes(result); ++i) {
            os::SleepThread(RetryWaitTime);
            result = f();
        }

        R_RETURN(result);
    }

    ALWAYS_INLINE Result RetryToAvoidTargetLocked(auto f) {
        R_RETURN((RetryFinitelyForTargetLocked<2, 25>(f)));
    }

    void AddCounter(void *counter, size_t counter_size, u64 value);

}

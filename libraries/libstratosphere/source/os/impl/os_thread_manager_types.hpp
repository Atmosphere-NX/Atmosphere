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
#pragma once
#include <stratosphere.hpp>

#ifdef ATMOSPHERE_OS_HORIZON
    #include "os_thread_manager_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ThreadManagerImpl"
#endif

namespace ams::os::impl {

    class ThreadManager {
        NON_COPYABLE(ThreadManager);
        NON_MOVEABLE(ThreadManager);
        private:
            class ThreadListTraits {
                public:
                    using ListType = util::IntrusiveList<ThreadType, ThreadListTraits>;
                private:
                    friend class util::IntrusiveList<ThreadType, ThreadListTraits>;

                    static constexpr util::IntrusiveListNode &GetNode(ThreadType &parent) {
                        return GetReference(parent.all_threads_node);
                    }

                    static constexpr util::IntrusiveListNode const &GetNode(ThreadType const &parent) {
                        return GetReference(parent.all_threads_node);
                    }

                    static ThreadType &GetParent(util::IntrusiveListNode &node) {
                        return *reinterpret_cast<ThreadType *>(reinterpret_cast<char *>(std::addressof(node)) - OFFSETOF(ThreadType, all_threads_node));
                    }

                    static ThreadType const &GetParent(util::IntrusiveListNode const &node) {
                        return *reinterpret_cast<const ThreadType *>(reinterpret_cast<const char *>(std::addressof(node)) - OFFSETOF(ThreadType, all_threads_node));
                    }
            };

            using AllThreadsList = ThreadListTraits::ListType;
        private:
            ThreadManagerImpl impl;
            ThreadType main_thread;
            InternalCriticalSection cs;
            AllThreadsList all_threads_list;
            size_t total_thread_stack_size;
            s32 num_created_threads;
        public:
            ThreadManager();

            void CleanupThread();
            s32 GetThreadCountForDebug() const { return this->num_created_threads; }

            Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority, s32 ideal_core);
            Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority);

            void DestroyThread(ThreadType *thread);
            void StartThread(ThreadType *thread);
            void WaitThread(ThreadType *thread);
            bool TryWaitThread(ThreadType *thread);

            void YieldThread() { return this->impl.YieldThread(); }

            bool ChangePriority(ThreadType *thread, s32 priority) { return this->impl.ChangePriority(thread, priority); }
            s32 GetCurrentPriority(const ThreadType *thread) const { return this->impl.GetCurrentPriority(thread); }
            ThreadType *GetCurrentThread() const { return this->impl.GetCurrentThread(); }

            s32 SuspendThread(ThreadType *thread);
            s32 ResumeThread(ThreadType *thread);

            void CancelThreadSynchronization(ThreadType *thread);

            /* TODO void GetThreadContext(ThreadContextInfo *out_context, const ThreadType *thread); */

            void SetInitialThreadNameUnsafe(ThreadType *thread);

            void NotifyThreadNameChanged(const ThreadType *thread) const { return this->impl.NotifyThreadNameChangedImpl(thread); }
            void SetCurrentThread(ThreadType *thread) const { return this->impl.SetCurrentThread(thread); }
            s32 GetCurrentCoreNumber() const { return this->impl.GetCurrentCoreNumber(); }
            void SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const { return this->impl.SetThreadCoreMask(thread, ideal_core, affinity_mask); }
            void GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const { return this->impl.GetThreadCoreMask(out_ideal_core, out_affinity_mask, thread); }
            u64 GetThreadAvailableCoreMask() const { return this->impl.GetThreadAvailableCoreMask(); }

            void PushBackToAllThreadsListUnsafe(ThreadType *thread) {
                this->all_threads_list.push_back(*thread);
                ++this->num_created_threads;
                this->total_thread_stack_size += thread->stack_size;
            }

            void EraseFromAllThreadsListUnsafe(ThreadType *thread) {
                this->all_threads_list.erase(this->all_threads_list.iterator_to(*thread));
                --this->num_created_threads;
                this->total_thread_stack_size -= thread->stack_size;
            }

            void PushBackToAllThreadsListSafe(ThreadType *thread) {
                std::scoped_lock lk(this->cs);
                this->PushBackToAllThreadsListUnsafe(thread);
            }

            void EraseFromAllThreadsListSafe(ThreadType *thread) {
                std::scoped_lock lk(this->cs);
                this->EraseFromAllThreadsListUnsafe(thread);
            }

            void PlaceThreadObjectUnderThreadManagerSafe(ThreadType *thread) {
                SetInitialThreadNameUnsafe(thread);
                {
                    std::scoped_lock lk(this->cs);
                    this->PushBackToAllThreadsListUnsafe(thread);
                }
            }

            ThreadType *AllocateThreadType() const {
                return reinterpret_cast<ThreadType *>(std::malloc(sizeof(ThreadType)));
            }

            void FreeThreadType(ThreadType *thread) const {
                std::free(thread);
            }

            const ThreadType *GetMainThread() const {
                return std::addressof(this->main_thread);
            }

            size_t GetTotalThreadStackSize() const {
                return this->total_thread_stack_size;
            }

            ThreadId GetThreadId(const ThreadType *thread) {
                return this->impl.GetThreadId(thread);
            }
        public:
            static void InvokeThread(ThreadType *thread);
    };

}

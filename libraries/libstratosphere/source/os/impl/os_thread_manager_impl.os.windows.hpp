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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class ThreadManagerWindowsImpl {
        NON_COPYABLE(ThreadManagerWindowsImpl);
        NON_MOVEABLE(ThreadManagerWindowsImpl);
        private:
            u32 m_tls_index;
        public:
            explicit ThreadManagerWindowsImpl(ThreadType *main_thread);

            Result CreateThread(ThreadType *thread, s32 ideal_core);
            void DestroyThreadUnsafe(ThreadType *thread);
            void StartThread(const ThreadType *thread);
            void WaitForThreadExit(ThreadType *thread);
            bool TryWaitForThreadExit(ThreadType *thread);
            void YieldThread();
            bool ChangePriority(ThreadType *thread, s32 priority);
            s32 GetCurrentPriority(const ThreadType *thread) const;
            ThreadId GetThreadId(const ThreadType *thread) const;

            void SuspendThreadUnsafe(ThreadType *thread);
            void ResumeThreadUnsafe(ThreadType *thread);

            /* TODO: void GetThreadContextUnsafe(ThreadContextInfo *out_context, const ThreadType *thread); */

            void NotifyThreadNameChangedImpl(const ThreadType *thread) const;

            void SetCurrentThread(ThreadType *thread) const;

            ThreadType *GetCurrentThread() const;

            s32 GetCurrentCoreNumber() const;
            s32 GetDefaultCoreNumber() const { return 0; }

            void SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const;
            void GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const;
            u64 GetThreadAvailableCoreMask() const;

            bool MapAliasStack(void **out, void *stack, size_t size) {
                AMS_UNUSED(stack, size);
                *out = nullptr;
                return true;
            }

            bool UnmapAliasStack(void *alias_stack, void *original_stack, size_t size) {
                AMS_UNUSED(alias_stack, original_stack, size);
                return true;
            }

            NORETURN void ExitProcessImpl() {
                AMS_ABORT("TODO: Just exit?");
            }

            NORETURN void QuickExit() {
                AMS_ABORT("TODO: Just exit?");
            }
    };

    using ThreadManagerImpl = ThreadManagerWindowsImpl;

}

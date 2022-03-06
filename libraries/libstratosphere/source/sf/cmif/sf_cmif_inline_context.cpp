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

namespace ams::sf {

    namespace cmif {

        namespace {

            #if !defined(ATMOSPHERE_COMPILER_CLANG)
            ALWAYS_INLINE util::AtomicRef<uintptr_t> GetAtomicSfInlineContext(os::ThreadType *thread = os::GetCurrentThread()) {
                uintptr_t * const p = std::addressof(os::GetSdkInternalTlsArray(thread)->sf_inline_context);

                return util::AtomicRef<uintptr_t>(*p);
            }
            #else
            ALWAYS_INLINE util::Atomic<uintptr_t> &GetAtomicSfInlineContext(os::ThreadType *thread = os::GetCurrentThread()) {
                uintptr_t * const p = std::addressof(os::GetSdkInternalTlsArray(thread)->sf_inline_context);
                static_assert(sizeof(std::atomic<uintptr_t>) == sizeof(uintptr_t));
                static_assert(sizeof(util::Atomic<uintptr_t>) == sizeof(std::atomic<uintptr_t>));

                return *reinterpret_cast<util::Atomic<uintptr_t> *>(p);
            }
            #endif

            ALWAYS_INLINE void OnSetInlineContext(os::ThreadType *thread) {
                #if defined(ATMOSPHERE_OS_HORIZON)
                /* Ensure that libnx receives the priority value. */
                ::fsSetPriority(static_cast<::FsPriority>(::ams::sf::GetFsInlineContext(thread)));
                #else
                AMS_UNUSED(thread);
                #endif
            }

        }

        InlineContext GetInlineContext() {
            /* Get the context. */
            uintptr_t thread_context = GetAtomicSfInlineContext().Load();

            /* Copy it out. */
            InlineContext ctx;
            static_assert(sizeof(ctx) <= sizeof(thread_context));
            std::memcpy(std::addressof(ctx), std::addressof(thread_context), sizeof(ctx));
            return ctx;
        }

        InlineContext SetInlineContext(InlineContext ctx) {
            /* Get current thread. */
            os::ThreadType * const cur_thread = os::GetCurrentThread();
            ON_SCOPE_EXIT { OnSetInlineContext(cur_thread); };

            /* Create the new context. */
            static_assert(sizeof(ctx) <= sizeof(uintptr_t));
            uintptr_t new_context_value = 0;
            std::memcpy(std::addressof(new_context_value), std::addressof(ctx), sizeof(ctx));

            /* Get the old context. */
            uintptr_t old_context_value = GetAtomicSfInlineContext(cur_thread).Exchange(new_context_value);

            /* Convert and copy it out. */
            InlineContext old_ctx;
            std::memcpy(std::addressof(old_ctx), std::addressof(old_context_value), sizeof(old_ctx));
            return old_ctx;
        }

    }

    namespace {

        #if !defined(ATMOSPHERE_COMPILER_CLANG)
        ALWAYS_INLINE util::AtomicRef<u8> GetAtomicFsInlineContext(os::ThreadType *thread) {
            uintptr_t * const p = std::addressof(os::GetSdkInternalTlsArray(thread)->sf_inline_context);

            return util::AtomicRef<u8>(*reinterpret_cast<u8 *>(p));
        }
        #else
        ALWAYS_INLINE util::Atomic<u8> &GetAtomicFsInlineContext(os::ThreadType *thread) {
            uintptr_t * const p = std::addressof(os::GetSdkInternalTlsArray(thread)->sf_inline_context);
            static_assert(sizeof(std::atomic<u8>) == sizeof(u8));
            static_assert(sizeof(util::Atomic<u8>) == sizeof(std::atomic<u8>));

            return *reinterpret_cast<util::Atomic<u8> *>(reinterpret_cast<u8 *>(p));
        }
        #endif

    }

    u8 GetFsInlineContext(os::ThreadType *thread) {
        return GetAtomicFsInlineContext(thread).Load();
    }

    u8 SetFsInlineContext(os::ThreadType *thread, u8 ctx) {
        ON_SCOPE_EXIT { cmif::OnSetInlineContext(thread); };

        return GetAtomicFsInlineContext(thread).Exchange(ctx);
    }

}

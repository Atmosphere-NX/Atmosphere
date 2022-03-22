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
#include <vapours.hpp>
#include <stratosphere/os.hpp>

namespace ams::fssystem::buffers {

    namespace impl {

        constexpr inline auto RetryWait = TimeSpan::FromMilliSeconds(10);

    }

    template<typename F, typename OnFailure>
    Result DoContinuouslyUntilBufferIsAllocated(F f, OnFailure on_failure, const char *function_name) {
        constexpr auto BufferAllocationRetryLogCountMax = 10;
        constexpr auto BufferAllocationRetryLogInterval = 100;
        for (auto count = 1; true; count++) {
            R_TRY_CATCH(f()) {
                R_CATCH(fs::ResultBufferAllocationFailed) {
                    if ((1 <= count && count <= BufferAllocationRetryLogCountMax) || ((count % BufferAllocationRetryLogInterval) == 0)) {
                        /* TODO: Log */
                        AMS_UNUSED(function_name);
                    }
                    R_TRY(on_failure());

                    os::SleepThread(impl::RetryWait);

                    continue;
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }
    }

    template<typename F>
    Result DoContinuouslyUntilBufferIsAllocated(F f, const char *function_name) {
        R_TRY(DoContinuouslyUntilBufferIsAllocated(f, []() ALWAYS_INLINE_LAMBDA { return ResultSuccess(); }, function_name));
        return ResultSuccess();
    }

    class BufferManagerContext {
        private:
            bool m_needs_blocking;
        public:
            constexpr BufferManagerContext() : m_needs_blocking(false) { /* ... */ }
        public:
            bool IsNeedBlocking() const { return m_needs_blocking; }

            void SetNeedBlocking(bool need) { m_needs_blocking = need; }
    };

    void RegisterBufferManagerContext(const BufferManagerContext *context);
    BufferManagerContext *GetBufferManagerContext();
    void EnableBlockingBufferManagerAllocation();

    class ScopedBufferManagerContextRegistration {
        private:
            BufferManagerContext m_cur_context;
            const BufferManagerContext *m_old_context;
        public:
            ALWAYS_INLINE explicit ScopedBufferManagerContextRegistration() {
                m_old_context = GetBufferManagerContext();
                if (m_old_context != nullptr) {
                    m_cur_context = *m_old_context;
                }
                RegisterBufferManagerContext(std::addressof(m_cur_context));
            }

            ALWAYS_INLINE ~ScopedBufferManagerContextRegistration() {
                RegisterBufferManagerContext(m_old_context);
            }
    };

    template<typename IsValidBufferFunction>
    Result AllocateBufferUsingBufferManagerContext(std::pair<uintptr_t, size_t> *out, fssystem::IBufferManager *buffer_manager, size_t size, const IBufferManager::BufferAttribute attribute, IsValidBufferFunction is_valid_buffer, const char *func_name) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(buffer_manager != nullptr);
        AMS_ASSERT(func_name != nullptr);

        /* Clear the output. */
        *out = std::pair<uintptr_t, size_t>(0, 0);

        /* Get the context. */
        auto context = GetBufferManagerContext();

        auto AllocateBufferImpl = [=]() -> Result {
            auto buffer = buffer_manager->AllocateBuffer(size, attribute);
            if (!is_valid_buffer(buffer)) {
                if (buffer.first != 0) {
                    buffer_manager->DeallocateBuffer(buffer.first, buffer.second);
                }
                return fs::ResultBufferAllocationFailed();
            }
            *out = buffer;
            return ResultSuccess();
        };

        if (context == nullptr || !context->IsNeedBlocking()) {
            /* If there's no context (or we don't need to block), just allocate the buffer. */
            R_TRY(AllocateBufferImpl());
        } else {
            /* Otherwise, try to allocate repeatedly. */
            R_TRY(DoContinuouslyUntilBufferIsAllocated(AllocateBufferImpl, func_name));
        }

        AMS_ASSERT(out->first != 0);
        return ResultSuccess();
    }

}

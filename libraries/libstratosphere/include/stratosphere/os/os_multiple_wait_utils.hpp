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
#include <stratosphere/os/os_message_queue_common.hpp>
#include <stratosphere/os/os_multiple_wait_api.hpp>
#include <stratosphere/os/os_multiple_wait_types.hpp>

namespace ams::os {

    namespace impl {

        class AutoMultiWaitHolder {
            private:
                MultiWaitHolderType m_holder;
            public:
                template<typename T>
                ALWAYS_INLINE explicit AutoMultiWaitHolder(MultiWaitType *multi_wait, T &&arg) {
                    InitializeMultiWaitHolder(std::addressof(m_holder), std::forward<T>(arg));
                    LinkMultiWaitHolder(multi_wait, std::addressof(m_holder));
                }

                ALWAYS_INLINE ~AutoMultiWaitHolder() {
                    UnlinkMultiWaitHolder(std::addressof(m_holder));
                    FinalizeMultiWaitHolder(std::addressof(m_holder));
                }

                ALWAYS_INLINE std::pair<MultiWaitHolderType *, int> ConvertResult(const std::pair<MultiWaitHolderType *, int> result, int index) {
                    if (result.first == std::addressof(m_holder)) {
                        return std::make_pair(static_cast<MultiWaitHolderType *>(nullptr), index);
                    } else {
                        return result;
                    }
                }
        };

        template<typename F>
        inline std::pair<MultiWaitHolderType *, int> WaitAnyImpl(F &&func, MultiWaitType *multi_wait, int) {
            return std::pair<MultiWaitHolderType *, int>(func(multi_wait), -1);
        }

        template<typename F, typename T, typename... Args>
        inline std::pair<MultiWaitHolderType *, int> WaitAnyImpl(F &&func, MultiWaitType *multi_wait, int index, T &&x, Args &&... args) {
            AutoMultiWaitHolder holder(multi_wait, std::forward<T>(x));
            return holder.ConvertResult(WaitAnyImpl(std::forward<F>(func), multi_wait, index + 1, std::forward<Args>(args)...), index);
        }

        template<typename F, typename... Args>
        inline std::pair<MultiWaitHolderType *, int> WaitAnyImpl(F &&func, MultiWaitType *multi_wait, Args &&... args) {
            return WaitAnyImpl(std::forward<F>(func), multi_wait, 0, std::forward<Args>(args)...);
        }

        class TempMultiWait {
            private:
                MultiWaitType m_multi_wait;
            public:
                ALWAYS_INLINE TempMultiWait() {
                    os::InitializeMultiWait(std::addressof(m_multi_wait));
                }

                ALWAYS_INLINE ~TempMultiWait() {
                    os::FinalizeMultiWait(std::addressof(m_multi_wait));
                }

                MultiWaitType *Get() {
                    return std::addressof(m_multi_wait);
                }
        };

        template<typename F, typename... Args>
        inline std::pair<MultiWaitHolderType *, int> WaitAnyImpl(F &&func, Args &&... args) {
            TempMultiWait temp_multi_wait;
            return WaitAnyImpl(std::forward<F>(func), temp_multi_wait.Get(), 0, std::forward<Args>(args)...);
        }

        using WaitAnyFunction = MultiWaitHolderType * (*)(MultiWaitType *);

        class NotBoolButInt {
            private:
                int m_value;
            public:
                constexpr ALWAYS_INLINE NotBoolButInt(int v) : m_value(v) { /* ... */ }

                constexpr ALWAYS_INLINE operator int() const { return m_value; }

                explicit operator bool() const = delete;
        };

    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline std::pair<MultiWaitHolderType *, int> WaitAny(MultiWaitType *multi_wait, Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::WaitAny), multi_wait, std::forward<Args>(args)...);
    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline int WaitAny(Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::WaitAny), std::forward<Args>(args)...).second;
    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline std::pair<MultiWaitHolderType *, int> TryWaitAny(MultiWaitType *multi_wait, Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::TryWaitAny), multi_wait, std::forward<Args>(args)...);
    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline impl::NotBoolButInt TryWaitAny(Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::TryWaitAny), std::forward<Args>(args)...).second;
    }

}

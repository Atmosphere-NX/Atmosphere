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
#include <vapours.hpp>
#include <stratosphere/os/os_message_queue_common.hpp>
#include <stratosphere/os/os_waitable_api.hpp>
#include <stratosphere/os/os_waitable_types.hpp>

namespace ams::os {

    namespace impl {

        class AutoWaitableHolder {
            private:
                WaitableHolderType m_holder;
            public:
                template<typename T>
                ALWAYS_INLINE explicit AutoWaitableHolder(WaitableManagerType *manager, T &&arg) {
                    InitializeWaitableHolder(std::addressof(m_holder), std::forward<T>(arg));
                    LinkWaitableHolder(manager, std::addressof(m_holder));
                }

                ALWAYS_INLINE ~AutoWaitableHolder() {
                    UnlinkWaitableHolder(std::addressof(m_holder));
                    FinalizeWaitableHolder(std::addressof(m_holder));
                }

                ALWAYS_INLINE std::pair<WaitableHolderType *, int> ConvertResult(const std::pair<WaitableHolderType *, int> result, int index) {
                    if (result.first == std::addressof(m_holder)) {
                        return std::make_pair(static_cast<WaitableHolderType *>(nullptr), index);
                    } else {
                        return result;
                    }
                }
        };

        template<typename F>
        inline std::pair<WaitableHolderType *, int> WaitAnyImpl(F &&func, WaitableManagerType *manager, int) {
            return std::pair<WaitableHolderType *, int>(func(manager), -1);
        }

        template<typename F, typename T, typename... Args>
        inline std::pair<WaitableHolderType *, int> WaitAnyImpl(F &&func, WaitableManagerType *manager, int index, T &&x, Args &&... args) {
            AutoWaitableHolder holder(manager, std::forward<T>(x));
            return holder.ConvertResult(WaitAnyImpl(std::forward<F>(func), manager, index + 1, std::forward<Args>(args)...), index);
        }

        template<typename F, typename... Args>
        inline std::pair<WaitableHolderType *, int> WaitAnyImpl(F &&func, WaitableManagerType *manager, Args &&... args) {
            return WaitAnyImpl(std::forward<F>(func), manager, 0, std::forward<Args>(args)...);
        }

        class TempWaitableManager {
            private:
                WaitableManagerType m_manager;
            public:
                ALWAYS_INLINE TempWaitableManager() {
                    os::InitializeWaitableManager(std::addressof(m_manager));
                }

                ALWAYS_INLINE ~TempWaitableManager() {
                    os::FinalizeWaitableManager(std::addressof(m_manager));
                }

                WaitableManagerType *Get() {
                    return std::addressof(m_manager);
                }
        };

        template<typename F, typename... Args>
        inline std::pair<WaitableHolderType *, int> WaitAnyImpl(F &&func, Args &&... args) {
            TempWaitableManager temp_manager;
            return WaitAnyImpl(std::forward<F>(func), temp_manager.Get(), 0, std::forward<Args>(args)...);
        }

        using WaitAnyFunction = WaitableHolderType * (*)(WaitableManagerType *);

    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline std::pair<WaitableHolderType *, int> WaitAny(WaitableManagerType *manager, Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::WaitAny), manager, std::forward<Args>(args)...);
    }

    template<typename... Args> requires (sizeof...(Args) > 0)
    inline int WaitAny(Args &&... args) {
        return impl::WaitAnyImpl(static_cast<impl::WaitAnyFunction>(&::ams::os::WaitAny), std::forward<Args>(args)...).second;
    }

}

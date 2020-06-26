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

namespace ams::mitm {

    template<typename T>
    concept IsModule = requires(T, void *arg) {
        { T::ThreadPriority      } -> std::convertible_to<s32>;
        { T::StackSize           } -> std::convertible_to<size_t>;
        { T::Stack               } -> std::convertible_to<void *>;
        { T::ThreadFunction(arg) } -> std::same_as<void>;
    };

    #define DEFINE_MITM_MODULE_CLASS(ss, prio) class MitmModule {                \
        public:                                                                  \
            static constexpr s32 ThreadPriority = prio;                          \
            static constexpr size_t StackSize = ss;                              \
            alignas(os::ThreadStackAlignment) static inline u8 Stack[StackSize]; \
        public:                                                                  \
            static void ThreadFunction(void *);                                  \
    }

    template<class M> requires IsModule<M>
    struct ModuleTraits {
        static constexpr void *Stack = &M::Stack[0];
        static constexpr size_t StackSize = M::StackSize;

        static constexpr s32 ThreadPriority = M::ThreadPriority;

        static constexpr ::ThreadFunc ThreadFunction = &M::ThreadFunction;
    };

}

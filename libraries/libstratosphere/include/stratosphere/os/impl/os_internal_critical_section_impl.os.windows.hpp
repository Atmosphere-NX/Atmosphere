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

namespace ams::os::impl {

    class InternalConditionVariableImpl;

    struct WindowsCriticalSection;

    using WindowsCriticalSectionStorage = util::TypedStorage<WindowsCriticalSection, 8 + 4 * sizeof(void *), alignof(void *)>;

    #if defined(ATMOSPHERE_ARCH_X64)
        #define AMS_OS_WINDOWS_CRITICAL_SECTION_CONSTANT_INITIALIZE_ARRAY_VALUES -1, -1, -1, 0, 0, 0, 0, 0, 0, 0
    #elif defined(ATMOSPHERE_ARCH_X86)
        #define AMS_OS_WINDOWS_CRITICAL_SECTION_CONSTANT_INITIALIZE_ARRAY_VALUES -1, -1, 0, 0, 0, 0
    #else
        #error "Unknown architecture for WindowsCriticalSection initializer"
    #endif

    class InternalCriticalSectionImpl {
        private:
            friend class InternalConditionVariableImpl;
        private:
            union {
                s32 _arr[sizeof(WindowsCriticalSectionStorage) / sizeof(s32)];
                WindowsCriticalSectionStorage m_windows_critical_section_storage;
            };
        public:
            constexpr InternalCriticalSectionImpl() : _arr{AMS_OS_WINDOWS_CRITICAL_SECTION_CONSTANT_INITIALIZE_ARRAY_VALUES} { /* ... */ }
            constexpr ~InternalCriticalSectionImpl() {
                if (!std::is_constant_evaluated()) {
                    this->Finalize();
                }
            }

            void Initialize();
            void Finalize();

            void Enter();
            bool TryEnter();
            void Leave();

            bool IsLockedByCurrentThread() const;

            ALWAYS_INLINE void Lock()    { return this->Enter(); }
            ALWAYS_INLINE bool TryLock() { return this->TryEnter(); }
            ALWAYS_INLINE void Unlock()  { return this->Leave(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

    struct InternalCriticalSectionStorageTypeForConstantInitialize { s32 _arr[sizeof(WindowsCriticalSectionStorage) / sizeof(s32)]; };

    #define AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CONSTANT_INITIALIZER { AMS_OS_WINDOWS_CRITICAL_SECTION_CONSTANT_INITIALIZE_ARRAY_VALUES }

    #define AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD

}

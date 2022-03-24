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

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */

    namespace impl {

        template<typename T, size_t Size>
        struct PimplHelper {
            static void Construct(void *);
            static void Destroy(void *);
        };

    }

    template<typename T, size_t Size>
    class Pimpl {
        private:
            #if defined(ATMOSPHERE_OS_HORIZON) || defined(ATMOSPHERE_OS_WINDOWS) || defined(ATMOSPHERE_OS_LINUX)
                static constexpr size_t ExtraSizeToEnsureCompatibility = 0;
            #elif defined(ATMOSPHERE_OS_MACOS)
                static constexpr size_t ExtraSizeToEnsureCompatibility = 0x20;
            #endif

            static constexpr size_t StorageSize = Size + ExtraSizeToEnsureCompatibility;
        private:
            alignas(0x10) u8 m_storage[StorageSize];
        public:
            ALWAYS_INLINE Pimpl()  { impl::PimplHelper<T, StorageSize>::Construct(m_storage); }
            ALWAYS_INLINE ~Pimpl() { impl::PimplHelper<T, StorageSize>::Destroy(m_storage); }

            ALWAYS_INLINE T *Get() { return reinterpret_cast<T *>(m_storage + 0); }
            ALWAYS_INLINE T *operator->() { return reinterpret_cast<T *>(m_storage + 0); }
    };

    #define AMS_FSSYSTEM_ENABLE_PIMPL(_CLASSNAME_)                    \
        namespace ams::fssystem::impl {                               \
                                                                      \
            template<size_t Size>                                     \
            struct PimplHelper<_CLASSNAME_, Size> {                   \
                static ALWAYS_INLINE void Construct(void *p) {        \
                    static_assert(sizeof(_CLASSNAME_) <= Size);       \
                    static_assert(alignof(_CLASSNAME_) <= 0x10);      \
                                                                      \
                    std::construct_at(static_cast<_CLASSNAME_ *>(p)); \
                }                                                     \
                                                                      \
                static ALWAYS_INLINE void Destroy(void *p) {          \
                    std::destroy_at(static_cast<_CLASSNAME_ *>(p));   \
                }                                                     \
            };                                                        \
                                                                      \
        }

}

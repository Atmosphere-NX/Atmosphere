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

namespace ams::tipc {

    class ServiceObjectBase;

    class ServiceObjectDeleter {
        public:
            virtual void DeleteServiceObject(ServiceObjectBase *object) = 0;
    };

    template<typename T>
    concept IsServiceObjectDeleter = std::derived_from<T, ServiceObjectDeleter>;

    class ServiceObjectBase {
        private:
            ServiceObjectDeleter *m_deleter;
        public:
            constexpr ALWAYS_INLINE ServiceObjectBase() : m_deleter(nullptr) { /* ... */ }

            ALWAYS_INLINE void SetDeleter(ServiceObjectDeleter *deleter) {
                m_deleter = deleter;
            }

            ALWAYS_INLINE ServiceObjectDeleter *GetDeleter() const {
                return m_deleter;
            }

            virtual ~ServiceObjectBase() { /* ... */ }
            virtual Result ProcessRequest() = 0;
    };

    template<typename T>
    concept IsServiceObject = std::derived_from<T, ServiceObjectBase>;

}


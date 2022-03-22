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

    class RngManager {
        private:
            util::TinyMT m_mt;
            os::SdkMutex m_lock;
            bool m_initialized;
        private:
            void Initialize();
        public:
            RngManager() : m_mt(), m_lock(), m_initialized() { /* ... */ }
        public:
            u64 GenerateRandomU64();
    };

}

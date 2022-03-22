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

namespace ams::tio {

    using SdCardInsertionCallback = void(*)(bool inserted);

    class SdCardObserver {
        private:
            bool m_inserted;
            SdCardInsertionCallback m_callback;
            os::ThreadType m_thread;
        public:
            constexpr SdCardObserver() : m_inserted(false), m_callback(nullptr), m_thread{} { /* ... */ }

            bool IsSdCardInserted() const { return m_inserted; }
        private:
            static void ThreadEntry(void *arg) {
                static_cast<SdCardObserver *>(arg)->ThreadFunc();
            }

            void ThreadFunc();
        public:
            void Initialize(void *thread_stack, size_t thread_stack_size);
            void SetCallback(SdCardInsertionCallback callback);
    };

}
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

namespace ams::ncm {

    class AutoBuffer {
        NON_COPYABLE(AutoBuffer);
        private:
            u8 *m_buffer;
            size_t m_size;
        public:
            AutoBuffer() : m_buffer(nullptr), m_size(0) { /* ... */ }

            ~AutoBuffer() {
                this->Reset();
            }

            AutoBuffer(AutoBuffer &&rhs) {
                m_buffer = rhs.m_buffer;
                m_size = rhs.m_size;
                rhs.m_buffer = nullptr;
                rhs.m_size = 0;
            }

            AutoBuffer &operator=(AutoBuffer &&rhs) {
                AutoBuffer(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(AutoBuffer &rhs) {
                std::swap(m_buffer, rhs.m_buffer);
                std::swap(m_size, rhs.m_size);
            }

            void Reset() {
                if (m_buffer != nullptr) {
                    delete[] m_buffer;
                    m_buffer = nullptr;
                    m_size = 0;
                }
            }

            u8 *Get() const {
                return m_buffer;
            }

            size_t GetSize() const {
                return m_size;
            }

            Result Initialize(size_t size) {
                /* Check that we're not already initialized. */
                AMS_ABORT_UNLESS(m_buffer == nullptr);

                /* Allocate a buffer. */
                m_buffer = new (std::nothrow) u8[size];
                R_UNLESS(m_buffer != nullptr, ncm::ResultAllocationFailed());

                m_size = size;
                return ResultSuccess();
            }

            Result Initialize(const void *buf, size_t size) {
                /* Create a new buffer of the right size. */
                R_TRY(this->Initialize(size));

                /* Copy the input data in. */
                std::memcpy(m_buffer, buf, size);

                return ResultSuccess();
            }
    };

}
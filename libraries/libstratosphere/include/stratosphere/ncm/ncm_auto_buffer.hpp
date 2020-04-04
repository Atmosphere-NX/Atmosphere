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

namespace ams::ncm {

    class AutoBuffer {
        NON_COPYABLE(AutoBuffer);
        private:
            u8 *buffer;
            size_t size;
        public:
            AutoBuffer() : buffer(nullptr), size(0) { /* ... */ }

            ~AutoBuffer() {
                this->Reset();
            }

            AutoBuffer(AutoBuffer &&rhs) {
                this->buffer = rhs.buffer;
                this->size = rhs.size;
                rhs.buffer = nullptr;
                rhs.size = 0;
            }

            AutoBuffer &operator=(AutoBuffer &&rhs) {
                AutoBuffer(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(AutoBuffer &rhs) {
                std::swap(this->buffer, rhs.buffer);
                std::swap(this->size, rhs.size);
            }

            void Reset() {
                if (this->buffer != nullptr) {
                    delete[] this->buffer;
                    this->buffer = nullptr;
                    this->size = 0;
                }
            }

            u8 *Get() const {
                return this->buffer;
            }

            size_t GetSize() const {
                return this->size;
            }

            Result Initialize(size_t size) {
                /* Check that we're not already initialized. */
                AMS_ABORT_UNLESS(this->buffer == nullptr);

                /* Allocate a buffer. */
                this->buffer = new (std::nothrow) u8[size];
                R_UNLESS(this->buffer != nullptr, ResultAllocationFailed());

                this->size = size;
                return ResultSuccess();
            }

            Result Initialize(const void *buf, size_t size) {
                /* Create a new buffer of the right size. */
                R_TRY(this->Initialize(size));

                /* Copy the input data in. */
                std::memcpy(this->buffer, buf, size);

                return ResultSuccess();
            }
    };

}
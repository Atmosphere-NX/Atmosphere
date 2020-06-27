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
#include <stratosphere/os/os_transfer_memory_types.hpp>
#include <stratosphere/os/os_transfer_memory_api.hpp>

namespace ams::os {

    class TransferMemory {
        NON_COPYABLE(TransferMemory);
        NON_MOVEABLE(TransferMemory);
        private:
            TransferMemoryType tmem;
        public:
            constexpr TransferMemory() : tmem{ .state = TransferMemoryType::State_NotInitialized } {
                /* ... */
            }

            TransferMemory(void *address, size_t size, MemoryPermission perm) {
                R_ABORT_UNLESS(CreateTransferMemory(std::addressof(this->tmem), address, size, perm));
            }

            TransferMemory(size_t size, Handle handle, bool managed) {
                this->Attach(size, handle, managed);
            }

            ~TransferMemory() {
                if (this->tmem.state == TransferMemoryType::State_NotInitialized) {
                    return;
                }
                DestroyTransferMemory(std::addressof(this->tmem));
            }

            void Attach(size_t size, Handle handle, bool managed) {
                AttachTransferMemory(std::addressof(this->tmem), size, handle, managed);
            }

            Handle Detach() {
                return DetachTransferMemory(std::addressof(this->tmem));
            }

            Result Map(void **out, MemoryPermission owner_perm) {
                return MapTransferMemory(out, std::addressof(this->tmem), owner_perm);
            }

            void Unmap() {
                UnmapTransferMemory(std::addressof(this->tmem));
            }

            operator TransferMemoryType &() {
                return this->tmem;
            }

            operator const TransferMemoryType &() const {
                return this->tmem;
            }

            TransferMemoryType *GetBase() {
                return std::addressof(this->tmem);
            }
    };

}

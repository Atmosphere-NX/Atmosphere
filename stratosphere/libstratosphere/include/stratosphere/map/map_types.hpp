/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include "../results.hpp"

namespace sts::map {

    /* Types. */
    struct AddressSpaceInfo {
        uintptr_t heap_base;
        size_t heap_size;
        uintptr_t heap_end;
        uintptr_t alias_base;
        size_t alias_size;
        uintptr_t alias_end;
        uintptr_t aslr_base;
        size_t aslr_size;
        uintptr_t aslr_end;
    };

    static constexpr uintptr_t AslrBase32Bit           = 0x0000200000ul;
    static constexpr size_t    AslrSize32Bit           = 0x003FE00000ul;
    static constexpr uintptr_t AslrBase64BitDeprecated = 0x0008000000ul;
    static constexpr size_t    AslrSize64BitDeprecated = 0x0078000000ul;
    static constexpr uintptr_t AslrBase64Bit           = 0x0008000000ul;
    static constexpr size_t    AslrSize64Bit           = 0x7FF8000000ul;

    class AutoCloseMap {
        private:
            Handle process_handle;
            Result result;
            void *mapped_address;
            uintptr_t base_address;
            size_t size;
        public:
            AutoCloseMap(uintptr_t mp, Handle p_h, uintptr_t ba, size_t sz) : process_handle(p_h), mapped_address(reinterpret_cast<void *>(mp)), base_address(ba), size(sz) {
                this->result = svcMapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size);
            }

            ~AutoCloseMap() {
                if (this->process_handle != INVALID_HANDLE && R_SUCCEEDED(this->result)) {
                    R_ASSERT(svcUnmapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size));
                }
            }

            Result GetResult() const {
                return this->result;
            }

            bool IsSuccess() const {
                return R_SUCCEEDED(this->result);
            }

            void Invalidate() {
                this->process_handle = INVALID_HANDLE;
            }
    };

    class MappedCodeMemory {
        private:
            Handle process_handle;
            Result result;
            uintptr_t dst_address;
            uintptr_t src_address;
            size_t size;
        public:
            MappedCodeMemory(Result init_res) : process_handle(INVALID_HANDLE), result(init_res), dst_address(0), src_address(0), size(0) {
                /* ... */
            }

            MappedCodeMemory(Handle p_h, uintptr_t dst, uintptr_t src, size_t sz) : process_handle(p_h), dst_address(dst), src_address(src), size(sz) {
                this->result = svcMapProcessCodeMemory(this->process_handle, this->dst_address, this->src_address, this->size);
            }

            ~MappedCodeMemory() {
                if (this->process_handle != INVALID_HANDLE && R_SUCCEEDED(this->result) && this->size > 0) {
                    R_ASSERT(svcUnmapProcessCodeMemory(this->process_handle, this->dst_address, this->src_address, this->size));
                }
            }

            uintptr_t GetDstAddress() const {
                return this->dst_address;
            }

            Result GetResult() const {
                return this->result;
            }

            bool IsSuccess() const {
                return R_SUCCEEDED(this->result);
            }

            void Invalidate() {
                this->process_handle = INVALID_HANDLE;
            }

            MappedCodeMemory &operator=(MappedCodeMemory &&o) {
                this->process_handle = o.process_handle;
                this->result = o.result;
                this->dst_address = o.dst_address;
                this->src_address = o.src_address;
                this->size = o.size;
                o.Invalidate();
                return *this;
            }
    };

}

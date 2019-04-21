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
#include <stratosphere.hpp>

class MappedCodeMemory {
    private:
        Handle process_handle = INVALID_HANDLE;
        Result result = ResultRoInternalError;
        u64 dst_address = 0;
        u64 src_address = 0;
        u64 size = 0;
    public:
        MappedCodeMemory() : process_handle(INVALID_HANDLE), result(ResultRoInternalError), dst_address(0), src_address(0), size(0) {
            /* ... */
        }

        MappedCodeMemory(Handle p_h, u64 dst, u64 src, u64 sz) : process_handle(p_h), dst_address(dst), src_address(src), size(sz) {
            this->result = svcMapProcessCodeMemory(this->process_handle, this->dst_address, this->src_address, this->size);
        }

        ~MappedCodeMemory() {
            if (this->process_handle != INVALID_HANDLE && this->size > 0 && R_SUCCEEDED(this->result)) {
                if (R_FAILED((this->result = svcUnmapProcessCodeMemory(this->process_handle, this->dst_address, this->src_address, this->size)))) {
                    std::abort();
                }
            }
        }

        u64 GetDstAddress() const {
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

class AutoCloseMap {
    private:
        Handle process_handle;
        Result result;
        void *mapped_address;
        u64 base_address;
        u64 size;
    public:
        AutoCloseMap(void *mp, Handle p_h, u64 ba, u64 sz) : process_handle(p_h), mapped_address(mp), base_address(ba), size(sz) {
            this->result = svcMapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size);
        }
        AutoCloseMap(u64 mp, Handle p_h, u64 ba, u64 sz) : process_handle(p_h), mapped_address(reinterpret_cast<void *>(mp)), base_address(ba), size(sz) {
            this->result = svcMapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size);
        }

        ~AutoCloseMap() {
            if (this->process_handle != INVALID_HANDLE && R_SUCCEEDED(this->result)) {
                if (R_FAILED((this->result = svcUnmapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size)))) {
                    std::abort();
                }
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


class MapUtils {
    public:
        static constexpr size_t GuardRegionSize = 0x4000;
        static constexpr size_t LocateRetryCount = 0x200;
    public:
        struct AddressSpaceInfo {
            u64 heap_base;
            u64 heap_size;
            u64 heap_end;
            u64 map_base;
            u64 map_size;
            u64 map_end;
            u64 addspace_base;
            u64 addspace_size;
            u64 addspace_end;
        };
    private:
        static Result GetAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h);
        static Result LocateSpaceForMapDeprecated(u64 *out, u64 out_size);
        static Result LocateSpaceForMapModern(u64 *out, u64 out_size);

        static Result MapCodeMemoryForProcessDeprecated(MappedCodeMemory &out_mcm, Handle process_handle, bool is_64_bit, u64 base_address, u64 size);
        static Result MapCodeMemoryForProcessModern(MappedCodeMemory &out_mcm, Handle process_handle, u64 base_address, u64 size);
    public:
        static Result LocateSpaceForMap(u64 *out, u64 out_size);
        static Result MapCodeMemoryForProcess(MappedCodeMemory &out_mcm, Handle process_handle, bool is_64_bit, u64 base_address, u64 size);
        static bool CanAddGuardRegions(Handle process_handle, u64 address, u64 size);
};
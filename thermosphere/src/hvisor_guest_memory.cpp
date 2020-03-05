/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_virtual_gic.hpp"
#include "hvisor_safe_io_copy.hpp"
#include "cpu/hvisor_cpu_caches.hpp"
#include "cpu/hvisor_cpu_interrupt_mask_guard.hpp"

using namespace ams::hvisor;
using namespace ams::hvisor::cpu;

namespace {

    template<typename T>
    T ReadBufferValue(const void *buf, size_t off)
    {
        static_assert(std::is_unsigned_v<T> && sizeof(T) <= 4);
        T ret;
        std::memcpy(&ret, reinterpret_cast<const u8 *>(buf) + off, sizeof(T));
        return ret;
    }

    template<typename T>
    void WriteBufferValue(void *buf, size_t off, T val)
    {
        static_assert(std::is_unsigned_v<T> && sizeof(T) <= 4);
        std::memcpy(reinterpret_cast<u8 *>(buf) + off, T, sizeof(T));
    }

    size_t GuestReadWriteGicd(size_t offset, size_t size, void *readBuf, const void *writeBuf)
    {
        auto &vgic = VirtualGic::GetInstance();
        if (readBuf != nullptr) {
            size_t readOffset = 0;
            size_t rem = size;
            while (rem > 0) {
                if ((offset + readOffset) % 4 == 0 && rem >= 4) {
                    // All accesses of this kind are valid
                    WriteBufferValue<u32>(readBuf, readOffset, vgic.ReadGicdRegister(offset + readOffset, 4));
                    readOffset += 4;
                    rem -= 4;
                } else if ((offset + readOffset) % 2 == 0 && rem >= 2) {
                    // All accesses of this kind would be translated to ldrh and are thus invalid. Abort.
                    return readOffset;
                } else if (VirtualGic::ValidateGicdRegisterAccess(offset + readOffset, 1)) {
                    // Valid byte access
                    WriteBufferValue<u8>(readBuf, readOffset, vgic.ReadGicdRegister(offset + readOffset, 1));
                    readOffset += 1;
                    rem -= 1;
                } else {
                    // Invalid byte access
                    return readOffset;
                }
            }
        }

        if (writeBuf != nullptr) {
            size_t writeOffset = 0;
            size_t rem = size;
            while (rem > 0) {
                if ((offset + writeOffset) % 4 == 0 && rem >= 4) {
                    // All accesses of this kind are valid
                    vgic.WriteGicdRegister(ReadBufferValue<u32>(writeBuf, writeOffset), offset + writeOffset, 4);
                    writeOffset += 4;
                    rem -= 4;
                } else if ((offset + writeOffset) % 2 == 0 && rem >= 2) {
                    // All accesses of this kind would be translated to ldrh and are thus invalid. Abort.
                    return writeOffset;
                } else if (VirtualGic::ValidateGicdRegisterAccess(offset + writeOffset, 1)) {
                    // Valid byte access
                    vgic.WriteGicdRegister(ReadBufferValue<u8>(writeBuf, writeOffset), offset + writeOffset, 1);
                    writeOffset += 1;
                    rem -= 1;
                } else {
                    // Invalid byte access
                    return writeOffset;
                }
            }
        }

        return size;
    }

    size_t GuestReadWriteDeviceMemory(void *addr, size_t size, void *readBuf, const void *writeBuf)
    {
        if (readBuf != nullptr) {
            size_t sz = SafeIoCopy(readBuf, addr, size);
            if (sz < size) {
                return sz;
            }
        }

        if (writeBuf != nullptr) {
            size_t sz = SafeIoCopy(addr, writeBuf, size);
            if (sz < size) {
                return sz;
            }
        }

        // Translation tables must be on Normal memory & Device memory isn't cacheable, so we don't have
        // that kind of thing to handle...

        return size;
    }

    size_t GuestReadWriteNormalMemory(void *addr, size_t size, void *readBuf, const void *writeBuf)
    {
        if (readBuf != nullptr) {
            std::memcpy(readBuf, addr, size);
        }

        if (writeBuf != nullptr) {
            std::memcpy(addr, writeBuf, size);

            // We may have written to executable memory or to translation tables...
            // & the page may have various aliases.
            // We need to ensure cache & TLB coherency.
            CleanDataCacheRangePoU(addr, size);
            u32 policy = GetInstructionCachePolicy();
            if (policy == 1 || policy == 2) {
                // AVIVT, VIVT
                InvalidateInstructionCache();
            } else {
                // VPIPT, PIPT
                // Ez coherency, just do range operations...
                InvalidateInstructionCacheRangePoU(addr, size);
            }
            TlbInvalidateEl1();
            dsb();
            isb();
        }

        return size;
    }

    size_t GuestReadWriteMemoryPage(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf)
    {
        InterruptMaskGuard ig{};
        size_t offset = addr & 0xFFFul;

        // Translate the VA, stages 1&2
        __asm__ __volatile__ ("at s12e1r, %0" :: "r"(addr) : "memory");
        u64 par = THERMOSPHERE_GET_SYSREG(par_el1);
        if (par & PAR_F) {
            // The translation failed. Why?
            if (par & PAR_S) {
                // Stage 2 fault. Could be an attempt to access the GICD, let's see what the IPA is...
                __asm__ __volatile__ ("at s1e1r, %0" :: "r"(addr) : "memory");
                par = THERMOSPHERE_GET_SYSREG(par_el1);
                if ((par & PAR_F) != 0 || (par & PAR_PA_MASK) != VirtualGic::gicdPhysicalAddress) {
                    // The guest doesn't have access to it...
                    // Read as 0, write ignored
                    if (readBuf != NULL) {
                        std::memset(readBuf, 0, size);
                    }
                } else {
                    // GICD mmio
                    size = GuestReadWriteGicd(offset, size, readBuf, writeBuf);
                }
            } else {
                // Oops, couldn't read/write anything (stage 1 fault)
                size = 0;
            }
        } else {
            /*
                Translation didn't fail.

                To avoid "B2.8 Mismatched memory attributes" we must use the same effective
                attributes & shareability as the guest.

                Note that par_el1 reports the effective shareablity of device and noncacheable memory as inner shareable.
                In fact, the VMSAv8-64 section in the Armv8 ARM reads:
                    "The shareability field is only relevant if the memory is a Normal Cacheable memory type. All Device and Normal
                    Non-cacheable memory regions are always treated as Outer Shareable, regardless of the translation table
                    shareability attributes."

                There's one corner case where we can't avoid it: another core is running,
                changes the attributes (other than permissions) of the page, and issues
                a broadcasting TLB maintenance instructions and/or accesses the page with the altered
                attribute itself. We don't handle this corner case -- just don't read/write that kind of memory...
            */
            u64 memAttribs = (par >> PAR_ATTR_SHIFT) & PAR_ATTR_MASK;
            u64 shrb = (par >> PAR_SH_SHIFT) & PAR_SH_MASK;
            uintptr_t pa = par & PAR_PA_MASK;

            uintptr_t va = MemoryMap::MapGuestPage(pa, memAttribs, shrb);
            void *vaddr = reinterpret_cast<void *>(va + offset);
            if (memAttribs & 0xF0) {
                // Normal memory, or unpredictable
                size = GuestReadWriteNormalMemory(vaddr, size, readBuf, writeBuf);
            } else {
                // Device memory, or unpredictable
                size = GuestReadWriteDeviceMemory(vaddr, size, readBuf, writeBuf);
            }

            MemoryMap::UnmapGuestPage();
        }

        return size;
    }

}

namespace ams::hvisor {

    size_t GuestReadWriteMemory(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf)
    {
        uintptr_t curAddr = addr;
        size_t remainingAmount = size;
        u8 *rb8 = reinterpret_cast<u8 *>(readBuf);
        const u8 *wb8 = reinterpret_cast<const u8 *>(writeBuf);
        while (remainingAmount > 0) {
            size_t expectedAmount = ((curAddr & ~0xFFFul) + 0x1000) - curAddr;
            expectedAmount = expectedAmount > remainingAmount ? remainingAmount : expectedAmount;
            size_t actualAmount = GuestReadWriteMemoryPage(curAddr, expectedAmount, rb8, wb8);
            curAddr += actualAmount;
            rb8 += actualAmount;
            wb8 += actualAmount;
            remainingAmount -= actualAmount;
            if (actualAmount != expectedAmount) {
                break;
            }
        }
        return curAddr - addr;
    }

}

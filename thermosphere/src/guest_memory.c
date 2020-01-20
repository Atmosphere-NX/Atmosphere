/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include <string.h>

#include "guest_memory.h"
#include "memory_map.h"
#include "mmu.h"
#include "spinlock.h"
#include "core_ctx.h"
#include "sysreg.h"
#include "vgic.h"
#include "irq.h"
#include "caches.h"

static size_t guestReadWriteGicd(size_t offset, size_t size, void *readBuf, const void *writeBuf)
{
    recursiveSpinlockLock(&g_irqManager.lock);

    if (readBuf != NULL) {
        size_t readOffset = 0;
        size_t rem = size;
        while (rem > 0) {
            if ((offset + readOffset) % 4 == 0 && rem >= 4) {
                // All accesses of this kind are valid
                *(u32 *)((uintptr_t)readBuf + readOffset) = vgicReadGicdRegister(offset + readOffset, 4);
                readOffset += 4;
                rem -= 4;
            } else if ((offset + readOffset) % 2 == 0 && rem >= 2) {
                // All accesses of this kind would be translated to ldrh and are thus invalid. Abort.
                size = readOffset;
                goto end;
            } else if (vgicValidateGicdRegisterAccess(offset + readOffset, 1)) {
                // Valid byte access
                *(u8 *)((uintptr_t)readBuf + readOffset) = vgicReadGicdRegister(offset + readOffset, 1);
                readOffset += 1;
                rem -= 1;
            } else {
                // Invalid byte access
                size = readOffset;
                goto end;
            }
        }
    }

    if (writeBuf != NULL) {
        size_t writeOffset = 0;
        size_t rem = size;
        while (rem > 0) {
            if ((offset + writeOffset) % 4 == 0 && rem >= 4) {
                // All accesses of this kind are valid
                vgicWriteGicdRegister(*(u32 *)((uintptr_t)writeBuf + writeOffset), offset + writeOffset, 4);
                writeOffset += 4;
                rem -= 4;
            } else if ((offset + writeOffset) % 2 == 0 && rem >= 2) {
                // All accesses of this kind would be translated to ldrh and are thus invalid. Abort.
                size = writeOffset;
                goto end;
            } else if (vgicValidateGicdRegisterAccess(offset + writeOffset, 1)) {
                // Valid byte access
                vgicWriteGicdRegister(*(u32 *)((uintptr_t)writeBuf + writeOffset), offset + writeOffset, 1);
                writeOffset += 1;
                rem -= 1;
            } else {
                // Invalid byte access
                size = writeOffset;
                goto end;
            }
        }
    }

end:
    recursiveSpinlockUnlock(&g_irqManager.lock);
    return size;
}

static size_t guestReadWriteDeviceMemory(void *addr, size_t size, void *readBuf, const void *writeBuf)
{
    // We might trigger bus errors... ignore the exception and return early if that's the case

    CoreCtx *curCtxBackup = currentCoreCtx;
    __compiler_barrier();
    currentCoreCtx = NULL;
    __compiler_barrier();

    uintptr_t addri = (uintptr_t)addr;

    if (readBuf != NULL) {
        size_t readOffset = 0;
        size_t rem = size;
        while (rem > 0 && (__compiler_barrier(), currentCoreCtx == NULL)) {
            if ((addri + readOffset) % 4 == 0 && rem >= 4) {
                *(vu32 *)((uintptr_t)readBuf + readOffset) = *(vu32 *)(addri + readOffset);
                readOffset += 4;
                rem -= 4;
            } else if (readOffset % 2 == 0 && rem >= 2) {
                *(vu16 *)((uintptr_t)readBuf + readOffset) = *(vu16 *)(addri + readOffset);
                readOffset += 2;
                rem -= 2;
            } else {
                *(vu8 *)((uintptr_t)readBuf + readOffset) = *(vu8 *)(addri + readOffset);
                readOffset += 1;
                rem -= 1;
            }
        }
        if (rem != 0) {
            size = readOffset;
            goto end;
        }
    }

    if (writeBuf != NULL) {
        size_t writeOffset = 0;
        size_t rem = size;
        while (rem > 0 && (__compiler_barrier(), currentCoreCtx == NULL)) {
            if ((addri + writeOffset) % 4 == 0 && rem >= 4) {
                *(vu32 *)(addri + writeOffset) = *(vu32 *)((uintptr_t)writeBuf + writeOffset);
                writeOffset += 4;
                rem -= 4;
            } else if (writeOffset % 2 == 0 && rem >= 2) {
                *(vu16 *)(addri + writeOffset) = *(vu16 *)((uintptr_t)writeBuf + writeOffset);
                writeOffset += 2;
                rem -= 2;
            } else {
                *(vu8 *)(addri + writeOffset) = *(vu8 *)((uintptr_t)writeBuf + writeOffset);
                writeOffset += 1;
                rem -= 1;
            }
        }
        if (rem != 0) {
            size = writeOffset;
            goto end;
        }
    }

end:
    __compiler_barrier();
    currentCoreCtx = curCtxBackup;
    __compiler_barrier();
    return size;
}

static size_t guestReadWriteNormalMemory(void *addr, size_t size, void *readBuf, const void *writeBuf)
{
    if (readBuf != NULL) {
        memcpy(readBuf, addr, size);
    }

    if (writeBuf != NULL) {
        memcpy(addr, writeBuf, size);

        // We may have written to executable memory or to translation tables...
        // & the page may have various aliases.
        // We need to ensure cache & TLB coherency.
        cacheCleanDataCacheRangePoU(addr, size);
        u32 policy = cacheGetInstructionCachePolicy();
        if (policy == 1 || policy == 2) {
            // AVIVT, VIVT
            cacheInvalidateInstructionCache();
        } else {
            // VPIPT, PIPT
            // Ez coherency, just do range operations...
            cacheInvalidateInstructionCacheRangePoU(addr, size);
        }
        __tlb_invalidate_el1();
        __dsb();
        __isb();
    }

    return size;
}

static size_t guestReadWriteMemoryPage(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf)
{
    u64 irqFlags = maskIrq();
    size_t offset = addr & 0xFFFull;

    // Translate the VA, stages 1&2
    __asm__ __volatile__ ("at s12e1r, %0" :: "r"(addr) : "memory");
    u64 par = GET_SYSREG(par_el1);
    if (par & PAR_F) {
        // The translation failed. Why?
        if (par & PAR_S) {
            // Stage 2 fault. Could be an attempt to access the GICD, let's see what the IPA is...
            __asm__ __volatile__ ("at s1e1r, %0" :: "r"(addr) : "memory");
            par = GET_SYSREG(par_el1);
            if ((par & PAR_F) != 0 || (par & PAR_PA_MASK) != MEMORY_MAP_VA_GICD) {
                // The guest doesn't have access to it...
                // Read as 0, write ignored
                if (readBuf != NULL) {
                    memset(readBuf, 0, size);
                }
            } else {
                // GICD mmio
                size = guestReadWriteGicd(offset, size, readBuf, writeBuf);
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
        u32 shrb = (par >> PAR_SH_SHIFT) & PAR_SH_MASK;
        uintptr_t pa = par & PAR_PA_MASK;
        uintptr_t va = MEMORY_MAP_VA_GUEST_MEM + 0x2000 * currentCoreCtx->coreId;

        u64 mair = GET_SYSREG(mair_el2);
        mair |= memAttribs << (8 * MEMORY_MAP_MEMTYPE_NORMAL_GUEST_SLOT);
        SET_SYSREG(mair_el2, mair);
        __isb();

        u64 attribs = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_SH(shrb) | MMU_PTE_BLOCK_MEMTYPE(MEMORY_MAP_MEMTYPE_NORMAL_GUEST_SLOT);
        mmu_map_page((uintptr_t *)MEMORY_MAP_VA_TTBL, va, pa, attribs);
        // Note: no need to broadcast here
        __tlb_invalidate_el2_page_local(pa);
        __dsb_local();

        void *vaddr = (void *)(va + offset);
        if (memAttribs & 0xF0) {
            // Normal memory, or unpredictable
            size = guestReadWriteNormalMemory(vaddr, size, readBuf, writeBuf);
        } else {
            // Device memory, or unpredictable
            size = guestReadWriteDeviceMemory(vaddr, size, readBuf, writeBuf);
        }

        __dsb_local();
        __isb();
        mmu_unmap_page((uintptr_t *)MEMORY_MAP_VA_TTBL, va);
        // Note: no need to broadcast here
        __tlb_invalidate_el2_page_local(pa);
        __dsb_local();

        mair &= ~(0xFFul << (8 * MEMORY_MAP_MEMTYPE_NORMAL_GUEST_SLOT));
        SET_SYSREG(mair_el2, mair);
        __isb();
    }

    restoreInterruptFlags(irqFlags);
    return size;
}

size_t guestReadWriteMemory(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf)
{
    uintptr_t curAddr = addr;
    size_t remainingAmount = size;
    u8 *rb8 = (u8 *)readBuf;
    const u8 *wb8 = (const u8*)writeBuf;
    while (remainingAmount > 0) {
        size_t expectedAmount = ((curAddr & ~0xFFFul) + 0x1000) - curAddr;
        expectedAmount = expectedAmount > remainingAmount ? remainingAmount : expectedAmount;
        size_t actualAmount = guestReadWriteMemoryPage(curAddr, expectedAmount, rb8, wb8);
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

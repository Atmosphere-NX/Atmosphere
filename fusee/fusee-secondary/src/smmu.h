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

#ifndef FUSEE_SMMU_H_
#define FUSEE_SMMU_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SMMU_HEAP_BASE_ADDR             0x81000000
#define SMMU_IRAM_BACKUP_ADDR           0x82000000
#define SMMU_AARCH64_PAYLOAD_ADDR       0x40003F00

#define SMMU_PAGE_SHIFT 12
#define SMMU_PAGE_SIZE  (1 << SMMU_PAGE_SHIFT)
#define SMMU_PDIR_COUNT 1024
#define SMMU_PDIR_SIZE  (sizeof(uint32_t) * SMMU_PDIR_COUNT)
#define SMMU_PTBL_COUNT 1024
#define SMMU_PTBL_SIZE  (sizeof(uint32_t) * SMMU_PTBL_COUNT)
#define SMMU_PDIR_SHIFT 12
#define SMMU_PDE_SHIFT  12
#define SMMU_PTE_SHIFT  12
#define SMMU_PFN_MASK   0x000fffff
#define SMMU_PDE_NEXT_SHIFT     28
#define SMMU_ADDR_TO_PFN(addr)  ((addr) >> 12)
#define SMMU_ADDR_TO_PDN(addr)  ((addr) >> 22)
#define SMMU_PDN_TO_ADDR(pdn)   ((pdn) << 22)
#define _READABLE   (1 << 31)
#define _WRITABLE   (1 << 30)
#define _NONSECURE  (1 << 29)
#define _PDE_NEXT   (1 << SMMU_PDE_NEXT_SHIFT)
#define _MASK_ATTR  (_READABLE | _WRITABLE | _NONSECURE)
#define _PDIR_ATTR  (_READABLE | _WRITABLE | _NONSECURE)
#define _PDE_ATTR   (_READABLE | _WRITABLE | _NONSECURE)
#define _PDE_ATTR_N (_PDE_ATTR | _PDE_NEXT)
#define _PDE_VACANT(pdn)    (((pdn) << 10) | _PDE_ATTR)
#define _PTE_ATTR   (_READABLE | _WRITABLE | _NONSECURE)
#define _PTE_VACANT(addr)   (((addr) >> SMMU_PAGE_SHIFT) | _PTE_ATTR)
#define SMMU_MK_PDIR(page, attr)    (((page) >> SMMU_PDIR_SHIFT) | (attr))
#define SMMU_MK_PDE(page, attr) (((page) >> SMMU_PDE_SHIFT) | (attr))
#define SMMU_EX_PTBL_PAGE(pde)  (((pde) & SMMU_PFN_MASK) << SMMU_PDIR_SHIFT)
#define SMMU_PFN_TO_PTE(pfn, attr)  ((pfn) | (attr))
#define SMMU_ASID_ENABLE(asid)  ((asid) | (1 << 31))
#define SMMU_ASID_DISABLE   0
#define SMMU_ASID_ASID(n)   ((n) & ~SMMU_ASID_ENABLE(0))

void smmu_emulate_tsec(void *tsec_keys, const void *package1, size_t package1_size, void *package1_dec);

#endif
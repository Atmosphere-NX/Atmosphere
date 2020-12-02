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
#include <exosphere.hpp>

namespace ams::secmon::fatal {

    namespace {

        /* Definitions. */
        constexpr size_t PageDirectorySize = mmu::PageSize;
        constexpr size_t PageTableSize     = mmu::PageSize;
        static_assert(PageDirectorySize == mmu::PageSize);

        using DeviceVirtualAddress = u64;

        constexpr size_t AsidCount = 0x80;
        constexpr size_t PhysicalAddressBits = 34;
        constexpr size_t PhysicalAddressMask = (1ul << PhysicalAddressBits) - 1ul;
        constexpr size_t DeviceVirtualAddressBits = 34;
        constexpr size_t DeviceVirtualAddressMask = (1ul << DeviceVirtualAddressBits) - 1ul;

        constexpr size_t DevicePageBits = 12;
        constexpr size_t DevicePageSize = (1ul << DevicePageBits);
        static_assert(DevicePageSize == mmu::PageSize);

        constexpr size_t DeviceLargePageBits = 22;
        constexpr size_t DeviceLargePageSize = (1ul << DeviceLargePageBits);
        static_assert(DeviceLargePageSize % DevicePageSize == 0);

        constexpr size_t DeviceRegionBits = 32;
        constexpr size_t DeviceRegionSize = (1ul << DeviceRegionBits);
        static_assert(DeviceRegionSize % DeviceLargePageSize == 0);

        constexpr const uintptr_t MC = secmon::MemoryRegionVirtualDeviceMemoryController.GetAddress();

        constexpr size_t TableCount = (1ul << DeviceVirtualAddressBits) / DeviceRegionSize;

        consteval u32 EncodeAsidRegisterValue(u8 asid) {
            u32 value = 0x80000000u;
            for (size_t t = 0; t < TableCount; t++) {
                value |= (asid << (BITSIZEOF(u8) * t));
            }
            return value;
        }

        constexpr u8 SdmmcAsid = 1;
        constexpr u8 DcAsid    = 2;

        constexpr u32 SdmmcAsidRegisterValue = EncodeAsidRegisterValue(SdmmcAsid);
        constexpr u32 DcAsidRegisterValue    = EncodeAsidRegisterValue(DcAsid);

        constexpr dd::PhysicalAddress DcL0PageTablePhysical    = MemoryRegionPhysicalDramDcL0DevicePageTable.GetAddress();
        constexpr dd::PhysicalAddress SdmmcL0PageTablePhysical = MemoryRegionPhysicalDramSdmmc1L0DevicePageTable.GetAddress();
        constexpr dd::PhysicalAddress SdmmcL1PageTablePhysical = MemoryRegionPhysicalDramSdmmc1L1DevicePageTable.GetAddress();

        /* Types. */
        class EntryBase {
            protected:
                enum Bit : u32 {
                    Bit_Table     = 28,
                    Bit_NonSecure = 29,
                    Bit_Writeable = 30,
                    Bit_Readable  = 31,
                };
            private:
                u32 value;
            protected:
                constexpr ALWAYS_INLINE u32 SelectBit(Bit n) const {
                    return (this->value & (1u << n));
                }

                constexpr ALWAYS_INLINE bool GetBit(Bit n) const {
                    return this->SelectBit(n) != 0;
                }

                static constexpr ALWAYS_INLINE u32 EncodeBit(Bit n, bool en) {
                    return en ? (1u << n) : 0;
                }

                static constexpr ALWAYS_INLINE u32 EncodeValue(bool r, bool w, bool ns, dd::PhysicalAddress addr, bool t) {
                    return EncodeBit(Bit_Readable, r) | EncodeBit(Bit_Writeable, w) | EncodeBit(Bit_NonSecure, ns) | EncodeBit(Bit_Table, t) | static_cast<u32>(addr >> DevicePageBits);
                }

                ALWAYS_INLINE void SetValue(u32 v) {
                    /* Prevent re-ordering around entry modifications. */
                    __asm__ __volatile__("" ::: "memory");
                    this->value = v;
                    __asm__ __volatile__("" ::: "memory");
                }
            public:
                static constexpr ALWAYS_INLINE u32 EncodePtbDataValue(dd::PhysicalAddress addr) {
                    return EncodeValue(true, true, true, addr, false);
                }
            public:
                constexpr ALWAYS_INLINE bool IsNonSecure() const { return this->GetBit(Bit_NonSecure); }
                constexpr ALWAYS_INLINE bool IsWriteable() const { return this->GetBit(Bit_Writeable); }
                constexpr ALWAYS_INLINE bool IsReadable()  const { return this->GetBit(Bit_Readable); }
                constexpr ALWAYS_INLINE bool IsValid()     const { return this->IsWriteable() || this->IsReadable(); }

                constexpr ALWAYS_INLINE u32 GetAttributes() const { return this->SelectBit(Bit_NonSecure) | this->SelectBit(Bit_Writeable) | this->SelectBit(Bit_Readable); }

                constexpr ALWAYS_INLINE dd::PhysicalAddress GetPhysicalAddress() const { return (static_cast<u64>(this->value) << DevicePageBits) & PhysicalAddressMask; }

                ALWAYS_INLINE void Invalidate() { this->SetValue(0); }
        };

        class PageDirectoryEntry : public EntryBase {
            public:
                constexpr ALWAYS_INLINE bool IsTable()     const { return this->GetBit(Bit_Table); }

                ALWAYS_INLINE void SetTable(bool r, bool w, bool ns, dd::PhysicalAddress addr) {
                    AMS_ASSERT(util::IsAligned(addr, DevicePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, true));
                }

                ALWAYS_INLINE void SetLargePage(bool r, bool w, bool ns, dd::PhysicalAddress addr) {
                    AMS_ASSERT(util::IsAligned(addr, DeviceLargePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, false));
                }
        };

        class PageTableEntry : public EntryBase {
            public:
                ALWAYS_INLINE void SetPage(bool r, bool w, bool ns, dd::PhysicalAddress addr) {
                    AMS_ASSERT(util::IsAligned(addr, DevicePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, true));
                }
        };

        /* Memory controller access functionality. */
        void WriteMcRegister(size_t offset, u32 value) {
            reg::Write(MC + offset, value);
        }

        u32 ReadMcRegister(size_t offset) {
            return reg::Read(MC + offset);
        }

        /* Memory controller utilities. */
        void SmmuSynchronizationBarrier() {
            ReadMcRegister(MC_SMMU_CONFIG);
        }

        void InvalidatePtc() {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, 0);
        }

        void InvalidatePtc(dd::PhysicalAddress address) {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_1, (static_cast<u64>(address) >> 32));
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, (address & 0xFFFFFFF0u) | 1u);
        }

        enum TlbFlushVaMatch : u32 {
            TlbFlushVaMatch_All     = 0,
            TlbFlushVaMatch_Section = 2,
            TlbFlushVaMatch_Group   = 3,
        };

        static constexpr ALWAYS_INLINE u32 EncodeTlbFlushValue(bool match_asid, u8 asid, dd::PhysicalAddress address, TlbFlushVaMatch match) {
            return ((match_asid ? 1u : 0u) << 31) | ((asid & 0x7F) << 24) | (((address & 0xFFC00000u) >> DevicePageBits)) | (match);
        }

        void InvalidateTlb() {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(false, 0, 0, TlbFlushVaMatch_All));
        }

        void InvalidateTlb(u8 asid) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, 0, TlbFlushVaMatch_All));
        }

        void InvalidateTlbSection(u8 asid, dd::PhysicalAddress address) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, address, TlbFlushVaMatch_Section));
        }

        void SetTable(u8 asid, dd::PhysicalAddress address) {
            /* Write the table address. */
            {
                WriteMcRegister(MC_SMMU_PTB_ASID, asid);
                WriteMcRegister(MC_SMMU_PTB_DATA, EntryBase::EncodePtbDataValue(address));

                SmmuSynchronizationBarrier();
            }

            /* Ensure consistency. */
            InvalidatePtc();
            InvalidateTlb(asid);
            SmmuSynchronizationBarrier();
        }

        void MapImpl(dd::PhysicalAddress phys_addr, size_t size, DeviceVirtualAddress address, u8 asid, void *l0_table, dd::PhysicalAddress l0_phys, void *l1_table, dd::PhysicalAddress l1_phys) {
            /* Validate L0. */
            AMS_ABORT_UNLESS(l0_table != nullptr);
            AMS_ABORT_UNLESS(l0_phys != 0);

            /* Cache permissions. */
            const bool read  = true;
            const bool write = true;

            /* Walk the directory. */
            u64 remaining = size;
            while (remaining > 0) {
                const size_t l1_index = (address % DeviceRegionSize)    / DeviceLargePageSize;
                const size_t l2_index = (address % DeviceLargePageSize) / DevicePageSize;

                /* Get and validate l1. */
                PageDirectoryEntry *l1 = static_cast<PageDirectoryEntry *>(l0_table);
                AMS_ASSERT(l1 != nullptr);

                /* Setup an l1 table/entry, if needed. */
                if (!l1[l1_index].IsTable()) {
                    /* Check that an entry doesn't already exist. */
                    AMS_ASSERT(!l1[l1_index].IsValid());

                    /* If we can make an l1 entry, do so. */
                    if (l2_index == 0 && util::IsAligned(phys_addr, DeviceLargePageSize) && remaining >= DeviceLargePageSize) {
                        /* Set the large page. */
                        l1[l1_index].SetLargePage(read, write, true, phys_addr);
                        hw::FlushDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                        /* Synchronize. */
                        InvalidatePtc(l0_phys + l1_index * sizeof(PageDirectoryEntry));
                        InvalidateTlbSection(asid, address);
                        SmmuSynchronizationBarrier();

                        /* Advance. */
                        phys_addr        += DeviceLargePageSize;
                        address          += DeviceLargePageSize;
                        remaining        -= DeviceLargePageSize;
                        continue;
                    } else {
                        /* Make an l1 table. */
                        AMS_ABORT_UNLESS(l1_table != nullptr);
                        AMS_ABORT_UNLESS(l1_phys != 0);

                        /* Clear the l1 table. */
                        std::memset(l1_table, 0, mmu::PageSize);
                        hw::FlushDataCache(l1_table, mmu::PageSize);

                        /* Set the l1 table. */
                        l1[l1_index].SetTable(true, true, true, l1_phys);
                        hw::FlushDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                        /* Synchronize. */
                        InvalidatePtc(l0_phys + l1_index * sizeof(PageDirectoryEntry));
                        InvalidateTlbSection(asid, address);
                        SmmuSynchronizationBarrier();
                    }
                }

                /* If we get to this point, l1 must be a table. */
                AMS_ASSERT(l1[l1_index].IsTable());
                AMS_ABORT_UNLESS(l1_table != nullptr);
                AMS_ABORT_UNLESS(l1_phys != 0);

                /* Map l2 entries. */
                {
                    PageTableEntry *l2 = static_cast<PageTableEntry *>(l1_table);

                    const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                    const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                    /* Set the entries. */
                    for (size_t i = 0; i < map_count; ++i) {
                        AMS_ASSERT(!l2[l2_index + i].IsValid());
                        l2[l2_index + i].SetPage(read, write, true, phys_addr + DevicePageSize * i);
                    }
                    hw::FlushDataCache(std::addressof(l2[l2_index]), map_count * sizeof(PageTableEntry));

                    /* Invalidate the page table cache. */
                    for (size_t i = util::AlignDown(l2_index, 4); i <= util::AlignDown(l2_index + map_count - 1, 4); i += 4) {
                        InvalidatePtc(l1_phys + i * sizeof(PageTableEntry));
                    }

                    /* Synchronize. */
                    InvalidateTlbSection(asid, address);
                    SmmuSynchronizationBarrier();

                    /* Advance. */
                    phys_addr        += map_count * DevicePageSize;
                    address          += map_count * DevicePageSize;
                    remaining        -= map_count * DevicePageSize;
                }
            }
        }

    }

    void InitializeDevicePageTableForSdmmc1() {
        /* Configure sdmmc to use our new page table. */
        WriteMcRegister(MC_SMMU_SDMMC1A_ASID, SdmmcAsidRegisterValue);
        SmmuSynchronizationBarrier();

        /* Ensure consistency. */
        InvalidatePtc();
        InvalidateTlb();
        SmmuSynchronizationBarrier();

        /* Clear the L0 Page Table. */
        std::memset(MemoryRegionVirtualDramSdmmc1L0DevicePageTable.GetPointer<void>(), 0, mmu::PageSize);
        hw::FlushDataCache(MemoryRegionVirtualDramSdmmc1L0DevicePageTable.GetPointer<void>(), mmu::PageSize);

        /* Set the page table for the sdmmc asid. */
        SetTable(SdmmcAsid, SdmmcL0PageTablePhysical);

        /* Map the appropriate region into the asid. */
        MapImpl(MemoryRegionPhysicalDramSdmmcMappedData.GetAddress(), MemoryRegionPhysicalDramSdmmcMappedData.GetSize(), MemoryRegionVirtualDramSdmmcMappedData.GetAddress(),
                SdmmcAsid,
                MemoryRegionVirtualDramSdmmc1L0DevicePageTable.GetPointer<void>(), SdmmcL0PageTablePhysical,
                MemoryRegionVirtualDramSdmmc1L1DevicePageTable.GetPointer<void>(), SdmmcL1PageTablePhysical);
    }

    void InitializeDevicePageTableForDc() {
        /* Configure dc to use our new page table. */
        WriteMcRegister(MC_SMMU_DC_ASID, DcAsidRegisterValue);
        SmmuSynchronizationBarrier();

        /* Ensure consistency. */
        InvalidatePtc();
        InvalidateTlb();
        SmmuSynchronizationBarrier();

        /* Clear the L0 Page Table. */
        std::memset(MemoryRegionVirtualDramDcL0DevicePageTable.GetPointer<void>(), 0, mmu::PageSize);
        hw::FlushDataCache(MemoryRegionVirtualDramDcL0DevicePageTable.GetPointer<void>(), mmu::PageSize);

        /* Set the page table for the dc asid. */
        SetTable(DcAsid, DcL0PageTablePhysical);

        /* Map the appropriate region into the asid. */
        static_assert(util::IsAligned(MemoryRegionDramDcFramebuffer.GetAddress(), DeviceLargePageSize));
        static_assert(util::IsAligned(MemoryRegionDramDcFramebuffer.GetSize(),    DeviceLargePageSize));

        MapImpl(MemoryRegionDramDcFramebuffer.GetAddress(), MemoryRegionDramDcFramebuffer.GetSize(), MemoryRegionDramDcFramebuffer.GetAddress(),
                DcAsid,
                MemoryRegionVirtualDramDcL0DevicePageTable.GetPointer<void>(), DcL0PageTablePhysical,
                nullptr, 0);
    }

}

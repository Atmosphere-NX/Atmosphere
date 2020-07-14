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
#include <mesosphere.hpp>
#include "kern_mc_registers.hpp"

namespace ams::kern::board::nintendo::nx {

    namespace {

        /* Definitions. */
        constexpr size_t PageDirectorySize = KPageTableManager::PageTableSize;
        constexpr size_t PageTableSize     = KPageTableManager::PageTableSize;
        static_assert(PageDirectorySize == PageSize);

        constexpr size_t AsidCount = 0x80;
        constexpr size_t PhysicalAddressBits = 34;
        constexpr size_t PhysicalAddressMask = (1ul << PhysicalAddressBits) - 1ul;
        constexpr size_t DeviceVirtualAddressBits = 34;
        constexpr size_t DeviceVirtualAddressMask = (1ul << DeviceVirtualAddressBits) - 1ul;

        constexpr size_t DevicePageBits      = 12;
        constexpr size_t DevicePageSize      = (1ul << DevicePageBits);
        constexpr size_t DeviceLargePageBits = 22;
        constexpr size_t DeviceLargePageSize = (1ul << DevicePageBits);
        static_assert(DevicePageSize == PageSize);

        constexpr size_t DeviceRegionSize    = (1ul << 32);

        constexpr size_t DeviceAsidRegisterOffsets[] = {
            [ams::svc::DeviceName_Afi]        = MC_SMMU_AFI_ASID,
            [ams::svc::DeviceName_Avpc]       = MC_SMMU_AVPC_ASID,
            [ams::svc::DeviceName_Dc]         = MC_SMMU_DC_ASID,
            [ams::svc::DeviceName_Dcb]        = MC_SMMU_DCB_ASID,
            [ams::svc::DeviceName_Hc]         = MC_SMMU_HC_ASID,
            [ams::svc::DeviceName_Hda]        = MC_SMMU_HDA_ASID,
            [ams::svc::DeviceName_Isp2]       = MC_SMMU_ISP2_ASID,
            [ams::svc::DeviceName_MsencNvenc] = MC_SMMU_MSENC_NVENC_ASID,
            [ams::svc::DeviceName_Nv]         = MC_SMMU_NV_ASID,
            [ams::svc::DeviceName_Nv2]        = MC_SMMU_NV2_ASID,
            [ams::svc::DeviceName_Ppcs]       = MC_SMMU_PPCS_ASID,
            [ams::svc::DeviceName_Sata]       = MC_SMMU_SATA_ASID,
            [ams::svc::DeviceName_Vi]         = MC_SMMU_VI_ASID,
            [ams::svc::DeviceName_Vic]        = MC_SMMU_VIC_ASID,
            [ams::svc::DeviceName_XusbHost]   = MC_SMMU_XUSB_HOST_ASID,
            [ams::svc::DeviceName_XusbDev]    = MC_SMMU_XUSB_DEV_ASID,
            [ams::svc::DeviceName_Tsec]       = MC_SMMU_TSEC_ASID,
            [ams::svc::DeviceName_Ppcs1]      = MC_SMMU_PPCS1_ASID,
            [ams::svc::DeviceName_Dc1]        = MC_SMMU_DC1_ASID,
            [ams::svc::DeviceName_Sdmmc1a]    = MC_SMMU_SDMMC1A_ASID,
            [ams::svc::DeviceName_Sdmmc2a]    = MC_SMMU_SDMMC2A_ASID,
            [ams::svc::DeviceName_Sdmmc3a]    = MC_SMMU_SDMMC3A_ASID,
            [ams::svc::DeviceName_Sdmmc4a]    = MC_SMMU_SDMMC4A_ASID,
            [ams::svc::DeviceName_Isp2b]      = MC_SMMU_ISP2B_ASID,
            [ams::svc::DeviceName_Gpu]        = MC_SMMU_GPU_ASID,
            [ams::svc::DeviceName_Gpub]       = MC_SMMU_GPUB_ASID,
            [ams::svc::DeviceName_Ppcs2]      = MC_SMMU_PPCS2_ASID,
            [ams::svc::DeviceName_Nvdec]      = MC_SMMU_NVDEC_ASID,
            [ams::svc::DeviceName_Ape]        = MC_SMMU_APE_ASID,
            [ams::svc::DeviceName_Se]         = MC_SMMU_SE_ASID,
            [ams::svc::DeviceName_Nvjpg]      = MC_SMMU_NVJPG_ASID,
            [ams::svc::DeviceName_Hc1]        = MC_SMMU_HC1_ASID,
            [ams::svc::DeviceName_Se1]        = MC_SMMU_SE1_ASID,
            [ams::svc::DeviceName_Axiap]      = MC_SMMU_AXIAP_ASID,
            [ams::svc::DeviceName_Etr]        = MC_SMMU_ETR_ASID,
            [ams::svc::DeviceName_Tsecb]      = MC_SMMU_TSECB_ASID,
            [ams::svc::DeviceName_Tsec1]      = MC_SMMU_TSEC1_ASID,
            [ams::svc::DeviceName_Tsecb1]     = MC_SMMU_TSECB1_ASID,
            [ams::svc::DeviceName_Nvdec1]     = MC_SMMU_NVDEC1_ASID,
        };
        static_assert(util::size(DeviceAsidRegisterOffsets) == ams::svc::DeviceName_Count);
        constexpr bool DeviceAsidRegistersValid = [] {
            for (size_t i = 0; i < ams::svc::DeviceName_Count; i++) {
                if (DeviceAsidRegisterOffsets[i] == 0 || !util::IsAligned(DeviceAsidRegisterOffsets[i], sizeof(u32))) {
                    return false;
                }
            }
            return true;
        }();

        static_assert(DeviceAsidRegistersValid);

        constexpr ALWAYS_INLINE int GetDeviceAsidRegisterOffset(ams::svc::DeviceName dev) {
            if (dev < ams::svc::DeviceName_Count) {
                return DeviceAsidRegisterOffsets[dev];
            } else {
                return -1;
            }
        }

        constexpr ams::svc::DeviceName HsDevices[] = {
            ams::svc::DeviceName_Afi,
            ams::svc::DeviceName_Dc,
            ams::svc::DeviceName_Dcb,
            ams::svc::DeviceName_Hda,
            ams::svc::DeviceName_Isp2,
            ams::svc::DeviceName_Sata,
            ams::svc::DeviceName_XusbHost,
            ams::svc::DeviceName_XusbDev,
            ams::svc::DeviceName_Tsec,
            ams::svc::DeviceName_Dc1,
            ams::svc::DeviceName_Sdmmc1a,
            ams::svc::DeviceName_Sdmmc2a,
            ams::svc::DeviceName_Sdmmc3a,
            ams::svc::DeviceName_Sdmmc4a,
            ams::svc::DeviceName_Isp2b,
            ams::svc::DeviceName_Gpu,
            ams::svc::DeviceName_Gpub,
            ams::svc::DeviceName_Axiap,
            ams::svc::DeviceName_Etr,
            ams::svc::DeviceName_Tsecb,
            ams::svc::DeviceName_Tsec1,
            ams::svc::DeviceName_Tsecb1,
        };
        constexpr size_t NumHsDevices = util::size(HsDevices);

        constexpr u64 HsDeviceMask = [] {
            u64 mask = 0;
            for (size_t i = 0; i < NumHsDevices; i++) {
                mask |= 1ul << HsDevices[i];
            }
            return mask;
        }();

        constexpr ALWAYS_INLINE bool IsHsSupported(ams::svc::DeviceName dv) {
            return (HsDeviceMask & (1ul << dv)) != 0;
        }

        constexpr ALWAYS_INLINE bool IsValidPhysicalAddress(KPhysicalAddress addr) {
            return (static_cast<u64>(GetInteger(addr)) & ~PhysicalAddressMask) == 0;
        }

        constexpr struct { u64 start; u64 end; } SmmuSupportedRanges[] = {
            [ams::svc::DeviceName_Afi]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Avpc]       = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Dc]         = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Dcb]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Hc]         = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Hda]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Isp2]       = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_MsencNvenc] = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Nv]         = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Nv2]        = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Ppcs]       = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Sata]       = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Vi]         = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Vic]        = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_XusbHost]   = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_XusbDev]    = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Tsec]       = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Ppcs1]      = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Dc1]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Sdmmc1a]    = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Sdmmc2a]    = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Sdmmc3a]    = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Sdmmc4a]    = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Isp2b]      = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Gpu]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Gpub]       = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Ppcs2]      = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Nvdec]      = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Ape]        = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Se]         = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Nvjpg]      = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Hc1]        = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Se1]        = { 0x00000000ul, 0x0FFFFFFFFul },
            [ams::svc::DeviceName_Axiap]      = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Etr]        = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Tsecb]      = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Tsec1]      = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Tsecb1]     = { 0x00000000ul, 0x3FFFFFFFFul },
            [ams::svc::DeviceName_Nvdec1]     = { 0x00000000ul, 0x0FFFFFFFFul },
        };
        static_assert(util::size(SmmuSupportedRanges) == ams::svc::DeviceName_Count);

        constexpr bool IsAttachable(ams::svc::DeviceName device_name, u64 space_address, u64 space_size) {
            if (0 <= device_name && device_name < ams::svc::DeviceName_Count) {
                const auto &range = SmmuSupportedRanges[device_name];
                return range.start <= space_address && (space_address + space_size - 1) <= range.end;
            }
            return false;
        }

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

                static constexpr ALWAYS_INLINE u32 EncodeValue(bool r, bool w, bool ns, KPhysicalAddress addr, bool t) {
                    return EncodeBit(Bit_Readable, r) | EncodeBit(Bit_Writeable, w) | EncodeBit(Bit_NonSecure, ns) | EncodeBit(Bit_Table, t) | static_cast<u32>(addr >> DevicePageBits);
                }

                ALWAYS_INLINE void SetValue(u32 v) {
                    /* Prevent re-ordering around entry modifications. */
                    __asm__ __volatile__("" ::: "memory");
                    this->value = v;
                    __asm__ __volatile__("" ::: "memory");
                }
            public:
                static constexpr ALWAYS_INLINE u32 EncodePtbDataValue(KPhysicalAddress addr) {
                    return EncodeValue(true, true, true, addr, false);
                }
            public:
                constexpr ALWAYS_INLINE bool IsNonSecure() const { return this->GetBit(Bit_NonSecure); }
                constexpr ALWAYS_INLINE bool IsWriteable() const { return this->GetBit(Bit_Writeable); }
                constexpr ALWAYS_INLINE bool IsReadable()  const { return this->GetBit(Bit_Readable); }
                constexpr ALWAYS_INLINE bool IsValid()     const { return this->IsWriteable() || this->IsReadable(); }

                constexpr ALWAYS_INLINE u32 GetAttributes() const { return this->SelectBit(Bit_NonSecure) | this->SelectBit(Bit_Writeable) | this->SelectBit(Bit_Readable); }

                constexpr ALWAYS_INLINE KPhysicalAddress GetPhysicalAddress() { return (static_cast<u64>(this->value) << DevicePageBits) & PhysicalAddressMask; }

                ALWAYS_INLINE void Invalidate() { this->SetValue(0); }
        };

        class PageDirectoryEntry : public EntryBase {
            public:
                constexpr ALWAYS_INLINE bool IsTable()     const { return this->GetBit(Bit_Table); }

                ALWAYS_INLINE void SetTable(bool r, bool w, bool ns, KPhysicalAddress addr) {
                    MESOSPHERE_ASSERT(IsValidPhysicalAddress(addr));
                    MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), DevicePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, true));
                }

                ALWAYS_INLINE void SetLargePage(bool r, bool w, bool ns, KPhysicalAddress addr) {
                    MESOSPHERE_ASSERT(IsValidPhysicalAddress(addr));
                    MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), DeviceLargePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, false));
                }
        };

        class PageTableEntry : public EntryBase {
            public:
                ALWAYS_INLINE void SetPage(bool r, bool w, bool ns, KPhysicalAddress addr) {
                    MESOSPHERE_ASSERT(IsValidPhysicalAddress(addr));
                    MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), DevicePageSize));
                    this->SetValue(EncodeValue(r, w, ns, addr, true));
                }
        };

        class KDeviceAsidManager {
            private:
                using WordType = u32;
                static constexpr u8 ReservedAsids[] = { 0, 1, 2, 3 };
                static constexpr size_t NumReservedAsids = util::size(ReservedAsids);
                static constexpr size_t BitsPerWord = BITSIZEOF(WordType);
                static constexpr size_t NumWords = AsidCount / BitsPerWord;
                static constexpr WordType FullWord = ~WordType(0u);
            private:
                WordType state[NumWords];
                KLightLock lock;
            private:
                constexpr void ReserveImpl(u8 asid) {
                    this->state[asid / BitsPerWord] |= (1u << (asid % BitsPerWord));
                }

                constexpr void ReleaseImpl(u8 asid) {
                    this->state[asid / BitsPerWord] &= ~(1u << (asid % BitsPerWord));
                }

                static constexpr ALWAYS_INLINE WordType ClearLeadingZero(WordType value) {
                    return __builtin_clzll(value) - (BITSIZEOF(unsigned long long) - BITSIZEOF(WordType));
                }
            public:
                constexpr KDeviceAsidManager() : state(), lock() {
                    for (size_t i = 0; i < NumReservedAsids; i++) {
                        this->ReserveImpl(ReservedAsids[i]);
                    }
                }

                Result Reserve(u8 *out, size_t num_desired) {
                    KScopedLightLock lk(this->lock);
                    MESOSPHERE_ASSERT(num_desired > 0);

                    size_t num_reserved = 0;
                    for (size_t i = 0; i < NumWords; i++) {
                        while (this->state[i] != FullWord) {
                            const WordType clear_bit = (this->state[i] + 1) ^ (this->state[i]);
                            this->state[i] |= clear_bit;
                            out[num_reserved++] = static_cast<u8>(BitsPerWord * i + BitsPerWord - 1 - ClearLeadingZero(clear_bit));
                            R_SUCCEED_IF(num_reserved == num_desired);
                        }
                    }

                    /* We failed, so free what we reserved. */
                    for (size_t i = 0; i < num_reserved; i++) {
                        this->ReleaseImpl(out[i]);
                    }
                    return svc::ResultOutOfResource();
                }

                void Release(u8 asid) {
                    KScopedLightLock lk(this->lock);
                    this->ReleaseImpl(asid);
                }
        };

        /* Globals. */
        KLightLock g_lock;
        u8 g_reserved_asid;
        KPhysicalAddress   g_memory_controller_address;
        KPhysicalAddress   g_reserved_table_phys_addr;
        KDeviceAsidManager g_asid_manager;

        /* Memory controller access functionality. */
        void WriteMcRegister(size_t offset, u32 value) {
            KSystemControl::WriteRegisterPrivileged(GetInteger(g_memory_controller_address) + offset, value);
        }

        u32 ReadMcRegister(size_t offset) {
            return KSystemControl::ReadRegisterPrivileged(GetInteger(g_memory_controller_address) + offset);
        }

        void SmmuSynchronizationBarrier() {
            ReadMcRegister(MC_SMMU_CONFIG);
        }

        void InvalidatePtc() {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, 0);
        }

/*
        void InvalidatePtc(KPhysicalAddress address) {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_1, (static_cast<u64>(GetInteger(address)) >> 32));
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, (GetInteger(address) & 0xFFFFFFF0u) | 1u);
        }
*/

        enum TlbFlushVaMatch : u32 {
            TlbFlushVaMatch_All     = 0,
            TlbFlushVaMatch_Section = 2,
            TlbFlushVaMatch_Group   = 3,
        };

        static constexpr ALWAYS_INLINE u32 EncodeTlbFlushValue(bool match_asid, u8 asid, KDeviceVirtualAddress address, TlbFlushVaMatch match) {
            return ((match_asid ? 1u : 0u) << 31) | ((asid & 0x7F) << 24) | (((address & 0xFFC00000u) >> DevicePageBits)) | (match);
        }

        void InvalidateTlb() {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(false, 0, 0, TlbFlushVaMatch_All));
        }

        void InvalidateTlb(u8 asid) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, 0, TlbFlushVaMatch_All));
        }

/*
        void InvalidateTlbSection(u8 asid, KDeviceVirtualAddress address) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, address, TlbFlushVaMatch_Section));
        }
*/

        void SetTable(u8 asid, KPhysicalAddress address) {
            /* Write the table address. */
            {
                KScopedLightLock lk(g_lock);

                WriteMcRegister(MC_SMMU_PTB_ASID, asid);
                WriteMcRegister(MC_SMMU_PTB_DATA, EntryBase::EncodePtbDataValue(address));

                SmmuSynchronizationBarrier();
            }

            /* Ensure consistency. */
            InvalidatePtc();
            InvalidateTlb(asid);
            SmmuSynchronizationBarrier();
        }

    }

    void KDevicePageTable::Initialize() {
        /* Set the memory controller register address. */
        g_memory_controller_address = KMemoryLayout::GetMemoryControllerRegion().GetAddress();

        /* Allocate a page to use as a reserved/no device table. */
        const KVirtualAddress table_virt_addr = Kernel::GetPageTableManager().Allocate();
        MESOSPHERE_ABORT_UNLESS(table_virt_addr != Null<KVirtualAddress>);
        const KPhysicalAddress table_phys_addr = GetPageTablePhysicalAddress(table_virt_addr);
        MESOSPHERE_ASSERT(IsValidPhysicalAddress(table_phys_addr));
        Kernel::GetPageTableManager().Open(table_virt_addr, 1);

        /* Clear the page and save it. */
        /* NOTE: Nintendo does not check the result of StoreDataCache. */
        cpu::ClearPageToZero(GetVoidPointer(table_virt_addr));
        cpu::StoreDataCache(GetVoidPointer(table_virt_addr), PageDirectorySize);
        g_reserved_table_phys_addr = table_phys_addr;

        /* Reserve an asid to correspond to no device. */
        MESOSPHERE_R_ABORT_UNLESS(g_asid_manager.Reserve(std::addressof(g_reserved_asid), 1));

        /* Set all asids to the reserved table. */
        static_assert(AsidCount <= std::numeric_limits<u8>::max());
        for (size_t i = 0; i < AsidCount; i++) {
            SetTable(static_cast<u8>(i), g_reserved_table_phys_addr);
        }

        /* Set all devices to the reserved asid. */
        for (size_t i = 0; i < ams::svc::DeviceName_Count; i++) {
            u32 value = 0x80000000u;
            if (IsHsSupported(static_cast<ams::svc::DeviceName>(i))) {
                for (size_t t = 0; t < TableCount; t++) {
                    value |= (g_reserved_asid << (BITSIZEOF(u8) * t));
                }
            } else {
                value |= g_reserved_asid;
            }

            WriteMcRegister(GetDeviceAsidRegisterOffset(static_cast<ams::svc::DeviceName>(i)), value);
            SmmuSynchronizationBarrier();
        }

        /* Ensure consistency. */
        InvalidatePtc();
        InvalidateTlb();
        SmmuSynchronizationBarrier();

        /* Clear int status. */
        WriteMcRegister(MC_INTSTATUS, ReadMcRegister(MC_INTSTATUS));

        /* Enable the SMMU */
        WriteMcRegister(MC_SMMU_CONFIG, 1);
        SmmuSynchronizationBarrier();

        /* TODO: Install interrupt handler. */
    }

    /* Member functions. */

    Result KDevicePageTable::Initialize(u64 space_address, u64 space_size) {
        /* Ensure space is valid. */
        R_UNLESS(((space_address + space_size - 1) & ~DeviceVirtualAddressMask) == 0, svc::ResultInvalidMemoryRegion());

        /* Determine extents. */
        const size_t start_index = space_address / DeviceRegionSize;
        const size_t end_index   = (space_address + space_size - 1) / DeviceRegionSize;

        /* Get the page table manager. */
        auto &ptm = Kernel::GetPageTableManager();

        /* Clear the tables. */
        static_assert(TableCount == (1ul << DeviceVirtualAddressBits) / DeviceRegionSize);
        for (size_t i = 0; i < TableCount; ++i) {
            this->tables[i] = Null<KVirtualAddress>;
        }

        /* Ensure that we clean up the tables on failure. */
        auto table_guard = SCOPE_GUARD {
            for (size_t i = start_index; i <= end_index; ++i) {
                if (this->tables[i] != Null<KVirtualAddress> && ptm.Close(this->tables[i], 1)) {
                    ptm.Free(this->tables[i]);
                }
            }
        };

        /* Allocate a table for all required indices. */
        for (size_t i = start_index; i <= end_index; ++i) {
            const KVirtualAddress table_vaddr = ptm.Allocate();
            R_UNLESS(table_vaddr != Null<KVirtualAddress>, svc::ResultOutOfMemory());

            MESOSPHERE_ASSERT((static_cast<u64>(GetInteger(GetPageTablePhysicalAddress(table_vaddr))) & ~PhysicalAddressMask) == 0);

            ptm.Open(table_vaddr, 1);
            cpu::StoreDataCache(GetVoidPointer(table_vaddr), PageDirectorySize);
            this->tables[i] = table_vaddr;
        }

        /* Clear asids. */
        for (size_t i = 0; i < TableCount; ++i) {
            this->table_asids[i] = g_reserved_asid;
        }

        /* Reserve asids for the tables. */
        R_TRY(g_asid_manager.Reserve(std::addressof(this->table_asids[start_index]), end_index - start_index + 1));

        /* Associate tables with asids. */
        for (size_t i = start_index; i <= end_index; ++i) {
            SetTable(this->table_asids[i], GetPageTablePhysicalAddress(this->tables[i]));
        }

        /* Set member variables. */
        this->attached_device = 0;
        this->attached_value  = (1u << 31) | this->table_asids[0];
        this->detached_value  = (1u << 31) | g_reserved_asid;

        this->hs_attached_value = (1u << 31);
        this->hs_detached_value = (1u << 31);
        for (size_t i = 0; i < TableCount; ++i) {
            this->hs_attached_value |= (this->table_asids[i] << (i * BITSIZEOF(u8)));
            this->hs_detached_value |= (g_reserved_asid      << (i * BITSIZEOF(u8)));
        }

        /* We succeeded. */
        table_guard.Cancel();
        return ResultSuccess();
    }

    void KDevicePageTable::Finalize() {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KDevicePageTable::Attach(ams::svc::DeviceName device_name, u64 space_address, u64 space_size) {
        /* Validate the device name. */
        R_UNLESS(0 <= device_name,                         svc::ResultNotFound());
        R_UNLESS(device_name < ams::svc::DeviceName_Count, svc::ResultNotFound());

        /* Check that the device isn't already attached. */
        R_UNLESS((this->attached_device & (1ul << device_name)) == 0, svc::ResultBusy());

        /* Validate that the space is allowed for the device. */
        const size_t end_index = (space_address + space_size - 1) / DeviceRegionSize;
        R_UNLESS(end_index == 0 || IsHsSupported(device_name), svc::ResultInvalidCombination());

        /* Validate that the device can be attached. */
        R_UNLESS(IsAttachable(device_name, space_address, space_size), svc::ResultInvalidCombination());

        /* Get the device asid register offset. */
        const int reg_offset = GetDeviceAsidRegisterOffset(device_name);
        R_UNLESS(reg_offset >= 0, svc::ResultNotFound());

        /* Determine the old/new values. */
        const u32 old_val = IsHsSupported(device_name) ? this->hs_detached_value : this->detached_value;
        const u32 new_val = IsHsSupported(device_name) ? this->hs_attached_value : this->attached_value;

        /* Attach the device. */
        {
            KScopedLightLock lk(g_lock);

            /* Validate that the device is unclaimed. */
            R_UNLESS((ReadMcRegister(reg_offset) | (1u << 31)) == (old_val | (1u << 31)), svc::ResultBusy());

            /* Claim the device. */
            WriteMcRegister(reg_offset, new_val);
            SmmuSynchronizationBarrier();

            /* Ensure that we claimed it successfully. */
            if (ReadMcRegister(reg_offset) != new_val) {
                WriteMcRegister(reg_offset, old_val);
                SmmuSynchronizationBarrier();
                return svc::ResultNotFound();
            }
        }

        /* Mark the device as attached. */
        this->attached_device |= (1ul << device_name);

        return ResultSuccess();
    }

    Result KDevicePageTable::Detach(ams::svc::DeviceName device_name) {
        MESOSPHERE_UNIMPLEMENTED();
    }

}

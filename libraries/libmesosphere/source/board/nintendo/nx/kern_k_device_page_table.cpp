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

#if defined(MESOSPHERE_BUILD_FOR_DEBUGGING) || defined(MESOSPHERE_BUILD_FOR_AUDITING)
#define MESOSPHERE_ENABLE_MEMORY_CONTROLLER_INTERRUPT
#endif

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

        constexpr size_t DevicePageBits = 12;
        constexpr size_t DevicePageSize = (1ul << DevicePageBits);
        static_assert(DevicePageSize == PageSize);

        constexpr size_t DeviceLargePageBits = 22;
        constexpr size_t DeviceLargePageSize = (1ul << DeviceLargePageBits);
        static_assert(DeviceLargePageSize % DevicePageSize == 0);

        constexpr size_t DeviceRegionBits = 32;
        constexpr size_t DeviceRegionSize = (1ul << DeviceRegionBits);
        static_assert(DeviceRegionSize % DeviceLargePageSize == 0);

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
                u32 m_value;
            protected:
                constexpr ALWAYS_INLINE u32 SelectBit(Bit n) const {
                    return (m_value & (1u << n));
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
                    m_value = v;
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

                constexpr ALWAYS_INLINE KPhysicalAddress GetPhysicalAddress() const { return (static_cast<u64>(m_value) << DevicePageBits) & PhysicalAddressMask; }

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
                WordType m_state[NumWords];
                KLightLock m_lock;
            private:
                constexpr void ReserveImpl(u8 asid) {
                    m_state[asid / BitsPerWord] |= (1u << (asid % BitsPerWord));
                }

                constexpr void ReleaseImpl(u8 asid) {
                    m_state[asid / BitsPerWord] &= ~(1u << (asid % BitsPerWord));
                }

                static constexpr ALWAYS_INLINE WordType ClearLeadingZero(WordType value) {
                    return __builtin_clzll(value) - (BITSIZEOF(unsigned long long) - BITSIZEOF(WordType));
                }
            public:
                constexpr KDeviceAsidManager() : m_state(), m_lock() {
                    for (size_t i = 0; i < NumReservedAsids; i++) {
                        this->ReserveImpl(ReservedAsids[i]);
                    }
                }

                Result Reserve(u8 *out, size_t num_desired) {
                    KScopedLightLock lk(m_lock);
                    MESOSPHERE_ASSERT(num_desired > 0);

                    size_t num_reserved = 0;
                    for (size_t i = 0; i < NumWords; i++) {
                        while (m_state[i] != FullWord) {
                            const WordType clear_bit = (m_state[i] + 1) ^ (m_state[i]);
                            m_state[i] |= clear_bit;
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
                    KScopedLightLock lk(m_lock);
                    this->ReleaseImpl(asid);
                }
        };

        /* Globals. */
        constinit KLightLock g_lock;
        constinit u8 g_reserved_asid;
        constinit KPhysicalAddress   g_memory_controller_address;
        constinit KPhysicalAddress   g_reserved_table_phys_addr;
        constinit KDeviceAsidManager g_asid_manager;
        constinit u32 g_saved_page_tables[AsidCount];
        constinit u32 g_saved_asid_registers[ams::svc::DeviceName_Count];

        /* Memory controller access functionality. */
        void WriteMcRegister(size_t offset, u32 value) {
            KSystemControl::WriteRegisterPrivileged(GetInteger(g_memory_controller_address) + offset, value);
        }

        u32 ReadMcRegister(size_t offset) {
            return KSystemControl::ReadRegisterPrivileged(GetInteger(g_memory_controller_address) + offset);
        }

        /* Memory controller interrupt functionality. */

        constexpr const char * const MemoryControllerClientNames[138] = {
            [  0] = "csr_ptcr (ptc)",
            [  1] = "csr_display0a (dc)",
            [  2] = "csr_display0ab (dcb)",
            [  3] = "csr_display0b (dc)",
            [  4] = "csr_display0bb (dcb)",
            [  5] = "csr_display0c (dc)",
            [  6] = "csr_display0cb (dcb)",
            [  7] = "Unknown Client",
            [  8] = "Unknown Client",
            [  9] = "Unknown Client",
            [ 10] = "Unknown Client",
            [ 11] = "Unknown Client",
            [ 12] = "Unknown Client",
            [ 13] = "Unknown Client",
            [ 14] = "csr_afir (afi)",
            [ 15] = "csr_avpcarm7r (avpc)",
            [ 16] = "csr_displayhc (dc)",
            [ 17] = "csr_displayhcb (dcb)",
            [ 18] = "Unknown Client",
            [ 19] = "Unknown Client",
            [ 20] = "Unknown Client",
            [ 21] = "csr_hdar (hda)",
            [ 22] = "csr_host1xdmar (hc)",
            [ 23] = "csr_host1xr (hc)",
            [ 24] = "Unknown Client",
            [ 25] = "Unknown Client",
            [ 26] = "Unknown Client",
            [ 27] = "Unknown Client",
            [ 28] = "csr_nvencsrd (nvenc)",
            [ 29] = "csr_ppcsahbdmar (ppcs)",
            [ 30] = "csr_ppcsahbslvr (ppcs)",
            [ 31] = "csr_satar (sata)",
            [ 32] = "Unknown Client",
            [ 33] = "Unknown Client",
            [ 34] = "Unknown Client",
            [ 35] = "Unknown Client",
            [ 36] = "Unknown Client",
            [ 37] = "Unknown Client",
            [ 38] = "Unknown Client",
            [ 39] = "csr_mpcorer (cpu)",
            [ 40] = "Unknown Client",
            [ 41] = "Unknown Client",
            [ 42] = "Unknown Client",
            [ 43] = "csw_nvencswr (nvenc)",
            [ 44] = "Unknown Client",
            [ 45] = "Unknown Client",
            [ 46] = "Unknown Client",
            [ 47] = "Unknown Client",
            [ 48] = "Unknown Client",
            [ 49] = "csw_afiw (afi)",
            [ 50] = "csw_avpcarm7w (avpc)",
            [ 51] = "Unknown Client",
            [ 52] = "Unknown Client",
            [ 53] = "csw_hdaw (hda)",
            [ 54] = "csw_host1xw (hc)",
            [ 55] = "Unknown Client",
            [ 56] = "Unknown Client",
            [ 57] = "csw_mpcorew (cpu)",
            [ 58] = "Unknown Client",
            [ 59] = "csw_ppcsahbdmaw (ppcs)",
            [ 60] = "csw_ppcsahbslvw (ppcs)",
            [ 61] = "csw_sataw (sata)",
            [ 62] = "Unknown Client",
            [ 63] = "Unknown Client",
            [ 64] = "Unknown Client",
            [ 65] = "Unknown Client",
            [ 66] = "Unknown Client",
            [ 67] = "Unknown Client",
            [ 68] = "csr_ispra (isp2)",
            [ 69] = "Unknown Client",
            [ 70] = "csw_ispwa (isp2)",
            [ 71] = "csw_ispwb (isp2)",
            [ 72] = "Unknown Client",
            [ 73] = "Unknown Client",
            [ 74] = "csr_xusb_hostr (xusb_host)",
            [ 75] = "csw_xusb_hostw (xusb_host)",
            [ 76] = "csr_xusb_devr (xusb_dev)",
            [ 77] = "csw_xusb_devw (xusb_dev)",
            [ 78] = "csr_isprab (isp2b)",
            [ 79] = "Unknown Client",
            [ 80] = "csw_ispwab (isp2b)",
            [ 81] = "csw_ispwbb (isp2b)",
            [ 82] = "Unknown Client",
            [ 83] = "Unknown Client",
            [ 84] = "csr_tsecsrd (tsec)",
            [ 85] = "csw_tsecswr (tsec)",
            [ 86] = "csr_a9avpscr (a9avp)",
            [ 87] = "csw_a9avpscw (a9avp)",
            [ 88] = "csr_gpusrd (gpu)",
            [ 89] = "csw_gpuswr (gpu)",
            [ 90] = "csr_displayt (dc)",
            [ 91] = "Unknown Client",
            [ 92] = "Unknown Client",
            [ 93] = "Unknown Client",
            [ 94] = "Unknown Client",
            [ 95] = "Unknown Client",
            [ 96] = "csr_sdmmcra (sdmmc1a)",
            [ 97] = "csr_sdmmcraa (sdmmc2a)",
            [ 98] = "csr_sdmmcr (sdmmc3a)",
            [ 99] = "csr_sdmmcrab (sdmmc4a)",
            [100] = "csw_sdmmcwa (sdmmc1a)",
            [101] = "csw_sdmmcwaa (sdmmc2a)",
            [102] = "csw_sdmmcw (sdmmc3a)",
            [103] = "csw_sdmmcwab (sdmmc4a)",
            [104] = "Unknown Client",
            [105] = "Unknown Client",
            [106] = "Unknown Client",
            [107] = "Unknown Client",
            [108] = "csr_vicsrd (vic)",
            [109] = "csw_vicswr (vic)",
            [110] = "Unknown Client",
            [111] = "Unknown Client",
            [112] = "Unknown Client",
            [113] = "Unknown Client",
            [114] = "csw_viw (vi)",
            [115] = "csr_displayd (dc)",
            [116] = "Unknown Client",
            [117] = "Unknown Client",
            [118] = "Unknown Client",
            [119] = "Unknown Client",
            [120] = "csr_nvdecsrd (nvdec)",
            [121] = "csw_nvdecswr (nvdec)",
            [122] = "csr_aper (ape)",
            [123] = "csw_apew (ape)",
            [124] = "Unknown Client",
            [125] = "Unknown Client",
            [126] = "csr_nvjpgsrd (nvjpg)",
            [127] = "csw_nvjpgswr (nvjpg)",
            [128] = "csr_sesrd (se)",
            [129] = "csw_seswr (se)",
            [130] = "csr_axiapr (axiap)",
            [131] = "csw_axiapw (axiap)",
            [132] = "csr_etrr (etr)",
            [133] = "csw_etrw (etr)",
            [134] = "csr_tsecsrdb (tsecb)",
            [135] = "csw_tsecswrb (tsecb)",
            [136] = "csr_gpusrd2 (gpu)",
            [137] = "csw_gpuswr2 (gpu)",
        };

        constexpr const char * GetMemoryControllerClientName(size_t i) {
            if (i < util::size(MemoryControllerClientNames)) {
                return MemoryControllerClientNames[i];
            }
            return "Unknown Client";
        }

        constexpr const char * const MemoryControllerErrorTypes[8] = {
            "RSVD",
            "Unknown",
            "DECERR_EMEM",
            "SECURITY_TRUSTZONE",
            "SECURITY_CARVEOUT",
            "Unknown",
            "INVALID_SMMU_PAGE",
            "Unknown",
        };

        class KMemoryControllerInterruptTask : public KInterruptTask {
            public:
                constexpr KMemoryControllerInterruptTask() : KInterruptTask() { /* ... */ }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);
                    return this;
                }

                virtual void DoTask() override {
                    #if defined(MESOSPHERE_ENABLE_MEMORY_CONTROLLER_INTERRUPT)
                    {
                        /* Clear the interrupt when we're done. */
                        ON_SCOPE_EXIT { Kernel::GetInterruptManager().ClearInterrupt(KInterruptName_MemoryController); };

                        /* Get and clear the interrupt status. */
                        u32 int_status, err_status, err_adr;
                        {
                            int_status = ReadMcRegister(MC_INTSTATUS);
                            err_status = ReadMcRegister(MC_ERR_STATUS);
                            err_adr    = ReadMcRegister(MC_ERR_ADR);

                            WriteMcRegister(MC_INTSTATUS, int_status);
                        }

                        /* Print the interrupt. */
                        {
                            constexpr auto GetBits = [] ALWAYS_INLINE_LAMBDA (u32 value, size_t ofs, size_t count) {
                                return (value >> ofs) & ((1u << count) - 1);
                            };

                            constexpr auto GetBit = [GetBits] ALWAYS_INLINE_LAMBDA (u32 value, size_t ofs) {
                                return (value >> ofs) & 1u;
                            };

                            MESOSPHERE_RELEASE_LOG("sMMU error interrupt\n");
                            MESOSPHERE_RELEASE_LOG("    MC_INTSTATUS=%08x\n", int_status);
                            MESOSPHERE_RELEASE_LOG("        DECERR_GENERALIZED_CARVEOUT=%d\n", GetBit(int_status, 17));
                            MESOSPHERE_RELEASE_LOG("        DECERR_MTS=%d\n",                  GetBit(int_status, 16));
                            MESOSPHERE_RELEASE_LOG("        SECERR_SEC=%d\n",                  GetBit(int_status, 13));
                            MESOSPHERE_RELEASE_LOG("        DECERR_VPR=%d\n",                  GetBit(int_status, 12));
                            MESOSPHERE_RELEASE_LOG("        INVALID_APB_ASID_UPDATE=%d\n",     GetBit(int_status, 11));
                            MESOSPHERE_RELEASE_LOG("        INVALID_SMMU_PAGE=%d\n",           GetBit(int_status, 10));
                            MESOSPHERE_RELEASE_LOG("        ARBITRATION_EMEM=%d\n",            GetBit(int_status,  9));
                            MESOSPHERE_RELEASE_LOG("        SECURITY_VIOLATION=%d\n",          GetBit(int_status,  8));
                            MESOSPHERE_RELEASE_LOG("        DECERR_EMEM=%d\n",                 GetBit(int_status,  6));
                            MESOSPHERE_RELEASE_LOG("    MC_ERRSTATUS=%08x\n", err_status);
                            MESOSPHERE_RELEASE_LOG("        ERR_TYPE=%d (%s)\n",                  GetBits(err_status, 28, 3), MemoryControllerErrorTypes[GetBits(err_status, 28, 3)]);
                            MESOSPHERE_RELEASE_LOG("        ERR_INVALID_SMMU_PAGE_READABLE=%d\n", GetBit (err_status, 27));
                            MESOSPHERE_RELEASE_LOG("        ERR_INVALID_SMMU_PAGE_WRITABLE=%d\n", GetBit (err_status, 26));
                            MESOSPHERE_RELEASE_LOG("        ERR_INVALID_SMMU_NONSECURE=%d\n",     GetBit (err_status, 25));
                            MESOSPHERE_RELEASE_LOG("        ERR_ADR_HI=%x\n",                     GetBits(err_status, 20, 2));
                            MESOSPHERE_RELEASE_LOG("        ERR_SWAP=%d\n",                       GetBit (err_status, 18));
                            MESOSPHERE_RELEASE_LOG("        ERR_SECURITY=%d %s\n",                GetBit (err_status, 17), GetBit(err_status, 17) ? "SECURE" : "NONSECURE");
                            MESOSPHERE_RELEASE_LOG("        ERR_RW=%d %s\n",                      GetBit (err_status, 16), GetBit(err_status, 16) ? "WRITE" : "READ");
                            MESOSPHERE_RELEASE_LOG("        ERR_ADR1=%x\n",                       GetBits(err_status, 12, 3));
                            MESOSPHERE_RELEASE_LOG("        ERR_ID=%d %s\n",                      GetBits(err_status,  0, 8), GetMemoryControllerClientName(GetBits(err_status,  0, 8)));
                            MESOSPHERE_RELEASE_LOG("    MC_ERRADR=%08x\n", err_adr);
                            MESOSPHERE_RELEASE_LOG("        ERR_ADR=%lx\n", (static_cast<u64>(GetBits(err_status, 20, 2)) << 32) | static_cast<u64>(err_adr));
                            MESOSPHERE_RELEASE_LOG("\n");
                        }
                    }
                    #endif
                }
        };

        /* Interrupt task global. */
        constinit KMemoryControllerInterruptTask g_mc_interrupt_task;

        /* Memory controller utilities. */
        ALWAYS_INLINE void SmmuSynchronizationBarrier() {
            ReadMcRegister(MC_SMMU_CONFIG);
        }

        ALWAYS_INLINE void InvalidatePtc() {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, 0);
        }

        ALWAYS_INLINE void InvalidatePtc(KPhysicalAddress address) {
            WriteMcRegister(MC_SMMU_PTC_FLUSH_1, (static_cast<u64>(GetInteger(address)) >> 32));
            WriteMcRegister(MC_SMMU_PTC_FLUSH_0, (GetInteger(address) & 0xFFFFFFF0u) | 1u);
        }

        enum TlbFlushVaMatch : u32 {
            TlbFlushVaMatch_All     = 0,
            TlbFlushVaMatch_Section = 2,
            TlbFlushVaMatch_Group   = 3,
        };

        static constexpr ALWAYS_INLINE u32 EncodeTlbFlushValue(bool match_asid, u8 asid, KDeviceVirtualAddress address, TlbFlushVaMatch match) {
            return ((match_asid ? 1u : 0u) << 31) | ((asid & 0x7F) << 24) | (((address & 0xFFC00000u) >> DevicePageBits)) | (match);
        }

        ALWAYS_INLINE void InvalidateTlb() {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(false, 0, 0, TlbFlushVaMatch_All));
        }

        ALWAYS_INLINE void InvalidateTlb(u8 asid) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, 0, TlbFlushVaMatch_All));
        }

        ALWAYS_INLINE void InvalidateTlbSection(u8 asid, KDeviceVirtualAddress address) {
            return WriteMcRegister(MC_SMMU_TLB_FLUSH, EncodeTlbFlushValue(true, asid, address, TlbFlushVaMatch_Section));
        }

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
        g_memory_controller_address = KMemoryLayout::GetDevicePhysicalAddress(KMemoryRegionType_MemoryController);

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

        /* If we're setting an interrupt handler, unmask all interrupts. */
        #if defined(MESOSPHERE_ENABLE_MEMORY_CONTROLLER_INTERRUPT)
        {
            WriteMcRegister(MC_INTMASK, 0x33D40);
        }
        #endif

        /* Enable the SMMU */
        WriteMcRegister(MC_SMMU_CONFIG, 1);
        SmmuSynchronizationBarrier();

        /* Install interrupt handler. */
        #if defined(MESOSPHERE_ENABLE_MEMORY_CONTROLLER_INTERRUPT)
        {
            Kernel::GetInterruptManager().BindHandler(std::addressof(g_mc_interrupt_task), KInterruptName_MemoryController, GetCurrentCoreId(), KInterruptController::PriorityLevel_High, true, true);
        }
        #endif
    }

    void KDevicePageTable::Lock() {
        g_lock.Lock();
    }

    void KDevicePageTable::Unlock() {
        g_lock.Unlock();
    }

    void KDevicePageTable::Sleep() {
        /* Save all page tables. */
        for (size_t i = 0; i < AsidCount; ++i) {
            WriteMcRegister(MC_SMMU_PTB_ASID, i);
            SmmuSynchronizationBarrier();
            g_saved_page_tables[i] = ReadMcRegister(MC_SMMU_PTB_DATA);
        }

        /* Save all asid registers. */
        for (size_t i = 0; i < ams::svc::DeviceName_Count; ++i) {
            g_saved_asid_registers[i] = ReadMcRegister(GetDeviceAsidRegisterOffset(static_cast<ams::svc::DeviceName>(i)));
        }
    }

    void KDevicePageTable::Wakeup() {
        /* Synchronize. */
        InvalidatePtc();
        InvalidateTlb();
        SmmuSynchronizationBarrier();

        /* Disable the SMMU */
        WriteMcRegister(MC_SMMU_CONFIG, 0);

        /* Restore the page tables. */
        for (size_t i = 0; i < AsidCount; ++i) {
            WriteMcRegister(MC_SMMU_PTB_ASID, i);
            SmmuSynchronizationBarrier();
            WriteMcRegister(MC_SMMU_PTB_DATA, g_saved_page_tables[i]);
        }
        SmmuSynchronizationBarrier();

        /* Restore the asid registers. */
        for (size_t i = 0; i < ams::svc::DeviceName_Count; ++i) {
            WriteMcRegister(GetDeviceAsidRegisterOffset(static_cast<ams::svc::DeviceName>(i)), g_saved_asid_registers[i]);
            SmmuSynchronizationBarrier();
        }

        /* Synchronize. */
        InvalidatePtc();
        InvalidateTlb();
        SmmuSynchronizationBarrier();

        /* Enable the SMMU */
        WriteMcRegister(MC_SMMU_CONFIG, 1);
        SmmuSynchronizationBarrier();
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
            m_tables[i] = Null<KVirtualAddress>;
        }

        /* Ensure that we clean up the tables on failure. */
        auto table_guard = SCOPE_GUARD {
            for (size_t i = start_index; i <= end_index; ++i) {
                if (m_tables[i] != Null<KVirtualAddress> && ptm.Close(m_tables[i], 1)) {
                    ptm.Free(m_tables[i]);
                }
            }
        };

        /* Allocate a table for all required indices. */
        for (size_t i = start_index; i <= end_index; ++i) {
            const KVirtualAddress table_vaddr = ptm.Allocate();
            R_UNLESS(table_vaddr != Null<KVirtualAddress>, svc::ResultOutOfMemory());

            MESOSPHERE_ASSERT(IsValidPhysicalAddress(GetPageTablePhysicalAddress(table_vaddr)));

            ptm.Open(table_vaddr, 1);
            cpu::StoreDataCache(GetVoidPointer(table_vaddr), PageDirectorySize);
            m_tables[i] = table_vaddr;
        }

        /* Clear asids. */
        for (size_t i = 0; i < TableCount; ++i) {
            m_table_asids[i] = g_reserved_asid;
        }

        /* Reserve asids for the tables. */
        R_TRY(g_asid_manager.Reserve(std::addressof(m_table_asids[start_index]), end_index - start_index + 1));

        /* Associate tables with asids. */
        for (size_t i = start_index; i <= end_index; ++i) {
            SetTable(m_table_asids[i], GetPageTablePhysicalAddress(m_tables[i]));
        }

        /* Set member variables. */
        m_attached_device = 0;
        m_attached_value  = (1u << 31) | m_table_asids[0];
        m_detached_value  = (1u << 31) | g_reserved_asid;

        m_hs_attached_value = (1u << 31);
        m_hs_detached_value = (1u << 31);
        for (size_t i = 0; i < TableCount; ++i) {
            m_hs_attached_value |= (m_table_asids[i] << (i * BITSIZEOF(u8)));
            m_hs_detached_value |= (g_reserved_asid      << (i * BITSIZEOF(u8)));
        }

        /* We succeeded. */
        table_guard.Cancel();
        return ResultSuccess();
    }

    void KDevicePageTable::Finalize() {
        /* Get the page table manager. */
        auto &ptm = Kernel::GetPageTableManager();

        /* Detach from all devices. */
        {
            KScopedLightLock lk(g_lock);
            for (size_t i = 0; i < ams::svc::DeviceName_Count; ++i) {
                const auto device_name = static_cast<ams::svc::DeviceName>(i);
                if ((m_attached_device & (1ul << device_name)) != 0) {
                    WriteMcRegister(GetDeviceAsidRegisterOffset(device_name), IsHsSupported(device_name) ? m_hs_detached_value : m_detached_value);
                    SmmuSynchronizationBarrier();
                }
            }
        }

        /* Forcibly unmap all pages. */
        this->UnmapImpl(0, (1ul << DeviceVirtualAddressBits), true);

        /* Release all asids. */
        for (size_t i = 0; i < TableCount; ++i) {
            if (m_table_asids[i] != g_reserved_asid) {
                /* Set the table to the reserved table. */
                SetTable(m_table_asids[i], g_reserved_table_phys_addr);

                /* Close the table. */
                const KVirtualAddress table_vaddr = m_tables[i];
                MESOSPHERE_ASSERT(ptm.GetRefCount(table_vaddr) == 1);
                MESOSPHERE_ABORT_UNLESS(ptm.Close(table_vaddr, 1));

                /* Free the table. */
                ptm.Free(table_vaddr);

                /* Release the asid. */
                g_asid_manager.Release(m_table_asids[i]);
            }
        }
    }

    Result KDevicePageTable::Attach(ams::svc::DeviceName device_name, u64 space_address, u64 space_size) {
        /* Validate the device name. */
        R_UNLESS(0 <= device_name,                         svc::ResultNotFound());
        R_UNLESS(device_name < ams::svc::DeviceName_Count, svc::ResultNotFound());

        /* Check that the device isn't already attached. */
        R_UNLESS((m_attached_device & (1ul << device_name)) == 0, svc::ResultBusy());

        /* Validate that the space is allowed for the device. */
        const size_t end_index = (space_address + space_size - 1) / DeviceRegionSize;
        R_UNLESS(end_index == 0 || IsHsSupported(device_name), svc::ResultInvalidCombination());

        /* Validate that the device can be attached. */
        R_UNLESS(IsAttachable(device_name, space_address, space_size), svc::ResultInvalidCombination());

        /* Get the device asid register offset. */
        const int reg_offset = GetDeviceAsidRegisterOffset(device_name);
        R_UNLESS(reg_offset >= 0, svc::ResultNotFound());

        /* Determine the old/new values. */
        const u32 old_val = IsHsSupported(device_name) ? m_hs_detached_value : m_detached_value;
        const u32 new_val = IsHsSupported(device_name) ? m_hs_attached_value : m_attached_value;

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
        m_attached_device |= (1ul << device_name);

        return ResultSuccess();
    }

    Result KDevicePageTable::Detach(ams::svc::DeviceName device_name) {
        /* Validate the device name. */
        R_UNLESS(0 <= device_name,                         svc::ResultNotFound());
        R_UNLESS(device_name < ams::svc::DeviceName_Count, svc::ResultNotFound());

        /* Check that the device is already attached. */
        R_UNLESS((m_attached_device & (1ul << device_name)) != 0, svc::ResultInvalidState());

        /* Get the device asid register offset. */
        const int reg_offset = GetDeviceAsidRegisterOffset(device_name);
        R_UNLESS(reg_offset >= 0, svc::ResultNotFound());

        /* Determine the old/new values. */
        const u32 old_val = IsHsSupported(device_name) ? m_hs_attached_value : m_attached_value;
        const u32 new_val = IsHsSupported(device_name) ? m_hs_detached_value : m_detached_value;

        /* When not building for debug, the old value might be unused. */
        AMS_UNUSED(old_val);

        /* Detach the device. */
        {
            KScopedLightLock lk(g_lock);

            /* Check that the device is attached. */
            MESOSPHERE_ASSERT(ReadMcRegister(reg_offset) == old_val);

            /* Release the device. */
            WriteMcRegister(reg_offset, new_val);
            SmmuSynchronizationBarrier();

            /* Check that the device was released. */
            MESOSPHERE_ASSERT((ReadMcRegister(reg_offset) | (1u << 31)) == (new_val | 1u << 31));
        }

        /* Mark the device as detached. */
        m_attached_device &= ~(1ul << device_name);

        return ResultSuccess();
    }

    bool KDevicePageTable::IsFree(KDeviceVirtualAddress address, u64 size) const {
        MESOSPHERE_ASSERT((address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Walk the directory, looking for entries. */
        u64 remaining = size;
        while (remaining > 0) {
            const size_t l0_index = (address / DeviceRegionSize);
            const size_t l1_index = (address % DeviceRegionSize)    / DeviceLargePageSize;
            const size_t l2_index = (address % DeviceLargePageSize) / DevicePageSize;

            const PageDirectoryEntry *l1 = GetPointer<PageDirectoryEntry>(m_tables[l0_index]);
            if (l1 == nullptr || !l1[l1_index].IsValid()) {
                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                address   += DevicePageSize * map_count;
                remaining -= DevicePageSize * map_count;
            } else if (l1[l1_index].IsTable()) {
                const PageTableEntry *l2 = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(l1[l1_index].GetPhysicalAddress()));

                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                for (size_t i = 0; i < map_count; ++i) {
                    if (l2[l2_index + i].IsValid()) {
                        return false;
                    }
                }

                address   += DevicePageSize * map_count;
                remaining -= DevicePageSize * map_count;
            } else {
                /* If we have an entry, we're not free. */
                return false;
            }
        }

        return true;
    }

    Result KDevicePageTable::MapDevicePage(size_t *out_mapped_size, s32 &num_pt, s32 max_pt, KPhysicalAddress phys_addr, u64 size, KDeviceVirtualAddress address, ams::svc::MemoryPermission device_perm) {
        /* Clear the output size. */
        *out_mapped_size = 0;

        /* Ensure that the physical address is valid. */
        R_UNLESS(IsValidPhysicalAddress(static_cast<u64>(GetInteger(phys_addr)) + size - 1), svc::ResultInvalidCurrentMemory());
        MESOSPHERE_ASSERT((address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Get the memory manager and page table manager. */
        KMemoryManager &mm     = Kernel::GetMemoryManager();
        KPageTableManager &ptm = Kernel::GetPageTableManager();

        /* Cache permissions. */
        const bool read  = (device_perm & ams::svc::MemoryPermission_Read)  != 0;
        const bool write = (device_perm & ams::svc::MemoryPermission_Write) != 0;

        /* Walk the directory. */
        u64 remaining = size;
        while (remaining > 0) {
            const size_t l0_index = (address / DeviceRegionSize);
            const size_t l1_index = (address % DeviceRegionSize)    / DeviceLargePageSize;
            const size_t l2_index = (address % DeviceLargePageSize) / DevicePageSize;

            /* Get and validate l1. */
            PageDirectoryEntry *l1 = GetPointer<PageDirectoryEntry>(m_tables[l0_index]);
            MESOSPHERE_ASSERT(l1 != nullptr);

            /* Setup an l1 table/entry, if needed. */
            if (!l1[l1_index].IsTable()) {
                /* Check that an entry doesn't already exist. */
                MESOSPHERE_ASSERT(!l1[l1_index].IsValid());

                /* If we can make an l1 entry, do so. */
                if (l2_index == 0 && util::IsAligned(GetInteger(phys_addr), DeviceLargePageSize) && remaining >= DeviceLargePageSize) {
                    /* Set the large page. */
                    l1[l1_index].SetLargePage(read, write, true, phys_addr);
                    cpu::StoreDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                    /* Synchronize. */
                    InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l1[l1_index]))));
                    InvalidateTlbSection(m_table_asids[l0_index], address);
                    SmmuSynchronizationBarrier();

                    /* Open references to the pages. */
                    mm.Open(GetHeapVirtualAddress(phys_addr), DeviceLargePageSize / PageSize);

                    /* Advance. */
                    phys_addr        += DeviceLargePageSize;
                    address          += DeviceLargePageSize;
                    *out_mapped_size += DeviceLargePageSize;
                    remaining        -= DeviceLargePageSize;
                    continue;
                } else if (num_pt == max_pt) {
                    break;
                } else {
                    /* Make an l1 table. */
                    const KVirtualAddress table_vaddr = ptm.Allocate();
                    R_UNLESS(table_vaddr != Null<KVirtualAddress>, svc::ResultOutOfMemory());
                    MESOSPHERE_ASSERT(IsValidPhysicalAddress(GetPageTablePhysicalAddress(table_vaddr)));
                    cpu::StoreDataCache(GetVoidPointer(table_vaddr), PageTableSize);

                    /* Set the l1 table. */
                    l1[l1_index].SetTable(true, true, true, GetPageTablePhysicalAddress(table_vaddr));
                    cpu::StoreDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                    /* Synchronize. */
                    InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l1[l1_index]))));
                    InvalidateTlbSection(m_table_asids[l0_index], address);
                    SmmuSynchronizationBarrier();

                    /* Increment the page table count. */
                    ++num_pt;
                }
            }

            /* If we get to this point, l1 must be a table. */
            MESOSPHERE_ASSERT(l1[l1_index].IsTable());

            /* Map l2 entries. */
            {
                PageTableEntry *l2 = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(l1[l1_index].GetPhysicalAddress()));

                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                /* Set the entries. */
                for (size_t i = 0; i < map_count; ++i) {
                    MESOSPHERE_ASSERT(!l2[l2_index + i].IsValid());
                    l2[l2_index + i].SetPage(read, write, true, phys_addr + DevicePageSize * i);

                    /* Add a reference to the l2 page (from the l2 entry page). */
                    ptm.Open(KVirtualAddress(l2), 1);
                }
                cpu::StoreDataCache(std::addressof(l2[l2_index]), map_count * sizeof(PageTableEntry));

                /* Invalidate the page table cache. */
                for (size_t i = util::AlignDown(l2_index, 4); i <= util::AlignDown(l2_index + map_count - 1, 4); i += 4) {
                    InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l2[i]))));
                }

                /* Synchronize. */
                InvalidateTlbSection(m_table_asids[l0_index], address);
                SmmuSynchronizationBarrier();

                /* Open references to the pages. */
                mm.Open(GetHeapVirtualAddress(phys_addr), (map_count * DevicePageSize) / PageSize);

                /* Advance. */
                phys_addr        += map_count * DevicePageSize;
                address          += map_count * DevicePageSize;
                *out_mapped_size += map_count * DevicePageSize;
                remaining        -= map_count * DevicePageSize;
            }
        }

        return ResultSuccess();
    }

    Result KDevicePageTable::MapImpl(size_t *out_mapped_size, s32 &num_pt, s32 max_pt, const KPageGroup &pg, KDeviceVirtualAddress device_address, ams::svc::MemoryPermission device_perm) {
        /* Clear the output size. */
        *out_mapped_size = 0;

        /* Get the size, and validate the address. */
        const u64 size = pg.GetNumPages() * PageSize;
        MESOSPHERE_ASSERT((device_address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((device_address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Ensure that the region we're mapping to is free. */
        R_UNLESS(this->IsFree(device_address, size), svc::ResultInvalidCurrentMemory());

        /* Ensure that if we fail, we unmap anything we mapped. */
        auto unmap_guard = SCOPE_GUARD { this->UnmapImpl(device_address, size, true); };

        /* Iterate, mapping device pages. */
        KDeviceVirtualAddress cur_addr = device_address;
        for (auto it = pg.begin(); it != pg.end(); ++it) {
            /* Require that we be able to map the device page. */
            R_UNLESS(IsHeapVirtualAddress(it->GetAddress()), svc::ResultInvalidCurrentMemory());

            /* Get the physical address for the page. */
            const KPhysicalAddress phys_addr = GetHeapPhysicalAddress(it->GetAddress());

            /* Map the device page. */
            const u64 block_size = it->GetSize();
            size_t mapped_size = 0;
            R_TRY(this->MapDevicePage(std::addressof(mapped_size), num_pt, max_pt, phys_addr, block_size, cur_addr, device_perm));

            /* Advance. */
            cur_addr         += block_size;
            *out_mapped_size += mapped_size;

            /* If we didn't map as much as we wanted, break. */
            if (mapped_size < block_size) {
                break;
            }
        }

        /* We're done, so cancel our guard. */
        unmap_guard.Cancel();

        return ResultSuccess();
    }

    void KDevicePageTable::UnmapImpl(KDeviceVirtualAddress address, u64 size, bool force) {
        MESOSPHERE_UNUSED(force);
        MESOSPHERE_ASSERT((address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Get the memory manager and page table manager. */
        KMemoryManager &mm     = Kernel::GetMemoryManager();
        KPageTableManager &ptm = Kernel::GetPageTableManager();

        /* Make a page group for the pages we're closing. */
        KPageGroup pg(std::addressof(Kernel::GetBlockInfoManager()));

        /* Walk the directory. */
        u64 remaining = size;
        while (remaining > 0) {
            const size_t l0_index = (address / DeviceRegionSize);
            const size_t l1_index = (address % DeviceRegionSize)    / DeviceLargePageSize;
            const size_t l2_index = (address % DeviceLargePageSize) / DevicePageSize;

            /* Get and validate l1. */
            PageDirectoryEntry *l1 = GetPointer<PageDirectoryEntry>(m_tables[l0_index]);

            /* Check if there's nothing mapped at l1. */
            if (l1 == nullptr || !l1[l1_index].IsValid()) {
                MESOSPHERE_ASSERT(force);

                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                /* Advance. */
                address   += map_count * DevicePageSize;
                remaining -= map_count * DevicePageSize;
            } else if (l1[l1_index].IsTable()) {
                /* Dealing with an l1 table. */
                PageTableEntry *l2 = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(l1[l1_index].GetPhysicalAddress()));

                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);
                size_t num_closed = 0;
                bool invalidated_tlb = false;

                for (size_t i = 0; i < map_count; ++i) {
                    if (l2[l2_index + i].IsValid()) {
                        /* Get the physical address. */
                        const KPhysicalAddress phys_addr = l2[l2_index + i].GetPhysicalAddress();
                        MESOSPHERE_ASSERT(IsHeapPhysicalAddress(phys_addr));

                        /* Invalidate the entry. */
                        l2[l2_index + i].Invalidate();
                        ++num_closed;

                        /* Try to add the page to the group. */
                        if (R_FAILED(pg.AddBlock(GetHeapVirtualAddress(phys_addr), DevicePageSize / PageSize))) {
                            /* If we can't add it for deferred close, close it now. */
                            cpu::StoreDataCache(std::addressof(l2[l2_index + i]), sizeof(PageTableEntry));
                            InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l2[l2_index + i]))));
                            SmmuSynchronizationBarrier();

                            /* Close the page's reference. */
                            mm.Close(GetHeapVirtualAddress(phys_addr), 1);
                        }
                    } else {
                        MESOSPHERE_ASSERT(force);
                    }
                }
                cpu::StoreDataCache(std::addressof(l2[l2_index]), map_count * sizeof(PageTableEntry));

                /* Invalidate the page table cache. */
                for (size_t i = util::AlignDown(l2_index, 4); i <= util::AlignDown(l2_index + map_count - 1, 4); i += 4) {
                    InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l2[i]))));
                }
                SmmuSynchronizationBarrier();

                /* Close the pages. */
                if (ptm.Close(KVirtualAddress(l2), num_closed)) {
                    /* Invalidate the l1 entry. */
                    l1[l1_index].Invalidate();
                    cpu::StoreDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                    /* Synchronize. */
                    InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l1[l1_index]))));
                    InvalidateTlbSection(m_table_asids[l0_index], address);
                    SmmuSynchronizationBarrier();

                    /* We invalidated the tlb. */
                    invalidated_tlb = true;

                    /* Free the l2 page. */
                    ptm.Free(KVirtualAddress(l2));
                }

                /* Invalidate the tlb if we haven't already. */
                if (!invalidated_tlb) {
                    InvalidateTlbSection(m_table_asids[l0_index], address);
                    SmmuSynchronizationBarrier();
                }

                /* Advance. */
                address   += map_count * DevicePageSize;
                remaining -= map_count * DevicePageSize;
            } else {
                /* Dealing with an l1 entry. */
                MESOSPHERE_ASSERT(l2_index == 0);

                /* Get the physical address. */
                const KPhysicalAddress phys_addr = l1[l1_index].GetPhysicalAddress();
                MESOSPHERE_ASSERT(IsHeapPhysicalAddress(phys_addr));

                /* Invalidate the entry. */
                l1[l1_index].Invalidate();
                cpu::StoreDataCache(std::addressof(l1[l1_index]), sizeof(PageDirectoryEntry));

                /* Synchronize. */
                InvalidatePtc(GetPageTablePhysicalAddress(KVirtualAddress(std::addressof(l1[l1_index]))));
                InvalidateTlbSection(m_table_asids[l0_index], address);
                SmmuSynchronizationBarrier();

                /* Close references. */
                mm.Close(GetHeapVirtualAddress(phys_addr), DeviceLargePageSize / PageSize);

                /* Advance. */
                address   += DeviceLargePageSize;
                remaining -= DeviceLargePageSize;
            }
        }

        /* Close references to the pages in the group. */
        pg.Close();
    }

    Result KDevicePageTable::MakePageGroup(KPageGroup *out, KDeviceVirtualAddress address, u64 size) const {
        MESOSPHERE_ASSERT((address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Walk the directory. */
        u64 remaining = size;
        bool first = true;
        u32 attr = 0;
        while (remaining > 0) {
            const size_t l0_index = (address / DeviceRegionSize);
            const size_t l1_index = (address % DeviceRegionSize)    / DeviceLargePageSize;
            const size_t l2_index = (address % DeviceLargePageSize) / DevicePageSize;

            /* Get and validate l1. */
            const PageDirectoryEntry *l1 = GetPointer<PageDirectoryEntry>(m_tables[l0_index]);
            R_UNLESS(l1 != nullptr,          svc::ResultInvalidCurrentMemory());
            R_UNLESS(l1[l1_index].IsValid(), svc::ResultInvalidCurrentMemory());

            if (l1[l1_index].IsTable()) {
                /* We're acting on an l2 entry. */
                const PageTableEntry *l2 = GetPointer<PageTableEntry>(GetPageTableVirtualAddress(l1[l1_index].GetPhysicalAddress()));

                const size_t remaining_in_entry = (PageTableSize / sizeof(PageTableEntry)) - l2_index;
                const size_t map_count = std::min<size_t>(remaining_in_entry, remaining / DevicePageSize);

                for (size_t i = 0; i < map_count; ++i) {
                    /* Ensure the l2 entry is valid. */
                    R_UNLESS(l2[l2_index + i].IsValid(), svc::ResultInvalidCurrentMemory());

                    /* Get the physical address. */
                    const KPhysicalAddress phys_addr = l2[l2_index + i].GetPhysicalAddress();
                    MESOSPHERE_ASSERT(IsHeapPhysicalAddress(phys_addr));

                    /* Add to the group. */
                    R_TRY(out->AddBlock(GetHeapVirtualAddress(phys_addr), DevicePageSize / PageSize));

                    /* If this is our first entry, get the attribute. */
                    if (first) {
                        attr = l2[l2_index + i].GetAttributes();
                        first = false;
                    } else {
                        /* Validate the attributes match the first entry. */
                        R_UNLESS(l2[l2_index + i].GetAttributes() == attr, svc::ResultInvalidCurrentMemory());
                    }
                }

                /* Advance. */
                address   += DevicePageSize * map_count;
                remaining -= DevicePageSize * map_count;
            } else {
                /* We're acting on an l1 entry. */
                R_UNLESS(l2_index == 0,                    svc::ResultInvalidCurrentMemory());
                R_UNLESS(remaining >= DeviceLargePageSize, svc::ResultInvalidCurrentMemory());

                /* Get the physical address. */
                const KPhysicalAddress phys_addr = l1[l1_index].GetPhysicalAddress();
                MESOSPHERE_ASSERT(IsHeapPhysicalAddress(phys_addr));

                /* Add to the group. */
                R_TRY(out->AddBlock(GetHeapVirtualAddress(phys_addr), DeviceLargePageSize / PageSize));

                /* If this is our first entry, get the attribute. */
                if (first) {
                    attr = l1[l1_index].GetAttributes();
                    first = false;
                } else {
                    /* Validate the attributes match the first entry. */
                    R_UNLESS(l1[l1_index].GetAttributes() == attr, svc::ResultInvalidCurrentMemory());
                }

                /* Advance. */
                address   += DeviceLargePageSize;
                remaining -= DeviceLargePageSize;
            }
        }

        return ResultSuccess();
    }

    bool KDevicePageTable::Compare(const KPageGroup &compare_pg, KDeviceVirtualAddress device_address) const {
        /* Check whether the page group we expect for the virtual address matches the page group we're validating. */
        KPageGroup calc_pg(std::addressof(Kernel::GetBlockInfoManager()));
        return (R_SUCCEEDED(this->MakePageGroup(std::addressof(calc_pg), device_address, compare_pg.GetNumPages() * PageSize))) &&
               calc_pg.IsEquivalentTo(compare_pg);
    }

    Result KDevicePageTable::Map(size_t *out_mapped_size, const KPageGroup &pg, KDeviceVirtualAddress device_address, ams::svc::MemoryPermission device_perm, bool refresh_mappings) {
        /* Clear the output size. */
        *out_mapped_size = 0;

        /* Map the pages. */
        s32 num_pt = 0;
        return this->MapImpl(out_mapped_size, num_pt, refresh_mappings ? 1 : std::numeric_limits<s32>::max(), pg, device_address, device_perm);
    }

    Result KDevicePageTable::Unmap(const KPageGroup &pg, KDeviceVirtualAddress device_address) {
        /* Validate address/size. */
        const size_t size = pg.GetNumPages() * PageSize;
        MESOSPHERE_ASSERT((device_address & ~DeviceVirtualAddressMask) == 0);
        MESOSPHERE_ASSERT(((device_address + size - 1) & ~DeviceVirtualAddressMask) == 0);

        /* Ensure the page group is correct. */
        R_UNLESS(this->Compare(pg, device_address), svc::ResultInvalidCurrentMemory());

        /* Unmap the pages. */
        this->UnmapImpl(device_address, size, false);

        return ResultSuccess();
    }

}

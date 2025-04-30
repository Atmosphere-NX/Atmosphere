/*
 * Copyright (c) Atmosphère-NX
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
#include "fuse_registers.hpp"

namespace ams::fuse {

    namespace {

        static constexpr SocType SocType_CommonInternal = static_cast<SocType>(-1);
        static_assert(SocType_CommonInternal != SocType_Erista);
        static_assert(SocType_CommonInternal != SocType_Mariko);

        constinit SocType g_soc_type = SocType_CommonInternal;

        struct BypassEntry {
            u32 offset;
            u32 value;
        };

        struct OdmWord2 {
            using DeviceUniqueKeyGeneration = util::BitPack32::Field<0,  5, int>;
            using Reserved                  = util::BitPack32::Field<5, 27, int>;
        };

        struct OdmWord4 {
            using HardwareState1                = util::BitPack32::Field<0,                                   2, int>;
            using HardwareType1                 = util::BitPack32::Field<HardwareState1::Next,                1, int>;
            using DramId1                       = util::BitPack32::Field<HardwareType1::Next,                 5, int>;
            using HardwareType2                 = util::BitPack32::Field<DramId1::Next,                       1, int>;
            using HardwareState2                = util::BitPack32::Field<HardwareType2::Next,                 1, int>;
            using RetailInteractiveDisplayState = util::BitPack32::Field<HardwareState2::Next,                1, int>;
            using FormatVersion                 = util::BitPack32::Field<RetailInteractiveDisplayState::Next, 1, int>;
            using DramId2                       = util::BitPack32::Field<FormatVersion::Next,                 3, int>;
            using Reserved                      = util::BitPack32::Field<DramId2::Next,                       1, int>;
            using HardwareType3                 = util::BitPack32::Field<Reserved::Next,                      4, int>;
        };

        struct OdmWord28 {
            using Regulator = util::BitPack32::Field<0,  1, int>;
            using Reserved  = util::BitPack32::Field<1, 31, int>;
        };

        constexpr ALWAYS_INLINE int GetHardwareStateValue(const util::BitPack32 odm_word4) {
            constexpr auto HardwareState1Shift = 0;
            constexpr auto HardwareState2Shift = OdmWord4::HardwareState1::Count + HardwareState1Shift;

            return (odm_word4.Get<OdmWord4::HardwareState1>() << HardwareState1Shift) |
                   (odm_word4.Get<OdmWord4::HardwareState2>() << HardwareState2Shift);
        }

        constexpr ALWAYS_INLINE int GetHardwareTypeValue(const util::BitPack32 odm_word4) {
            constexpr auto HardwareType1Shift = 0;
            constexpr auto HardwareType2Shift = OdmWord4::HardwareType1::Count + HardwareType1Shift;
            constexpr auto HardwareType3Shift = OdmWord4::HardwareType2::Count + HardwareType2Shift;

            return (odm_word4.Get<OdmWord4::HardwareType1>() << HardwareType1Shift) |
                   (odm_word4.Get<OdmWord4::HardwareType2>() << HardwareType2Shift) |
                   (odm_word4.Get<OdmWord4::HardwareType3>() << HardwareType3Shift);
        }

        constexpr ALWAYS_INLINE int GetDramIdValue(const util::BitPack32 odm_word4) {
            constexpr auto DramId1Shift = 0;
            constexpr auto DramId2Shift = OdmWord4::DramId1::Count + DramId1Shift;


            return (odm_word4.Get<OdmWord4::DramId1>() << DramId1Shift) |
                   (odm_word4.Get<OdmWord4::DramId2>() << DramId2Shift);
        }

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceFuses.GetAddress();

        constinit bool g_checked_for_rcm_bug_patch  = false;
        constinit bool g_has_rcm_bug_patch          = false;

        ALWAYS_INLINE volatile FuseRegisterRegion *GetRegisterRegion() {
            return reinterpret_cast<volatile FuseRegisterRegion *>(g_register_address);
        }

        ALWAYS_INLINE volatile FuseRegisters &GetRegisters() {
            return GetRegisterRegion()->fuse;
        }

        ALWAYS_INLINE volatile FuseChipRegistersCommon &GetChipRegistersCommon() {
            return GetRegisterRegion()->chip_common;
        }

        ALWAYS_INLINE volatile FuseChipRegistersErista &GetChipRegistersErista() {
            return GetRegisterRegion()->chip_erista;
        }

        ALWAYS_INLINE volatile FuseChipRegistersMariko &GetChipRegistersMariko() {
            return GetRegisterRegion()->chip_mariko;
        }

        bool IsIdle() {
            return reg::HasValue(GetRegisters().FUSE_FUSECTRL, FUSE_REG_BITS_ENUM(FUSECTRL_STATE, IDLE));
        }

        void WaitForIdle() {
            while (!IsIdle()) { /* ... */ }
        }

        u32 GetOdmWordImpl(int index, fuse::SocType soc_type) {
            if (index < 8) {
                volatile auto &chip = GetChipRegistersCommon();
                return chip.FUSE_RESERVED_ODM_0[index - 0];
            } else if (soc_type == SocType_Mariko) {
                volatile auto &chip = GetChipRegistersMariko();
                if (index < 22) {
                    return chip.FUSE_RESERVED_ODM_8[index - 8];
                } else if (index < 25) {
                    return chip.FUSE_RESERVED_ODM_22[index - 22];
                } else if (index < 26) {
                    return chip.FUSE_RESERVED_ODM_25[index - 25];
                } else if (index < 29) {
                    return chip.FUSE_RESERVED_ODM_26[index - 26];
                } else if (index < 30) {
                    return chip.FUSE_RESERVED_ODM_29[index - 29];
                }
            }
            AMS_ABORT("Invalid ODM fuse read");
        }

        u32 GetCommonOdmWord(int index) {
            return GetOdmWordImpl(index, SocType_CommonInternal);
        }

        bool IsNewFuseFormat() {
            /* On mariko, this should always be true. */
            if (GetSocType() != SocType_Erista) {
                return true;
            }

            /* Require that the format version be non-zero in odm4. */
            if (util::BitPack32{GetCommonOdmWord(4)}.Get<OdmWord4::FormatVersion>() == 0) {
                return false;
            }

            /* Check that odm word 0/1 are fused with the magic values. */
            constexpr u32 NewFuseFormatMagic0 = 0x8E61ECAE;
            constexpr u32 NewFuseFormatMagic1 = 0xF2BA3BB2;

            const u32 w0 = GetCommonOdmWord(0);
            const u32 w1 = GetCommonOdmWord(1);

            return w0 == NewFuseFormatMagic0 && w1 == NewFuseFormatMagic1;
        }

        constexpr u32 CompressLotCode(u32 lot0) {
            constexpr int Radix = 36;
            constexpr int Count = 5;
            constexpr int Width = 6;
            constexpr u32 Mask  = (1u << Width) - 1;

            u32 compressed = 0;

            for (int i = Count - 1; i >= 0; --i) {
                compressed *= Radix;
                compressed += (lot0 >> (i * Width)) & Mask;
            }

            return compressed;
        }

        constexpr const TargetFirmware FuseVersionIncrementFirmwares[] = {
            TargetFirmware_20_0_0,
            TargetFirmware_19_0_0,
            TargetFirmware_17_0_0,
            TargetFirmware_16_0_0,
            TargetFirmware_15_0_0,
            TargetFirmware_13_2_1,
            TargetFirmware_12_0_2,
            TargetFirmware_11_0_0,
            TargetFirmware_10_0_0,
            TargetFirmware_9_1_0,
            TargetFirmware_9_0_0,
            TargetFirmware_8_1_0,
            TargetFirmware_7_0_0,
            TargetFirmware_6_2_0,
            TargetFirmware_6_0_0,
            TargetFirmware_5_0_0,
            TargetFirmware_4_0_0,
            TargetFirmware_3_0_2,
            TargetFirmware_3_0_0,
            TargetFirmware_2_0_0,
            TargetFirmware_1_0_0,
        };

        constexpr inline int NumFuseIncrements = util::size(FuseVersionIncrementFirmwares);

        constexpr const BypassEntry FuseBypassEntries[] = {
            /* Don't configure any fuse bypass entries. */
        };

        constexpr inline int NumFuseBypassEntries = util::size(FuseBypassEntries);

        /* Verify that the fuse version increment list is sorted. */
        static_assert([] {
            for (size_t i = 0; i < util::size(FuseVersionIncrementFirmwares) - 1; ++i) {
                if (FuseVersionIncrementFirmwares[i] <= FuseVersionIncrementFirmwares[i + 1]) {
                    return false;
                }
            }
            return true;
        }());

        constexpr int GetExpectedFuseVersionImpl(TargetFirmware target_fw) {
            for (int i = 0; i < NumFuseIncrements; ++i) {
                if (target_fw >= FuseVersionIncrementFirmwares[i]) {
                    return NumFuseIncrements - i;
                }
            }
            return 0;
        }

        static_assert(GetExpectedFuseVersionImpl(TargetFirmware_11_0_0)          == 14);
        static_assert(GetExpectedFuseVersionImpl(TargetFirmware_1_0_0)           ==  1);
        static_assert(GetExpectedFuseVersionImpl(static_cast<TargetFirmware>(0)) ==  0);

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void SetWriteSecureOnly() {
        reg::Write(GetRegisters().FUSE_PRIVATEKEYDISABLE, FUSE_REG_BITS_ENUM(PRIVATEKEYDISABLE_TZ_STICKY_BIT_VAL, KEY_INVISIBLE));
    }

    void Lockout() {
        reg::Write(GetRegisters().FUSE_DISABLEREGPROGRAM, FUSE_REG_BITS_ENUM(DISABLEREGPROGRAM_VAL, ENABLE));
    }

    u32 ReadWord(int address) {
        /* Require that the fuse array be idle. */
        AMS_ABORT_UNLESS(IsIdle());

        /* Get the registers. */
        volatile auto &FUSE = GetRegisters();

        /* Write the address to read. */
        reg::Write(FUSE.FUSE_FUSEADDR, address);

        /* Set control to read. */
        reg::ReadWrite(FUSE.FUSE_FUSECTRL, FUSE_REG_BITS_ENUM(FUSECTRL_CMD, READ));

        /* Wait 1 us. */
        util::WaitMicroSeconds(1);

        /* Wait for the array to be idle. */
        WaitForIdle();

        return reg::Read(FUSE.FUSE_FUSERDATA);
    }

    u32 GetOdmWord(int index) {
        return GetOdmWordImpl(index, GetSocType());
    }

    void GetEcid(br::BootEcid *out) {
        /* Get the registers. */
        volatile auto &chip = GetChipRegistersCommon();

        /* Read the ecid components. */
        const u32 vendor    = reg::Read(chip.FUSE_OPT_VENDOR_CODE)  & ((1u <<  4) - 1);
        const u32 fab       = reg::Read(chip.FUSE_OPT_FAB_CODE)     & ((1u <<  6) - 1);
        const u32 lot0      = reg::Read(chip.FUSE_OPT_LOT_CODE_0)   /* all 32 bits */ ;
        const u32 lot1      = reg::Read(chip.FUSE_OPT_LOT_CODE_1)   & ((1u << 28) - 1);
        const u32 wafer     = reg::Read(chip.FUSE_OPT_WAFER_ID)     & ((1u <<  6) - 1);
        const u32 x_coord   = reg::Read(chip.FUSE_OPT_X_COORDINATE) & ((1u <<  9) - 1);
        const u32 y_coord   = reg::Read(chip.FUSE_OPT_Y_COORDINATE) & ((1u <<  9) - 1);
        const u32 reserved  = reg::Read(chip.FUSE_OPT_OPS_RESERVED) & ((1u <<  6) - 1);

        /* Clear the output. */
        util::ClearMemory(out, sizeof(*out));

        /* Copy the component bits. */
        out->ecid[0] = static_cast<u32>((lot1 << 30) | (wafer << 24) | (x_coord << 15) | (y_coord << 6) | (reserved));
        out->ecid[1] = static_cast<u32>((lot0 << 26) | (lot1 >> 2));
        out->ecid[2] = static_cast<u32>((fab << 26) | (lot0 >> 6));
        out->ecid[3] = static_cast<u32>(vendor);
    }

    u64 GetDeviceId() {
        /* Get the registers. */
        volatile auto &chip = GetChipRegistersCommon();

        /* Read the device id components. */
        /* NOTE: Device ID is "basically" just an alternate encoding of Ecid. */
        /* It elides lot1 (and compresses lot0), but this is fine because */
        /* lot1 is fixed-value for all fused devices. */
        const u64 fab       = reg::Read(chip.FUSE_OPT_FAB_CODE)     & ((1u <<  6) - 1);
        const u32 lot0      = reg::Read(chip.FUSE_OPT_LOT_CODE_0)   /* all 32 bits */ ;
        const u64 wafer     = reg::Read(chip.FUSE_OPT_WAFER_ID)     & ((1u <<  6) - 1);
        const u64 x_coord   = reg::Read(chip.FUSE_OPT_X_COORDINATE) & ((1u <<  9) - 1);
        const u64 y_coord   = reg::Read(chip.FUSE_OPT_Y_COORDINATE) & ((1u <<  9) - 1);

        /* Compress lot0 down from 32-bits to 26. */
        const u64 clot0 = CompressLotCode(lot0) & ((1u << 26) - 1);

        return (y_coord <<  0) |
               (x_coord <<  9) |
               (wafer   << 18) |
               (clot0   << 24) |
               (fab     << 50);
    }

    DramId GetDramId() {
        /* Get the value. */
        return static_cast<DramId>(GetDramIdValue(util::BitPack32{GetCommonOdmWord(4)}));
    }

    HardwareType GetHardwareType() {
        /* Read the odm word. */
        const util::BitPack32 odm_word4 = { GetCommonOdmWord(4) };

        /* Get the value. */
        const auto value = GetHardwareTypeValue(odm_word4);

        switch (value) {
            case 0x01: return HardwareType_Icosa;
            case 0x02: return (true /* TODO: GetSocType() == SocType_Mariko */) ? HardwareType_Calcio : HardwareType_Copper;
            case 0x04: return HardwareType_Iowa;
            case 0x08: return HardwareType_Hoag;
            case 0x10: return HardwareType_Aula;
            default:   return HardwareType_Undefined;
        }
    }

    HardwareState GetHardwareState() {
        /* Read the odm word. */
        const util::BitPack32 odm_word4 = { GetCommonOdmWord(4) };

        /* Get the value. */
        const auto value = GetHardwareStateValue(odm_word4);

        switch (value) {
            case 3:  return HardwareState_Development;
            case 4:  return HardwareState_Production;
            default: return HardwareState_Undefined;
        }
    }

    PatchVersion GetPatchVersion() {
        const auto patch_version = reg::Read(GetChipRegistersCommon().FUSE_SOC_SPEEDO_1_CALIB);
        return static_cast<PatchVersion>(static_cast<int>(GetSocType() << 12) | patch_version);
    }

    RetailInteractiveDisplayState GetRetailInteractiveDisplayState() {
        return static_cast<RetailInteractiveDisplayState>(util::BitPack32{GetCommonOdmWord(4)}.Get<OdmWord4::RetailInteractiveDisplayState>());
    }

    pmic::Regulator GetRegulator() {
        if (GetSocType() == SocType_Mariko) {
            /* Read the odm word. */
            const util::BitPack32 odm_word28 = { GetOdmWordImpl(28, SocType_Mariko) };

            return static_cast<pmic::Regulator>(odm_word28.Get<OdmWord28::Regulator>() + 1);
        } else /* if (GetSocType() == SocType_Erista) */ {
            return pmic::Regulator_Erista_Max77621;
        }
    }

    int GetDeviceUniqueKeyGeneration() {
        if (IsNewFuseFormat()) {
            return util::BitPack32{GetCommonOdmWord(2)}.Get<OdmWord2::DeviceUniqueKeyGeneration>();
        } else {
            return 0;
        }
    }

    SocType GetSocType() {
        if (AMS_LIKELY(g_soc_type != SocType_CommonInternal)) {
            return g_soc_type;
        } else {
            switch (GetHardwareType()) {
                case HardwareType_Icosa:
                case HardwareType_Copper:
                    g_soc_type = SocType_Erista;
                    break;
                case HardwareType_Iowa:
                case HardwareType_Hoag:
                case HardwareType_Calcio:
                case HardwareType_Aula:
                    g_soc_type = SocType_Mariko;
                    break;
                default:
                    g_soc_type = SocType_Undefined;
                    break;
            }

            return g_soc_type;
        }
    }

    int GetExpectedFuseVersion(TargetFirmware target_fw) {
        return GetExpectedFuseVersionImpl(target_fw);
    }

    int GetFuseVersion() {
        return util::PopCount(GetCommonOdmWord(7));
    }

    bool HasRcmVulnerabilityPatch() {
        /* Only check for RCM bug patch once, and cache our result. */
        if (!g_checked_for_rcm_bug_patch) {
            do {
                /* Mariko units are necessarily patched. */
                if (fuse::GetSocType() != SocType_Erista) {
                    g_has_rcm_bug_patch = true;
                    break;
                }

                /* Some patched units use XUSB in RCM. */
                if (reg::Read(GetChipRegistersCommon().FUSE_RESERVED_SW) & 0x80) {
                    g_has_rcm_bug_patch = true;
                    break;
                }

                /* Other units have a proper ipatch instead. */
                u32 word_count = reg::Read(GetChipRegistersCommon().FUSE_FIRST_BOOTROM_PATCH_SIZE) & 0x7F;
                u32 word_addr  = 191;

                while (word_count && !g_has_rcm_bug_patch) {
                    u32 word0        = ReadWord(word_addr);
                    u32 ipatch_count = (word0 >> 16) & 0xF;

                    for (u32 i = 0; i < ipatch_count && !g_has_rcm_bug_patch; ++i) {
                        u32 word = ReadWord(word_addr - (i + 1));
                        u32 addr = (word >> 16) * 2;

                        if (addr == 0x769a) {
                            g_has_rcm_bug_patch = true;
                            break;
                        }
                    }

                    word_addr -= word_count;
                    word_count = word0 >> 25;
                }
            } while (0);

            g_checked_for_rcm_bug_patch = true;
        }

        return g_has_rcm_bug_patch;
    }

    bool IsOdmProductionMode() {
        return reg::HasValue(GetChipRegistersCommon().FUSE_SECURITY_MODE, FUSE_REG_BITS_ENUM(SECURITY_MODE_SECURITY_MODE, ENABLED));
    }

    bool GetSecureBootKey(void *dst) {
        /* Get the sbk from fuse data. */
        bool valid = false;
        for (size_t i = 0; i < 4; ++i) {
            const u32 key_word = GetChipRegistersCommon().FUSE_PRIVATE_KEY[i];

            static_cast<u32 *>(dst)[i] = key_word;
            valid |= key_word != 0xFFFFFFFF;
        }

        return valid;
    }

    void ConfigureFuseBypass() {
        /* Make the fuse registers visible. */
        clkrst::SetFuseVisibility(true);

        /* Only perform bypass configuration if fuse programming is allowed. */
        if (!reg::HasValue(GetRegisters().FUSE_DISABLEREGPROGRAM, FUSE_REG_BITS_ENUM(DISABLEREGPROGRAM_VAL, DISABLE))) {
            return;
        }

        /* Enable software writes to fuses. */
        reg::ReadWrite(GetRegisters().FUSE_WRITE_ACCESS_SW, FUSE_REG_BITS_ENUM(WRITE_ACCESS_SW_CTRL,   READWRITE),
                                                            FUSE_REG_BITS_ENUM(WRITE_ACCESS_SW_STATUS,     WRITE));

        /* Enable fuse bypass. */
        reg::Write(GetRegisters().FUSE_FUSEBYPASS, FUSE_REG_BITS_ENUM(FUSEBYPASS_VAL, ENABLE));

        /* Override fuses. */
        for (const auto &entry : FuseBypassEntries) {
            reg::Write(g_register_address + entry.offset, entry.value);
        }

        /* Disable software writes to fuses. */
        reg::ReadWrite(GetRegisters().FUSE_WRITE_ACCESS_SW, FUSE_REG_BITS_ENUM(WRITE_ACCESS_SW_CTRL, READONLY));

        /* NOTE: Here, NVidia almost certainly intends to *disable* fuse bypass, but they write enable instead... */
        reg::Write(GetRegisters().FUSE_FUSEBYPASS, FUSE_REG_BITS_ENUM(FUSEBYPASS_VAL, ENABLE));

        /* NOTE: Here, NVidia intends to disable fuse programming. However, they fuck up -- and *clear* the disable bit. */
        /* It should be noted that this is a sticky bit, and thus software clears have no effect. */
        reg::ReadWrite(GetRegisters().FUSE_DISABLEREGPROGRAM, FUSE_REG_BITS_ENUM(DISABLEREGPROGRAM_VAL, DISABLE));

        /* Configure FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT. */
        constexpr const uintptr_t PMC = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        const bool key_invisible = reg::HasValue(PMC + APBDEV_PMC_SECURE_SCRATCH21, FUSE_REG_BITS_ENUM(PRIVATEKEYDISABLE_TZ_STICKY_BIT_VAL, KEY_INVISIBLE));

        reg::ReadWrite(GetRegisters().FUSE_PRIVATEKEYDISABLE, FUSE_REG_BITS_ENUM_SEL(PRIVATEKEYDISABLE_TZ_STICKY_BIT_VAL, key_invisible, KEY_INVISIBLE, KEY_VISIBLE));

        /* Write-lock PMC_SECURE_SCRATCH21. */
        reg::ReadWrite(PMC + APBDEV_PMC_SEC_DISABLE2, PMC_REG_BITS_ENUM(SEC_DISABLE2_WRITE21, ON));
    }

}

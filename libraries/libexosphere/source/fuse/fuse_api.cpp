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
#include "fuse_registers.hpp"

namespace ams::fuse {

    namespace {

        struct OdmWord4 {
            using HardwareState1 = util::BitPack32::Field<0,                    2, int>;
            using HardwareType1  = util::BitPack32::Field<HardwareState1::Next, 1, int>;
            using DramId         = util::BitPack32::Field<HardwareType1::Next,  5, int>;
            using HardwareType2  = util::BitPack32::Field<DramId::Next,         1, int>;
            using HardwareState2 = util::BitPack32::Field<HardwareType2::Next,  1, int>;
            using QuestState     = util::BitPack32::Field<HardwareState2::Next, 1, int>;
            using FormatVersion  = util::BitPack32::Field<QuestState::Next,     1, int>;
            using Reserved       = util::BitPack32::Field<FormatVersion::Next,  4, int>;
            using HardwareType3  = util::BitPack32::Field<Reserved::Next,       4, int>;
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

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceFuses.GetAddress();

        ALWAYS_INLINE volatile FuseRegisterRegion *GetRegisterRegion() {
            return reinterpret_cast<volatile FuseRegisterRegion *>(g_register_address);
        }

        ALWAYS_INLINE volatile FuseRegisters &GetRegisters() {
            return GetRegisterRegion()->fuse;
        }

        ALWAYS_INLINE volatile FuseChipRegisters &GetChipRegisters() {
            return GetRegisterRegion()->chip;
        }

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void SetWriteSecureOnly() {
        reg::Write(GetRegisters().FUSE_PRIVATEKEYDISABLE, FUSE_REG_BITS_ENUM(PRIVATEKEYDISABLE_TZ_STICKY_BIT_VAL, KEY_INVISIBLE));
    }

    void Lockout() {
        reg::Write(GetRegisters().FUSE_DISABLEREGPROGRAM, FUSE_REG_BITS_ENUM(DISABLEREGPROGRAM_DISABLEREGPROGRAM_VAL, ENABLE));
    }

    u32 GetOdmWord(int index) {
        return GetChipRegisters().FUSE_RESERVED_ODM[index];
    }

    HardwareType GetHardwareType() {
        /* Read the odm word. */
        const util::BitPack32 odm_word4 = { GetOdmWord(4) };

        /* Get the value. */
        const auto value = GetHardwareTypeValue(odm_word4);

        switch (value) {
            case 0x01: return HardwareType_Icosa;
            case 0x02: return (true /* TODO: GetSocType() == SocType_Mariko */) ? HardwareType_Calcio : HardwareType_Copper;
            case 0x04: return HardwareType_Iowa;
            case 0x08: return HardwareType_Hoag;
            case 0x10: return HardwareType_Five;
            default:   return HardwareType_Undefined;
        }
    }

    HardwareState GetHardwareState() {
        /* Read the odm word. */
        const util::BitPack32 odm_word4 = { GetOdmWord(4) };

        /* Get the value. */
        const auto value = GetHardwareStateValue(odm_word4);

        switch (value) {
            case 3:  return HardwareState_Development;
            case 4:  return HardwareState_Production;
            default: return HardwareState_Undefined;
        }
    }

    pmic::Regulator GetRegulator() {
        /* TODO: How should mariko be handled? This reads from ODM word 28 in fuses (not presesnt in erista...). */
        return pmic::Regulator_Erista_Max77621;
    }

}
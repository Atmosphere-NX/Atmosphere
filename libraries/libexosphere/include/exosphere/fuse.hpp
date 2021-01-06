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
#pragma once
#include <vapours.hpp>
#include <exosphere/br.hpp>
#include <exosphere/pmic.hpp>

namespace ams::fuse {

    enum HardwareType {
        HardwareType_Icosa  = 0,
        HardwareType_Copper = 1,
        HardwareType_Hoag   = 2,
        HardwareType_Iowa   = 3,
        HardwareType_Calcio = 4,
        HardwareType_Aula   = 5,

        HardwareType_Undefined = 0xF,
    };

    enum SocType {
        SocType_Erista    = 0,
        SocType_Mariko    = 1,
        SocType_Undefined = 0xF,
    };

    enum HardwareState {
        HardwareState_Development = 0,
        HardwareState_Production  = 1,
        HardwareState_Undefined   = 2,
    };

    enum PatchVersion {
        PatchVersion_Odnx02A2 = (SocType_Erista << 12) | 0x07F,
    };

    enum DramId {
        DramId_IcosaSamsung4GB    =  0,
        DramId_IcosaHynix4GB      =  1,
        DramId_IcosaMicron4GB     =  2,
        DramId_AulaHynix1y4GB     =  3,
        DramId_IcosaSamsung6GB    =  4,
        DramId_CopperHynix4GB     =  5,
        DramId_CopperMicron4GB    =  6,
        DramId_IowaX1X2Samsung4GB =  7,
        DramId_IowaSansung4GB     =  8,
        DramId_IowaSamsung8GB     =  9,
        DramId_IowaHynix4GB       = 10,
        DramId_IowaMicron4GB      = 11,
        DramId_HoagSamsung4GB     = 12,
        DramId_HoagSamsung8GB     = 13,
        DramId_HoagHynix4GB       = 14,
        DramId_HoagMicron4GB      = 15,
        DramId_IowaSamsung4GBY    = 16,
        DramId_IowaSamsung1y4GBX  = 17,
        DramId_IowaSamsung1y8GBX  = 18,
        DramId_HoagSamsung1y4GBX  = 19,
        DramId_IowaSamsung1y4GBY  = 20,
        DramId_IowaSamsung1y8GBY  = 21,
        DramId_AulaSamsung1y4GB   = 22,
        DramId_HoagSamsung1y8GBX  = 23,
        DramId_AulaSamsung1y4GBX  = 24,
        DramId_IowaMicron1y4GB    = 25,
        DramId_HoagMicron1y4GB    = 26,
        DramId_AulaMicron1y4GB    = 27,
        DramId_AulaSamsung1y8GBX  = 28,

        DramId_Count,
    };

    enum QuestState {
        QuestState_Disabled = 0,
        QuestState_Enabled  = 1,
    };

    void SetRegisterAddress(uintptr_t address);
    void SetWriteSecureOnly();
    void Lockout();

    void Activate();
    void Deactivate();
    void Reload();

    u32 ReadWord(int address);

    u32 GetOdmWord(int index);

    DramId GetDramId();

    void            GetEcid(br::BootEcid *out);
    HardwareType    GetHardwareType();
    HardwareState   GetHardwareState();
    u64             GetDeviceId();
    PatchVersion    GetPatchVersion();
    QuestState      GetQuestState();
    pmic::Regulator GetRegulator();
    int             GetDeviceUniqueKeyGeneration();

    SocType         GetSocType();
    int             GetExpectedFuseVersion(TargetFirmware target_fw);
    bool            HasRcmVulnerabilityPatch();

    bool IsOdmProductionMode();
    void ConfigureFuseBypass();

}
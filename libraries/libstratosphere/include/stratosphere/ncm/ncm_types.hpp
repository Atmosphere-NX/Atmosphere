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
#include <vapours.hpp>

namespace ams::ncm {

    /* Storage IDs. */
    enum class StorageId : u8 {
        #define DEFINE_ENUM_MEMBER(nm) nm = NcmStorageId_##nm
        DEFINE_ENUM_MEMBER(None),
        DEFINE_ENUM_MEMBER(Host),
        DEFINE_ENUM_MEMBER(GameCard),
        DEFINE_ENUM_MEMBER(BuiltInSystem),
        DEFINE_ENUM_MEMBER(BuiltInUser),
        DEFINE_ENUM_MEMBER(SdCard),
        DEFINE_ENUM_MEMBER(Any),
        #undef DEFINE_ENUM_MEMBER
    };

    /* Program IDs (Formerly: Title IDs). */
    struct ProgramId {
        u64 value;

        inline explicit operator u64() const {
            return this->value;
        }

        /* Invalid Program ID. */
        static const ProgramId Invalid;

        /* System Modules. */
        static const ProgramId SystemStart;

        static const ProgramId Fs;
        static const ProgramId Loader;
        static const ProgramId Ncm;
        static const ProgramId Pm;
        static const ProgramId Sm;
        static const ProgramId Boot;
        static const ProgramId Usb;
        static const ProgramId Tma;
        static const ProgramId Boot2;
        static const ProgramId Settings;
        static const ProgramId Bus;
        static const ProgramId Bluetooth;
        static const ProgramId Bcat;
        static const ProgramId Dmnt;
        static const ProgramId Friends;
        static const ProgramId Nifm;
        static const ProgramId Ptm;
        static const ProgramId Shell;
        static const ProgramId BsdSockets;
        static const ProgramId Hid;
        static const ProgramId Audio;
        static const ProgramId LogManager;
        static const ProgramId Wlan;
        static const ProgramId Cs;
        static const ProgramId Ldn;
        static const ProgramId NvServices;
        static const ProgramId Pcv;
        static const ProgramId Ppc;
        static const ProgramId NvnFlinger;
        static const ProgramId Pcie;
        static const ProgramId Account;
        static const ProgramId Ns;
        static const ProgramId Nfc;
        static const ProgramId Psc;
        static const ProgramId CapSrv;
        static const ProgramId Am;
        static const ProgramId Ssl;
        static const ProgramId Nim;
        static const ProgramId Cec;
        static const ProgramId Tspm;
        static const ProgramId Spl;
        static const ProgramId Lbl;
        static const ProgramId Btm;
        static const ProgramId Erpt;
        static const ProgramId Time;
        static const ProgramId Vi;
        static const ProgramId Pctl;
        static const ProgramId Npns;
        static const ProgramId Eupld;
        static const ProgramId Arp;
        static const ProgramId Glue;
        static const ProgramId Eclct;
        static const ProgramId Es;
        static const ProgramId Fatal;
        static const ProgramId Grc;
        static const ProgramId Creport;
        static const ProgramId Ro;
        static const ProgramId Profiler;
        static const ProgramId Sdb;
        static const ProgramId Migration;
        static const ProgramId Jit;
        static const ProgramId JpegDec;
        static const ProgramId SafeMode;
        static const ProgramId Olsc;
        static const ProgramId Dt;
        static const ProgramId Nd;
        static const ProgramId Ngct;

        static const ProgramId SystemEnd;

        /* System Data Archives. */
        static const ProgramId ArchiveStart;
        static const ProgramId ArchiveCertStore;
        static const ProgramId ArchiveErrorMessage;
        static const ProgramId ArchiveMiiModel;
        static const ProgramId ArchiveBrowserDll;
        static const ProgramId ArchiveHelp;
        static const ProgramId ArchiveSharedFont;
        static const ProgramId ArchiveNgWord;
        static const ProgramId ArchiveSsidList;
        static const ProgramId ArchiveDictionary;
        static const ProgramId ArchiveSystemVersion;
        static const ProgramId ArchiveAvatarImage;
        static const ProgramId ArchiveLocalNews;
        static const ProgramId ArchiveEula;
        static const ProgramId ArchiveUrlBlackList;
        static const ProgramId ArchiveTimeZoneBinar;
        static const ProgramId ArchiveCertStoreCruiser;
        static const ProgramId ArchiveFontNintendoExtension;
        static const ProgramId ArchiveFontStandard;
        static const ProgramId ArchiveFontKorean;
        static const ProgramId ArchiveFontChineseTraditional;
        static const ProgramId ArchiveFontChineseSimple;
        static const ProgramId ArchiveFontBfcpx;
        static const ProgramId ArchiveSystemUpdate;

        static const ProgramId ArchiveFirmwareDebugSettings;
        static const ProgramId ArchiveBootImagePackage;
        static const ProgramId ArchiveBootImagePackageSafe;
        static const ProgramId ArchiveBootImagePackageExFat;
        static const ProgramId ArchiveBootImagePackageExFatSafe;
        static const ProgramId ArchiveFatalMessage;
        static const ProgramId ArchiveControllerIcon;
        static const ProgramId ArchivePlatformConfigIcosa;
        static const ProgramId ArchivePlatformConfigCopper;
        static const ProgramId ArchivePlatformConfigHoag;
        static const ProgramId ArchiveControllerFirmware;
        static const ProgramId ArchiveNgWord2;
        static const ProgramId ArchivePlatformConfigIcosaMariko;
        static const ProgramId ArchiveApplicationBlackList;
        static const ProgramId ArchiveRebootlessSystemUpdateVersion;
        static const ProgramId ArchiveContentActionTable;

        static const ProgramId ArchiveEnd;

        /* System Applets. */
        static const ProgramId AppletStart;

        static const ProgramId AppletQlaunch;
        static const ProgramId AppletAuth;
        static const ProgramId AppletCabinet;
        static const ProgramId AppletController;
        static const ProgramId AppletDataErase;
        static const ProgramId AppletError;
        static const ProgramId AppletNetConnect;
        static const ProgramId AppletPlayerSelect;
        static const ProgramId AppletSwkbd;
        static const ProgramId AppletMiiEdit;
        static const ProgramId AppletWeb;
        static const ProgramId AppletShop;
        static const ProgramId AppletOverlayDisp;
        static const ProgramId AppletPhotoViewer;
        static const ProgramId AppletSet;
        static const ProgramId AppletOfflineWeb;
        static const ProgramId AppletLoginShare;
        static const ProgramId AppletWifiWebAuth;
        static const ProgramId AppletStarter;
        static const ProgramId AppletMyPage;
        static const ProgramId AppletPlayReport;
        static const ProgramId AppletMaintenanceMenu;

        static const ProgramId AppletGift;
        static const ProgramId AppletDummyShop;
        static const ProgramId AppletUserMigration;
        static const ProgramId AppletEncounter;

        static const ProgramId AppletStory;

        static const ProgramId AppletEnd;

        /* Debug Applets. */

        /* Debug Modules. */

        /* Factory Setup. */

        /* Applications. */
        static const ProgramId ApplicationStart;
        static const ProgramId ApplicationEnd;

        /* Atmosphere Extensions. */
        static const ProgramId AtmosphereMitm;
    };

    /* Invalid Program ID. */
    inline constexpr const ProgramId ProgramId::Invalid = {};

    inline constexpr const ProgramId InvalidProgramId = ProgramId::Invalid;

    /* System Modules. */
    inline constexpr const ProgramId ProgramId::SystemStart = { 0x0100000000000000ul };

    inline constexpr const ProgramId ProgramId::Fs          = { 0x0100000000000000ul };
    inline constexpr const ProgramId ProgramId::Loader      = { 0x0100000000000001ul };
    inline constexpr const ProgramId ProgramId::Ncm         = { 0x0100000000000002ul };
    inline constexpr const ProgramId ProgramId::Pm          = { 0x0100000000000003ul };
    inline constexpr const ProgramId ProgramId::Sm          = { 0x0100000000000004ul };
    inline constexpr const ProgramId ProgramId::Boot        = { 0x0100000000000005ul };
    inline constexpr const ProgramId ProgramId::Usb         = { 0x0100000000000006ul };
    inline constexpr const ProgramId ProgramId::Tma         = { 0x0100000000000007ul };
    inline constexpr const ProgramId ProgramId::Boot2       = { 0x0100000000000008ul };
    inline constexpr const ProgramId ProgramId::Settings    = { 0x0100000000000009ul };
    inline constexpr const ProgramId ProgramId::Bus         = { 0x010000000000000Aul };
    inline constexpr const ProgramId ProgramId::Bluetooth   = { 0x010000000000000Bul };
    inline constexpr const ProgramId ProgramId::Bcat        = { 0x010000000000000Cul };
    inline constexpr const ProgramId ProgramId::Dmnt        = { 0x010000000000000Dul };
    inline constexpr const ProgramId ProgramId::Friends     = { 0x010000000000000Eul };
    inline constexpr const ProgramId ProgramId::Nifm        = { 0x010000000000000Ful };
    inline constexpr const ProgramId ProgramId::Ptm         = { 0x0100000000000010ul };
    inline constexpr const ProgramId ProgramId::Shell       = { 0x0100000000000011ul };
    inline constexpr const ProgramId ProgramId::BsdSockets  = { 0x0100000000000012ul };
    inline constexpr const ProgramId ProgramId::Hid         = { 0x0100000000000013ul };
    inline constexpr const ProgramId ProgramId::Audio       = { 0x0100000000000014ul };
    inline constexpr const ProgramId ProgramId::LogManager  = { 0x0100000000000015ul };
    inline constexpr const ProgramId ProgramId::Wlan        = { 0x0100000000000016ul };
    inline constexpr const ProgramId ProgramId::Cs          = { 0x0100000000000017ul };
    inline constexpr const ProgramId ProgramId::Ldn         = { 0x0100000000000018ul };
    inline constexpr const ProgramId ProgramId::NvServices  = { 0x0100000000000019ul };
    inline constexpr const ProgramId ProgramId::Pcv         = { 0x010000000000001Aul };
    inline constexpr const ProgramId ProgramId::Ppc         = { 0x010000000000001Bul };
    inline constexpr const ProgramId ProgramId::NvnFlinger  = { 0x010000000000001Cul };
    inline constexpr const ProgramId ProgramId::Pcie        = { 0x010000000000001Dul };
    inline constexpr const ProgramId ProgramId::Account     = { 0x010000000000001Eul };
    inline constexpr const ProgramId ProgramId::Ns          = { 0x010000000000001Ful };
    inline constexpr const ProgramId ProgramId::Nfc         = { 0x0100000000000020ul };
    inline constexpr const ProgramId ProgramId::Psc         = { 0x0100000000000021ul };
    inline constexpr const ProgramId ProgramId::CapSrv      = { 0x0100000000000022ul };
    inline constexpr const ProgramId ProgramId::Am          = { 0x0100000000000023ul };
    inline constexpr const ProgramId ProgramId::Ssl         = { 0x0100000000000024ul };
    inline constexpr const ProgramId ProgramId::Nim         = { 0x0100000000000025ul };
    inline constexpr const ProgramId ProgramId::Cec         = { 0x0100000000000026ul };
    inline constexpr const ProgramId ProgramId::Tspm        = { 0x0100000000000027ul };
    inline constexpr const ProgramId ProgramId::Spl         = { 0x0100000000000028ul };
    inline constexpr const ProgramId ProgramId::Lbl         = { 0x0100000000000029ul };
    inline constexpr const ProgramId ProgramId::Btm         = { 0x010000000000002Aul };
    inline constexpr const ProgramId ProgramId::Erpt        = { 0x010000000000002Bul };
    inline constexpr const ProgramId ProgramId::Time        = { 0x010000000000002Cul };
    inline constexpr const ProgramId ProgramId::Vi          = { 0x010000000000002Dul };
    inline constexpr const ProgramId ProgramId::Pctl        = { 0x010000000000002Eul };
    inline constexpr const ProgramId ProgramId::Npns        = { 0x010000000000002Ful };
    inline constexpr const ProgramId ProgramId::Eupld       = { 0x0100000000000030ul };
    inline constexpr const ProgramId ProgramId::Arp         = { 0x0100000000000031ul };
    inline constexpr const ProgramId ProgramId::Glue        = { 0x0100000000000031ul };
    inline constexpr const ProgramId ProgramId::Eclct       = { 0x0100000000000032ul };
    inline constexpr const ProgramId ProgramId::Es          = { 0x0100000000000033ul };
    inline constexpr const ProgramId ProgramId::Fatal       = { 0x0100000000000034ul };
    inline constexpr const ProgramId ProgramId::Grc         = { 0x0100000000000035ul };
    inline constexpr const ProgramId ProgramId::Creport     = { 0x0100000000000036ul };
    inline constexpr const ProgramId ProgramId::Ro          = { 0x0100000000000037ul };
    inline constexpr const ProgramId ProgramId::Profiler    = { 0x0100000000000038ul };
    inline constexpr const ProgramId ProgramId::Sdb         = { 0x0100000000000039ul };
    inline constexpr const ProgramId ProgramId::Migration   = { 0x010000000000003Aul };
    inline constexpr const ProgramId ProgramId::Jit         = { 0x010000000000003Bul };
    inline constexpr const ProgramId ProgramId::JpegDec     = { 0x010000000000003Cul };
    inline constexpr const ProgramId ProgramId::SafeMode    = { 0x010000000000003Dul };
    inline constexpr const ProgramId ProgramId::Olsc        = { 0x010000000000003Eul };
    inline constexpr const ProgramId ProgramId::Dt          = { 0x010000000000003Ful };
    inline constexpr const ProgramId ProgramId::Nd          = { 0x0100000000000040ul };
    inline constexpr const ProgramId ProgramId::Ngct        = { 0x0100000000000041ul };

    inline constexpr const ProgramId ProgramId::SystemEnd   = { 0x01000000000007FFul };

    /* System Data Archives. */
    inline constexpr const ProgramId ProgramId::ArchiveStart                         = { 0x0100000000000800ul };
    inline constexpr const ProgramId ProgramId::ArchiveCertStore                     = { 0x0100000000000800ul };
    inline constexpr const ProgramId ProgramId::ArchiveErrorMessage                  = { 0x0100000000000801ul };
    inline constexpr const ProgramId ProgramId::ArchiveMiiModel                      = { 0x0100000000000802ul };
    inline constexpr const ProgramId ProgramId::ArchiveBrowserDll                    = { 0x0100000000000803ul };
    inline constexpr const ProgramId ProgramId::ArchiveHelp                          = { 0x0100000000000804ul };
    inline constexpr const ProgramId ProgramId::ArchiveSharedFont                    = { 0x0100000000000805ul };
    inline constexpr const ProgramId ProgramId::ArchiveNgWord                        = { 0x0100000000000806ul };
    inline constexpr const ProgramId ProgramId::ArchiveSsidList                      = { 0x0100000000000807ul };
    inline constexpr const ProgramId ProgramId::ArchiveDictionary                    = { 0x0100000000000808ul };
    inline constexpr const ProgramId ProgramId::ArchiveSystemVersion                 = { 0x0100000000000809ul };
    inline constexpr const ProgramId ProgramId::ArchiveAvatarImage                   = { 0x010000000000080Aul };
    inline constexpr const ProgramId ProgramId::ArchiveLocalNews                     = { 0x010000000000080Bul };
    inline constexpr const ProgramId ProgramId::ArchiveEula                          = { 0x010000000000080Cul };
    inline constexpr const ProgramId ProgramId::ArchiveUrlBlackList                  = { 0x010000000000080Dul };
    inline constexpr const ProgramId ProgramId::ArchiveTimeZoneBinar                 = { 0x010000000000080Eul };
    inline constexpr const ProgramId ProgramId::ArchiveCertStoreCruiser              = { 0x010000000000080Ful };
    inline constexpr const ProgramId ProgramId::ArchiveFontNintendoExtension         = { 0x0100000000000810ul };
    inline constexpr const ProgramId ProgramId::ArchiveFontStandard                  = { 0x0100000000000811ul };
    inline constexpr const ProgramId ProgramId::ArchiveFontKorean                    = { 0x0100000000000812ul };
    inline constexpr const ProgramId ProgramId::ArchiveFontChineseTraditional        = { 0x0100000000000813ul };
    inline constexpr const ProgramId ProgramId::ArchiveFontChineseSimple             = { 0x0100000000000814ul };
    inline constexpr const ProgramId ProgramId::ArchiveFontBfcpx                     = { 0x0100000000000815ul };
    inline constexpr const ProgramId ProgramId::ArchiveSystemUpdate                  = { 0x0100000000000816ul };

    inline constexpr const ProgramId ProgramId::ArchiveFirmwareDebugSettings         = { 0x0100000000000818ul };
    inline constexpr const ProgramId ProgramId::ArchiveBootImagePackage              = { 0x0100000000000819ul };
    inline constexpr const ProgramId ProgramId::ArchiveBootImagePackageSafe          = { 0x010000000000081Aul };
    inline constexpr const ProgramId ProgramId::ArchiveBootImagePackageExFat         = { 0x010000000000081Bul };
    inline constexpr const ProgramId ProgramId::ArchiveBootImagePackageExFatSafe     = { 0x010000000000081Cul };
    inline constexpr const ProgramId ProgramId::ArchiveFatalMessage                  = { 0x010000000000081Dul };
    inline constexpr const ProgramId ProgramId::ArchiveControllerIcon                = { 0x010000000000081Eul };
    inline constexpr const ProgramId ProgramId::ArchivePlatformConfigIcosa           = { 0x010000000000081Ful };
    inline constexpr const ProgramId ProgramId::ArchivePlatformConfigCopper          = { 0x0100000000000820ul };
    inline constexpr const ProgramId ProgramId::ArchivePlatformConfigHoag            = { 0x0100000000000821ul };
    inline constexpr const ProgramId ProgramId::ArchiveControllerFirmware            = { 0x0100000000000822ul };
    inline constexpr const ProgramId ProgramId::ArchiveNgWord2                       = { 0x0100000000000823ul };
    inline constexpr const ProgramId ProgramId::ArchivePlatformConfigIcosaMariko     = { 0x0100000000000824ul };
    inline constexpr const ProgramId ProgramId::ArchiveApplicationBlackList          = { 0x0100000000000825ul };
    inline constexpr const ProgramId ProgramId::ArchiveRebootlessSystemUpdateVersion = { 0x0100000000000826ul };
    inline constexpr const ProgramId ProgramId::ArchiveContentActionTable            = { 0x0100000000000827ul };

    inline constexpr const ProgramId ProgramId::ArchiveEnd                           = { 0x0100000000000FFFul };

    /* System Applets. */
    inline constexpr const ProgramId ProgramId::AppletStart           = { 0x0100000000001000ul };

    inline constexpr const ProgramId ProgramId::AppletQlaunch         = { 0x0100000000001000ul };
    inline constexpr const ProgramId ProgramId::AppletAuth            = { 0x0100000000001001ul };
    inline constexpr const ProgramId ProgramId::AppletCabinet         = { 0x0100000000001002ul };
    inline constexpr const ProgramId ProgramId::AppletController      = { 0x0100000000001003ul };
    inline constexpr const ProgramId ProgramId::AppletDataErase       = { 0x0100000000001004ul };
    inline constexpr const ProgramId ProgramId::AppletError           = { 0x0100000000001005ul };
    inline constexpr const ProgramId ProgramId::AppletNetConnect      = { 0x0100000000001006ul };
    inline constexpr const ProgramId ProgramId::AppletPlayerSelect    = { 0x0100000000001007ul };
    inline constexpr const ProgramId ProgramId::AppletSwkbd           = { 0x0100000000001008ul };
    inline constexpr const ProgramId ProgramId::AppletMiiEdit         = { 0x0100000000001009ul };
    inline constexpr const ProgramId ProgramId::AppletWeb             = { 0x010000000000100Aul };
    inline constexpr const ProgramId ProgramId::AppletShop            = { 0x010000000000100Bul };
    inline constexpr const ProgramId ProgramId::AppletOverlayDisp     = { 0x010000000000100Cul };
    inline constexpr const ProgramId ProgramId::AppletPhotoViewer     = { 0x010000000000100Dul };
    inline constexpr const ProgramId ProgramId::AppletSet             = { 0x010000000000100Eul };
    inline constexpr const ProgramId ProgramId::AppletOfflineWeb      = { 0x010000000000100Ful };
    inline constexpr const ProgramId ProgramId::AppletLoginShare      = { 0x0100000000001010ul };
    inline constexpr const ProgramId ProgramId::AppletWifiWebAuth     = { 0x0100000000001011ul };
    inline constexpr const ProgramId ProgramId::AppletStarter         = { 0x0100000000001012ul };
    inline constexpr const ProgramId ProgramId::AppletMyPage          = { 0x0100000000001013ul };
    inline constexpr const ProgramId ProgramId::AppletPlayReport      = { 0x0100000000001014ul };
    inline constexpr const ProgramId ProgramId::AppletMaintenanceMenu = { 0x0100000000001015ul };

    inline constexpr const ProgramId ProgramId::AppletGift            = { 0x010000000000101Aul };
    inline constexpr const ProgramId ProgramId::AppletDummyShop       = { 0x010000000000101Bul };
    inline constexpr const ProgramId ProgramId::AppletUserMigration   = { 0x010000000000101Cul };
    inline constexpr const ProgramId ProgramId::AppletEncounter       = { 0x010000000000101Dul };

    inline constexpr const ProgramId ProgramId::AppletStory           = { 0x0100000000001020ul };

    inline constexpr const ProgramId ProgramId::AppletEnd             = { 0x0100000000001FFFul };

    /* Debug Applets. */

    /* Debug Modules. */

    /* Factory Setup. */

    /* Applications. */
    inline constexpr const ProgramId ProgramId::ApplicationStart   = { 0x0100000000010000ul };
    inline constexpr const ProgramId ProgramId::ApplicationEnd     = { 0x01FFFFFFFFFFFFFFul };

    /* Atmosphere Extensions. */
    inline constexpr const ProgramId ProgramId::AtmosphereMitm = { 0x010041544D530000ul };

    inline constexpr bool operator==(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value >= rhs.value;
    }

    inline constexpr bool IsSystemProgramId(const ProgramId &program_id) {
        return ProgramId::SystemStart <= program_id && program_id <= ProgramId::SystemEnd;
    }

    inline constexpr bool IsArchiveProgramId(const ProgramId &program_id) {
        return ProgramId::ArchiveStart <= program_id && program_id <= ProgramId::ArchiveEnd;
    }

    inline constexpr bool IsAppletProgramId(const ProgramId &program_id) {
        return ProgramId::AppletStart <= program_id && program_id <= ProgramId::AppletEnd;
    }

    inline constexpr bool IsApplicationProgramId(const ProgramId &program_id) {
        return ProgramId::ApplicationStart <= program_id && program_id <= ProgramId::ApplicationEnd;
    }

    inline constexpr bool IsWebAppletProgramId(const ProgramId &program_id) {
        return program_id == ProgramId::AppletWeb ||
               program_id == ProgramId::AppletShop ||
               program_id == ProgramId::AppletOfflineWeb ||
               program_id == ProgramId::AppletLoginShare ||
               program_id == ProgramId::AppletWifiWebAuth;
    }

    static_assert(sizeof(ProgramId) == sizeof(u64) && std::is_pod<ProgramId>::value, "ProgramId definition!");

    /* Program Location. */
    struct ProgramLocation {
        ProgramId program_id;
        u8 storage_id;

        static constexpr ProgramLocation Make(ProgramId program_id, StorageId storage_id) {
            return { .program_id = program_id, .storage_id = static_cast<u8>(storage_id), };
        }
    };

    static_assert(sizeof(ProgramLocation) == 0x10 && std::is_pod<ProgramLocation>::value, "ProgramLocation definition!");
    static_assert(sizeof(ProgramLocation) == sizeof(::NcmProgramLocation) && alignof(ProgramLocation) == alignof(::NcmProgramLocation), "ProgramLocation Libnx Compatibility");

}

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

#include <type_traits>

namespace sts::ncm {

    /* Storage IDs. */
    enum class StorageId : u8 {
        None        = 0,
        Host        = 1,
        GameCard    = 2,
        NandSystem  = 3,
        NandUser    = 4,
        SdCard      = 5,
    };

    /* Title IDs. */
    struct TitleId {
        u64 value;

        inline explicit operator u64() const {
            return this->value;
        }

        /* Invalid Title ID. */
        static const TitleId Invalid;

        /* System Modules. */
        static const TitleId SystemStart;

        static const TitleId Fs;
        static const TitleId Loader;
        static const TitleId Ncm;
        static const TitleId Pm;
        static const TitleId Sm;
        static const TitleId Boot;
        static const TitleId Usb;
        static const TitleId Tma;
        static const TitleId Boot2;
        static const TitleId Settings;
        static const TitleId Bus;
        static const TitleId Bluetooth;
        static const TitleId Bcat;
        static const TitleId Dmnt;
        static const TitleId Friends;
        static const TitleId Nifm;
        static const TitleId Ptm;
        static const TitleId Shell;
        static const TitleId BsdSockets;
        static const TitleId Hid;
        static const TitleId Audio;
        static const TitleId LogManager;
        static const TitleId Wlan;
        static const TitleId Cs;
        static const TitleId Ldn;
        static const TitleId NvServices;
        static const TitleId Pcv;
        static const TitleId Ppc;
        static const TitleId NvnFlinger;
        static const TitleId Pcie;
        static const TitleId Account;
        static const TitleId Ns;
        static const TitleId Nfc;
        static const TitleId Psc;
        static const TitleId CapSrv;
        static const TitleId Am;
        static const TitleId Ssl;
        static const TitleId Nim;
        static const TitleId Cec;
        static const TitleId Tspm;
        static const TitleId Spl;
        static const TitleId Lbl;
        static const TitleId Btm;
        static const TitleId Erpt;
        static const TitleId Time;
        static const TitleId Vi;
        static const TitleId Pctl;
        static const TitleId Npns;
        static const TitleId Eupld;
        static const TitleId Arp;
        static const TitleId Glue;
        static const TitleId Eclct;
        static const TitleId Es;
        static const TitleId Fatal;
        static const TitleId Grc;
        static const TitleId Creport;
        static const TitleId Ro;
        static const TitleId Profiler;
        static const TitleId Sdb;
        static const TitleId Migration;
        static const TitleId Jit;
        static const TitleId JpegDec;
        static const TitleId SafeMode;
        static const TitleId Olsc;
        static const TitleId Dt;
        static const TitleId Nd;
        static const TitleId Ngct;

        static const TitleId SystemEnd;

        /* System Data Archives. */
        static const TitleId ArchiveStart;
        static const TitleId ArchiveCertStore;
        static const TitleId ArchiveErrorMessage;
        static const TitleId ArchiveMiiModel;
        static const TitleId ArchiveBrowserDll;
        static const TitleId ArchiveHelp;
        static const TitleId ArchiveSharedFont;
        static const TitleId ArchiveNgWord;
        static const TitleId ArchiveSsidList;
        static const TitleId ArchiveDictionary;
        static const TitleId ArchiveSystemVersion;
        static const TitleId ArchiveAvatarImage;
        static const TitleId ArchiveLocalNews;
        static const TitleId ArchiveEula;
        static const TitleId ArchiveUrlBlackList;
        static const TitleId ArchiveTimeZoneBinar;
        static const TitleId ArchiveCertStoreCruiser;
        static const TitleId ArchiveFontNintendoExtension;
        static const TitleId ArchiveFontStandard;
        static const TitleId ArchiveFontKorean;
        static const TitleId ArchiveFontChineseTraditional;
        static const TitleId ArchiveFontChineseSimple;
        static const TitleId ArchiveFontBfcpx;
        static const TitleId ArchiveSystemUpdate;

        static const TitleId ArchiveFirmwareDebugSettings;
        static const TitleId ArchiveBootImagePackage;
        static const TitleId ArchiveBootImagePackageSafe;
        static const TitleId ArchiveBootImagePackageExFat;
        static const TitleId ArchiveBootImagePackageExFatSafe;
        static const TitleId ArchiveFatalMessage;
        static const TitleId ArchiveControllerIcon;
        static const TitleId ArchivePlatformConfigIcosa;
        static const TitleId ArchivePlatformConfigCopper;
        static const TitleId ArchivePlatformConfigHoag;
        static const TitleId ArchiveControllerFirmware;
        static const TitleId ArchiveNgWord2;
        static const TitleId ArchivePlatformConfigIcosaMariko;
        static const TitleId ArchiveApplicationBlackList;
        static const TitleId ArchiveRebootlessSystemUpdateVersion;
        static const TitleId ArchiveContentActionTable;

        static const TitleId ArchiveEnd;

        /* System Applets. */
        static const TitleId AppletStart;

        static const TitleId AppletQlaunch;
        static const TitleId AppletAuth;
        static const TitleId AppletCabinet;
        static const TitleId AppletController;
        static const TitleId AppletDataErase;
        static const TitleId AppletError;
        static const TitleId AppletNetConnect;
        static const TitleId AppletPlayerSelect;
        static const TitleId AppletSwkbd;
        static const TitleId AppletMiiEdit;
        static const TitleId AppletWeb;
        static const TitleId AppletShop;
        static const TitleId AppletOverlayDisp;
        static const TitleId AppletPhotoViewer;
        static const TitleId AppletSet;
        static const TitleId AppletOfflineWeb;
        static const TitleId AppletLoginShare;
        static const TitleId AppletWifiWebAuth;
        static const TitleId AppletStarter;
        static const TitleId AppletMyPage;
        static const TitleId AppletPlayReport;
        static const TitleId AppletMaintenanceMenu;

        static const TitleId AppletGift;
        static const TitleId AppletDummyShop;
        static const TitleId AppletUserMigration;
        static const TitleId AppletEncounter;

        static const TitleId AppletStory;

        static const TitleId AppletEnd;

        /* Debug Applets. */

        /* Debug Modules. */

        /* Factory Setup. */

        /* Applications. */
        static const TitleId ApplicationStart;
        static const TitleId ApplicationEnd;

        /* Atmosphere Extensions. */
        static const TitleId AtmosphereMitm;
    };

    /* Invalid Title ID. */
    inline constexpr const TitleId TitleId::Invalid = {};

    /* System Modules. */
    inline constexpr const TitleId TitleId::SystemStart = { 0x0100000000000000ul };

    inline constexpr const TitleId TitleId::Fs          = { 0x0100000000000000ul };
    inline constexpr const TitleId TitleId::Loader      = { 0x0100000000000001ul };
    inline constexpr const TitleId TitleId::Ncm         = { 0x0100000000000002ul };
    inline constexpr const TitleId TitleId::Pm          = { 0x0100000000000003ul };
    inline constexpr const TitleId TitleId::Sm          = { 0x0100000000000004ul };
    inline constexpr const TitleId TitleId::Boot        = { 0x0100000000000005ul };
    inline constexpr const TitleId TitleId::Usb         = { 0x0100000000000006ul };
    inline constexpr const TitleId TitleId::Tma         = { 0x0100000000000007ul };
    inline constexpr const TitleId TitleId::Boot2       = { 0x0100000000000008ul };
    inline constexpr const TitleId TitleId::Settings    = { 0x0100000000000009ul };
    inline constexpr const TitleId TitleId::Bus         = { 0x010000000000000Aul };
    inline constexpr const TitleId TitleId::Bluetooth   = { 0x010000000000000Bul };
    inline constexpr const TitleId TitleId::Bcat        = { 0x010000000000000Cul };
    inline constexpr const TitleId TitleId::Dmnt        = { 0x010000000000000Dul };
    inline constexpr const TitleId TitleId::Friends     = { 0x010000000000000Eul };
    inline constexpr const TitleId TitleId::Nifm        = { 0x010000000000000Ful };
    inline constexpr const TitleId TitleId::Ptm         = { 0x0100000000000010ul };
    inline constexpr const TitleId TitleId::Shell       = { 0x0100000000000011ul };
    inline constexpr const TitleId TitleId::BsdSockets  = { 0x0100000000000012ul };
    inline constexpr const TitleId TitleId::Hid         = { 0x0100000000000013ul };
    inline constexpr const TitleId TitleId::Audio       = { 0x0100000000000014ul };
    inline constexpr const TitleId TitleId::LogManager  = { 0x0100000000000015ul };
    inline constexpr const TitleId TitleId::Wlan        = { 0x0100000000000016ul };
    inline constexpr const TitleId TitleId::Cs          = { 0x0100000000000017ul };
    inline constexpr const TitleId TitleId::Ldn         = { 0x0100000000000018ul };
    inline constexpr const TitleId TitleId::NvServices  = { 0x0100000000000019ul };
    inline constexpr const TitleId TitleId::Pcv         = { 0x010000000000001Aul };
    inline constexpr const TitleId TitleId::Ppc         = { 0x010000000000001Bul };
    inline constexpr const TitleId TitleId::NvnFlinger  = { 0x010000000000001Cul };
    inline constexpr const TitleId TitleId::Pcie        = { 0x010000000000001Dul };
    inline constexpr const TitleId TitleId::Account     = { 0x010000000000001Eul };
    inline constexpr const TitleId TitleId::Ns          = { 0x010000000000001Ful };
    inline constexpr const TitleId TitleId::Nfc         = { 0x0100000000000020ul };
    inline constexpr const TitleId TitleId::Psc         = { 0x0100000000000021ul };
    inline constexpr const TitleId TitleId::CapSrv      = { 0x0100000000000022ul };
    inline constexpr const TitleId TitleId::Am          = { 0x0100000000000023ul };
    inline constexpr const TitleId TitleId::Ssl         = { 0x0100000000000024ul };
    inline constexpr const TitleId TitleId::Nim         = { 0x0100000000000025ul };
    inline constexpr const TitleId TitleId::Cec         = { 0x0100000000000026ul };
    inline constexpr const TitleId TitleId::Tspm        = { 0x0100000000000027ul };
    inline constexpr const TitleId TitleId::Spl         = { 0x0100000000000028ul };
    inline constexpr const TitleId TitleId::Lbl         = { 0x0100000000000029ul };
    inline constexpr const TitleId TitleId::Btm         = { 0x010000000000002Aul };
    inline constexpr const TitleId TitleId::Erpt        = { 0x010000000000002Bul };
    inline constexpr const TitleId TitleId::Time        = { 0x010000000000002Cul };
    inline constexpr const TitleId TitleId::Vi          = { 0x010000000000002Dul };
    inline constexpr const TitleId TitleId::Pctl        = { 0x010000000000002Eul };
    inline constexpr const TitleId TitleId::Npns        = { 0x010000000000002Ful };
    inline constexpr const TitleId TitleId::Eupld       = { 0x0100000000000030ul };
    inline constexpr const TitleId TitleId::Arp         = { 0x0100000000000031ul };
    inline constexpr const TitleId TitleId::Glue        = { 0x0100000000000031ul };
    inline constexpr const TitleId TitleId::Eclct       = { 0x0100000000000032ul };
    inline constexpr const TitleId TitleId::Es          = { 0x0100000000000033ul };
    inline constexpr const TitleId TitleId::Fatal       = { 0x0100000000000034ul };
    inline constexpr const TitleId TitleId::Grc         = { 0x0100000000000035ul };
    inline constexpr const TitleId TitleId::Creport     = { 0x0100000000000036ul };
    inline constexpr const TitleId TitleId::Ro          = { 0x0100000000000037ul };
    inline constexpr const TitleId TitleId::Profiler    = { 0x0100000000000038ul };
    inline constexpr const TitleId TitleId::Sdb         = { 0x0100000000000039ul };
    inline constexpr const TitleId TitleId::Migration   = { 0x010000000000003Aul };
    inline constexpr const TitleId TitleId::Jit         = { 0x010000000000003Bul };
    inline constexpr const TitleId TitleId::JpegDec     = { 0x010000000000003Cul };
    inline constexpr const TitleId TitleId::SafeMode    = { 0x010000000000003Dul };
    inline constexpr const TitleId TitleId::Olsc        = { 0x010000000000003Eul };
    inline constexpr const TitleId TitleId::Dt          = { 0x010000000000003Ful };
    inline constexpr const TitleId TitleId::Nd          = { 0x0100000000000040ul };
    inline constexpr const TitleId TitleId::Ngct        = { 0x0100000000000041ul };

    inline constexpr const TitleId TitleId::SystemEnd   = { 0x01000000000007FFul };

    /* System Data Archives. */
    inline constexpr const TitleId TitleId::ArchiveStart                         = { 0x0100000000000800ul };
    inline constexpr const TitleId TitleId::ArchiveCertStore                     = { 0x0100000000000800ul };
    inline constexpr const TitleId TitleId::ArchiveErrorMessage                  = { 0x0100000000000801ul };
    inline constexpr const TitleId TitleId::ArchiveMiiModel                      = { 0x0100000000000802ul };
    inline constexpr const TitleId TitleId::ArchiveBrowserDll                    = { 0x0100000000000803ul };
    inline constexpr const TitleId TitleId::ArchiveHelp                          = { 0x0100000000000804ul };
    inline constexpr const TitleId TitleId::ArchiveSharedFont                    = { 0x0100000000000805ul };
    inline constexpr const TitleId TitleId::ArchiveNgWord                        = { 0x0100000000000806ul };
    inline constexpr const TitleId TitleId::ArchiveSsidList                      = { 0x0100000000000807ul };
    inline constexpr const TitleId TitleId::ArchiveDictionary                    = { 0x0100000000000808ul };
    inline constexpr const TitleId TitleId::ArchiveSystemVersion                 = { 0x0100000000000809ul };
    inline constexpr const TitleId TitleId::ArchiveAvatarImage                   = { 0x010000000000080Aul };
    inline constexpr const TitleId TitleId::ArchiveLocalNews                     = { 0x010000000000080Bul };
    inline constexpr const TitleId TitleId::ArchiveEula                          = { 0x010000000000080Cul };
    inline constexpr const TitleId TitleId::ArchiveUrlBlackList                  = { 0x010000000000080Dul };
    inline constexpr const TitleId TitleId::ArchiveTimeZoneBinar                 = { 0x010000000000080Eul };
    inline constexpr const TitleId TitleId::ArchiveCertStoreCruiser              = { 0x010000000000080Ful };
    inline constexpr const TitleId TitleId::ArchiveFontNintendoExtension         = { 0x0100000000000810ul };
    inline constexpr const TitleId TitleId::ArchiveFontStandard                  = { 0x0100000000000811ul };
    inline constexpr const TitleId TitleId::ArchiveFontKorean                    = { 0x0100000000000812ul };
    inline constexpr const TitleId TitleId::ArchiveFontChineseTraditional        = { 0x0100000000000813ul };
    inline constexpr const TitleId TitleId::ArchiveFontChineseSimple             = { 0x0100000000000814ul };
    inline constexpr const TitleId TitleId::ArchiveFontBfcpx                     = { 0x0100000000000815ul };
    inline constexpr const TitleId TitleId::ArchiveSystemUpdate                  = { 0x0100000000000816ul };

    inline constexpr const TitleId TitleId::ArchiveFirmwareDebugSettings         = { 0x0100000000000818ul };
    inline constexpr const TitleId TitleId::ArchiveBootImagePackage              = { 0x0100000000000819ul };
    inline constexpr const TitleId TitleId::ArchiveBootImagePackageSafe          = { 0x010000000000081Aul };
    inline constexpr const TitleId TitleId::ArchiveBootImagePackageExFat         = { 0x010000000000081Bul };
    inline constexpr const TitleId TitleId::ArchiveBootImagePackageExFatSafe     = { 0x010000000000081Cul };
    inline constexpr const TitleId TitleId::ArchiveFatalMessage                  = { 0x010000000000081Dul };
    inline constexpr const TitleId TitleId::ArchiveControllerIcon                = { 0x010000000000081Eul };
    inline constexpr const TitleId TitleId::ArchivePlatformConfigIcosa           = { 0x010000000000081Ful };
    inline constexpr const TitleId TitleId::ArchivePlatformConfigCopper          = { 0x0100000000000820ul };
    inline constexpr const TitleId TitleId::ArchivePlatformConfigHoag            = { 0x0100000000000821ul };
    inline constexpr const TitleId TitleId::ArchiveControllerFirmware            = { 0x0100000000000822ul };
    inline constexpr const TitleId TitleId::ArchiveNgWord2                       = { 0x0100000000000823ul };
    inline constexpr const TitleId TitleId::ArchivePlatformConfigIcosaMariko     = { 0x0100000000000824ul };
    inline constexpr const TitleId TitleId::ArchiveApplicationBlackList          = { 0x0100000000000825ul };
    inline constexpr const TitleId TitleId::ArchiveRebootlessSystemUpdateVersion = { 0x0100000000000826ul };
    inline constexpr const TitleId TitleId::ArchiveContentActionTable            = { 0x0100000000000827ul };

    inline constexpr const TitleId TitleId::ArchiveEnd                           = { 0x0100000000000FFFul };

    /* System Applets. */
    inline constexpr const TitleId TitleId::AppletStart           = { 0x0100000000001000ul };

    inline constexpr const TitleId TitleId::AppletQlaunch         = { 0x0100000000001000ul };
    inline constexpr const TitleId TitleId::AppletAuth            = { 0x0100000000001001ul };
    inline constexpr const TitleId TitleId::AppletCabinet         = { 0x0100000000001002ul };
    inline constexpr const TitleId TitleId::AppletController      = { 0x0100000000001003ul };
    inline constexpr const TitleId TitleId::AppletDataErase       = { 0x0100000000001004ul };
    inline constexpr const TitleId TitleId::AppletError           = { 0x0100000000001005ul };
    inline constexpr const TitleId TitleId::AppletNetConnect      = { 0x0100000000001006ul };
    inline constexpr const TitleId TitleId::AppletPlayerSelect    = { 0x0100000000001007ul };
    inline constexpr const TitleId TitleId::AppletSwkbd           = { 0x0100000000001008ul };
    inline constexpr const TitleId TitleId::AppletMiiEdit         = { 0x0100000000001009ul };
    inline constexpr const TitleId TitleId::AppletWeb             = { 0x010000000000100Aul };
    inline constexpr const TitleId TitleId::AppletShop            = { 0x010000000000100Bul };
    inline constexpr const TitleId TitleId::AppletOverlayDisp     = { 0x010000000000100Cul };
    inline constexpr const TitleId TitleId::AppletPhotoViewer     = { 0x010000000000100Dul };
    inline constexpr const TitleId TitleId::AppletSet             = { 0x010000000000100Eul };
    inline constexpr const TitleId TitleId::AppletOfflineWeb      = { 0x010000000000100Ful };
    inline constexpr const TitleId TitleId::AppletLoginShare      = { 0x0100000000001010ul };
    inline constexpr const TitleId TitleId::AppletWifiWebAuth     = { 0x0100000000001011ul };
    inline constexpr const TitleId TitleId::AppletStarter         = { 0x0100000000001012ul };
    inline constexpr const TitleId TitleId::AppletMyPage          = { 0x0100000000001013ul };
    inline constexpr const TitleId TitleId::AppletPlayReport      = { 0x0100000000001014ul };
    inline constexpr const TitleId TitleId::AppletMaintenanceMenu = { 0x0100000000001015ul };

    inline constexpr const TitleId TitleId::AppletGift            = { 0x010000000000101Aul };
    inline constexpr const TitleId TitleId::AppletDummyShop       = { 0x010000000000101Bul };
    inline constexpr const TitleId TitleId::AppletUserMigration   = { 0x010000000000101Cul };
    inline constexpr const TitleId TitleId::AppletEncounter       = { 0x010000000000101Dul };

    inline constexpr const TitleId TitleId::AppletStory           = { 0x0100000000001020ul };

    inline constexpr const TitleId TitleId::AppletEnd             = { 0x0100000000001FFFul };

    /* Debug Applets. */

    /* Debug Modules. */

    /* Factory Setup. */

    /* Applications. */
    inline constexpr const TitleId TitleId::ApplicationStart   = { 0x0100000000010000ul };
    inline constexpr const TitleId TitleId::ApplicationEnd     = { 0x01FFFFFFFFFFFFFFul };

    /* Atmosphere Extensions. */
    inline constexpr const TitleId TitleId::AtmosphereMitm = { 0x010041544D530000ul };

    inline constexpr bool operator==(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const TitleId &lhs, const TitleId &rhs) {
        return lhs.value >= rhs.value;
    }

    inline constexpr bool IsSystemTitleId(const TitleId &title_id) {
        return TitleId::SystemStart <= title_id && title_id <= TitleId::SystemEnd;
    }

    inline constexpr bool IsArchiveTitleId(const TitleId &title_id) {
        return TitleId::ArchiveStart <= title_id && title_id <= TitleId::ArchiveEnd;
    }

    inline constexpr bool IsAppletTitleId(const TitleId &title_id) {
        return TitleId::AppletStart <= title_id && title_id <= TitleId::AppletEnd;
    }

    inline constexpr bool IsApplicationTitleId(const TitleId &title_id) {
        return TitleId::ApplicationStart <= title_id && title_id <= TitleId::ApplicationEnd;
    }

    static_assert(sizeof(TitleId) == sizeof(u64) && std::is_pod<TitleId>::value, "TitleId definition!");

    /* Title Location. */
    struct TitleLocation {
        TitleId title_id;
        u8 storage_id;

        static constexpr TitleLocation Make(TitleId title_id, StorageId storage_id) {
            return { .title_id = title_id, .storage_id = static_cast<u8>(storage_id), };
        }
    };

    static_assert(sizeof(TitleLocation) == 0x10 && std::is_pod<TitleLocation>::value, "TitleLocation definition!");

}

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
#pragma once
#include <stratosphere/ncm/ncm_data_id.hpp>
#include <stratosphere/ncm/ncm_program_id.hpp>

namespace ams::ncm {

    struct SystemProgramId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }

        static const SystemProgramId Start;

        static const SystemProgramId Fs;
        static const SystemProgramId Loader;
        static const SystemProgramId Ncm;
        static const SystemProgramId Pm;
        static const SystemProgramId Sm;
        static const SystemProgramId Boot;
        static const SystemProgramId Usb;
        static const SystemProgramId Tma;
        static const SystemProgramId Boot2;
        static const SystemProgramId Settings;
        static const SystemProgramId Bus;
        static const SystemProgramId Bluetooth;
        static const SystemProgramId Bcat;
        static const SystemProgramId Dmnt;
        static const SystemProgramId Friends;
        static const SystemProgramId Nifm;
        static const SystemProgramId Ptm;
        static const SystemProgramId Shell;
        static const SystemProgramId BsdSockets;
        static const SystemProgramId Hid;
        static const SystemProgramId Audio;
        static const SystemProgramId LogManager;
        static const SystemProgramId Wlan;
        static const SystemProgramId Cs;
        static const SystemProgramId Ldn;
        static const SystemProgramId NvServices;
        static const SystemProgramId Pcv;
        static const SystemProgramId Ppc;
        static const SystemProgramId NvnFlinger;
        static const SystemProgramId Pcie;
        static const SystemProgramId Account;
        static const SystemProgramId Ns;
        static const SystemProgramId Nfc;
        static const SystemProgramId Psc;
        static const SystemProgramId CapSrv;
        static const SystemProgramId Am;
        static const SystemProgramId Ssl;
        static const SystemProgramId Nim;
        static const SystemProgramId Cec;
        static const SystemProgramId Tspm;
        static const SystemProgramId Spl;
        static const SystemProgramId Lbl;
        static const SystemProgramId Btm;
        static const SystemProgramId Erpt;
        static const SystemProgramId Time;
        static const SystemProgramId Vi;
        static const SystemProgramId Pctl;
        static const SystemProgramId Npns;
        static const SystemProgramId Eupld;
        static const SystemProgramId Arp;
        static const SystemProgramId Glue;
        static const SystemProgramId Eclct;
        static const SystemProgramId Es;
        static const SystemProgramId Fatal;
        static const SystemProgramId Grc;
        static const SystemProgramId Creport;
        static const SystemProgramId Ro;
        static const SystemProgramId Profiler;
        static const SystemProgramId Sdb;
        static const SystemProgramId Migration;
        static const SystemProgramId Jit;
        static const SystemProgramId JpegDec;
        static const SystemProgramId SafeMode;
        static const SystemProgramId Olsc;
        static const SystemProgramId Dt;
        static const SystemProgramId Nd;
        static const SystemProgramId Ngct;
        static const SystemProgramId Pgl;
        static const SystemProgramId Omm;
        static const SystemProgramId Eth;
        static const SystemProgramId Ngc;

        static const SystemProgramId End;

        static const SystemProgramId Manu;
        static const SystemProgramId Htc;
        static const SystemProgramId DmntGen2;
        static const SystemProgramId DevServer;
    };

    struct AtmosphereProgramId {
        u64 value;

        constexpr operator SystemProgramId() const {
            return { this->value };
        }

        constexpr operator ProgramId() const {
            return static_cast<SystemProgramId>(*this);
        }

        static const AtmosphereProgramId Mitm;
        static const AtmosphereProgramId AtmosphereLogManager;
        static const AtmosphereProgramId AtmosphereMemlet;
    };

    inline constexpr const AtmosphereProgramId AtmosphereProgramId::Mitm = { 0x010041544D530000ul };
    inline constexpr const AtmosphereProgramId AtmosphereProgramId::AtmosphereLogManager = { 0x0100000000000420ul };
    inline constexpr const AtmosphereProgramId AtmosphereProgramId::AtmosphereMemlet = { 0x0100000000000421ul };

    inline constexpr bool IsAtmosphereProgramId(const ProgramId &program_id) {
        return program_id == AtmosphereProgramId::Mitm || program_id == AtmosphereProgramId::AtmosphereLogManager || program_id == AtmosphereProgramId::AtmosphereMemlet;
    }

    inline constexpr bool IsSystemProgramId(const AtmosphereProgramId &) {
        return true;
    }

    inline constexpr const SystemProgramId SystemProgramId::Start = { 0x0100000000000000ul };

    inline constexpr const SystemProgramId SystemProgramId::Fs          = { 0x0100000000000000ul };
    inline constexpr const SystemProgramId SystemProgramId::Loader      = { 0x0100000000000001ul };
    inline constexpr const SystemProgramId SystemProgramId::Ncm         = { 0x0100000000000002ul };
    inline constexpr const SystemProgramId SystemProgramId::Pm          = { 0x0100000000000003ul };
    inline constexpr const SystemProgramId SystemProgramId::Sm          = { 0x0100000000000004ul };
    inline constexpr const SystemProgramId SystemProgramId::Boot        = { 0x0100000000000005ul };
    inline constexpr const SystemProgramId SystemProgramId::Usb         = { 0x0100000000000006ul };
    inline constexpr const SystemProgramId SystemProgramId::Tma         = { 0x0100000000000007ul };
    inline constexpr const SystemProgramId SystemProgramId::Boot2       = { 0x0100000000000008ul };
    inline constexpr const SystemProgramId SystemProgramId::Settings    = { 0x0100000000000009ul };
    inline constexpr const SystemProgramId SystemProgramId::Bus         = { 0x010000000000000Aul };
    inline constexpr const SystemProgramId SystemProgramId::Bluetooth   = { 0x010000000000000Bul };
    inline constexpr const SystemProgramId SystemProgramId::Bcat        = { 0x010000000000000Cul };
    inline constexpr const SystemProgramId SystemProgramId::Dmnt        = { 0x010000000000000Dul };
    inline constexpr const SystemProgramId SystemProgramId::Friends     = { 0x010000000000000Eul };
    inline constexpr const SystemProgramId SystemProgramId::Nifm        = { 0x010000000000000Ful };
    inline constexpr const SystemProgramId SystemProgramId::Ptm         = { 0x0100000000000010ul };
    inline constexpr const SystemProgramId SystemProgramId::Shell       = { 0x0100000000000011ul };
    inline constexpr const SystemProgramId SystemProgramId::BsdSockets  = { 0x0100000000000012ul };
    inline constexpr const SystemProgramId SystemProgramId::Hid         = { 0x0100000000000013ul };
    inline constexpr const SystemProgramId SystemProgramId::Audio       = { 0x0100000000000014ul };
    inline constexpr const SystemProgramId SystemProgramId::LogManager  = { 0x0100000000000015ul };
    inline constexpr const SystemProgramId SystemProgramId::Wlan        = { 0x0100000000000016ul };
    inline constexpr const SystemProgramId SystemProgramId::Cs          = { 0x0100000000000017ul };
    inline constexpr const SystemProgramId SystemProgramId::Ldn         = { 0x0100000000000018ul };
    inline constexpr const SystemProgramId SystemProgramId::NvServices  = { 0x0100000000000019ul };
    inline constexpr const SystemProgramId SystemProgramId::Pcv         = { 0x010000000000001Aul };
    inline constexpr const SystemProgramId SystemProgramId::Ppc         = { 0x010000000000001Bul };
    inline constexpr const SystemProgramId SystemProgramId::NvnFlinger  = { 0x010000000000001Cul };
    inline constexpr const SystemProgramId SystemProgramId::Pcie        = { 0x010000000000001Dul };
    inline constexpr const SystemProgramId SystemProgramId::Account     = { 0x010000000000001Eul };
    inline constexpr const SystemProgramId SystemProgramId::Ns          = { 0x010000000000001Ful };
    inline constexpr const SystemProgramId SystemProgramId::Nfc         = { 0x0100000000000020ul };
    inline constexpr const SystemProgramId SystemProgramId::Psc         = { 0x0100000000000021ul };
    inline constexpr const SystemProgramId SystemProgramId::CapSrv      = { 0x0100000000000022ul };
    inline constexpr const SystemProgramId SystemProgramId::Am          = { 0x0100000000000023ul };
    inline constexpr const SystemProgramId SystemProgramId::Ssl         = { 0x0100000000000024ul };
    inline constexpr const SystemProgramId SystemProgramId::Nim         = { 0x0100000000000025ul };
    inline constexpr const SystemProgramId SystemProgramId::Cec         = { 0x0100000000000026ul };
    inline constexpr const SystemProgramId SystemProgramId::Tspm        = { 0x0100000000000027ul };
    inline constexpr const SystemProgramId SystemProgramId::Spl         = { 0x0100000000000028ul };
    inline constexpr const SystemProgramId SystemProgramId::Lbl         = { 0x0100000000000029ul };
    inline constexpr const SystemProgramId SystemProgramId::Btm         = { 0x010000000000002Aul };
    inline constexpr const SystemProgramId SystemProgramId::Erpt        = { 0x010000000000002Bul };
    inline constexpr const SystemProgramId SystemProgramId::Time        = { 0x010000000000002Cul };
    inline constexpr const SystemProgramId SystemProgramId::Vi          = { 0x010000000000002Dul };
    inline constexpr const SystemProgramId SystemProgramId::Pctl        = { 0x010000000000002Eul };
    inline constexpr const SystemProgramId SystemProgramId::Npns        = { 0x010000000000002Ful };
    inline constexpr const SystemProgramId SystemProgramId::Eupld       = { 0x0100000000000030ul };
    inline constexpr const SystemProgramId SystemProgramId::Arp         = { 0x0100000000000031ul };
    inline constexpr const SystemProgramId SystemProgramId::Glue        = { 0x0100000000000031ul };
    inline constexpr const SystemProgramId SystemProgramId::Eclct       = { 0x0100000000000032ul };
    inline constexpr const SystemProgramId SystemProgramId::Es          = { 0x0100000000000033ul };
    inline constexpr const SystemProgramId SystemProgramId::Fatal       = { 0x0100000000000034ul };
    inline constexpr const SystemProgramId SystemProgramId::Grc         = { 0x0100000000000035ul };
    inline constexpr const SystemProgramId SystemProgramId::Creport     = { 0x0100000000000036ul };
    inline constexpr const SystemProgramId SystemProgramId::Ro          = { 0x0100000000000037ul };
    inline constexpr const SystemProgramId SystemProgramId::Profiler    = { 0x0100000000000038ul };
    inline constexpr const SystemProgramId SystemProgramId::Sdb         = { 0x0100000000000039ul };
    inline constexpr const SystemProgramId SystemProgramId::Migration   = { 0x010000000000003Aul };
    inline constexpr const SystemProgramId SystemProgramId::Jit         = { 0x010000000000003Bul };
    inline constexpr const SystemProgramId SystemProgramId::JpegDec     = { 0x010000000000003Cul };
    inline constexpr const SystemProgramId SystemProgramId::SafeMode    = { 0x010000000000003Dul };
    inline constexpr const SystemProgramId SystemProgramId::Olsc        = { 0x010000000000003Eul };
    inline constexpr const SystemProgramId SystemProgramId::Dt          = { 0x010000000000003Ful };
    inline constexpr const SystemProgramId SystemProgramId::Nd          = { 0x0100000000000040ul };
    inline constexpr const SystemProgramId SystemProgramId::Ngct        = { 0x0100000000000041ul };
    inline constexpr const SystemProgramId SystemProgramId::Pgl         = { 0x0100000000000042ul };
    inline constexpr const SystemProgramId SystemProgramId::Omm         = { 0x0100000000000045ul };
    inline constexpr const SystemProgramId SystemProgramId::Eth         = { 0x0100000000000046ul };
    inline constexpr const SystemProgramId SystemProgramId::Ngc         = { 0x0100000000000050ul };

    inline constexpr const SystemProgramId SystemProgramId::End   = { 0x01000000000007FFul };

    inline constexpr const SystemProgramId SystemProgramId::Manu        = { 0x010000000000B14Aul };
    inline constexpr const SystemProgramId SystemProgramId::Htc         = { 0x010000000000B240ul };
    inline constexpr const SystemProgramId SystemProgramId::DmntGen2    = { 0x010000000000D609ul };
    inline constexpr const SystemProgramId SystemProgramId::DevServer   = { 0x010000000000D623ul };

    inline constexpr bool IsSystemProgramId(const ProgramId &program_id) {
        return (SystemProgramId::Start <= program_id && program_id <= SystemProgramId::End) || IsAtmosphereProgramId(program_id);
    }

    inline constexpr bool IsSystemProgramId(const SystemProgramId &) {
        return true;
    }

    struct SystemDataId {
        u64 value;

        constexpr operator DataId() const {
            return { this->value };
        }

        constexpr inline bool operator==(const SystemDataId &) const = default;
        constexpr inline bool operator!=(const SystemDataId &) const = default;

        static const SystemDataId Start;

        static const SystemDataId CertStore;
        static const SystemDataId ErrorMessage;
        static const SystemDataId MiiModel;
        static const SystemDataId BrowserDll;
        static const SystemDataId Help;
        static const SystemDataId SharedFont;
        static const SystemDataId NgWord;
        static const SystemDataId SsidList;
        static const SystemDataId Dictionary;
        static const SystemDataId SystemVersion;
        static const SystemDataId AvatarImage;
        static const SystemDataId LocalNews;
        static const SystemDataId Eula;
        static const SystemDataId UrlBlackList;
        static const SystemDataId TimeZoneBinar;
        static const SystemDataId CertStoreCruiser;
        static const SystemDataId FontNintendoExtension;
        static const SystemDataId FontStandard;
        static const SystemDataId FontKorean;
        static const SystemDataId FontChineseTraditional;
        static const SystemDataId FontChineseSimple;
        static const SystemDataId FontBfcpx;
        static const SystemDataId SystemUpdate;

        static const SystemDataId FirmwareDebugSettings;
        static const SystemDataId BootImagePackage;
        static const SystemDataId BootImagePackageSafe;
        static const SystemDataId BootImagePackageExFat;
        static const SystemDataId BootImagePackageExFatSafe;
        static const SystemDataId FatalMessage;
        static const SystemDataId ControllerIcon;
        static const SystemDataId PlatformConfigIcosa;
        static const SystemDataId PlatformConfigCopper;
        static const SystemDataId PlatformConfigHoag;
        static const SystemDataId ControllerFirmware;
        static const SystemDataId NgWord2;
        static const SystemDataId PlatformConfigIcosaMariko;
        static const SystemDataId ApplicationBlackList;
        static const SystemDataId RebootlessSystemUpdateVersion;
        static const SystemDataId ContentActionTable;

        static const SystemDataId PlatformConfigCalcio;

        static const SystemDataId PlatformConfigAula;

        static const SystemDataId End;
    };

    inline constexpr const SystemDataId SystemDataId::Start                         = { 0x0100000000000800ul };
    inline constexpr const SystemDataId SystemDataId::CertStore                     = { 0x0100000000000800ul };
    inline constexpr const SystemDataId SystemDataId::ErrorMessage                  = { 0x0100000000000801ul };
    inline constexpr const SystemDataId SystemDataId::MiiModel                      = { 0x0100000000000802ul };
    inline constexpr const SystemDataId SystemDataId::BrowserDll                    = { 0x0100000000000803ul };
    inline constexpr const SystemDataId SystemDataId::Help                          = { 0x0100000000000804ul };
    inline constexpr const SystemDataId SystemDataId::SharedFont                    = { 0x0100000000000805ul };
    inline constexpr const SystemDataId SystemDataId::NgWord                        = { 0x0100000000000806ul };
    inline constexpr const SystemDataId SystemDataId::SsidList                      = { 0x0100000000000807ul };
    inline constexpr const SystemDataId SystemDataId::Dictionary                    = { 0x0100000000000808ul };
    inline constexpr const SystemDataId SystemDataId::SystemVersion                 = { 0x0100000000000809ul };
    inline constexpr const SystemDataId SystemDataId::AvatarImage                   = { 0x010000000000080Aul };
    inline constexpr const SystemDataId SystemDataId::LocalNews                     = { 0x010000000000080Bul };
    inline constexpr const SystemDataId SystemDataId::Eula                          = { 0x010000000000080Cul };
    inline constexpr const SystemDataId SystemDataId::UrlBlackList                  = { 0x010000000000080Dul };
    inline constexpr const SystemDataId SystemDataId::TimeZoneBinar                 = { 0x010000000000080Eul };
    inline constexpr const SystemDataId SystemDataId::CertStoreCruiser              = { 0x010000000000080Ful };
    inline constexpr const SystemDataId SystemDataId::FontNintendoExtension         = { 0x0100000000000810ul };
    inline constexpr const SystemDataId SystemDataId::FontStandard                  = { 0x0100000000000811ul };
    inline constexpr const SystemDataId SystemDataId::FontKorean                    = { 0x0100000000000812ul };
    inline constexpr const SystemDataId SystemDataId::FontChineseTraditional        = { 0x0100000000000813ul };
    inline constexpr const SystemDataId SystemDataId::FontChineseSimple             = { 0x0100000000000814ul };
    inline constexpr const SystemDataId SystemDataId::FontBfcpx                     = { 0x0100000000000815ul };
    inline constexpr const SystemDataId SystemDataId::SystemUpdate                  = { 0x0100000000000816ul };

    inline constexpr const SystemDataId SystemDataId::FirmwareDebugSettings         = { 0x0100000000000818ul };
    inline constexpr const SystemDataId SystemDataId::BootImagePackage              = { 0x0100000000000819ul };
    inline constexpr const SystemDataId SystemDataId::BootImagePackageSafe          = { 0x010000000000081Aul };
    inline constexpr const SystemDataId SystemDataId::BootImagePackageExFat         = { 0x010000000000081Bul };
    inline constexpr const SystemDataId SystemDataId::BootImagePackageExFatSafe     = { 0x010000000000081Cul };
    inline constexpr const SystemDataId SystemDataId::FatalMessage                  = { 0x010000000000081Dul };
    inline constexpr const SystemDataId SystemDataId::ControllerIcon                = { 0x010000000000081Eul };
    inline constexpr const SystemDataId SystemDataId::PlatformConfigIcosa           = { 0x010000000000081Ful };
    inline constexpr const SystemDataId SystemDataId::PlatformConfigCopper          = { 0x0100000000000820ul };
    inline constexpr const SystemDataId SystemDataId::PlatformConfigHoag            = { 0x0100000000000821ul };
    inline constexpr const SystemDataId SystemDataId::ControllerFirmware            = { 0x0100000000000822ul };
    inline constexpr const SystemDataId SystemDataId::NgWord2                       = { 0x0100000000000823ul };
    inline constexpr const SystemDataId SystemDataId::PlatformConfigIcosaMariko     = { 0x0100000000000824ul };
    inline constexpr const SystemDataId SystemDataId::ApplicationBlackList          = { 0x0100000000000825ul };
    inline constexpr const SystemDataId SystemDataId::RebootlessSystemUpdateVersion = { 0x0100000000000826ul };
    inline constexpr const SystemDataId SystemDataId::ContentActionTable            = { 0x0100000000000827ul };

    inline constexpr const SystemDataId SystemDataId::PlatformConfigCalcio          = { 0x0100000000000829ul };

    inline constexpr const SystemDataId SystemDataId::PlatformConfigAula            = { 0x0100000000000831ul };

    inline constexpr const SystemDataId SystemDataId::End                           = { 0x0100000000000FFFul };

    inline constexpr bool IsSystemDataId(const DataId &data_id) {
        return SystemDataId::Start <= data_id && data_id <= SystemDataId::End;
    }

    inline constexpr bool IsSystemDataId(const SystemDataId &) {
        return true;
    }

    struct SystemUpdateId {
        u64 value;

        constexpr operator DataId() const {
            return { this->value };
        }

        constexpr inline bool operator==(const SystemUpdateId &) const = default;
        constexpr inline bool operator!=(const SystemUpdateId &) const = default;
    };

    struct SystemAppletId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }

        constexpr inline bool operator==(const SystemAppletId &) const = default;
        constexpr inline bool operator!=(const SystemAppletId &) const = default;

        static const SystemAppletId Start;

        static const SystemAppletId Qlaunch;
        static const SystemAppletId Auth;
        static const SystemAppletId Cabinet;
        static const SystemAppletId Controller;
        static const SystemAppletId DataErase;
        static const SystemAppletId Error;
        static const SystemAppletId NetConnect;
        static const SystemAppletId PlayerSelect;
        static const SystemAppletId Swkbd;
        static const SystemAppletId MiiEdit;
        static const SystemAppletId Web;
        static const SystemAppletId Shop;
        static const SystemAppletId OverlayDisp;
        static const SystemAppletId PhotoViewer;
        static const SystemAppletId Set;
        static const SystemAppletId OfflineWeb;
        static const SystemAppletId LoginShare;
        static const SystemAppletId WifiWebAuth;
        static const SystemAppletId Starter;
        static const SystemAppletId MyPage;
        static const SystemAppletId PlayReport;
        static const SystemAppletId MaintenanceMenu;

        static const SystemAppletId Gift;
        static const SystemAppletId DummyShop;
        static const SystemAppletId UserMigration;
        static const SystemAppletId Encounter;

        static const SystemAppletId Story;

        static const SystemAppletId End;
    };

    inline constexpr const SystemAppletId SystemAppletId::Start           = { 0x0100000000001000ul };

    inline constexpr const SystemAppletId SystemAppletId::Qlaunch         = { 0x0100000000001000ul };
    inline constexpr const SystemAppletId SystemAppletId::Auth            = { 0x0100000000001001ul };
    inline constexpr const SystemAppletId SystemAppletId::Cabinet         = { 0x0100000000001002ul };
    inline constexpr const SystemAppletId SystemAppletId::Controller      = { 0x0100000000001003ul };
    inline constexpr const SystemAppletId SystemAppletId::DataErase       = { 0x0100000000001004ul };
    inline constexpr const SystemAppletId SystemAppletId::Error           = { 0x0100000000001005ul };
    inline constexpr const SystemAppletId SystemAppletId::NetConnect      = { 0x0100000000001006ul };
    inline constexpr const SystemAppletId SystemAppletId::PlayerSelect    = { 0x0100000000001007ul };
    inline constexpr const SystemAppletId SystemAppletId::Swkbd           = { 0x0100000000001008ul };
    inline constexpr const SystemAppletId SystemAppletId::MiiEdit         = { 0x0100000000001009ul };
    inline constexpr const SystemAppletId SystemAppletId::Web             = { 0x010000000000100Aul };
    inline constexpr const SystemAppletId SystemAppletId::Shop            = { 0x010000000000100Bul };
    inline constexpr const SystemAppletId SystemAppletId::OverlayDisp     = { 0x010000000000100Cul };
    inline constexpr const SystemAppletId SystemAppletId::PhotoViewer     = { 0x010000000000100Dul };
    inline constexpr const SystemAppletId SystemAppletId::Set             = { 0x010000000000100Eul };
    inline constexpr const SystemAppletId SystemAppletId::OfflineWeb      = { 0x010000000000100Ful };
    inline constexpr const SystemAppletId SystemAppletId::LoginShare      = { 0x0100000000001010ul };
    inline constexpr const SystemAppletId SystemAppletId::WifiWebAuth     = { 0x0100000000001011ul };
    inline constexpr const SystemAppletId SystemAppletId::Starter         = { 0x0100000000001012ul };
    inline constexpr const SystemAppletId SystemAppletId::MyPage          = { 0x0100000000001013ul };
    inline constexpr const SystemAppletId SystemAppletId::PlayReport      = { 0x0100000000001014ul };
    inline constexpr const SystemAppletId SystemAppletId::MaintenanceMenu = { 0x0100000000001015ul };

    inline constexpr const SystemAppletId SystemAppletId::Gift            = { 0x010000000000101Aul };
    inline constexpr const SystemAppletId SystemAppletId::DummyShop       = { 0x010000000000101Bul };
    inline constexpr const SystemAppletId SystemAppletId::UserMigration   = { 0x010000000000101Cul };
    inline constexpr const SystemAppletId SystemAppletId::Encounter       = { 0x010000000000101Dul };

    inline constexpr const SystemAppletId SystemAppletId::Story           = { 0x0100000000001020ul };

    inline constexpr const SystemAppletId SystemAppletId::End             = { 0x0100000000001FFFul };

    inline constexpr bool IsSystemAppletId(const ProgramId &program_id) {
        return SystemAppletId::Start <= program_id && program_id <= SystemAppletId::End;
    }

    inline constexpr bool IsSystemAppletId(const SystemAppletId &) {
        return true;
    }

    struct SystemDebugAppletId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }

        constexpr inline bool operator==(const SystemDebugAppletId &) const = default;
        constexpr inline bool operator!=(const SystemDebugAppletId &) const = default;

        static const SystemDebugAppletId Start;

        static const SystemDebugAppletId SnapShotDumper;

        static const SystemDebugAppletId End;
    };

    inline constexpr const SystemDebugAppletId SystemDebugAppletId::Start           = { 0x0100000000002000ul };

    inline constexpr const SystemDebugAppletId SystemDebugAppletId::SnapShotDumper  = { 0x0100000000002071ul };

    inline constexpr const SystemDebugAppletId SystemDebugAppletId::End             = { 0x0100000000002FFFul };

    inline constexpr bool IsSystemDebugAppletId(const ProgramId &program_id) {
        return SystemDebugAppletId::Start <= program_id && program_id <= SystemDebugAppletId::End;
    }

    inline constexpr bool IsSystemDebugAppletId(const SystemDebugAppletId &) {
        return true;
    }

    struct LibraryAppletId {
        u64 value;

        constexpr operator SystemAppletId() const {
            return { this->value };
        }

        constexpr operator ProgramId() const {
            return static_cast<SystemAppletId>(*this);
        }

        constexpr inline bool operator==(const LibraryAppletId &) const = default;
        constexpr inline bool operator!=(const LibraryAppletId &) const = default;

        static const LibraryAppletId Auth;
        static const LibraryAppletId Controller;
        static const LibraryAppletId Error;
        static const LibraryAppletId PlayerSelect;
        static const LibraryAppletId Swkbd;
        static const LibraryAppletId Web;
        static const LibraryAppletId Shop;
        static const LibraryAppletId PhotoViewer;
        static const LibraryAppletId OfflineWeb;
        static const LibraryAppletId LoginShare;
        static const LibraryAppletId WifiWebAuth;
        static const LibraryAppletId MyPage;

    };

    inline constexpr const LibraryAppletId LibraryAppletId::Auth         = { SystemAppletId::Auth.value          };
    inline constexpr const LibraryAppletId LibraryAppletId::Controller   = { SystemAppletId::Controller.value    };
    inline constexpr const LibraryAppletId LibraryAppletId::Error        = { SystemAppletId::Error.value         };
    inline constexpr const LibraryAppletId LibraryAppletId::PlayerSelect = { SystemAppletId::PlayerSelect.value  };
    inline constexpr const LibraryAppletId LibraryAppletId::Swkbd        = { SystemAppletId::Swkbd.value         };
    inline constexpr const LibraryAppletId LibraryAppletId::Web          = { SystemAppletId::Web.value           };
    inline constexpr const LibraryAppletId LibraryAppletId::Shop         = { SystemAppletId::Shop.value          };
    inline constexpr const LibraryAppletId LibraryAppletId::PhotoViewer  = { SystemAppletId::PhotoViewer.value   };
    inline constexpr const LibraryAppletId LibraryAppletId::OfflineWeb   = { SystemAppletId::OfflineWeb.value    };
    inline constexpr const LibraryAppletId LibraryAppletId::LoginShare   = { SystemAppletId::LoginShare.value    };
    inline constexpr const LibraryAppletId LibraryAppletId::WifiWebAuth  = { SystemAppletId::WifiWebAuth.value   };
    inline constexpr const LibraryAppletId LibraryAppletId::MyPage       = { SystemAppletId::MyPage.value        };

    inline constexpr bool IsLibraryAppletId(const ProgramId &id) {
        return id == LibraryAppletId::Auth         ||
               id == LibraryAppletId::Controller   ||
               id == LibraryAppletId::Error        ||
               id == LibraryAppletId::PlayerSelect ||
               id == LibraryAppletId::Swkbd        ||
               id == LibraryAppletId::Web          ||
               id == LibraryAppletId::Shop         ||
               id == LibraryAppletId::PhotoViewer  ||
               id == LibraryAppletId::OfflineWeb   ||
               id == LibraryAppletId::LoginShare   ||
               id == LibraryAppletId::WifiWebAuth  ||
               id == LibraryAppletId::MyPage;
    }

    inline constexpr bool IsLibraryAppletId(const LibraryAppletId &) {
        return true;
    }

    struct WebAppletId {
        u64 value;

        constexpr operator LibraryAppletId() const {
            return { this->value };
        }

        constexpr operator SystemAppletId() const {
            return static_cast<LibraryAppletId>(*this);
        }

        constexpr operator ProgramId() const {
            return static_cast<SystemAppletId>(*this);
        }

        constexpr inline bool operator==(const WebAppletId &) const = default;
        constexpr inline bool operator!=(const WebAppletId &) const = default;

        static const WebAppletId Web;
        static const WebAppletId Shop;
        static const WebAppletId OfflineWeb;
        static const WebAppletId LoginShare;
        static const WebAppletId WifiWebAuth;
    };

    inline constexpr const WebAppletId WebAppletId::Web          = { LibraryAppletId::Web.value           };
    inline constexpr const WebAppletId WebAppletId::Shop         = { LibraryAppletId::Shop.value          };
    inline constexpr const WebAppletId WebAppletId::OfflineWeb   = { LibraryAppletId::OfflineWeb.value    };
    inline constexpr const WebAppletId WebAppletId::LoginShare   = { LibraryAppletId::LoginShare.value    };
    inline constexpr const WebAppletId WebAppletId::WifiWebAuth  = { LibraryAppletId::WifiWebAuth.value   };

    inline constexpr bool IsWebAppletId(const ProgramId &id) {
        return id == WebAppletId::Web          ||
               id == WebAppletId::Shop         ||
               id == WebAppletId::OfflineWeb   ||
               id == WebAppletId::LoginShare   ||
               id == WebAppletId::WifiWebAuth;
    }

    inline constexpr bool IsWebAppletId(const WebAppletId &) {
        return true;
    }

    struct SystemApplicationId {
        u64 value;

        constexpr operator ProgramId() const {
            return { this->value };
        }

        constexpr inline bool operator==(const SystemApplicationId &) const = default;
        constexpr inline bool operator!=(const SystemApplicationId &) const = default;
    };

}

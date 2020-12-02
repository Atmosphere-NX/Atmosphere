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
struct MinimumProgramVersion {
    ncm::ProgramId program_id;
    u32 version;
};

constexpr u32 MakeSystemVersion(u32 major, u32 minor, u32 micro) {
    return (major << 26) | (minor << 20) | (micro << 16);
}

constexpr MinimumProgramVersion g_MinimumProgramVersions810[] = {
    {ncm::SystemProgramId::Settings,    1},
    {ncm::SystemProgramId::Bus,         1},
    {ncm::SystemProgramId::Audio,       1},
    {ncm::SystemProgramId::NvServices,  1},
    {ncm::SystemProgramId::Ns,          1},
    {ncm::SystemProgramId::Ssl,         1},
    {ncm::SystemProgramId::Es,          1},
    {ncm::SystemProgramId::Creport,     1},
    {ncm::SystemProgramId::Ro,          1},
};
constexpr size_t g_MinimumProgramVersionsCount810 = util::size(g_MinimumProgramVersions810);

constexpr MinimumProgramVersion g_MinimumProgramVersions900[] = {
    /* All non-Development System Modules. */
    {ncm::SystemProgramId::Usb,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Tma,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Boot2,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Settings,    MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bus,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bluetooth,   MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bcat,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Dmnt,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Friends,     MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Nifm,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ptm,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Shell,       MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::BsdSockets,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Hid,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Audio,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::LogManager,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Wlan,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Cs,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Ldn,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::NvServices,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pcv,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ppc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::NvnFlinger,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pcie,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Account,     MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ns,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Nfc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Psc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::CapSrv,      MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Am,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ssl,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Nim,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Cec,         MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Tspm,        MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Spl,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Lbl,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Btm,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Erpt,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Time,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Vi,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pctl,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Npns,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Eupld,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Glue,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Eclct,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Es,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Fatal,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Grc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Creport,     MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ro,          MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Profiler,    MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Sdb,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Migration,   MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Jit,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::JpegDec,     MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::SafeMode,    MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Olsc,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Dt,          MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Nd,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Ngct,        MakeSystemVersion(9, 0, 0)},

    /* All Web Applets. */
    {ncm::WebAppletId::Web,           MakeSystemVersion(9, 0, 0)},
    {ncm::WebAppletId::Shop,          MakeSystemVersion(9, 0, 0)},
    {ncm::WebAppletId::OfflineWeb,    MakeSystemVersion(9, 0, 0)},
    {ncm::WebAppletId::LoginShare,    MakeSystemVersion(9, 0, 0)},
    {ncm::WebAppletId::WifiWebAuth,   MakeSystemVersion(9, 0, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount900 = util::size(g_MinimumProgramVersions900);

constexpr MinimumProgramVersion g_MinimumProgramVersions910[] = {
    /* All non-Development System Modules. */
    {ncm::SystemProgramId::Usb,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Tma,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Boot2,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Settings,    MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bus,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bluetooth,   MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Bcat,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Dmnt,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Friends,     MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Nifm,        MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Ptm,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::SystemProgramId::Shell,       MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::BsdSockets,  MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Hid,         MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Audio,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::LogManager,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Wlan,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Cs,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Ldn,         MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::NvServices,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pcv,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Ppc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::NvnFlinger,  MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pcie,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Account,     MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Ns,          MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Nfc,         MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Psc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::CapSrv,      MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Am,          MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Ssl,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Nim,         MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Cec,         MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Tspm,        MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Spl,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Lbl,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Btm,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Erpt,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Time,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Vi,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Pctl,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Npns,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Eupld,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Glue,        MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Eclct,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Es,          MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Fatal,       MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Grc,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Creport,     MakeSystemVersion(9, 1, 0)},
    {ncm::SystemProgramId::Ro,          MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Profiler,    MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Sdb,         MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Migration,   MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Jit,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::JpegDec,     MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::SafeMode,    MakeSystemVersion(9, 0, 0)},
    {ncm::SystemProgramId::Olsc,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::SystemProgramId::Dt,          MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::SystemProgramId::Nd,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::SystemProgramId::Ngct,        MakeSystemVersion(9, 1, 0)},

    /* All Web Applets. */
    {ncm::WebAppletId::Web,           MakeSystemVersion(9, 1, 0)},
    {ncm::WebAppletId::Shop,          MakeSystemVersion(9, 1, 0)},
    {ncm::WebAppletId::OfflineWeb,    MakeSystemVersion(9, 1, 0)},
    {ncm::WebAppletId::LoginShare,    MakeSystemVersion(9, 1, 0)},
    {ncm::WebAppletId::WifiWebAuth,   MakeSystemVersion(9, 1, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount910 = util::size(g_MinimumProgramVersions910);

constexpr MinimumProgramVersion g_MinimumProgramVersions1000[] = {
    /* All non-Development System Modules. */
    {ncm::SystemProgramId::Usb,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Tma,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Boot2,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Settings,    MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Bus,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Bluetooth,   MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Bcat,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Dmnt,        MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Friends,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Nifm,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ptm,         MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Shell,       MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::BsdSockets,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Hid,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Audio,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::LogManager,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Wlan,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Cs,          MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Ldn,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::NvServices,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Pcv,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ppc,         MakeSystemVersion( 9, 0, 0)},
    {ncm::SystemProgramId::NvnFlinger,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Pcie,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Account,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ns,          MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Nfc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Psc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::CapSrv,      MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Am,          MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ssl,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Nim,         MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Cec,         MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Tspm,        MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Spl,         MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Lbl,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Btm,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Erpt,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Time,        MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Vi,          MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Pctl,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Npns,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Eupld,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Glue,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Eclct,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Es,          MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Fatal,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Grc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Creport,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ro,          MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Profiler,    MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Sdb,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Migration,   MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Jit,         MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::JpegDec,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::SafeMode,    MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Olsc,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Dt,          MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Nd,          MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Ngct,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Pgl,         MakeSystemVersion(10, 0, 0)}, */

    /* All Web Applets. */
    {ncm::WebAppletId::Web,           MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::Shop,          MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::OfflineWeb,    MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::LoginShare,    MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::WifiWebAuth,   MakeSystemVersion(10, 0, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount1000 = util::size(g_MinimumProgramVersions1000);

constexpr MinimumProgramVersion g_MinimumProgramVersions1010[] = {
    /* All non-Development System Modules. */
    {ncm::SystemProgramId::Usb,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Tma,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Boot2,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Settings,    MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Bus,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Bluetooth,   MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Bcat,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Dmnt,        MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Friends,     MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Nifm,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ptm,         MakeSystemVersion(10, 0, 2)},
 /* {ncm::SystemProgramId::Shell,       MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::BsdSockets,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Hid,         MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Audio,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::LogManager,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Wlan,        MakeSystemVersion(10, 1, 0)},
 /* {ncm::SystemProgramId::Cs,          MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Ldn,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::NvServices,  MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Pcv,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ppc,         MakeSystemVersion( 9, 0, 0)},
    {ncm::SystemProgramId::NvnFlinger,  MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Pcie,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Account,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ns,          MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Nfc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Psc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::CapSrv,      MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Am,          MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Ssl,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Nim,         MakeSystemVersion(10, 1, 0)},
 /* {ncm::SystemProgramId::Cec,         MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Tspm,        MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Spl,         MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Lbl,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Btm,         MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Erpt,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Time,        MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Vi,          MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Pctl,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Npns,        MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Eupld,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Glue,        MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Eclct,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Es,          MakeSystemVersion(10, 1, 0)},
    {ncm::SystemProgramId::Fatal,       MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Grc,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Creport,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Ro,          MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Profiler,    MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Sdb,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Migration,   MakeSystemVersion(10, 1, 0)},
 /* {ncm::SystemProgramId::Jit,         MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::JpegDec,     MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::SafeMode,    MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Olsc,        MakeSystemVersion(10, 1, 0)},
 /* {ncm::SystemProgramId::Dt,          MakeSystemVersion(10, 0, 0)}, */
 /* {ncm::SystemProgramId::Nd,          MakeSystemVersion(10, 0, 0)}, */
    {ncm::SystemProgramId::Ngct,        MakeSystemVersion(10, 0, 0)},
 /* {ncm::SystemProgramId::Pgl,         MakeSystemVersion(10, 0, 0)}, */

    /* All Web Applets. */
    {ncm::WebAppletId::Web,           MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::Shop,          MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::OfflineWeb,    MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::LoginShare,    MakeSystemVersion(10, 0, 0)},
    {ncm::WebAppletId::WifiWebAuth,   MakeSystemVersion(10, 0, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount1010 = util::size(g_MinimumProgramVersions1010);

constexpr MinimumProgramVersion g_MinimumProgramVersions1100[] = {
    /* All non-Development System Modules. */
    {ncm::SystemProgramId::Usb,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Tma,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Boot2,       MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Settings,    MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Bus,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Bluetooth,   MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Bcat,        MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Dmnt,        MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Friends,     MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Nifm,        MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Ptm,         MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Shell,       MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::BsdSockets,  MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Hid,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Audio,       MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::LogManager,  MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Wlan,        MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Cs,          MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Ldn,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::NvServices,  MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Pcv,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Ppc,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::NvnFlinger,  MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Pcie,        MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Account,     MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Ns,          MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Nfc,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Psc,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::CapSrv,      MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Am,          MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Ssl,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Nim,         MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Cec,         MakeSystemVersion(11, 0, 0)}, */
 /* {ncm::SystemProgramId::Tspm,        MakeSystemVersion(11, 0, 0)}, */
 /* {ncm::SystemProgramId::Spl,         MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Lbl,         MakeSystemVersion(10, 0, 0)},
    {ncm::SystemProgramId::Btm,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Erpt,        MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Time,        MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Vi,          MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Pctl,        MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Npns,        MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Eupld,       MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Glue,        MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Eclct,       MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Es,          MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Fatal,       MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Grc,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Creport,     MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Ro,          MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Profiler,    MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Sdb,         MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Migration,   MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Jit,         MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::JpegDec,     MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::SafeMode,    MakeSystemVersion(11, 0, 0)},
    {ncm::SystemProgramId::Olsc,        MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Dt,          MakeSystemVersion(11, 0, 0)}, */
 /* {ncm::SystemProgramId::Nd,          MakeSystemVersion(11, 0, 0)}, */
    {ncm::SystemProgramId::Ngct,        MakeSystemVersion(11, 0, 0)},
 /* {ncm::SystemProgramId::Pgl,         MakeSystemVersion(11, 0, 0)}, */

    /* All Web Applets. */
    {ncm::WebAppletId::Web,           MakeSystemVersion(11, 0, 0)},
    {ncm::WebAppletId::Shop,          MakeSystemVersion(11, 0, 0)},
    {ncm::WebAppletId::OfflineWeb,    MakeSystemVersion(11, 0, 0)},
    {ncm::WebAppletId::LoginShare,    MakeSystemVersion(11, 0, 0)},
    {ncm::WebAppletId::WifiWebAuth,   MakeSystemVersion(11, 0, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount1100 = util::size(g_MinimumProgramVersions1100);

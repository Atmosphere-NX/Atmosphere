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
    {ncm::ProgramId::Settings,    1},
    {ncm::ProgramId::Bus,         1},
    {ncm::ProgramId::Audio,       1},
    {ncm::ProgramId::NvServices,  1},
    {ncm::ProgramId::Ns,          1},
    {ncm::ProgramId::Ssl,         1},
    {ncm::ProgramId::Es,          1},
    {ncm::ProgramId::Creport,     1},
    {ncm::ProgramId::Ro,          1},
};
constexpr size_t g_MinimumProgramVersionsCount810 = util::size(g_MinimumProgramVersions810);

constexpr MinimumProgramVersion g_MinimumProgramVersions900[] = {
    /* All non-Development System Modules. */
    {ncm::ProgramId::Usb,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Tma,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Boot2,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Settings,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bus,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bluetooth,   MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bcat,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Dmnt,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Friends,     MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Nifm,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ptm,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Shell,       MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::BsdSockets,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Hid,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Audio,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::LogManager,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Wlan,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Cs,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Ldn,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::NvServices,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pcv,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ppc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::NvnFlinger,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pcie,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Account,     MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ns,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Nfc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Psc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::CapSrv,      MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Am,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ssl,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Nim,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Cec,         MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Tspm,        MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Spl,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Lbl,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Btm,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Erpt,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Time,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Vi,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pctl,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Npns,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Eupld,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Glue,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Eclct,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Es,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Fatal,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Grc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Creport,     MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ro,          MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Profiler,    MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Sdb,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Migration,   MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Jit,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::JpegDec,     MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::SafeMode,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Olsc,        MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Dt,          MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Nd,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Ngct,        MakeSystemVersion(9, 0, 0)},

    /* All Web Applets. */
    {ncm::ProgramId::AppletWeb,           MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::AppletShop,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::AppletOfflineWeb,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::AppletLoginShare,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::AppletWifiWebAuth,   MakeSystemVersion(9, 0, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount900 = util::size(g_MinimumProgramVersions900);

constexpr MinimumProgramVersion g_MinimumProgramVersions910[] = {
    /* All non-Development System Modules. */
    {ncm::ProgramId::Usb,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Tma,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Boot2,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Settings,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bus,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bluetooth,   MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Bcat,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Dmnt,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Friends,     MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Nifm,        MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Ptm,         MakeSystemVersion(9, 0, 0)},
 /* {ncm::ProgramId::Shell,       MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::BsdSockets,  MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Hid,         MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Audio,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::LogManager,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Wlan,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Cs,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Ldn,         MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::NvServices,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pcv,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Ppc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::NvnFlinger,  MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pcie,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Account,     MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Ns,          MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Nfc,         MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Psc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::CapSrv,      MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Am,          MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Ssl,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Nim,         MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Cec,         MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Tspm,        MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Spl,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Lbl,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Btm,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Erpt,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Time,        MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Vi,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Pctl,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Npns,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Eupld,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Glue,        MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Eclct,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Es,          MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Fatal,       MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Grc,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Creport,     MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::Ro,          MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Profiler,    MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Sdb,         MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Migration,   MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Jit,         MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::JpegDec,     MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::SafeMode,    MakeSystemVersion(9, 0, 0)},
    {ncm::ProgramId::Olsc,        MakeSystemVersion(9, 1, 0)},
 /* {ncm::ProgramId::Dt,          MakeSystemVersion(9, 0, 0)}, */
 /* {ncm::ProgramId::Nd,          MakeSystemVersion(9, 0, 0)}, */
    {ncm::ProgramId::Ngct,        MakeSystemVersion(9, 1, 0)},

    /* All Web Applets. */
    {ncm::ProgramId::AppletWeb,           MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::AppletShop,          MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::AppletOfflineWeb,    MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::AppletLoginShare,    MakeSystemVersion(9, 1, 0)},
    {ncm::ProgramId::AppletWifiWebAuth,   MakeSystemVersion(9, 1, 0)},
};
constexpr size_t g_MinimumProgramVersionsCount910 = util::size(g_MinimumProgramVersions910);

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
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */
    constexpr inline size_t GameCardCidSize = 0x10;
    constexpr inline size_t GameCardDeviceIdSize = 0x10;

    enum class GameCardPartition {
        Update = 0,
        Normal = 1,
        Secure = 2,
        Logo   = 3,
    };

    enum class GameCardPartitionRaw {
        NormalReadable,
        SecureReadable,
        RootWriteable,
    };

    enum GameCardAttribute : u8 {
        GameCardAttribute_AutoBootFlag                         = (1 << 0),
        GameCardAttribute_HistoryEraseFlag                     = (1 << 1),
        GameCardAttribute_RepairToolFlag                       = (1 << 2),
        GameCardAttribute_DifferentRegionCupToTerraDeviceFlag  = (1 << 3),
        GameCardAttribute_DifferentRegionCupToGlobalDeviceFlag = (1 << 4),

        GameCardAttribute_HasCa10CertificateFlag               = (1 << 7),
    };

    enum class GameCardCompatibilityType : u8 {
        Normal = 0,
        Terra  = 1,
    };

    struct GameCardErrorReportInfo {
        u16 game_card_crc_error_num;
        u16 reserved1;
        u16 asic_crc_error_num;
        u16 reserved2;
        u16 refresh_num;
        u16 reserved3;
        u16 retry_limit_out_num;
        u16 timeout_retry_num;
        u16 asic_reinitialize_failure_detail;
        u16 insertion_count;
        u16 removal_count;
        u16 asic_reinitialize_num;
        u32 initialize_count;
        u16 asic_reinitialize_failure_num;
        u16 awaken_failure_num;
        u16 reserved4;
        u16 refresh_succeeded_count;
        u32 last_read_error_page_address;
        u32 last_read_error_page_count;
        u32 awaken_count;
        u32 read_count_from_insert;
        u32 read_count_from_awaken;
        u8  reserved5[8];
    };
    static_assert(util::is_pod<GameCardErrorReportInfo>::value);
    static_assert(sizeof(GameCardErrorReportInfo) == 0x40);

    using GameCardHandle = u32;

    Result GetGameCardHandle(GameCardHandle *out);
    Result MountGameCardPartition(const char *name, GameCardHandle handle, GameCardPartition partition);

    Result GetGameCardCid(void *dst, size_t size);
    Result GetGameCardDeviceId(void *dst, size_t size);

    Result GetGameCardErrorReportInfo(GameCardErrorReportInfo *out);

    bool IsGameCardInserted();

}

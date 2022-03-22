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
#include <vapours.hpp>
#include <stratosphere/settings/settings_types.hpp>

namespace ams::settings::system {

    struct alignas(4) FirmwareVersion {
        u8 major;
        u8 minor;
        u8 micro;
        u8 padding1;
        u8 revision_major;
        u8 revision_minor;
        u8 padding2;
        u8 padding3;
        char platform[0x20];
        char revision[0x40];
        char display_version[0x18];
        char display_name[0x80];

        constexpr int GetComparableVersion() const {
            return (static_cast<int>(major) << 16) | (static_cast<int>(minor) << 8) | (static_cast<int>(micro) << 0);
        }

        constexpr friend bool operator==(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() == rhs.GetComparableVersion();
        }

        constexpr friend bool operator!=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() != rhs.GetComparableVersion();
        }

        constexpr friend bool operator<=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() <= rhs.GetComparableVersion();
        }

        constexpr friend bool operator>=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() >= rhs.GetComparableVersion();
        }

        constexpr friend bool operator<(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() < rhs.GetComparableVersion();
        }

        constexpr friend bool operator>(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
            return lhs.GetComparableVersion() > rhs.GetComparableVersion();
        }
    };

    void GetFirmwareVersion(FirmwareVersion *out);

}

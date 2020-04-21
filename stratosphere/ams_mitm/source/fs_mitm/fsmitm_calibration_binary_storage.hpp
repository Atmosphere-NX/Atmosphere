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
#include <stratosphere.hpp>
#include "fsmitm_boot0storage.hpp"
#include "../amsmitm_prodinfo_utils.hpp"

namespace ams::mitm::fs {

    /* Represents a protected calibration binary partition. */
    class CalibrationBinaryStorage : public SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200> {
        public:
            using Base = SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200>;

            static constexpr s64 BlankStartOffset = 0x0;
            static constexpr s64 BlankSize        = static_cast<s64>(CalibrationBinarySize);
            static constexpr s64 BlankEndOffset   = BlankStartOffset + BlankSize;

            static constexpr s64 FakeSecureStartOffset = SecureCalibrationInfoBackupOffset;
            static constexpr s64 FakeSecureSize        = static_cast<s64>(SecureCalibrationBinaryBackupSize);
            static constexpr s64 FakeSecureEndOffset   = FakeSecureStartOffset + FakeSecureSize;
        private:
            sm::MitmProcessInfo client_info;
            bool read_blank;
            bool allow_writes;
        public:
            CalibrationBinaryStorage(FsStorage &s,  const sm::MitmProcessInfo &c)
                : Base(s), client_info(c),
                  read_blank(mitm::ShouldReadBlankCalibrationBinary()),
                  allow_writes(mitm::IsWriteToCalibrationBinaryAllowed())
            {
                /* ... */
            }
        public:
            virtual Result Read(s64 offset, void *_buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *_buffer, size_t size) override;
    };

}

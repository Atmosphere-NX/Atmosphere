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

namespace ams::mitm::fs {

    template<class Base, size_t SectorSize>
    class SectoredStorageAdapter : public Base {
        static_assert(std::is_base_of<ams::fs::IStorage, Base>::value);
        private:
            u8  sector_buf[SectorSize];
        public:
            /* Inherit constructors. */
            using Base::Base;
        public:
            virtual Result Read(s64 offset, void *_buffer, size_t size) override {
                u8 *buffer = static_cast<u8 *>(_buffer);

                const s64 seek = util::AlignDown(offset, SectorSize);
                const s64 sector_ofs = offset - seek;

                /* Check if we have nothing to do. */
                if (size == 0) {
                    return ResultSuccess();
                }

                /* Fast case. */
                if (sector_ofs == 0 && util::IsAligned(size, SectorSize)) {
                    return Base::Read(offset, buffer, size);
                }

                R_TRY(Base::Read(seek, this->sector_buf, SectorSize));

                if (size + sector_ofs <= SectorSize) {
                    /* Staying within the sector. */
                    std::memcpy(buffer, this->sector_buf + sector_ofs, size);
                } else {
                    /* Leaving the sector. */
                    const size_t size_in_sector = SectorSize - sector_ofs;
                    std::memcpy(buffer, this->sector_buf + sector_ofs, size_in_sector);
                    size -= size_in_sector;

                    /* Read as many guaranteed aligned sectors as we can. */
                    const size_t aligned_remaining_size = util::AlignDown(size, SectorSize);
                    if (aligned_remaining_size) {
                        R_TRY(Base::Read(offset + size_in_sector, buffer + size_in_sector, aligned_remaining_size));
                        size -= aligned_remaining_size;
                    }

                    /* Read any leftover data. */
                    if (size) {
                        R_TRY(Base::Read(offset + size_in_sector + aligned_remaining_size, this->sector_buf, SectorSize));
                        std::memcpy(buffer + size_in_sector + aligned_remaining_size, this->sector_buf, size);
                    }
                }

                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *_buffer, size_t size) override {
                const u8 *buffer = static_cast<const u8 *>(_buffer);

                const s64 seek = util::AlignDown(offset, SectorSize);
                const s64 sector_ofs = offset - seek;

                /* Check if we have nothing to do. */
                if (size == 0) {
                    return ResultSuccess();
                }

                /* Fast case. */
                if (sector_ofs == 0 && util::IsAligned(size, SectorSize)) {
                    return Base::Write(offset, buffer, size);
                }

                /* Load existing sector data. */
                R_TRY(Base::Read(seek, this->sector_buf, SectorSize));

                if (size + sector_ofs <= SectorSize) {
                    /* Staying within the sector. */
                    std::memcpy(this->sector_buf + sector_ofs, buffer, size);
                    R_TRY(Base::Write(seek, this->sector_buf, SectorSize));
                } else {
                    /* Leaving the sector. */
                    const size_t size_in_sector = SectorSize - sector_ofs;
                    std::memcpy(this->sector_buf + sector_ofs, buffer, size_in_sector);
                    R_TRY(Base::Write(seek, this->sector_buf, SectorSize));
                    size -= size_in_sector;

                    /* Write as many guaranteed aligned sectors as we can. */
                    const size_t aligned_remaining_size = util::AlignDown(size, SectorSize);
                    if (aligned_remaining_size) {
                        R_TRY(Base::Write(offset + size_in_sector, buffer + size_in_sector, aligned_remaining_size));
                        size -= aligned_remaining_size;
                    }

                    /* Write any leftover data. */
                    if (size) {
                        R_TRY(Base::Read(offset + size_in_sector + aligned_remaining_size, this->sector_buf, SectorSize));
                        std::memcpy(this->sector_buf, buffer + size_in_sector + aligned_remaining_size, size);
                        R_TRY(Base::Write(offset + size_in_sector + aligned_remaining_size, this->sector_buf, SectorSize));
                    }
                }

                return ResultSuccess();
            }
    };

    /* Represents an RCM-preserving BOOT0 partition. */
    class Boot0Storage : public SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200> {
        public:
            using Base = SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200>;

            static constexpr s64 BctEndOffset = 0xFC000;
            static constexpr s64 BctSize = static_cast<s64>(ams::updater::BctSize);
            static constexpr s64 BctPubkStart = 0x210;
            static constexpr s64 BctPubkSize = 0x100;
            static constexpr s64 BctPubkEnd = BctPubkStart + BctPubkSize;

            static constexpr s64 EksStart = 0x180000;
            static constexpr s64 EksSize = static_cast<s64>(ams::updater::EksSize);
            static constexpr s64 EksEnd = EksStart + EksSize;
        private:
            sm::MitmProcessInfo client_info;
        private:
            bool CanModifyBctPublicKey();
        public:
            Boot0Storage(FsStorage &s, const sm::MitmProcessInfo &c) : Base(s), client_info(c) { /* ... */ }
        public:
            virtual Result Read(s64 offset, void *_buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *_buffer, size_t size) override;
    };

    class CustomPublicKeyBoot0Storage : public SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200> {
        public:
            using Base = SectoredStorageAdapter<ams::fs::RemoteStorage, 0x200>;
        private:
            sm::MitmProcessInfo client_info;
            spl::SocType soc_type;
        public:
            CustomPublicKeyBoot0Storage(FsStorage &s, const sm::MitmProcessInfo &c, spl::SocType soc);
        public:
            virtual Result Read(s64 offset, void *_buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *_buffer, size_t size) override;
    };

    bool DetectBoot0CustomPublicKey(::FsStorage &storage);

}

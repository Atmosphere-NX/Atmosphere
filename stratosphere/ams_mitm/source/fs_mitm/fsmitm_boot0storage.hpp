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
#include <switch.h>
#include <cstring>
#include <stratosphere.hpp>

#include "fs_istorage.hpp"

/* Represents a sectored storage. */
template<u64 SectorSize>
class SectoredProxyStorage : public ProxyStorage {
    private:
        u64 cur_seek = 0;
        u64 cur_sector = 0;
        u64 cur_sector_ofs = 0;
        u8  sector_buf[SectorSize];
    private:
        void Seek(u64 offset) {
            this->cur_sector_ofs = offset % SectorSize;
            this->cur_seek = offset - this->cur_sector_ofs;
        }
    public:
        SectoredProxyStorage(FsStorage *s) : ProxyStorage(s) { }
        SectoredProxyStorage(FsStorage s) : ProxyStorage(s) { }
    public:
        virtual Result Read(void *_buffer, size_t size, u64 offset) override {
            u8 *buffer = static_cast<u8 *>(_buffer);
            this->Seek(offset);

            if (this->cur_sector_ofs == 0 && size % SectorSize == 0) {
                /* Fast case. */
                return ProxyStorage::Read(buffer, size, offset);
            }

            R_TRY(ProxyStorage::Read(this->sector_buf, SectorSize, this->cur_seek));

            if (size + this->cur_sector_ofs <= SectorSize) {
                memcpy(buffer, sector_buf + this->cur_sector_ofs, size);
            } else {
                /* Leaving the sector... */
                size_t ofs = SectorSize - this->cur_sector_ofs;
                memcpy(buffer, sector_buf + this->cur_sector_ofs, ofs);
                size -= ofs;

                /* We're guaranteed alignment, here. */
                const size_t aligned_remaining_size = size - (size % SectorSize);
                if (aligned_remaining_size) {
                    R_TRY(ProxyStorage::Read(buffer + ofs, aligned_remaining_size, offset + ofs));
                    ofs += aligned_remaining_size;
                    size -= aligned_remaining_size;
                }

                /* Read any leftover data. */
                if (size) {
                    R_TRY(ProxyStorage::Read(this->sector_buf, SectorSize, offset + ofs));
                    memcpy(buffer + ofs, sector_buf, size);
                }
            }

            return ResultSuccess;
        }

        virtual Result Write(void *_buffer, size_t size, u64 offset) override {
            u8 *buffer = static_cast<u8 *>(_buffer);
            this->Seek(offset);

            if (this->cur_sector_ofs == 0 && size % SectorSize == 0) {
                /* Fast case. */
                return ProxyStorage::Write(buffer, size, offset);
            }

            R_TRY(ProxyStorage::Read(this->sector_buf, SectorSize, this->cur_seek));

            if (size + this->cur_sector_ofs <= SectorSize) {
                memcpy(this->sector_buf + this->cur_sector_ofs, buffer, size);
                R_TRY(ProxyStorage::Write(this->sector_buf, SectorSize, this->cur_seek));
            } else {
                /* Leaving the sector... */
                size_t ofs = SectorSize - this->cur_sector_ofs;
                memcpy(this->sector_buf + this->cur_sector_ofs, buffer, ofs);
                R_TRY(ProxyStorage::Write(this->sector_buf, ofs, this->cur_seek));
                size -= ofs;

                /* We're guaranteed alignment, here. */
                const size_t aligned_remaining_size = size - (size % SectorSize);
                if (aligned_remaining_size) {
                    R_TRY(ProxyStorage::Write(buffer + ofs, aligned_remaining_size, offset + ofs));
                    ofs += aligned_remaining_size;
                    size -= aligned_remaining_size;
                }

                /* Write any leftover data. */
                if (size) {
                    R_TRY(ProxyStorage::Read(this->sector_buf, SectorSize, offset + ofs));
                    memcpy(this->sector_buf, buffer + ofs, size);
                    R_TRY(ProxyStorage::Write(this->sector_buf, SectorSize, this->cur_seek));
                }
            }

            return ResultSuccess;
        }
};

/* Represents an RCM-preserving BOOT0 partition. */
class Boot0Storage : public SectoredProxyStorage<0x200> {
    using Base = SectoredProxyStorage<0x200>;

    public:
        static constexpr u64 BctEndOffset = 0xFC000;
        static constexpr u64 BctSize = 0x4000;
        static constexpr u64 BctPubkStart = 0x210;
        static constexpr u64 BctPubkSize = 0x100;
        static constexpr u64 BctPubkEnd = BctPubkStart + BctPubkSize;

        static constexpr u64 EksStart = 0x180000;
        static constexpr u64 EksSize = 0x4000;
        static constexpr u64 EksEnd = EksStart + EksSize;
    private:
        sts::ncm::TitleId title_id;
    private:
        bool CanModifyBctPubks();
    public:
        Boot0Storage(FsStorage *s, sts::ncm::TitleId t) : Base(s), title_id(t) { }
        Boot0Storage(FsStorage s, sts::ncm::TitleId t) : Base(s), title_id(t) { }
    public:
        virtual Result Read(void *_buffer, size_t size, u64 offset) override;
        virtual Result Write(void *_buffer, size_t size, u64 offset) override;
};
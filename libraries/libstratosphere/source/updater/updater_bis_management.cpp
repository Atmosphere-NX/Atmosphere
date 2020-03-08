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
#include "updater_bis_management.hpp"

namespace ams::updater {

    Result BisAccessor::Initialize() {
        return fs::OpenBisPartition(std::addressof(this->storage), this->partition_id);
    }

    void BisAccessor::Finalize() {
        /* ... */
    }

    Result BisAccessor::Read(void *dst, size_t size, u64 offset) {
        AMS_ABORT_UNLESS((offset % SectorAlignment) == 0);
        return this->storage->Read(static_cast<u32>(offset), dst, size);
    }

    Result BisAccessor::Write(u64 offset, const void *src, size_t size) {
        AMS_ABORT_UNLESS((offset % SectorAlignment) == 0);
        return this->storage->Write(static_cast<u32>(offset), src, size);
    }

    Result BisAccessor::Write(u64 offset, size_t size, const char *bip_path, void *work_buffer, size_t work_buffer_size) {
        AMS_ABORT_UNLESS((offset % SectorAlignment) == 0);
        AMS_ABORT_UNLESS((work_buffer_size % SectorAlignment) == 0);

        fs::FileHandle file;
        R_TRY_CATCH(fs::OpenFile(std::addressof(file), bip_path, fs::OpenMode_Read)) {
            R_CONVERT(fs::ResultPathNotFound, ResultInvalidBootImagePackage())
        } R_END_TRY_CATCH;
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        size_t written = 0;
        while (true) {
            std::memset(work_buffer, 0, work_buffer_size);
            size_t read_size;
            R_TRY(fs::ReadFile(std::addressof(read_size), file, written, work_buffer, work_buffer_size, fs::ReadOption()));
            AMS_ABORT_UNLESS(written + read_size <= size);

            size_t aligned_size = ((read_size + SectorAlignment - 1) / SectorAlignment) * SectorAlignment;
            R_TRY(this->Write(offset + written, work_buffer, aligned_size));
            written += read_size;

            if (read_size != work_buffer_size) {
                break;
            }
        }
        return ResultSuccess();
    }

    Result BisAccessor::Clear(u64 offset, u64 size, void *work_buffer, size_t work_buffer_size) {
        AMS_ABORT_UNLESS((offset % SectorAlignment) == 0);
        AMS_ABORT_UNLESS((work_buffer_size % SectorAlignment) == 0);

        std::memset(work_buffer, 0, work_buffer_size);

        size_t written = 0;
        while (written < size) {
            size_t cur_write_size = std::min(work_buffer_size, size - written);
            R_TRY(this->Write(offset + written, work_buffer, cur_write_size));
            written += cur_write_size;
        }
        return ResultSuccess();
    }

    Result BisAccessor::GetHash(void *dst, u64 offset, u64 size, u64 hash_size, void *work_buffer, size_t work_buffer_size) {
        AMS_ABORT_UNLESS((offset % SectorAlignment) == 0);
        AMS_ABORT_UNLESS((work_buffer_size % SectorAlignment) == 0);

        crypto::Sha256Generator generator;
        generator.Initialize();

        size_t total_read = 0;
        while (total_read < hash_size) {
            size_t cur_read_size = std::min(work_buffer_size, size - total_read);
            size_t cur_update_size = std::min(cur_read_size, hash_size - total_read);
            R_TRY(this->Read(work_buffer, cur_read_size, offset + total_read));
            generator.Update(work_buffer, cur_update_size);

            total_read += cur_read_size;
        }
        generator.GetHash(dst, hash_size);

        return ResultSuccess();
    }

    size_t Boot0Accessor::GetBootloaderVersion(void *bct) {
        u32 version = *reinterpret_cast<u32 *>(reinterpret_cast<uintptr_t>(bct) + BctVersionOffset);
        AMS_ABORT_UNLESS(version <= BctVersionMax);
        return static_cast<size_t>(version);
    }

    size_t Boot0Accessor::GetEksIndex(size_t bootloader_version) {
        AMS_ABORT_UNLESS(bootloader_version <= BctVersionMax);
        return (bootloader_version > 0) ? bootloader_version - 1 : 0;
    }

    void Boot0Accessor::CopyEks(void *dst_bct, const void *src_eks, size_t eks_index) {
        std::memcpy(reinterpret_cast<u8 *>(dst_bct) + BctEksOffset, reinterpret_cast<const u8 *>(src_eks) + eks_index * EksEntrySize, EksBlobSize);
    }

    Result Boot0Accessor::UpdateEks(void *dst_bct, void *eks_work_buffer) {
        size_t read_size;
        R_TRY(this->Read(&read_size, eks_work_buffer, EksSize, Boot0Partition::Eks));

        return this->UpdateEksManually(dst_bct, eks_work_buffer);
    }

    Result Boot0Accessor::UpdateEksManually(void *dst_bct, const void *src_eks) {
        this->CopyEks(dst_bct, src_eks, GetEksIndex(GetBootloaderVersion(dst_bct)));
        return ResultSuccess();
    }

    Result Boot0Accessor::PreserveAutoRcm(void *dst_bct, void *work_buffer, Boot0Partition which) {
        std::memset(work_buffer, 0, BctSize);

        size_t read_size;
        R_TRY(this->Read(&read_size, work_buffer, BctSize, which));

        void *dst_pubk = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(dst_bct)     + BctPubkOffset);
        void *src_pubk = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctPubkOffset);
        std::memcpy(dst_pubk, src_pubk, BctPubkSize);

        return ResultSuccess();
    }

}

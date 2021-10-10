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
#include <stratosphere.hpp>

namespace ams::fssystem {

    Result IntegrityRomFsStorage::Initialize(save::HierarchicalIntegrityVerificationInformation level_hash_info, Hash master_hash, save::HierarchicalIntegrityVerificationStorage::HierarchicalStorageInformation storage_info, IBufferManager *bm) {
        /* Validate preconditions. */
        AMS_ASSERT(bm != nullptr);

        /* Set master hash. */
        m_master_hash = master_hash;
        m_master_hash_storage = std::make_unique<fs::MemoryStorage>(std::addressof(m_master_hash), sizeof(Hash));
        R_UNLESS(m_master_hash_storage != nullptr, fs::ResultAllocationFailureInIntegrityRomFsStorageA());

        /* Set the master hash storage. */
        storage_info[0] = fs::SubStorage(m_master_hash_storage.get(), 0, sizeof(Hash));

        /* Set buffers. */
        for (size_t i = 0; i < util::size(m_buffers.buffers); ++i) {
            m_buffers.buffers[i] = bm;
        }

        /* Initialize our integrity storage. */
        return m_integrity_storage.Initialize(level_hash_info, storage_info, std::addressof(m_buffers), std::addressof(m_mutex), fs::StorageType_RomFs);
    }

    void IntegrityRomFsStorage::Finalize() {
        m_integrity_storage.Finalize();
    }

}

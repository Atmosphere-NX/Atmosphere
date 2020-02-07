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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        KVirtualAddress GetInitialProcessBinaryAddress() {
            return KMemoryLayout::GetPageTableHeapRegion().GetEndAddress() - InitialProcessBinarySizeMax;
        }

        void LoadInitialProcessBinaryHeader(InitialProcessBinaryHeader *header) {
            if (header->magic != InitialProcessBinaryMagic) {
                *header = *GetPointer<InitialProcessBinaryHeader>(GetInitialProcessBinaryAddress());
            }

            MESOSPHERE_ABORT_UNLESS(header->magic == InitialProcessBinaryMagic);
            MESOSPHERE_ABORT_UNLESS(header->num_processes <= init::GetSlabResourceCounts().num_KProcess);
        }

        KVirtualAddress g_initial_process_binary_address;
        InitialProcessBinaryHeader g_initial_process_binary_header;
        u64 g_initial_process_id_min = std::numeric_limits<u64>::max();
        u64 g_initial_process_id_max = std::numeric_limits<u64>::min();

    }

    u64 GetInitialProcessIdMin() {
        return g_initial_process_id_min;
    }

    u64 GetInitialProcessIdMax() {
        return g_initial_process_id_max;
    }

    void CopyInitialProcessBinaryToKernelMemory() {
        LoadInitialProcessBinaryHeader(&g_initial_process_binary_header);

        if (g_initial_process_binary_header.num_processes > 0) {
            /* Reserve pages for the initial process binary from the system resource limit. */
            auto &mm = Kernel::GetMemoryManager();
            const size_t total_size = util::AlignUp(g_initial_process_binary_header.size, PageSize);
            const size_t num_pages  = total_size / PageSize;
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, total_size));

            /* Allocate memory for the image. */
            const KMemoryManager::Pool pool = static_cast<KMemoryManager::Pool>(KSystemControl::GetInitialProcessBinaryPool());
            const auto allocate_option = KMemoryManager::EncodeOption(pool, KMemoryManager::Direction_FromFront);
            KVirtualAddress allocated_memory = mm.AllocateContinuous(num_pages, 1, allocate_option);
            MESOSPHERE_ABORT_UNLESS(allocated_memory != Null<KVirtualAddress>);
            mm.Open(allocated_memory, num_pages);

            /* Relocate the image. */
            std::memmove(GetVoidPointer(allocated_memory), GetVoidPointer(GetInitialProcessBinaryAddress()), g_initial_process_binary_header.size);
            std::memset(GetVoidPointer(GetInitialProcessBinaryAddress()), 0, g_initial_process_binary_header.size);
            g_initial_process_binary_address = allocated_memory;
        }
    }

}

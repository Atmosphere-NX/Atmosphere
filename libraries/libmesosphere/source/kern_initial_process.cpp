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

        struct InitialProcessInfo {
            KProcess *process;
            size_t stack_size;
            s32 priority;
        };

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

        void CreateProcesses(InitialProcessInfo *infos, KVirtualAddress binary_address, const InitialProcessBinaryHeader &header) {
            u8 *current = GetPointer<u8>(binary_address + sizeof(InitialProcessBinaryHeader));
            const u8 * const end = GetPointer<u8>(binary_address + header.size - sizeof(KInitialProcessHeader));

            const size_t num_processes = header.num_processes;
            for (size_t i = 0; i < num_processes; i++) {
                /* Validate that we can read the current KIP. */
                MESOSPHERE_ABORT_UNLESS(current <= end);
                KInitialProcessReader reader;
                MESOSPHERE_ABORT_UNLESS(reader.Attach(current));

                /* Parse process parameters and reserve memory. */
                ams::svc::CreateProcessParameter params;
                MESOSPHERE_R_ABORT_UNLESS(reader.MakeCreateProcessParameter(std::addressof(params), true));
                MESOSPHERE_TODO("Reserve memory");

                /* Create the process, and ensure we don't leak pages. */
                {
                    /* Allocate memory for the process. */
                    MESOSPHERE_TODO("Allocate memory for the process");

                    /* Map the process's memory into the temporary region. */
                    MESOSPHERE_TODO("Map the process's page group");

                    /* Load the process. */
                    MESOSPHERE_TODO("Load the process");

                    /* Unmap the temporary mapping. */
                    MESOSPHERE_TODO("Unmap the process's page group");

                    /* Create a KProcess object. */
                    MESOSPHERE_TODO("Create a KProcess");

                    /* Initialize the process. */
                    MESOSPHERE_TODO("Initialize the process");
                }

                /* Set the process's memory permissions. */
                MESOSPHERE_TODO("Set process's memory permissions");

                /* Register the process. */
                MESOSPHERE_TODO("Register the process");

                /* Save the process info. */
                infos[i].process    = /* TODO */ nullptr;
                infos[i].stack_size = reader.GetStackSize();
                infos[i].priority   = reader.GetPriority();

                /* Advance the reader. */
                current += reader.GetBinarySize();
            }
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

    void CreateAndRunInitialProcesses() {
        /* Allocate space for the processes. */
        InitialProcessInfo *infos = static_cast<InitialProcessInfo *>(__builtin_alloca(sizeof(InitialProcessInfo) * g_initial_process_binary_header.num_processes));

        /* Create the processes. */
        CreateProcesses(infos, g_initial_process_binary_address, g_initial_process_binary_header);

        /* Release the memory used by the image. */
        {
            const size_t total_size = util::AlignUp(g_initial_process_binary_header.size, PageSize);
            const size_t num_pages  = total_size / PageSize;
            Kernel::GetMemoryManager().Close(g_initial_process_binary_address, num_pages);
            Kernel::GetSystemResourceLimit().Release(ams::svc::LimitableResource_PhysicalMemoryMax, total_size);
        }

        /* Determine the initial process id range. */
        for (size_t i = 0; i < g_initial_process_binary_header.num_processes; i++) {
            const auto pid = infos[i].process->GetId();
            g_initial_process_id_min = std::min(g_initial_process_id_min, pid);
            g_initial_process_id_max = std::max(g_initial_process_id_max, pid);
        }

        /* Run the processes. */
        for (size_t i = 0; i < g_initial_process_binary_header.num_processes; i++) {
            MESOSPHERE_TODO("infos[i].process->Run(infos[i].priority, infos[i].stack_size);");
        }
    }

}

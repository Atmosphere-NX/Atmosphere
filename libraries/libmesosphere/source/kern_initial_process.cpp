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
            const uintptr_t end_address = KMemoryLayout::GetPageTableHeapRegion().GetEndAddress();
            MESOSPHERE_ABORT_UNLESS(end_address != 0);
            return end_address - InitialProcessBinarySizeMax;
        }

        void LoadInitialProcessBinaryHeader(InitialProcessBinaryHeader *header) {
            if (header->magic != InitialProcessBinaryMagic) {
                *header = *GetPointer<InitialProcessBinaryHeader>(GetInitialProcessBinaryAddress());
            }

            MESOSPHERE_ABORT_UNLESS(header->magic == InitialProcessBinaryMagic);
            MESOSPHERE_ABORT_UNLESS(header->num_processes <= init::GetSlabResourceCounts().num_KProcess);
        }

        size_t GetProcessesSecureMemorySize(KVirtualAddress binary_address, const InitialProcessBinaryHeader &header) {
            u8 *current = GetPointer<u8>(binary_address + sizeof(InitialProcessBinaryHeader));
            const u8 * const end = GetPointer<u8>(binary_address + header.size - sizeof(KInitialProcessHeader));

            size_t size = 0;
            const size_t num_processes = header.num_processes;
            for (size_t i = 0; i < num_processes; i++) {
                /* Validate that we can read the current KIP. */
                MESOSPHERE_ABORT_UNLESS(current <= end);
                KInitialProcessReader reader;
                MESOSPHERE_ABORT_UNLESS(reader.Attach(current));

                /* If the process uses secure memory, account for that. */
                if (reader.UsesSecureMemory()) {
                    size += util::AlignUp(reader.GetSize(), PageSize);
                }

                /* Advance the reader. */
                current += reader.GetBinarySize();
            }

            return size;
        }

        void CreateProcesses(InitialProcessInfo *infos, KVirtualAddress binary_address, const InitialProcessBinaryHeader &header) {
            u8 *current = GetPointer<u8>(binary_address + sizeof(InitialProcessBinaryHeader));
            const u8 * const end = GetPointer<u8>(binary_address + header.size - sizeof(KInitialProcessHeader));

            /* Decide on pools to use. */
            const auto unsafe_pool = static_cast<KMemoryManager::Pool>(KSystemControl::GetCreateProcessMemoryPool());
            const auto secure_pool = (GetTargetFirmware() >= TargetFirmware_2_0_0) ? KMemoryManager::Pool_Secure : unsafe_pool;

            const size_t num_processes = header.num_processes;
            for (size_t i = 0; i < num_processes; i++) {
                /* Validate that we can read the current KIP. */
                MESOSPHERE_ABORT_UNLESS(current <= end);
                KInitialProcessReader reader;
                MESOSPHERE_ABORT_UNLESS(reader.Attach(current));

                /* Parse process parameters and reserve memory. */
                ams::svc::CreateProcessParameter params;
                MESOSPHERE_R_ABORT_UNLESS(reader.MakeCreateProcessParameter(std::addressof(params), true));
                MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, params.code_num_pages * PageSize));

                /* Create the process. */
                KProcess *new_process = nullptr;
                {
                    /* Declare page group to use for process memory. */
                    KPageGroup pg(std::addressof(Kernel::GetBlockInfoManager()));

                    /* Allocate memory for the process. */
                    auto &mm = Kernel::GetMemoryManager();
                    const auto pool = reader.UsesSecureMemory() ? secure_pool : unsafe_pool;
                    MESOSPHERE_R_ABORT_UNLESS(mm.AllocateAndOpen(std::addressof(pg), params.code_num_pages, KMemoryManager::EncodeOption(pool, KMemoryManager::Direction_FromFront)));

                    {
                        /* Ensure that we do not leak pages. */
                        ON_SCOPE_EXIT { pg.Close(); };

                        /* Get the temporary region. */
                        const auto &temp_region = KMemoryLayout::GetTempRegion();
                        MESOSPHERE_ABORT_UNLESS(temp_region.GetEndAddress() != 0);

                        /* Map the process's memory into the temporary region. */
                        KProcessAddress temp_address = Null<KProcessAddress>;
                        MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().MapPageGroup(std::addressof(temp_address), pg, temp_region.GetAddress(), temp_region.GetSize() / PageSize, KMemoryState_Kernel, KMemoryPermission_KernelReadWrite));

                        /* Load the process. */
                        MESOSPHERE_R_ABORT_UNLESS(reader.Load(temp_address, params));

                        /* Unmap the temporary mapping. */
                        MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().UnmapPageGroup(temp_address, pg, KMemoryState_Kernel));

                        /* Create a KProcess object. */
                        new_process = KProcess::Create();
                        MESOSPHERE_ABORT_UNLESS(new_process != nullptr);

                        /* Initialize the process. */
                        MESOSPHERE_R_ABORT_UNLESS(new_process->Initialize(params, pg, reader.GetCapabilities(), reader.GetNumCapabilities(), std::addressof(Kernel::GetSystemResourceLimit()), pool));
                    }
                }

                /* Set the process's memory permissions. */
                MESOSPHERE_R_ABORT_UNLESS(reader.SetMemoryPermissions(new_process->GetPageTable(), params));

                /* Register the process. */
                KProcess::Register(new_process);

                /* Set the ideal core id. */
                new_process->SetIdealCoreId(reader.GetIdealCoreId());

                /* Save the process info. */
                infos[i].process    = new_process;
                infos[i].stack_size = reader.GetStackSize();
                infos[i].priority   = reader.GetPriority();

                /* Advance the reader. */
                current += reader.GetBinarySize();
            }
        }

        constinit KVirtualAddress g_initial_process_binary_address = Null<KVirtualAddress>;
        constinit InitialProcessBinaryHeader g_initial_process_binary_header = {};
        constinit u64 g_initial_process_id_min = std::numeric_limits<u64>::max();
        constinit u64 g_initial_process_id_max = std::numeric_limits<u64>::min();

    }

    u64 GetInitialProcessIdMin() {
        return g_initial_process_id_min;
    }

    u64 GetInitialProcessIdMax() {
        return g_initial_process_id_max;
    }

    size_t GetInitialProcessesSecureMemorySize() {
        LoadInitialProcessBinaryHeader(&g_initial_process_binary_header);

        return GetProcessesSecureMemorySize(g_initial_process_binary_address != Null<KVirtualAddress> ? g_initial_process_binary_address : GetInitialProcessBinaryAddress(), g_initial_process_binary_header);
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
            const KMemoryManager::Pool pool = static_cast<KMemoryManager::Pool>(KSystemControl::GetCreateProcessMemoryPool());
            const auto allocate_option = KMemoryManager::EncodeOption(pool, KMemoryManager::Direction_FromFront);
            KVirtualAddress allocated_memory = mm.AllocateAndOpenContinuous(num_pages, 1, allocate_option);
            MESOSPHERE_ABORT_UNLESS(allocated_memory != Null<KVirtualAddress>);

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
            MESOSPHERE_R_ABORT_UNLESS(infos[i].process->Run(infos[i].priority, infos[i].stack_size));
        }
    }

}

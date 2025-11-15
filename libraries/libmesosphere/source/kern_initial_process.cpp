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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        struct InitialProcessInfo {
            KProcess *process;
            size_t stack_size;
            s32 priority;
        };

        constinit KPhysicalAddress g_initial_process_binary_phys_addr = Null<KPhysicalAddress>;
        constinit KVirtualAddress g_initial_process_binary_address = Null<KVirtualAddress>;
        constinit size_t g_initial_process_binary_size = 0;
        constinit InitialProcessBinaryHeader g_initial_process_binary_header = {};
        constinit size_t g_initial_process_secure_memory_size = 0;
        constinit u64 g_initial_process_id_min = std::numeric_limits<u64>::max();
        constinit u64 g_initial_process_id_max = std::numeric_limits<u64>::min();

        void LoadInitialProcessBinaryHeader() {
            if (g_initial_process_binary_header.magic != InitialProcessBinaryMagic) {
                /* Get the virtual address. */
                MESOSPHERE_INIT_ABORT_UNLESS(g_initial_process_binary_phys_addr != Null<KPhysicalAddress>);
                const KVirtualAddress virt_addr = KMemoryLayout::GetLinearVirtualAddress(g_initial_process_binary_phys_addr);

                /* Copy and validate the header. */
                g_initial_process_binary_header = *GetPointer<InitialProcessBinaryHeader>(virt_addr);
                MESOSPHERE_ABORT_UNLESS(g_initial_process_binary_header.magic == InitialProcessBinaryMagic);
                MESOSPHERE_ABORT_UNLESS(g_initial_process_binary_header.num_processes <= init::GetSlabResourceCounts().num_KProcess);

                /* Set the image address. */
                g_initial_process_binary_address = virt_addr;

                /* Process/calculate the secure memory size. */
                KVirtualAddress current = g_initial_process_binary_address + sizeof(InitialProcessBinaryHeader);
                const KVirtualAddress end = g_initial_process_binary_address + g_initial_process_binary_header.size;
                const size_t num_processes = g_initial_process_binary_header.num_processes;
                for (size_t i = 0; i < num_processes; ++i) {
                    /* Validate that we can read the current KIP. */
                    MESOSPHERE_ABORT_UNLESS(current <= end - sizeof(KInitialProcessHeader));

                    /* Attach to the current KIP. */
                    KInitialProcessReader reader;
                    KVirtualAddress data = reader.Attach(current);
                    MESOSPHERE_ABORT_UNLESS(data != Null<KVirtualAddress>);

                    /* If the process uses secure memory, account for that. */
                    if (reader.UsesSecureMemory()) {
                        g_initial_process_secure_memory_size += reader.GetSize() + util::AlignUp(reader.GetStackSize(), PageSize);
                    }

                    /* Advance to the next KIP. */
                    current = data + reader.GetBinarySize();
                }
            }
        }

        void CreateProcesses(InitialProcessInfo *infos) {
            /* Determine process image extents. */
            KVirtualAddress current = g_initial_process_binary_address + sizeof(InitialProcessBinaryHeader);
            KVirtualAddress end = g_initial_process_binary_address + g_initial_process_binary_header.size;

            /* Decide on pools to use. */
            const auto unsafe_pool = static_cast<KMemoryManager::Pool>(KSystemControl::GetCreateProcessMemoryPool());
            const auto secure_pool = (GetTargetFirmware() >= TargetFirmware_2_0_0) ? KMemoryManager::Pool_Secure : unsafe_pool;

            const size_t num_processes = g_initial_process_binary_header.num_processes;
            for (size_t i = 0; i < num_processes; ++i) {
                /* Validate that we can read the current KIP header. */
                MESOSPHERE_ABORT_UNLESS(current <= end - sizeof(KInitialProcessHeader));

                /* Attach to the current kip. */
                KInitialProcessReader reader;
                KVirtualAddress data = reader.Attach(current);
                MESOSPHERE_ABORT_UNLESS(data != Null<KVirtualAddress>);

                /* Ensure that the remainder of our parse is page aligned. */
                if (!util::IsAligned(GetInteger(data), PageSize)) {
                    const KVirtualAddress aligned_data = util::AlignDown(GetInteger(data), PageSize);
                    std::memmove(GetVoidPointer(aligned_data), GetVoidPointer(data), end - data);

                    data  = aligned_data;
                    end  -= (data - aligned_data);
                }

                /* If we crossed a page boundary, free the pages we're done using. */
                if (KVirtualAddress aligned_current = util::AlignDown(GetInteger(current), PageSize); aligned_current != data) {
                    const size_t freed_size = data - aligned_current;
                    Kernel::GetMemoryManager().Close(KMemoryLayout::GetLinearPhysicalAddress(aligned_current), freed_size / PageSize);
                    Kernel::GetSystemResourceLimit().Release(ams::svc::LimitableResource_PhysicalMemoryMax, freed_size);
                }

                /* Parse process parameters. */
                ams::svc::CreateProcessParameter params;
                MESOSPHERE_R_ABORT_UNLESS(reader.MakeCreateProcessParameter(std::addressof(params), true));

                /* Get the binary size for the kip. */
                const size_t binary_size  = reader.GetBinarySize();
                const size_t binary_pages = binary_size / PageSize;

                /* Get the pool for both the current (compressed) image, and the decompressed process. */
                const auto src_pool = Kernel::GetMemoryManager().GetPool(KMemoryLayout::GetLinearPhysicalAddress(data));
                const auto dst_pool = reader.UsesSecureMemory() ? secure_pool : unsafe_pool;

                /* Determine the process size, and how much memory isn't already reserved. */
                const size_t process_size    = params.code_num_pages * PageSize;
                const size_t unreserved_size = process_size - (src_pool == dst_pool ? util::AlignDown(binary_size, PageSize) : 0);

                /* Reserve however much memory we need to reserve. */
                MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, unreserved_size));

                /* Create the process. */
                KProcess *new_process = nullptr;
                {
                    /* Make page groups to represent the data. */
                    KPageGroup pg(Kernel::GetSystemSystemResource().GetBlockInfoManagerPointer());
                    KPageGroup workaround_pg(Kernel::GetSystemSystemResource().GetBlockInfoManagerPointer());

                    /* Populate the page group to represent the data. */
                    {
                        /* Allocate the previously unreserved pages. */
                        KPageGroup unreserve_pg(Kernel::GetSystemSystemResource().GetBlockInfoManagerPointer());
                        MESOSPHERE_R_ABORT_UNLESS(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(unreserve_pg), unreserved_size / PageSize, 1, KMemoryManager::EncodeOption(dst_pool, KMemoryManager::Direction_FromFront)));

                        /* Add the previously reserved pages. */
                        if (src_pool == dst_pool && binary_pages != 0) {
                            MESOSPHERE_R_ABORT_UNLESS(pg.AddBlock(KMemoryLayout::GetLinearPhysicalAddress(data), binary_pages));
                        }

                        /* Add the previously unreserved pages. */
                        for (const auto &block : unreserve_pg) {
                            MESOSPHERE_R_ABORT_UNLESS(pg.AddBlock(block.GetAddress(), block.GetNumPages()));
                        }
                    }
                    MESOSPHERE_ABORT_UNLESS(pg.GetNumPages() == static_cast<size_t>(params.code_num_pages));

                    /* Ensure that we do not leak pages. */
                    KPageGroup *process_pg = std::addressof(pg);
                    ON_SCOPE_EXIT { process_pg->Close(); };

                    /* Load the process. */
                    reader.Load(pg, data);

                    /* If necessary, close/release the aligned part of the data we just loaded. */
                    if (const size_t aligned_bin_size = util::AlignDown(binary_size, PageSize); aligned_bin_size != 0 && src_pool != dst_pool) {
                        Kernel::GetMemoryManager().Close(KMemoryLayout::GetLinearPhysicalAddress(data), aligned_bin_size / PageSize);
                        Kernel::GetSystemResourceLimit().Release(ams::svc::LimitableResource_PhysicalMemoryMax, aligned_bin_size);
                    }

                    /* Create a KProcess object. */
                    new_process = KProcess::Create();
                    MESOSPHERE_ABORT_UNLESS(new_process != nullptr);

                    /* Ensure the page group is usable for the process. */
                    /* If the pool is the same, we need to use the workaround page group. */
                    if (src_pool == dst_pool) {
                        /* Allocate a new, usable group for the process. */
                        MESOSPHERE_R_ABORT_UNLESS(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(workaround_pg), static_cast<size_t>(params.code_num_pages), 1, KMemoryManager::EncodeOption(dst_pool, KMemoryManager::Direction_FromFront)));

                        /* Copy data from the working page group to the usable one. */
                        auto work_it = pg.begin();
                        MESOSPHERE_ABORT_UNLESS(work_it != pg.end());
                        {
                            auto work_address   = work_it->GetAddress();
                            auto work_remaining = work_it->GetNumPages();
                            for (const auto &block : workaround_pg) {
                                auto block_address   = block.GetAddress();
                                auto block_remaining = block.GetNumPages();
                                while (block_remaining > 0) {
                                    if (work_remaining == 0) {
                                        ++work_it;
                                        work_address   = work_it->GetAddress();
                                        work_remaining = work_it->GetNumPages();
                                    }

                                    const size_t cur_pages = std::min(block_remaining, work_remaining);
                                    const size_t cur_size  = cur_pages * PageSize;
                                    std::memcpy(GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(block_address)), GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(work_address)), cur_size);

                                    block_address += cur_size;
                                    work_address  += cur_size;

                                    block_remaining -= cur_pages;
                                    work_remaining  -= cur_pages;
                                }
                            }

                            ++work_it;
                        }
                        MESOSPHERE_ABORT_UNLESS(work_it == pg.end());

                        /* We want to use the new page group. */
                        process_pg = std::addressof(workaround_pg);
                        pg.Close();
                    }

                    /* Initialize the process. */
                    MESOSPHERE_R_ABORT_UNLESS(new_process->Initialize(params, *process_pg, reader.GetCapabilities(), reader.GetNumCapabilities(), std::addressof(Kernel::GetSystemResourceLimit()), dst_pool, reader.IsImmortal()));
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
                current = data + binary_size;
            }

            /* Release remaining memory used by the image. */
            {
                const size_t remaining_size  = util::AlignUp(GetInteger(g_initial_process_binary_address) + g_initial_process_binary_header.size, PageSize) - util::AlignDown(GetInteger(current), PageSize);
                const size_t remaining_pages = remaining_size / PageSize;
                Kernel::GetMemoryManager().Close(KMemoryLayout::GetLinearPhysicalAddress(util::AlignDown(GetInteger(current), PageSize)), remaining_pages);
                Kernel::GetSystemResourceLimit().Release(ams::svc::LimitableResource_PhysicalMemoryMax, remaining_size);
            }
        }

    }

    void SetInitialProcessBinaryPhysicalAddress(KPhysicalAddress phys_addr, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(g_initial_process_binary_phys_addr == Null<KPhysicalAddress>);

        g_initial_process_binary_phys_addr = phys_addr;
        g_initial_process_binary_size      = size;
    }

    KPhysicalAddress GetInitialProcessBinaryPhysicalAddress() {
        MESOSPHERE_INIT_ABORT_UNLESS(g_initial_process_binary_phys_addr != Null<KPhysicalAddress>);

        return g_initial_process_binary_phys_addr;
    }

    size_t GetInitialProcessBinarySize() {
        MESOSPHERE_INIT_ABORT_UNLESS(g_initial_process_binary_phys_addr != Null<KPhysicalAddress>);

        return g_initial_process_binary_size;
    }

    u64 GetInitialProcessIdMin() {
        return g_initial_process_id_min;
    }

    u64 GetInitialProcessIdMax() {
        return g_initial_process_id_max;
    }

    size_t GetInitialProcessesSecureMemorySize() {
        LoadInitialProcessBinaryHeader();

        return g_initial_process_secure_memory_size;
    }

    size_t CopyInitialProcessBinaryToKernelMemory() {
        LoadInitialProcessBinaryHeader();

        if (g_initial_process_binary_header.num_processes > 0) {
            /* Ensure that we have a non-zero size. */
            const size_t expected_size = g_initial_process_binary_size;
            MESOSPHERE_INIT_ABORT_UNLESS(expected_size != 0);

            /* Ensure that the size we need to reserve is as we expect it to be. */
            const size_t total_size = util::AlignUp(g_initial_process_binary_header.size, PageSize);
            MESOSPHERE_ABORT_UNLESS(total_size == expected_size);
            MESOSPHERE_ABORT_UNLESS(total_size <= InitialProcessBinarySizeMax);

            /* Reserve pages for the initial process binary from the system resource limit. */
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, total_size));

            return total_size;
        } else {
            return 0;
        }
    }

    void CreateAndRunInitialProcesses() {
        /* Allocate space for the processes. */
        InitialProcessInfo *infos = static_cast<InitialProcessInfo *>(__builtin_alloca(sizeof(InitialProcessInfo) * g_initial_process_binary_header.num_processes));

        /* Create the processes. */
        CreateProcesses(infos);

        /* Determine the initial process id range. */
        for (size_t i = 0; i < g_initial_process_binary_header.num_processes; i++) {
            const auto pid = infos[i].process->GetId();
            g_initial_process_id_min = std::min(g_initial_process_id_min, pid);
            g_initial_process_id_max = std::max(g_initial_process_id_max, pid);
        }

        /* Run the processes. */
        for (size_t i = 0; i < g_initial_process_binary_header.num_processes; i++) {
            MESOSPHERE_R_ABORT_UNLESS(infos[i].process->Run(infos[i].priority, infos[i].stack_size));
            infos[i].process->Close();
        }
    }

}

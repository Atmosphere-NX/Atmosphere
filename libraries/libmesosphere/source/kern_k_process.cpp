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

        constexpr u64 InitialProcessIdMin = 1;
        constexpr u64 InitialProcessIdMax = 0x50;
        std::atomic<u64> g_initial_process_id = InitialProcessIdMin;

    }

    void KProcess::Finalize() {
        MESOSPHERE_TODO_IMPLEMENT();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params) {
        MESOSPHERE_TODO_IMPLEMENT();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);
        MESOSPHERE_ABORT_UNLESS((params.code_num_pages * PageSize) / PageSize == params.code_num_pages);

        /* Set members. */
        this->memory_pool               = pool;
        this->resource_limit            = resource_limit;
        this->system_resource_address   = Null<KVirtualAddress>;
        this->system_resource_num_pages = 0;

        /* Setup page table. */
        /* NOTE: Nintendo passes process ID despite not having set it yet. */
        /* This goes completely unused, but even so... */
        {
            const auto as_type       = static_cast<ams::svc::CreateProcessFlag>(params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask);
            const bool enable_aslr   = (params.flags & ams::svc::CreateProcessFlag_EnableAslr);
            const bool is_app        = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
            auto *mem_block_manager  = std::addressof(is_app ? Kernel::GetApplicationMemoryBlockManager() : Kernel::GetSystemMemoryBlockManager());
            auto *block_info_manager = std::addressof(Kernel::GetBlockInfoManager());
            auto *pt_manager         = std::addressof(Kernel::GetPageTableManager());
            R_TRY(this->page_table.Initialize(this->process_id, as_type, enable_aslr, !enable_aslr, pool, params.code_address, params.code_num_pages * PageSize, mem_block_manager, block_info_manager, pt_manager));
        }
        auto pt_guard = SCOPE_GUARD { this->page_table.Finalize(); };

        /* Ensure we can insert the code region. */
        R_UNLESS(this->page_table.CanContain(params.code_address, params.code_num_pages * PageSize, KMemoryState_Code), svc::ResultInvalidMemoryRegion());

        /* Map the code region. */
        R_TRY(this->page_table.MapPageGroup(params.code_address, pg, KMemoryState_Code, KMemoryPermission_KernelRead));

        /* Initialize capabilities. */
        R_TRY(this->capabilities.Initialize(caps, num_caps, std::addressof(this->page_table)));

        /* Initialize the process id. */
        this->process_id = g_initial_process_id++;
        MESOSPHERE_ABORT_UNLESS(InitialProcessIdMin <= this->process_id);
        MESOSPHERE_ABORT_UNLESS(this->process_id <= InitialProcessIdMax);

        /* Initialize the rest of the process. */
        R_TRY(this->Initialize(params));

        /* Open a reference to the resource limit. */
        this->resource_limit->Open();

        /* We succeeded! */
        pt_guard.Cancel();
        return ResultSuccess();
    }

    void KProcess::DoWorkerTask() {
        MESOSPHERE_TODO_IMPLEMENT();
    }

    void KProcess::SetPreemptionState() {
        MESOSPHERE_TODO_IMPLEMENT();
    }

}

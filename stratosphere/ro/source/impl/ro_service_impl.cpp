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
#include "ro_nrr_utils.hpp"
#include "ro_nro_utils.hpp"
#include "ro_patcher.hpp"
#include "ro_service_impl.hpp"

namespace ams::ro::impl {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaxSessions = 0x4;
        constexpr size_t MaxNrrInfos = 0x40;
        constexpr size_t MaxNroInfos = 0x40;

        /* Types. */
        struct Sha256Hash {
            u8 hash[SHA256_HASH_SIZE];

            bool operator==(const Sha256Hash &o) const {
                return std::memcmp(this, &o, sizeof(*this)) == 0;
            }
            bool operator!=(const Sha256Hash &o) const {
                return std::memcmp(this, &o, sizeof(*this)) != 0;
            }
            bool operator<(const Sha256Hash &o) const {
                return std::memcmp(this, &o, sizeof(*this)) < 0;
            }
            bool operator>(const Sha256Hash &o) const {
                return std::memcmp(this, &o, sizeof(*this)) > 0;
            }
        };
        static_assert(sizeof(Sha256Hash) == sizeof(Sha256Hash::hash), "Sha256Hash definition!");

        struct NroInfo {
            u64 base_address;
            u64 nro_heap_address;
            u64 nro_heap_size;
            u64 bss_heap_address;
            u64 bss_heap_size;
            u64 code_size;
            u64 rw_size;
            ModuleId module_id;
            bool in_use;
        };

        struct NrrInfo {
            const NrrHeader *header;
            u64 nrr_heap_address;
            u64 nrr_heap_size;
            u64 mapped_code_address;
            bool in_use;
        };

        struct ProcessContext {
            NroInfo nro_infos[MaxNroInfos];
            NrrInfo nrr_infos[MaxNrrInfos];
            Handle process_handle;
            os::ProcessId process_id;
            bool in_use;

            ncm::ProgramId GetProgramId(Handle other_process_h) const {
                /* Automatically select a handle, allowing for override. */
                Handle process_h = this->process_handle;
                if (other_process_h != INVALID_HANDLE) {
                    process_h = other_process_h;
                }

                ncm::ProgramId program_id = ncm::ProgramId::Invalid;
                if (hos::GetVersion() >= hos::Version_300) {
                    /* 3.0.0+: Use svcGetInfo. */
                    R_ASSERT(svcGetInfo(&program_id.value, InfoType_ProgramId, process_h, 0));
                } else {
                    /* 1.0.0-2.3.0: We're not inside loader, so ask pm. */
                    R_ASSERT(pm::info::GetProgramId(&program_id, os::GetProcessId(process_h)));
                }
                return program_id;
            }

            Result GetNrrInfoByAddress(NrrInfo **out, u64 nrr_heap_address) {
                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (this->nrr_infos[i].in_use && this->nrr_infos[i].nrr_heap_address == nrr_heap_address) {
                        if (out != nullptr) {
                            *out = &this->nrr_infos[i];
                        }
                        return ResultSuccess();
                    }
                }
                return ResultNotRegistered();
            }

            Result GetFreeNrrInfo(NrrInfo **out) {
                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (!this->nrr_infos[i].in_use) {
                        if (out != nullptr) {
                            *out = &this->nrr_infos[i];
                        }
                        return ResultSuccess();
                    }
                }
                return ResultTooManyNrr();
            }

            Result GetNroInfoByAddress(NroInfo **out, u64 nro_address) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (this->nro_infos[i].in_use && this->nro_infos[i].base_address == nro_address) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess();
                    }
                }
                return ResultNotLoaded();
            }

            Result GetNroInfoByModuleId(NroInfo **out, const ModuleId *module_id) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (this->nro_infos[i].in_use && std::memcmp(&this->nro_infos[i].module_id, module_id, sizeof(*module_id)) == 0) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess();
                    }
                }
                return ResultNotLoaded();
            }

            Result GetFreeNroInfo(NroInfo **out) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (!this->nro_infos[i].in_use) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess();
                    }
                }
                return ResultTooManyNro();
            }

            Result ValidateHasNroHash(const NroHeader *nro_header) const {
                /* Calculate hash. */
                Sha256Hash hash;
                sha256CalculateHash(&hash, nro_header, nro_header->GetSize());

                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (this->nrr_infos[i].in_use) {
                        const NrrHeader *nrr_header = this->nrr_infos[i].header;
                        const Sha256Hash *nro_hashes = reinterpret_cast<const Sha256Hash *>(nrr_header->GetHashes());
                        R_UNLESS(!std::binary_search(nro_hashes, nro_hashes + nrr_header->GetNumHashes(), hash), ResultSuccess());
                    }
                }

                return ResultNotAuthorized();
            }

            Result ValidateNro(ModuleId *out_module_id, u64 *out_rx_size, u64 *out_ro_size, u64 *out_rw_size, u64 base_address, u64 expected_nro_size, u64 expected_bss_size) {
                /* Find space to map the NRO. */
                uintptr_t map_address;
                R_UNLESS(R_SUCCEEDED(map::LocateMappableSpace(&map_address, expected_nro_size)), ResultOutOfAddressSpace());

                /* Actually map the NRO. */
                map::AutoCloseMap nro_map(map_address, this->process_handle, base_address, expected_nro_size);
                R_TRY(nro_map.GetResult());

                /* Validate header. */
                const NroHeader *header = reinterpret_cast<const NroHeader *>(map_address);
                R_UNLESS(header->IsMagicValid(), ResultInvalidNro());

                /* Read sizes from header. */
                const u64 nro_size = header->GetSize();
                const u64 text_ofs = header->GetTextOffset();
                const u64 text_size = header->GetTextSize();
                const u64 ro_ofs = header->GetRoOffset();
                const u64 ro_size = header->GetRoSize();
                const u64 rw_ofs = header->GetRwOffset();
                const u64 rw_size = header->GetRwSize();
                const u64 bss_size = header->GetBssSize();

                /* Validate sizes meet expected. */
                R_UNLESS(nro_size == expected_nro_size, ResultInvalidNro());
                R_UNLESS(bss_size == expected_bss_size, ResultInvalidNro());

                /* Validate all sizes are aligned. */
                R_UNLESS(util::IsAligned(text_size, os::MemoryPageSize), ResultInvalidNro());
                R_UNLESS(util::IsAligned(ro_size,   os::MemoryPageSize), ResultInvalidNro());
                R_UNLESS(util::IsAligned(rw_size,   os::MemoryPageSize), ResultInvalidNro());
                R_UNLESS(util::IsAligned(bss_size,  os::MemoryPageSize), ResultInvalidNro());

                /* Validate sections are in order. */
                R_UNLESS(text_ofs <= ro_ofs, ResultInvalidNro());
                R_UNLESS(ro_ofs   <= rw_ofs, ResultInvalidNro());

                /* Validate sections are sequential and contiguous. */
                R_UNLESS(text_ofs == 0,                    ResultInvalidNro());
                R_UNLESS(text_ofs + text_size == ro_ofs,   ResultInvalidNro());
                R_UNLESS(ro_ofs + ro_size     == rw_ofs,   ResultInvalidNro());
                R_UNLESS(rw_ofs + rw_size     == nro_size, ResultInvalidNro());

                /* Verify NRO hash. */
                R_TRY(this->ValidateHasNroHash(header));

                /* Check if NRO has already been loaded. */
                const ModuleId *module_id = header->GetModuleId();
                R_UNLESS(R_FAILED(this->GetNroInfoByModuleId(nullptr, module_id)), ResultAlreadyLoaded());

                /* Apply patches to NRO. */
                LocateAndApplyIpsPatchesToModule(module_id, reinterpret_cast<u8 *>(map_address), nro_size);

                /* Copy to output. */
                *out_module_id = *module_id;
                *out_rx_size = text_size;
                *out_ro_size = ro_size;
                *out_rw_size = rw_size;
                return ResultSuccess();
            }
        };

        /* Globals. */
        ProcessContext g_process_contexts[MaxSessions] = {};
        bool g_is_development_hardware = false;
        bool g_is_development_function_enabled = false;

        /* Context Helpers. */
        ProcessContext *GetContextById(size_t context_id) {
            if (context_id == InvalidContextId) {
                return nullptr;
            }

            AMS_ASSERT(context_id < MaxSessions);
            return &g_process_contexts[context_id];
        }

        ProcessContext *GetContextByProcessId(os::ProcessId process_id) {
            for (size_t i = 0; i < MaxSessions; i++) {
                if (g_process_contexts[i].process_id == process_id) {
                    return &g_process_contexts[i];
                }
            }
            return nullptr;
        }

        size_t AllocateContext(Handle process_handle, os::ProcessId process_id) {
            /* Find a free process context. */
            for (size_t i = 0; i < MaxSessions; i++) {
                ProcessContext *context = &g_process_contexts[i];

                if (!context->in_use) {
                    std::memset(context, 0, sizeof(*context));
                    context->process_id = process_id;
                    context->process_handle = process_handle;
                    context->in_use = true;
                    return i;
                }
            }
            /* Failure to find a free context is actually an abort condition. */
            AMS_ASSERT(false);
        }

        void FreeContext(size_t context_id) {
            ProcessContext *context = GetContextById(context_id);
            if (context != nullptr) {
                if (context->process_handle != INVALID_HANDLE) {
                    for (size_t i = 0; i < MaxNrrInfos; i++) {
                        if (context->nrr_infos[i].in_use) {
                            UnmapNrr(context->process_handle, context->nrr_infos[i].header, context->nrr_infos[i].nrr_heap_address, context->nrr_infos[i].nrr_heap_size, context->nrr_infos[i].mapped_code_address);
                        }
                    }
                    svcCloseHandle(context->process_handle);
                }
                std::memset(context, 0, sizeof(*context));
                context->in_use = false;
            }
        }

        constexpr inline Result ValidateAddressAndNonZeroSize(u64 address, u64 size) {
            R_UNLESS(util::IsAligned(address, os::MemoryPageSize), ResultInvalidAddress());
            R_UNLESS(size != 0,                                    ResultInvalidSize());
            R_UNLESS(util::IsAligned(size, os::MemoryPageSize),    ResultInvalidSize());
            R_UNLESS(address < address + size,                     ResultInvalidSize());
            return ResultSuccess();
        }

        constexpr inline Result ValidateAddressAndSize(u64 address, u64 size) {
            R_UNLESS(util::IsAligned(address, os::MemoryPageSize), ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, os::MemoryPageSize),    ResultInvalidSize());
            R_UNLESS(size == 0 || address < address + size,        ResultInvalidSize());
            return ResultSuccess();
        }

    }

    /* Access utilities. */
    void SetDevelopmentHardware(bool is_development_hardware) {
        g_is_development_hardware = is_development_hardware;
    }

    void SetDevelopmentFunctionEnabled(bool is_development_function_enabled) {
        g_is_development_function_enabled = is_development_function_enabled;
    }

    bool IsDevelopmentHardware() {
        return g_is_development_hardware;
    }

    bool IsDevelopmentFunctionEnabled() {
        return g_is_development_function_enabled;
    }

    bool ShouldEaseNroRestriction() {
        /* Retrieve whether we should ease restrictions from set:sys. */
        u8 should_ease = 0;
        if (settings::fwdbg::GetSettingsItemValue(&should_ease, sizeof(should_ease), "ro", "ease_nro_restriction") != sizeof(should_ease)) {
            return false;
        }

        /* Nintendo only allows easing restriction on dev, we will allow on production, as well. */
        /* should_ease &= IsDevelopmentFunctionEnabled(); */
        return should_ease != 0;
    }

    /* Context utilities. */
    Result RegisterProcess(size_t *out_context_id, Handle process_handle, os::ProcessId process_id) {
        /* Validate process handle. */
        {
            os::ProcessId handle_pid = os::InvalidProcessId;

            /* Validate handle is a valid process handle. */
            R_UNLESS(R_SUCCEEDED(os::TryGetProcessId(&handle_pid, process_handle)), ResultInvalidProcess());

            /* Validate process id. */
            R_UNLESS(handle_pid == process_id, ResultInvalidProcess());
        }

        /* Check if a process context already exists. */
        R_UNLESS(GetContextByProcessId(process_id) == nullptr, ResultInvalidSession());

        *out_context_id = AllocateContext(process_handle, process_id);
        return ResultSuccess();
    }

    Result ValidateProcess(size_t context_id, os::ProcessId process_id) {
        const ProcessContext *ctx = GetContextById(context_id);
        R_UNLESS(ctx != nullptr,                ResultInvalidProcess());
        R_UNLESS(ctx->process_id == process_id, ResultInvalidProcess());
        return ResultSuccess();
    }

    void UnregisterProcess(size_t context_id) {
        FreeContext(context_id);
    }

    /* Service implementations. */
    Result LoadNrr(size_t context_id, Handle process_h, u64 nrr_address, u64 nrr_size, ModuleType expected_type, bool enforce_type) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ASSERT(context != nullptr);

        /* Get program id. */
        const ncm::ProgramId program_id = context->GetProgramId(process_h);

        /* Validate address/size. */
        R_TRY(ValidateAddressAndNonZeroSize(nrr_address, nrr_size));

        /* Check we have space for a new NRR. */
        NrrInfo *nrr_info = nullptr;
        R_TRY(context->GetFreeNrrInfo(&nrr_info));

        /* Map. */
        NrrHeader *header = nullptr;
        u64 mapped_code_address = 0;
        R_TRY(MapAndValidateNrr(&header, &mapped_code_address, context->process_handle, program_id, nrr_address, nrr_size, expected_type, enforce_type));

        /* Set NRR info. */
        nrr_info->in_use = true;
        nrr_info->header = header;
        nrr_info->nrr_heap_address = nrr_address;
        nrr_info->nrr_heap_size = nrr_size;
        nrr_info->mapped_code_address = mapped_code_address;

        return ResultSuccess();
    }

    Result UnloadNrr(size_t context_id, u64 nrr_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ASSERT(context != nullptr);

        /* Validate address. */
        R_UNLESS(util::IsAligned(nrr_address, os::MemoryPageSize), ResultInvalidAddress());

        /* Check the NRR is loaded. */
        NrrInfo *nrr_info = nullptr;
        R_TRY(context->GetNrrInfoByAddress(&nrr_info, nrr_address));

        /* Unmap. */
        const NrrInfo nrr_backup = *nrr_info;
        {
            /* Nintendo does this unconditionally, whether or not the actual unmap succeeds. */
            nrr_info->in_use = false;
            std::memset(nrr_info, 0, sizeof(*nrr_info));
        }
        return UnmapNrr(context->process_handle, nrr_backup.header, nrr_backup.nrr_heap_address, nrr_backup.nrr_heap_size, nrr_backup.mapped_code_address);
    }

    Result LoadNro(u64 *out_address, size_t context_id, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ASSERT(context != nullptr);

        /* Validate address/size. */
        R_TRY(ValidateAddressAndNonZeroSize(nro_address, nro_size));
        R_TRY(ValidateAddressAndSize(bss_address, bss_size));

        const u64 total_size = nro_size + bss_size;
        R_UNLESS(total_size >= nro_size, ResultInvalidSize());
        R_UNLESS(total_size >= bss_size, ResultInvalidSize());

        /* Check we have space for a new NRO. */
        NroInfo *nro_info = nullptr;
        R_TRY(context->GetFreeNroInfo(&nro_info));
        nro_info->nro_heap_address = nro_address;
        nro_info->nro_heap_size = nro_size;
        nro_info->bss_heap_address = bss_address;
        nro_info->bss_heap_size = bss_size;

        /* Map the NRO. */
        R_TRY(MapNro(&nro_info->base_address, context->process_handle, nro_address, nro_size, bss_address, bss_size));

        /* Validate the NRO (parsing region extents). */
        u64 rx_size = 0, ro_size = 0, rw_size = 0;
        {
            auto unmap_guard = SCOPE_GUARD { UnmapNro(context->process_handle, nro_info->base_address, nro_address, bss_address, bss_size, nro_size, 0); };
            R_TRY(context->ValidateNro(&nro_info->module_id, &rx_size, &ro_size, &rw_size, nro_info->base_address, nro_size, bss_size));
            unmap_guard.Cancel();
        }

        /* Set NRO perms. */
        {
            auto unmap_guard = SCOPE_GUARD { UnmapNro(context->process_handle, nro_info->base_address, nro_address, bss_address, bss_size, rx_size + ro_size, rw_size); };
            R_TRY(SetNroPerms(context->process_handle, nro_info->base_address, rx_size, ro_size, rw_size + bss_size));
            unmap_guard.Cancel();
        }

        nro_info->code_size = rx_size + ro_size;
        nro_info->rw_size = rw_size;
        nro_info->in_use = true;
        *out_address = nro_info->base_address;
        return ResultSuccess();
    }

    Result UnloadNro(size_t context_id, u64 nro_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ASSERT(context != nullptr);

        /* Validate address. */
        R_UNLESS(util::IsAligned(nro_address, os::MemoryPageSize), ResultInvalidAddress());

        /* Check the NRO is loaded. */
        NroInfo *nro_info = nullptr;
        R_TRY(context->GetNroInfoByAddress(&nro_info, nro_address));

        /* Unmap. */
        const NroInfo nro_backup = *nro_info;
        {
            /* Nintendo does this unconditionally, whether or not the actual unmap succeeds. */
            nro_info->in_use = false;
            std::memset(nro_info, 0, sizeof(*nro_info));
        }
        return UnmapNro(context->process_handle, nro_backup.base_address, nro_backup.nro_heap_address, nro_backup.bss_heap_address, nro_backup.bss_heap_size, nro_backup.code_size, nro_backup.rw_size);
    }

    /* Debug service implementations. */
    Result GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, os::ProcessId process_id) {
        size_t count = 0;
        const ProcessContext *context = GetContextByProcessId(process_id);
        if (context != nullptr) {
            for (size_t i = 0; i < MaxNroInfos && count < max_out_count; i++) {
                const NroInfo *nro_info = &context->nro_infos[i];
                if (!nro_info->in_use) {
                    continue;
                }

                /* Just copy out the info. */
                LoaderModuleInfo *out_info = &out_infos[count++];
                memcpy(out_info->build_id, &nro_info->module_id, sizeof(nro_info->module_id));
                out_info->base_address = nro_info->base_address;
                out_info->size = nro_info->nro_heap_size + nro_info->bss_heap_size;
            }
        }

        *out_count = static_cast<u32>(count);
        return ResultSuccess();
    }

}

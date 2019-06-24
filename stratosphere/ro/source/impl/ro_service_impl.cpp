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

#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>
#include <stratosphere/map.hpp>

#include "ro_nrr_utils.hpp"
#include "ro_nro_utils.hpp"
#include "ro_patcher.hpp"
#include "ro_service_impl.hpp"

namespace sts::ro::impl {

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
            u64 process_id;
            bool in_use;

            u64 GetTitleId(Handle other_process_h) const {
                /* Automatically select a handle, allowing for override. */
                Handle process_h = this->process_handle;
                if (other_process_h != INVALID_HANDLE) {
                    process_h = other_process_h;
                }

                u64 title_id = 0;
                if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
                    /* 3.0.0+: Use svcGetInfo. */
                    R_ASSERT(svcGetInfo(&title_id, InfoType_TitleId, process_h, 0));
                } else {
                    /* 1.0.0-2.3.0: We're not inside loader, so ask pm. */
                    u64 process_id = 0;
                    R_ASSERT(svcGetProcessId(&process_id, process_h));
                    R_ASSERT(pminfoGetTitleId(&title_id, process_id));
                }
                return title_id;
            }

            Result GetNrrInfoByAddress(NrrInfo **out, u64 nrr_heap_address) {
                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (this->nrr_infos[i].in_use && this->nrr_infos[i].nrr_heap_address == nrr_heap_address) {
                        if (out != nullptr) {
                            *out = &this->nrr_infos[i];
                        }
                        return ResultSuccess;
                    }
                }
                return ResultRoNotRegistered;
            }

            Result GetFreeNrrInfo(NrrInfo **out) {
                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (!this->nrr_infos[i].in_use) {
                        if (out != nullptr) {
                            *out = &this->nrr_infos[i];
                        }
                        return ResultSuccess;
                    }
                }
                return ResultRoTooManyNrr;
            }

            Result GetNroInfoByAddress(NroInfo **out, u64 nro_address) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (this->nro_infos[i].in_use && this->nro_infos[i].base_address == nro_address) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess;
                    }
                }
                return ResultRoNotLoaded;
            }

            Result GetNroInfoByModuleId(NroInfo **out, const ModuleId *module_id) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (this->nro_infos[i].in_use && std::memcmp(&this->nro_infos[i].module_id, module_id, sizeof(*module_id)) == 0) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess;
                    }
                }
                return ResultRoNotLoaded;
            }

            Result GetFreeNroInfo(NroInfo **out) {
                for (size_t i = 0; i < MaxNroInfos; i++) {
                    if (!this->nro_infos[i].in_use) {
                        if (out != nullptr) {
                            *out = &this->nro_infos[i];
                        }
                        return ResultSuccess;
                    }
                }
                return ResultRoTooManyNro;
            }

            Result ValidateHasNroHash(const NroHeader *nro_header) const {
                /* Calculate hash. */
                Sha256Hash hash;
                sha256CalculateHash(&hash, nro_header, nro_header->GetSize());

                for (size_t i = 0; i < MaxNrrInfos; i++) {
                    if (this->nrr_infos[i].in_use) {
                        const NrrHeader *nrr_header = this->nrr_infos[i].header;
                        const Sha256Hash *nro_hashes = reinterpret_cast<const Sha256Hash *>(nrr_header->GetHashes());
                        if (std::binary_search(nro_hashes, nro_hashes + nrr_header->GetNumHashes(), hash)) {
                            return ResultSuccess;
                        }
                    }
                }

                return ResultRoNotAuthorized;
            }

            Result ValidateNro(ModuleId *out_module_id, u64 *out_rx_size, u64 *out_ro_size, u64 *out_rw_size, u64 base_address, u64 expected_nro_size, u64 expected_bss_size) {
                /* Find space to map the NRO. */
                uintptr_t map_address;
                if (R_FAILED(map::LocateMappableSpace(&map_address, expected_nro_size))) {
                    return ResultRoInsufficientAddressSpace;
                }

                /* Actually map the NRO. */
                map::AutoCloseMap nro_map(map_address, this->process_handle, base_address, expected_nro_size);
                if (!nro_map.IsSuccess()) {
                    return nro_map.GetResult();
                }

                /* Validate header. */
                const NroHeader *header = reinterpret_cast<const NroHeader *>(map_address);
                if (!header->IsMagicValid()) {
                    return ResultRoInvalidNro;
                }

                const u64 nro_size = header->GetSize();
                const u64 text_ofs = header->GetTextOffset();
                const u64 text_size = header->GetTextSize();
                const u64 ro_ofs = header->GetRoOffset();
                const u64 ro_size = header->GetRoSize();
                const u64 rw_ofs = header->GetRwOffset();
                const u64 rw_size = header->GetRwSize();
                const u64 bss_size = header->GetBssSize();
                if (nro_size != expected_nro_size || bss_size != expected_bss_size) {
                    return ResultRoInvalidNro;
                }
                if ((text_size & 0xFFF) || (ro_size & 0xFFF) || (rw_size & 0xFFF) || (bss_size & 0xFFF)) {
                    return ResultRoInvalidNro;
                }
                if (text_ofs > ro_ofs || ro_ofs > rw_ofs) {
                    return ResultRoInvalidNro;
                }
                if (text_ofs != 0 || text_ofs + text_size != ro_ofs || ro_ofs + ro_size != rw_ofs || rw_ofs + rw_size != nro_size) {
                    return ResultRoInvalidNro;
                }

                /* Verify NRO hash. */
                R_TRY(this->ValidateHasNroHash(header));

                /* Check if NRO has already been loaded. */
                const ModuleId *module_id = header->GetModuleId();
                if (R_SUCCEEDED(this->GetNroInfoByModuleId(nullptr, module_id))) {
                    return ResultRoAlreadyLoaded;
                }

                /* Apply patches to NRO. */
                LocateAndApplyIpsPatchesToModule(module_id, reinterpret_cast<u8 *>(map_address), nro_size);

                /* Copy to output. */
                *out_module_id = *module_id;
                *out_rx_size = text_size;
                *out_ro_size = ro_size;
                *out_rw_size = rw_size;
                return ResultSuccess;
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
            } else if (context_id < MaxSessions) {
                return &g_process_contexts[context_id];
            } else {
                std::abort();
            }
        }

        ProcessContext *GetContextByProcessId(u64 process_id) {
            for (size_t i = 0; i < MaxSessions; i++) {
                if (g_process_contexts[i].process_id == process_id) {
                    return &g_process_contexts[i];
                }
            }
            return nullptr;
        }

        size_t AllocateContext(Handle process_handle, u64 process_id) {
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
            std::abort();
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
        bool should_ease = false;
        if (R_FAILED(setsysGetSettingsItemValue("ro", "ease_nro_restriction", &should_ease, sizeof(should_ease)))) {
            return false;
        }

        /* Nintendo only allows easing restriction on dev, we will allow on production, as well. */
        /* should_ease &= IsDevelopmentFunctionEnabled(); */
        return should_ease;
    }

    /* Context utilities. */
    Result RegisterProcess(size_t *out_context_id, Handle process_handle, u64 process_id) {
        /* Validate process handle. */
        {
            u64 handle_pid = 0;

            /* Validate handle is a valid process handle. */
            if (R_FAILED(svcGetProcessId(&handle_pid, process_handle))) {
                return ResultRoInvalidProcess;
            }

            /* Validate process id. */
            if (handle_pid != process_id) {
                return ResultRoInvalidProcess;
            }
        }

        /* Check if a process context already exists. */
        if (GetContextByProcessId(process_id) != nullptr) {
            return ResultRoInvalidSession;
        }

        *out_context_id = AllocateContext(process_handle, process_id);
        return ResultSuccess;
    }

    Result ValidateProcess(size_t context_id, u64 process_id) {
        const ProcessContext *ctx = GetContextById(context_id);
        if (ctx == nullptr || ctx->process_id != process_id) {
            return ResultRoInvalidProcess;
        }
        return ResultSuccess;
    }

    void UnregisterProcess(size_t context_id) {
        FreeContext(context_id);
    }

    /* Service implementations. */
    Result LoadNrr(size_t context_id, Handle process_h, u64 nrr_address, u64 nrr_size, ModuleType expected_type, bool enforce_type) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        if (context == nullptr) {
            std::abort();
        }

        /* Get title id. */
        const u64 title_id = context->GetTitleId(process_h);

        /* Validate address/size. */
        if (nrr_address & 0xFFF) {
            return ResultRoInvalidAddress;
        }
        if (nrr_size == 0 || (nrr_size & 0xFFF) || !(nrr_address < nrr_address + nrr_size)) {
            return ResultRoInvalidSize;
        }

        /* Check we have space for a new NRR. */
        NrrInfo *nrr_info = nullptr;
        R_TRY(context->GetFreeNrrInfo(&nrr_info));

        /* Map. */
        NrrHeader *header = nullptr;
        u64 mapped_code_address = 0;
        R_TRY(MapAndValidateNrr(&header, &mapped_code_address, context->process_handle, title_id, nrr_address, nrr_size, expected_type, enforce_type));

        /* Set NRR info. */
        nrr_info->in_use = true;
        nrr_info->header = header;
        nrr_info->nrr_heap_address = nrr_address;
        nrr_info->nrr_heap_size = nrr_size;
        nrr_info->mapped_code_address = mapped_code_address;

        return ResultSuccess;
    }

    Result UnloadNrr(size_t context_id, u64 nrr_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        if (context == nullptr) {
            std::abort();
        }

        /* Validate address. */
        if (nrr_address & 0xFFF) {
            return ResultRoInvalidAddress;
        }

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
        if (context == nullptr) {
            std::abort();
        }

        /* Validate address/size. */
        if (nro_address & 0xFFF) {
            return ResultRoInvalidAddress;
        }
        if (nro_size == 0 || (nro_size & 0xFFF) || !(nro_address < nro_address + nro_size)) {
            return ResultRoInvalidSize;
        }
        if (bss_address & 0xFFF) {
            return ResultRoInvalidAddress;
        }
        if ((bss_size & 0xFFF) || (bss_size > 0 && !(bss_address < bss_address + bss_size))) {
            return ResultRoInvalidSize;
        }

        const u64 total_size = nro_size + bss_size;
        if (total_size < nro_size || total_size < bss_size) {
            return ResultRoInvalidSize;
        }

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
        u64 rx_size, ro_size, rw_size;
        R_TRY_CLEANUP(context->ValidateNro(&nro_info->module_id, &rx_size, &ro_size, &rw_size, nro_info->base_address, nro_size, bss_size), {
            UnmapNro(context->process_handle, nro_info->base_address, nro_address, bss_address, bss_size, nro_size, 0);
        });

        /* Set NRO perms. */
        R_TRY_CLEANUP(SetNroPerms(context->process_handle, nro_info->base_address, rx_size, ro_size, rw_size + bss_size), {
            UnmapNro(context->process_handle, nro_info->base_address, nro_address, bss_address, bss_size, rx_size + ro_size, rw_size);
        });

        nro_info->code_size = rx_size + ro_size;
        nro_info->rw_size = rw_size;
        nro_info->in_use = true;
        *out_address = nro_info->base_address;
        return ResultSuccess;
    }

    Result UnloadNro(size_t context_id, u64 nro_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        if (context == nullptr) {
            std::abort();
        }

        /* Validate address. */
        if (nro_address & 0xFFF) {
            return ResultRoInvalidAddress;
        }

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
    Result GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, u64 process_id) {
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
        return ResultSuccess;
    }

}

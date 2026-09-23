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
#include "ro_nrr_utils.hpp"
#include "ro_nro_utils.hpp"
#include "ro_patcher.hpp"
#include "ro_random.hpp"
#include "ro_service_impl.hpp"

namespace ams::ro::impl {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaxSessions = 0x3;  /* 2 official sessions (applet + application, 1 homebrew session). */
        constexpr size_t MaxNrrInfos = 0x40;
        constexpr size_t MaxNroInfos = 0x40;

        /* Types. */
        struct Sha256Hash {
            u8 hash[crypto::Sha256Generator::HashSize];

            bool operator==(const Sha256Hash &o) const {
                return std::memcmp(this, std::addressof(o), sizeof(*this)) == 0;
            }
            bool operator!=(const Sha256Hash &o) const {
                return std::memcmp(this, std::addressof(o), sizeof(*this)) != 0;
            }
            bool operator<(const Sha256Hash &o) const {
                return std::memcmp(this, std::addressof(o), sizeof(*this)) < 0;
            }
            bool operator>(const Sha256Hash &o) const {
                return std::memcmp(this, std::addressof(o), sizeof(*this)) > 0;
            }
        };
        static_assert(sizeof(Sha256Hash) == sizeof(Sha256Hash::hash));
        
        struct NroInfo {
            u64 base_address;
            u64 nro_heap_address;
            u64 nro_heap_size;
            u64 bss_heap_address;
            u64 bss_heap_size;
            u64 code_size;
            u64 ro_size;        /* 23.0.0+ */
            u64 rw_size;
            ModuleId module_id;
            bool is_compress;   /* 23.0.0+ */
        };

        struct NrrInfo {
            const NrrHeader *mapped_header;
            u64 nrr_heap_address;
            u64 nrr_heap_size;
            u64 mapped_code_address;

            /* Verification. */
            u32 cached_signed_area_size;
            u32 cached_hashes_offset;
            u32 cached_num_hashes;
            u8  cached_signed_area[sizeof(NrrHeader) - NrrHeader::GetSignedAreaOffset()];
            Sha256Hash signed_area_hash;
        };
        
        struct AdditionalNroHeaderInfo {
            u32 signature;
            u8  reserved_04[0xC];
            u32 compressed_size;
            u32 reserved_14;
            u32 hash_size;
            u32 hash_offset;
            u8  reserved_20[0x40];
        };
        
        struct ProcessContext {
            private:
                bool m_nro_in_use[MaxNroInfos]{};
                bool m_nrr_in_use[MaxNrrInfos]{};
                NroInfo m_nro_infos[MaxNroInfos]{};
                NrrInfo m_nrr_infos[MaxNrrInfos]{};
                os::NativeHandle m_process_handle = os::InvalidNativeHandle;
                os::ProcessId m_process_id = os::InvalidProcessId;
                bool m_in_use{};
            public:
                constexpr ProcessContext() = default;

                void Initialize(os::NativeHandle process_handle, os::ProcessId process_id) {
                    AMS_ABORT_UNLESS(!m_in_use);

                    std::memset(m_nro_in_use, 0, sizeof(m_nro_in_use));
                    std::memset(m_nrr_in_use, 0, sizeof(m_nrr_in_use));
                    std::memset(m_nro_infos, 0, sizeof(m_nro_infos));
                    std::memset(m_nrr_infos, 0, sizeof(m_nrr_infos));

                    m_process_handle = process_handle;
                    m_process_id     = process_id;
                    m_in_use         = true;
                }

                void Finalize() {
                    AMS_ABORT_UNLESS(m_in_use);

                    if (m_process_handle != os::InvalidNativeHandle) {
                        for (size_t i = 0; i < MaxNrrInfos; i++) {
                            if (m_nrr_in_use[i]) {
                                R_DISCARD(UnmapNrr(m_process_handle, m_nrr_infos[i].mapped_header, m_nrr_infos[i].nrr_heap_address, m_nrr_infos[i].nrr_heap_size, m_nrr_infos[i].mapped_code_address));
                            }
                        }
                        os::CloseNativeHandle(m_process_handle);
                    }

                    std::memset(m_nro_in_use, 0, sizeof(m_nro_in_use));
                    std::memset(m_nrr_in_use, 0, sizeof(m_nrr_in_use));
                    std::memset(m_nro_infos, 0, sizeof(m_nro_infos));
                    std::memset(m_nrr_infos, 0, sizeof(m_nrr_infos));

                    m_process_handle = os::InvalidNativeHandle;
                    m_process_id     = os::InvalidProcessId;

                    m_in_use         = false;
                }

                os::NativeHandle GetProcessHandle() const {
                    return m_process_handle;
                }

                os::ProcessId GetProcessId() const {
                    return m_process_id;
                }

                bool IsFree() const {
                    return !m_in_use;
                }

                ncm::ProgramId GetProgramId(os::NativeHandle other_process_h) const {
                    /* Automatically select a handle, allowing for override. */
                    if (other_process_h != os::InvalidNativeHandle) {
                        return os::GetProgramId(other_process_h);
                    } else {
                        return os::GetProgramId(m_process_handle);
                    }
                }

                Result GetNrrInfoByAddress(NrrInfo **out, u64 nrr_heap_address) {
                    for (size_t i = 0; i < MaxNrrInfos; i++) {
                        if (m_nrr_in_use[i] && m_nrr_infos[i].nrr_heap_address == nrr_heap_address) {
                            if (out != nullptr) {
                                *out = m_nrr_infos + i;
                            }
                            R_SUCCEED();
                        }
                    }
                    R_THROW(ro::ResultNotRegistered());
                }

                Result GetFreeNrrInfo(NrrInfo **out) {
                    for (size_t i = 0; i < MaxNrrInfos; i++) {
                        if (!m_nrr_in_use[i]) {
                            if (out != nullptr) {
                                *out = m_nrr_infos + i;
                            }
                            R_SUCCEED();
                        }
                    }
                    R_THROW(ro::ResultTooManyNrr());
                }

                Result GetNroInfoByAddress(NroInfo **out, u64 nro_address) {
                    for (size_t i = 0; i < MaxNroInfos; i++) {
                        if (m_nro_in_use[i] && m_nro_infos[i].base_address == nro_address) {
                            if (out != nullptr) {
                                *out = m_nro_infos + i;
                            }
                            R_SUCCEED();
                        }
                    }
                    R_THROW(ro::ResultNotLoaded());
                }

                Result GetNroInfoByModuleId(NroInfo **out, const ModuleId *module_id) {
                    for (size_t i = 0; i < MaxNroInfos; i++) {
                        if (m_nro_in_use[i] && std::memcmp(std::addressof(m_nro_infos[i].module_id), module_id, sizeof(*module_id)) == 0) {
                            if (out != nullptr) {
                                *out = m_nro_infos + i;
                            }
                            R_SUCCEED();
                        }
                    }
                    R_THROW(ro::ResultNotLoaded());
                }

                Result GetFreeNroInfo(NroInfo **out) {
                    for (size_t i = 0; i < MaxNroInfos; i++) {
                        if (!m_nro_in_use[i]) {
                            if (out != nullptr) {
                                *out = m_nro_infos + i;
                            }
                            R_SUCCEED();
                        }
                    }
                    R_THROW(ro::ResultTooManyNro());
                }

                Result ValidateHasNroHash(const NroHeader *nro_header) const {
                    /* Calculate hash. */
                    Sha256Hash hash;
                    crypto::GenerateSha256(std::addressof(hash), sizeof(hash), nro_header, nro_header->GetSize());

                    for (size_t i = 0; i < MaxNrrInfos; i++) {
                        /* Ensure we only check NRRs that are used. */
                        if (!m_nrr_in_use[i]) {
                            continue;
                        }

                        /* Get the mapped header, ensure that it has hashes. */
                        const NrrHeader *mapped_nrr_header = m_nrr_infos[i].mapped_header;
                        const size_t mapped_num_hashes = mapped_nrr_header->GetNumHashes();
                        if (mapped_num_hashes == 0) {
                            continue;
                        }

                        /* Locate the hash within the mapped array. */
                        const Sha256Hash *mapped_nro_hashes_start = reinterpret_cast<const Sha256Hash *>(mapped_nrr_header->GetHashes());
                        const Sha256Hash *mapped_nro_hashes_end   = mapped_nro_hashes_start + mapped_nrr_header->GetNumHashes();

                        const Sha256Hash *mapped_lower_bound = std::lower_bound(mapped_nro_hashes_start, mapped_nro_hashes_end, hash);
                        if (mapped_lower_bound == mapped_nro_hashes_end || (*mapped_lower_bound != hash)) {
                            continue;
                        }

                        /* Check that the hash entry is valid, since our heuristic passed. */
                        const void *nrr_hash          = std::addressof(m_nrr_infos[i].signed_area_hash);
                        const void *signed_area       = m_nrr_infos[i].cached_signed_area;
                        const size_t signed_area_size = m_nrr_infos[i].cached_signed_area_size;
                        const size_t hashes_offset    = m_nrr_infos[i].cached_hashes_offset;
                        const size_t num_hashes       = m_nrr_infos[i].cached_num_hashes;
                        const u8 *hash_table          = reinterpret_cast<const u8 *>(mapped_nro_hashes_start);
                        if (!ValidateNrrHashTableEntry(signed_area, signed_area_size, hashes_offset, num_hashes, nrr_hash, hash_table, std::addressof(hash))) {
                            continue;
                        }

                        /* The hash is valid! */
                        R_SUCCEED();
                    }

                    R_THROW(ro::ResultNotAuthorized());
                }
                
                Result ValidateNro(NroHeader **out_header, AdditionalNroHeaderInfo **out_additional_header_info, void *out_additional_header_hash, u8 *mapped_memory, u64 expected_nro_size, u64 expected_bss_size) {
                    /* Validate header. */
                    NroHeader *header = reinterpret_cast<NroHeader *>(mapped_memory);
                    R_UNLESS(header->IsMagicValid(), ro::ResultInvalidNro());
                    
                    /* Read sizes from header. */
                    const u64 nro_size = header->GetSize();
                    const u64 text_ofs = header->GetTextOffset();
                    const u64 text_size = header->GetTextSize();
                    const u64 ro_ofs = header->GetRoOffset();
                    const u64 ro_size = header->GetRoSize();
                    const u64 rw_ofs = header->GetRwOffset();
                    const u64 rw_size = header->GetRwSize();
                    const u64 bss_size = header->GetBssSize();
                    
                    /* Validate all sizes are aligned. */
                    R_UNLESS(util::IsAligned(text_size, os::MemoryPageSize), ro::ResultInvalidNro());
                    R_UNLESS(util::IsAligned(ro_size,   os::MemoryPageSize), ro::ResultInvalidNro());
                    R_UNLESS(util::IsAligned(rw_size,   os::MemoryPageSize), ro::ResultInvalidNro());
                    R_UNLESS(util::IsAligned(bss_size,  os::MemoryPageSize), ro::ResultInvalidNro());

                    /* Validate sizes meet expected. */
                    R_UNLESS(nro_size == expected_nro_size, ro::ResultInvalidNro());
                    
                    /* Account for compression when checking bss size. */
                    if (header->IsCompress()) {
                        R_UNLESS((bss_size - expected_nro_size + (rw_ofs + rw_size)) == expected_bss_size, ro::ResultInvalidBssSize());
                    } else {
                        /* 23.0.0+ uses the new error code here too. */
                        if (hos::GetVersion() >= hos::Version_23_0_0) {
                            R_UNLESS(bss_size == expected_bss_size, ro::ResultInvalidBssSize());
                        } else {
                            R_UNLESS(bss_size == expected_bss_size, ro::ResultInvalidNro());
                        }
                    }

                    /* Validate sections are in order. */
                    R_UNLESS(text_ofs <= ro_ofs, ro::ResultInvalidNro());
                    R_UNLESS(ro_ofs   <= rw_ofs, ro::ResultInvalidNro());

                    /* Validate sections are sequential and contiguous. */
                    R_UNLESS(text_ofs == 0,                    ro::ResultInvalidNro());
                    R_UNLESS(text_ofs + text_size == ro_ofs,   ro::ResultInvalidNro());
                    R_UNLESS(ro_ofs + ro_size     == rw_ofs,   ro::ResultInvalidNro());
                    
                    /* Only check data section if not using compression. */
                    if (!header->IsCompress()) {
                        R_UNLESS(rw_ofs + rw_size == nro_size, ro::ResultInvalidNro());
                    }
                    
                    /* Verify NRO hash. */
                    R_TRY(this->ValidateHasNroHash(header));

                    /* Check if NRO has already been loaded. */
                    const ModuleId *module_id = header->GetModuleId();
                    R_UNLESS(R_FAILED(this->GetNroInfoByModuleId(nullptr, module_id)), ro::ResultAlreadyLoaded());
                    
                    /* Validate compression related fields. */
                    if (header->IsCompress()) {
                        const u64 additional_header_offset = header->GetAdditionalHeaderOffset();
                        
                        /* Compression must have aligned header as well. */
                        R_UNLESS(header->IsAlignedHeader(), ro::ResultInvalidNro());
                        
                        /* Validate additional header offset. */
                        R_UNLESS(additional_header_offset == 0x80, ro::ResultInvalidNro());
                        
                        /* Validate additional header. */
                        AdditionalNroHeader *additional_header = reinterpret_cast<AdditionalNroHeader *>(mapped_memory + additional_header_offset);
                        R_UNLESS(additional_header->IsMagicValid(), ro::ResultInvalidNro());
                        
                        /* Read sizes from header. */
                        const u64 hash_size = additional_header->GetHashSize();
                        const u64 hash_offset = additional_header->GetHashOffset();
                        const u64 compressed_size = additional_header->GetCompressedSize();
                        
                        /* Validate additional header hash size and offset. */
                        R_UNLESS(hash_size == 0x20, ro::ResultInvalidNro());
                        R_UNLESS(hash_offset == 0xE0, ro::ResultInvalidNro());
                        
                        /* Copy the additional header hash to output. */
                        std::memcpy(out_additional_header_hash, additional_header->GetHash(), hash_size);
                        
                        /* Validate compressed size. */
                        R_UNLESS(rw_ofs + rw_size > nro_size, ro::ResultInvalidNro());
                        R_UNLESS(compressed_size >= 0x1000, ro::ResultInvalidNro());
                        R_UNLESS(util::AlignUp(compressed_size, os::MemoryPageSize) == expected_nro_size, ro::ResultInvalidNro());
                        
                        /* Copy the additional header info to output. This is AdditionalNroHeader minus the hash. */
                        std::memcpy(out_additional_header_info, additional_header, sizeof(*additional_header) - hash_size);
                    } else {
                        /* Explicitly clear additional header hash and info. */
                        std::memset(out_additional_header_hash, 0, crypto::Sha256Generator::HashSize);
                        std::memset(out_additional_header_info, 0, sizeof(AdditionalNroHeaderInfo));
                    }
                    
                    /* Copy the header to output. */
                    *out_header = header;
                    R_SUCCEED();
                }
                
                Result DecompressNro(u8 *mapped_nro, u64 expected_nro_size, const NroHeader *nro_header, u32 compressed_size) {
                    /* TODO */
                    AMS_UNUSED(mapped_nro, expected_nro_size, nro_header, compressed_size);
                    R_SUCCEED();
                }
                
                Result CheckAdditionalHeaderHash(const u8 *mapped_nro, const void *additional_nro_header_hash, size_t additional_nro_header_hash_size, u64 rw_ofs, u64 rw_size) {
                    /* Calculate hash. */
                    Sha256Hash nro_rw_hash;
                    crypto::GenerateSha256(std::addressof(nro_rw_hash), additional_nro_header_hash_size, mapped_nro + 0x1000, (rw_ofs + rw_size) - 0x1000);
                    
                    /* Compare the hashes. */
                    const Sha256Hash *expected_hash = reinterpret_cast<const Sha256Hash *>(additional_nro_header_hash);
                    if (*expected_hash != nro_rw_hash) {
                        R_THROW(ro::ResultInvalidNro());
                    }
                    R_SUCCEED();
                }

                Result ParseNro(ModuleId *out_module_id, u64 *out_rx_size, u64 *out_ro_size, u64 *out_rw_size, bool *out_aligned_header, bool *out_is_compress, u64 base_address, u64 expected_nro_size, u64 expected_bss_size) {
                    /* Map the NRO. */
                    void *mapped_memory = nullptr;
                    R_TRY_CATCH(os::MapProcessMemory(std::addressof(mapped_memory), m_process_handle, base_address, expected_nro_size, ro::impl::GenerateSecureRandom)) {
                        R_CONVERT(os::ResultOutOfAddressSpace, ro::ResultOutOfAddressSpace())
                    } R_END_TRY_CATCH;

                    /* When we're done, unmap the memory. */
                    ON_SCOPE_EXIT { os::UnmapProcessMemory(mapped_memory, m_process_handle, base_address, expected_nro_size); };
                    
                    /* ValidateNro */
                    NroHeader *nro_header = nullptr;
                    AdditionalNroHeaderInfo *additional_nro_header_info = nullptr;
                    u8 additional_nro_header_hash[crypto::Sha256Generator::HashSize];
                    R_TRY(this->ValidateNro(std::addressof(nro_header), std::addressof(additional_nro_header_info), std::addressof(additional_nro_header_hash), static_cast<u8 *>(mapped_memory), expected_nro_size, expected_bss_size));
                    
                    /* DecompressNro */
                    if (nro_header->IsCompress()) {
                        R_TRY(this->DecompressNro(static_cast<u8 *>(mapped_memory), expected_nro_size, nro_header, additional_nro_header_info->compressed_size));
                        
                        /* CheckAdditionalHeaderHash */
                        if (nro_header->GetAdditionalHeaderOffset() != 0) {
                            R_TRY(this->CheckAdditionalHeaderHash(static_cast<u8 *>(mapped_memory), std::addressof(additional_nro_header_hash), sizeof(additional_nro_header_hash), nro_header->GetRwOffset(), nro_header->GetRwSize()));
                        }
                    }
                    
                    /* Apply patches to NRO. */
                    const ModuleId *module_id = nro_header->GetModuleId();
                    LocateAndApplyIpsPatchesToModule(module_id, static_cast<u8 *>(mapped_memory), nro_header->GetSize());

                    /* Copy to output. */
                    *out_module_id = *module_id;
                    *out_rx_size = nro_header->GetTextSize();
                    *out_ro_size = nro_header->GetRoSize();
                    *out_rw_size = nro_header->GetRwSize();
                    *out_aligned_header = nro_header->IsAlignedHeader();
                    *out_is_compress = nro_header->IsCompress();
                    R_SUCCEED();
                }

                void SetNrrInfoInUse(const NrrInfo *info, bool in_use) {
                    AMS_ASSERT(std::addressof(m_nrr_infos[0]) <= info && info <= std::addressof(m_nrr_infos[MaxNrrInfos - 1]));
                    const size_t index = info - std::addressof(m_nrr_infos[0]);
                    m_nrr_in_use[index] = in_use;
                }

                void SetNroInfoInUse(const NroInfo *info, bool in_use) {
                    AMS_ASSERT(std::addressof(m_nro_infos[0]) <= info && info <= std::addressof(m_nro_infos[MaxNroInfos - 1]));
                    const size_t index = info - std::addressof(m_nro_infos[0]);
                    m_nro_in_use[index] = in_use;
                }

                void GetProcessModuleInfo(u32 *out_count, ldr::ModuleInfo *out_infos, size_t max_out_count) const {
                    size_t count = 0;

                    for (size_t i = 0; i < MaxNroInfos && count < max_out_count; i++) {
                        if (!m_nro_in_use[i]) {
                            continue;
                        }

                        const NroInfo *nro_info = m_nro_infos + i;

                        /* Just copy out the info. */
                        auto &out_info = out_infos[count++];

                        std::memcpy(out_info.module_id, nro_info->module_id.data, sizeof(out_info.module_id));
                        out_info.address = nro_info->base_address;
                        out_info.size    = nro_info->nro_heap_size + nro_info->bss_heap_size;
                    }

                    *out_count = static_cast<u32>(count);
                }
        };

        /* Globals. */
        constinit ProcessContext g_process_contexts[MaxSessions] = {};
        constinit bool g_is_development_hardware = false;
        constinit bool g_is_development_function_enabled = false;

        /* Context Helpers. */
        ProcessContext *GetContextById(size_t context_id) {
            if (context_id == InvalidContextId) {
                return nullptr;
            }

            AMS_ABORT_UNLESS(context_id < MaxSessions);
            return g_process_contexts + context_id;
        }

        ProcessContext *GetContextByProcessId(os::ProcessId process_id) {
            for (size_t i = 0; i < MaxSessions; i++) {
                if (g_process_contexts[i].GetProcessId() == process_id) {
                    return g_process_contexts + i;
                }
            }
            return nullptr;
        }

        size_t AllocateContext(os::NativeHandle process_handle, os::ProcessId process_id) {
            /* Find a free process context. */
            for (size_t i = 0; i < MaxSessions; i++) {
                ProcessContext *context = g_process_contexts + i;

                if (context->IsFree()) {
                    context->Initialize(process_handle, process_id);
                    return i;
                }
            }
            /* Failure to find a free context is actually an abort condition. */
            AMS_ABORT_UNLESS(false);
        }

        void FreeContext(size_t context_id) {
            if (ProcessContext *context = GetContextById(context_id); context != nullptr) {
                context->Finalize();
            }
        }

        constexpr inline Result ValidateAddressAndNonZeroSize(u64 address, u64 size) {
            R_UNLESS(util::IsAligned(address, os::MemoryPageSize), ro::ResultInvalidAddress());
            R_UNLESS(size != 0,                                    ro::ResultInvalidSize());
            R_UNLESS(util::IsAligned(size, os::MemoryPageSize),    ro::ResultInvalidSize());
            R_UNLESS(address < address + size,                     ro::ResultInvalidSize());
            R_SUCCEED();
        }

        constexpr inline Result ValidateAddressAndSize(u64 address, u64 size) {
            R_UNLESS(util::IsAligned(address, os::MemoryPageSize), ro::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, os::MemoryPageSize),    ro::ResultInvalidSize());
            R_UNLESS(size == 0 || address < address + size,        ro::ResultInvalidSize());
            R_SUCCEED();
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
        if (settings::fwdbg::GetSettingsItemValue(std::addressof(should_ease), sizeof(should_ease), "ro", "ease_nro_restriction") != sizeof(should_ease)) {
            return false;
        }

        /* Nintendo only allows easing restriction on dev, we will allow on production, as well. */
        /* should_ease &= IsDevelopmentFunctionEnabled(); */
        return should_ease != 0;
    }

    /* Context utilities. */
    Result RegisterProcess(size_t *out_context_id, sf::NativeHandle &&process_handle, os::ProcessId process_id) {
        /* Validate process handle. */
        {
            /* Validate handle is a valid process handle. */
            os::ProcessId handle_pid;
            R_UNLESS(R_SUCCEEDED(os::GetProcessId(std::addressof(handle_pid), process_handle.GetOsHandle())), ro::ResultInvalidProcess());

            /* Validate process id. */
            R_UNLESS(handle_pid == process_id, ro::ResultInvalidProcess());
        }

        /* Check if a process context already exists. */
        R_UNLESS(GetContextByProcessId(process_id) == nullptr, ro::ResultInvalidSession());

        /* Allocate a context to manage the process handle. */
        *out_context_id = AllocateContext(process_handle.GetOsHandle(), process_id);
        process_handle.Detach();

        R_SUCCEED();
    }

    Result ValidateProcess(size_t context_id, os::ProcessId process_id) {
        const ProcessContext *ctx = GetContextById(context_id);
        R_UNLESS(ctx != nullptr,                    ro::ResultInvalidProcess());
        R_UNLESS(ctx->GetProcessId() == process_id, ro::ResultInvalidProcess());
        R_SUCCEED();
    }

    void UnregisterProcess(size_t context_id) {
        FreeContext(context_id);
    }

    /* Service implementations. */
    Result RegisterModuleInfo(size_t context_id, os::NativeHandle process_handle, u64 nrr_address, u64 nrr_size, NrrKind nrr_kind, bool enforce_nrr_kind) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ABORT_UNLESS(context != nullptr);

        /* Get program id. */
        const ncm::ProgramId program_id = context->GetProgramId(process_handle);

        /* Validate address/size. */
        R_TRY(ValidateAddressAndNonZeroSize(nrr_address, nrr_size));

        /* Check we have space for a new NRR. */
        NrrInfo *nrr_info = nullptr;
        R_TRY(context->GetFreeNrrInfo(std::addressof(nrr_info)));

        /* Prepare to cache the NRR's signature hash. */
        Sha256Hash signed_area_hash;
        ON_SCOPE_EXIT { crypto::ClearMemory(std::addressof(signed_area_hash), sizeof(signed_area_hash)); };

        /* Map. */
        NrrHeader *header = nullptr;
        u64 mapped_code_address = 0;
        R_TRY(MapAndValidateNrr(std::addressof(header), std::addressof(mapped_code_address), std::addressof(signed_area_hash), sizeof(signed_area_hash), context->GetProcessHandle(), program_id, nrr_address, nrr_size, nrr_kind, enforce_nrr_kind));

        /* Set NRR info. */
        context->SetNrrInfoInUse(nrr_info, true);
        nrr_info->mapped_header = header;
        nrr_info->nrr_heap_address = nrr_address;
        nrr_info->nrr_heap_size = nrr_size;
        nrr_info->mapped_code_address = mapped_code_address;

        nrr_info->cached_signed_area_size = header->GetSignedAreaSize();
        nrr_info->cached_hashes_offset    = header->GetHashesOffset();
        nrr_info->cached_num_hashes       = header->GetNumHashes();

        std::memcpy(nrr_info->cached_signed_area, header->GetSignedArea(), std::min(sizeof(nrr_info->cached_signed_area), header->GetHashesOffset() - header->GetSignedAreaOffset()));
        std::memcpy(std::addressof(nrr_info->signed_area_hash), std::addressof(signed_area_hash), sizeof(signed_area_hash));

        R_SUCCEED();
    }

    Result UnregisterModuleInfo(size_t context_id, u64 nrr_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ABORT_UNLESS(context != nullptr);

        /* Validate address. */
        R_UNLESS(util::IsAligned(nrr_address, os::MemoryPageSize), ro::ResultInvalidAddress());

        /* Check the NRR is loaded. */
        NrrInfo *nrr_info = nullptr;
        R_TRY(context->GetNrrInfoByAddress(std::addressof(nrr_info), nrr_address));

        /* Unmap. */
        const NrrInfo nrr_backup = *nrr_info;
        {
            /* Nintendo does this unconditionally, whether or not the actual unmap succeeds. */
            context->SetNrrInfoInUse(nrr_info, false);
            std::memset(nrr_info, 0, sizeof(*nrr_info));
        }
        R_RETURN(UnmapNrr(context->GetProcessHandle(), nrr_backup.mapped_header, nrr_backup.nrr_heap_address, nrr_backup.nrr_heap_size, nrr_backup.mapped_code_address));
    }

    Result MapManualLoadModuleMemory(u64 *out_address, size_t context_id, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ABORT_UNLESS(context != nullptr);

        /* Validate address/size. */
        R_TRY(ValidateAddressAndNonZeroSize(nro_address, nro_size));
        R_TRY(ValidateAddressAndSize(bss_address, bss_size));

        const u64 total_size = nro_size + bss_size;
        R_UNLESS(total_size >= nro_size, ro::ResultInvalidSize());
        R_UNLESS(total_size >= bss_size, ro::ResultInvalidSize());

        /* Check we have space for a new NRO. */
        NroInfo *nro_info = nullptr;
        R_TRY(context->GetFreeNroInfo(std::addressof(nro_info)));
        
        /* Map the NRO. */
        R_TRY(MapNro(std::addressof(nro_info->base_address), context->GetProcessHandle(), nro_address, nro_size, bss_address, bss_size));
        ON_RESULT_FAILURE { R_DISCARD(UnmapNro(context->GetProcessHandle(), nro_info->base_address, nro_address, nro_size, bss_address, bss_size)); };

        /* Parse the NRO. */
        u64 rx_size = 0, ro_size = 0, rw_size = 0;
        bool aligned_header = false;
        bool is_compress = false;
        R_TRY(context->ParseNro(std::addressof(nro_info->module_id), std::addressof(rx_size), std::addressof(ro_size), std::addressof(rw_size), std::addressof(aligned_header), std::addressof(is_compress), nro_info->base_address, nro_size, bss_size));

        /* Set NRO perms. */
        R_TRY(SetNroPerms(context->GetProcessHandle(), nro_info->base_address, rx_size, ro_size, rw_size + bss_size, aligned_header));

        context->SetNroInfoInUse(nro_info, true);
        nro_info->nro_heap_address = nro_address;
        nro_info->nro_heap_size = nro_size;
        nro_info->bss_heap_address = bss_address;
        nro_info->bss_heap_size = bss_size;
        if (hos::GetVersion() >= hos::Version_23_0_0) {
            nro_info->code_size = rx_size;
            nro_info->ro_size = ro_size;
        } else {
            nro_info->code_size = rx_size + ro_size;
            nro_info->ro_size = 0;
        }
        nro_info->rw_size = rw_size;
        nro_info->is_compress = is_compress;
        *out_address = nro_info->base_address;
        R_SUCCEED();
    }

    Result UnmapManualLoadModuleMemory(size_t context_id, u64 nro_address) {
        /* Get context. */
        ProcessContext *context = GetContextById(context_id);
        AMS_ABORT_UNLESS(context != nullptr);

        /* Validate address. */
        R_UNLESS(util::IsAligned(nro_address, os::MemoryPageSize), ro::ResultInvalidAddress());

        /* Check the NRO is loaded. */
        NroInfo *nro_info = nullptr;
        R_TRY(context->GetNroInfoByAddress(std::addressof(nro_info), nro_address));
        
        /* Handle compression related mappings. */
        if (nro_info->is_compress) {
            const u64 code_address = nro_info->base_address + 0x1000;
            const u64 code_size = nro_info->code_size - 0x1000;
            
            /* Remove code region permissions. */
            R_TRY(os::SetProcessMemoryPermission(context->GetProcessHandle(), code_address, code_size, os::MemoryPermission_None));
            
            /* Map the code region. */
            void *mapped_memory = nullptr;
            R_TRY(os::MapProcessMemory(std::addressof(mapped_memory), context->GetProcessHandle(), code_address, code_size, ro::impl::GenerateSecureRandom));
            
            /* Clear the memory. */
            std::memset(mapped_memory, 0, code_size);
            
            /* Unmap the code region. */
            os::UnmapProcessMemory(mapped_memory, context->GetProcessHandle(), code_address, code_size);
        }

        /* Unmap. */
        const NroInfo nro_backup = *nro_info;
        {
            /* Nintendo does this unconditionally, whether or not the actual unmap succeeds. */
            context->SetNroInfoInUse(nro_info, false);
            std::memset(nro_info, 0, sizeof(*nro_info));
        }
        R_RETURN(UnmapNro(context->GetProcessHandle(), nro_backup.base_address, nro_backup.nro_heap_address, nro_backup.code_size + nro_backup.rw_size, nro_backup.bss_heap_address, nro_backup.bss_heap_size));
    }

    /* Debug service implementations. */
    Result GetProcessModuleInfo(u32 *out_count, ldr::ModuleInfo *out_infos, size_t max_out_count, os::ProcessId process_id) {
        if (const ProcessContext *context = GetContextByProcessId(process_id); context != nullptr) {
            context->GetProcessModuleInfo(out_count, out_infos, max_out_count);
        }

        R_SUCCEED();
    }

}

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
#include "dmnt_cheat_api.hpp"
#include "dmnt_cheat_vm.hpp"
#include "dmnt_cheat_debug_events_manager.hpp"

namespace ams::dmnt::cheat::impl {

    namespace {

        /* Helper definitions. */
        constexpr size_t MaxCheatCount = 0x80;
        constexpr size_t MaxFrozenAddressCount = 0x80;

        class FrozenAddressMapEntry : public util::IntrusiveRedBlackTreeBaseNode<FrozenAddressMapEntry> {
            public:
                using RedBlackKeyType = u64;
            private:
                u64 m_address;
                FrozenAddressValue m_value;
            public:
                FrozenAddressMapEntry(u64 address, FrozenAddressValue value) : m_address(address), m_value(value) { /* ... */ }

                constexpr u64 GetAddress() const { return m_address; }

                constexpr const FrozenAddressValue &GetValue() const { return m_value; }
                constexpr FrozenAddressValue &GetValue() { return m_value; }

                static constexpr ALWAYS_INLINE int Compare(const RedBlackKeyType &lval, const FrozenAddressMapEntry &rhs) {
                    const auto rval = rhs.GetAddress();

                    if (lval < rval) {
                        return -1;
                    } else if (lval == rval) {
                        return 0;
                    } else {
                        return 1;
                    }
                }

                static constexpr ALWAYS_INLINE int Compare(const FrozenAddressMapEntry &lhs, const FrozenAddressMapEntry &rhs) {
                    return Compare(lhs.GetAddress(), rhs);
                }
        };

        constinit os::SdkMutex g_text_file_buffer_lock;
        constinit char g_text_file_buffer[64_KB];

        constinit u8 g_frozen_address_map_memory[sizeof(FrozenAddressMapEntry) * MaxFrozenAddressCount];
        constinit lmem::HeapHandle g_frozen_address_map_heap;

        FrozenAddressMapEntry *AllocateFrozenAddress(u64 address, FrozenAddressValue value) {
            FrozenAddressMapEntry *entry = static_cast<FrozenAddressMapEntry *>(lmem::AllocateFromUnitHeap(g_frozen_address_map_heap));
            if (entry != nullptr) {
                std::construct_at(entry, address, value);
            }
            return entry;
        }

        void DeallocateFrozenAddress(FrozenAddressMapEntry *entry) {
            std::destroy_at(entry);
            lmem::FreeToUnitHeap(g_frozen_address_map_heap, entry);
        }

        using FrozenAddressMap = typename util::IntrusiveRedBlackTreeBaseTraits<FrozenAddressMapEntry>::TreeType<FrozenAddressMapEntry>;

        /* Manager class. */
        class CheatProcessManager {
            private:
                static constexpr size_t ThreadStackSize = 0x4000;
            private:
                os::SdkMutex m_cheat_lock;
                os::Event m_unsafe_break_event;
                os::Event m_debug_events_event; /* Autoclear. */
                os::ThreadType m_detect_thread, m_debug_events_thread;
                os::SystemEvent m_cheat_process_event;
                os::NativeHandle m_cheat_process_debug_handle = os::InvalidNativeHandle;
                CheatProcessMetadata m_cheat_process_metadata = {};

                os::ThreadType m_vm_thread;
                bool m_broken_unsafe = false;
                bool m_needs_reload_vm = false;
                CheatVirtualMachine m_cheat_vm;

                bool m_enable_cheats_by_default = true;
                bool m_always_save_cheat_toggles = false;
                bool m_should_save_cheat_toggles = false;
                CheatEntry m_cheat_entries[MaxCheatCount] = {};
                FrozenAddressMap m_frozen_addresses_map = {};

                alignas(os::MemoryPageSize) u8 m_detect_thread_stack[ThreadStackSize] = {};
                alignas(os::MemoryPageSize) u8 m_debug_events_thread_stack[ThreadStackSize] = {};
                alignas(os::MemoryPageSize) u8 m_vm_thread_stack[ThreadStackSize] = {};
            private:
                static void DetectLaunchThread(void *_this);
                static void VirtualMachineThread(void *_this);
                static void DebugEventsThread(void *_this);

                Result AttachToApplicationProcess(bool on_process_launch);

                bool ParseCheats(const char *s, size_t len);
                bool LoadCheats(const ncm::ProgramId program_id, const u8 *module_id);
                bool ParseCheatToggles(const char *s, size_t len);
                bool LoadCheatToggles(const ncm::ProgramId program_id);
                void SaveCheatToggles(const ncm::ProgramId program_id);

                bool GetNeedsReloadVm() const {
                    return m_needs_reload_vm;
                }

                void SetNeedsReloadVm(bool reload) {
                    m_needs_reload_vm = reload;
                }


                void ResetCheatEntry(size_t i) {
                    if (i < MaxCheatCount) {
                        std::memset(m_cheat_entries + i, 0, sizeof(m_cheat_entries[i]));
                        m_cheat_entries[i].cheat_id = i;

                        this->SetNeedsReloadVm(true);
                    }
                }

                void ResetAllCheatEntries() {
                    for (size_t i = 0; i < MaxCheatCount; i++) {
                        this->ResetCheatEntry(i);
                    }

                    m_cheat_vm.ResetStaticRegisters();
                }

                CheatEntry *GetCheatEntryById(size_t i) {
                    if (i < MaxCheatCount) {
                        return m_cheat_entries + i;
                    }

                    return nullptr;
                }

                CheatEntry *GetCheatEntryByReadableName(const char *readable_name) {
                    /* Check all non-master cheats for match. */
                    for (size_t i = 1; i < MaxCheatCount; i++) {
                        if (std::strncmp(m_cheat_entries[i].definition.readable_name, readable_name, sizeof(m_cheat_entries[i].definition.readable_name)) == 0) {
                            return m_cheat_entries + i;
                        }
                    }

                    return nullptr;
                }

                CheatEntry *GetFreeCheatEntry() {
                    /* Check all non-master cheats for availability. */
                    for (size_t i = 1; i < MaxCheatCount; i++) {
                        if (m_cheat_entries[i].definition.num_opcodes == 0) {
                            return m_cheat_entries + i;
                        }
                    }

                    return nullptr;
                }

                void CloseActiveCheatProcess() {
                    if (m_cheat_process_debug_handle != os::InvalidNativeHandle) {
                        /* We don't need to do any unsafe brekaing. */
                        m_broken_unsafe = false;
                        m_unsafe_break_event.Signal();

                        /* Knock out the debug events thread. */
                        os::CancelThreadSynchronization(std::addressof(m_debug_events_thread));

                        /* Close resources. */
                        R_ABORT_UNLESS(svc::CloseHandle(m_cheat_process_debug_handle));
                        m_cheat_process_debug_handle = os::InvalidNativeHandle;

                        /* Save cheat toggles. */
                        if (m_always_save_cheat_toggles || m_should_save_cheat_toggles) {
                            this->SaveCheatToggles(m_cheat_process_metadata.program_id);
                            m_should_save_cheat_toggles = false;
                        }

                        /* Clear metadata. */
                        static_assert(util::is_pod<decltype(m_cheat_process_metadata)>::value, "CheatProcessMetadata definition!");
                        std::memset(std::addressof(m_cheat_process_metadata), 0, sizeof(m_cheat_process_metadata));

                        /* Clear cheat list. */
                        this->ResetAllCheatEntries();

                        /* Clear frozen addresses. */
                        {
                            auto it = m_frozen_addresses_map.begin();
                            while (it != m_frozen_addresses_map.end()) {
                                FrozenAddressMapEntry *entry = std::addressof(*it);
                                it = m_frozen_addresses_map.erase(it);
                                DeallocateFrozenAddress(entry);
                            }
                        }

                        /* Signal to our fans. */
                        m_cheat_process_event.Signal();
                    }
                }

                bool HasActiveCheatProcess() {
                    /* Note: This function *MUST* be called only with the cheat lock held. */
                    os::ProcessId pid;
                    bool has_cheat_process = m_cheat_process_debug_handle != os::InvalidNativeHandle;
                    has_cheat_process &= R_SUCCEEDED(os::GetProcessId(std::addressof(pid), m_cheat_process_debug_handle));
                    has_cheat_process &= R_SUCCEEDED(pm::dmnt::GetApplicationProcessId(std::addressof(pid)));
                    has_cheat_process &= (pid == m_cheat_process_metadata.process_id);

                    if (!has_cheat_process) {
                        this->CloseActiveCheatProcess();
                    }

                    return has_cheat_process;
                }

                Result EnsureCheatProcess() {
                    R_UNLESS(this->HasActiveCheatProcess(), dmnt::cheat::ResultCheatNotAttached());
                    return ResultSuccess();
                }

                os::NativeHandle GetCheatProcessHandle() const {
                    return m_cheat_process_debug_handle;
                }

                os::NativeHandle HookToCreateApplicationProcess() const {
                    os::NativeHandle h;
                    R_ABORT_UNLESS(pm::dmnt::HookToCreateApplicationProcess(std::addressof(h)));
                    return h;
                }

                void StartProcess(os::ProcessId process_id) const {
                    R_ABORT_UNLESS(pm::dmnt::StartProcess(process_id));
                }
            public:
                CheatProcessManager() : m_cheat_lock(), m_unsafe_break_event(os::EventClearMode_ManualClear), m_debug_events_event(os::EventClearMode_AutoClear), m_cheat_process_event(os::EventClearMode_AutoClear, true) {
                    /* Learn whether we should enable cheats by default. */
                    {
                        u8 en = 0;
                        if (settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "dmnt_cheats_enabled_by_default") == sizeof(en)) {
                            m_enable_cheats_by_default = (en != 0);
                        }

                        en = 0;
                        if (settings::fwdbg::GetSettingsItemValue( std::addressof(en), sizeof(en), "atmosphere", "dmnt_always_save_cheat_toggles") == sizeof(en)) {
                            m_always_save_cheat_toggles = (en != 0);
                        }
                    }

                    /* Spawn application detection thread, spawn cheat vm thread. */
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(m_detect_thread), DetectLaunchThread, this, m_detect_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatDetect)));
                    os::SetThreadNamePointer(std::addressof(m_detect_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatDetect));
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(m_vm_thread), VirtualMachineThread, this, m_vm_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatVirtualMachine)));
                    os::SetThreadNamePointer(std::addressof(m_vm_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatVirtualMachine));
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(m_debug_events_thread), DebugEventsThread, this, m_debug_events_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatDebugEvents)));
                    os::SetThreadNamePointer(std::addressof(m_debug_events_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatDebugEvents));

                    /* Start threads. */
                    os::StartThread(std::addressof(m_detect_thread));
                    os::StartThread(std::addressof(m_vm_thread));
                    os::StartThread(std::addressof(m_debug_events_thread));
                }

                bool GetHasActiveCheatProcess() {
                    std::scoped_lock lk(m_cheat_lock);

                    return this->HasActiveCheatProcess();
                }

                os::NativeHandle GetCheatProcessEventHandle() const {
                    return m_cheat_process_event.GetReadableHandle();
                }

                Result GetCheatProcessMetadata(CheatProcessMetadata *out) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    std::memcpy(out, std::addressof(m_cheat_process_metadata), sizeof(*out));
                    return ResultSuccess();
                }

                Result ForceOpenCheatProcess() {
                    return this->AttachToApplicationProcess(false);
                }

                Result ForceCloseCheatProcess() {
                    this->CloseActiveCheatProcess();
                    return ResultSuccess();
                }

                Result ReadCheatProcessMemoryUnsafe(u64 proc_addr, void *out_data, size_t size) {
                    return svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(out_data), this->GetCheatProcessHandle(), proc_addr, size);
                }

                Result WriteCheatProcessMemoryUnsafe(u64 proc_addr, const void *data, size_t size) {
                    R_TRY(svc::WriteDebugProcessMemory(this->GetCheatProcessHandle(), reinterpret_cast<uintptr_t>(data), proc_addr, size));

                    for (auto &entry : m_frozen_addresses_map) {
                        /* Get address/value. */
                        const u64 address = entry.GetAddress();
                        auto &value = entry.GetValue();

                        /* Map is ordered, so break when we can. */
                        if (address >= proc_addr + size) {
                            break;
                        }

                        /* Check if we need to write. */
                        if (proc_addr <= address && address < proc_addr + size) {
                            const size_t offset = (address - proc_addr);
                            const size_t copy_size = std::min(sizeof(value.value), size - offset);
                            std::memcpy(std::addressof(value.value), reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data) + offset), copy_size);
                        }
                    }

                    return ResultSuccess();
                }

                Result PauseCheatProcessUnsafe() {
                    m_broken_unsafe = true;
                    m_unsafe_break_event.Clear();
                    return svc::BreakDebugProcess(this->GetCheatProcessHandle());
                }

                Result ResumeCheatProcessUnsafe() {
                    m_broken_unsafe = false;
                    m_unsafe_break_event.Signal();
                    dmnt::cheat::impl::ContinueCheatProcess(this->GetCheatProcessHandle());
                    return ResultSuccess();
                }

                Result GetCheatProcessMappingCount(u64 *out_count) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    svc::MemoryInfo mem_info;
                    svc::PageInfo page_info;
                    u64 address = 0, count = 0;
                    do {
                        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mem_info), std::addressof(page_info), this->GetCheatProcessHandle(), address))) {
                            break;
                        }

                        if (mem_info.permission != svc::MemoryPermission_None) {
                            count++;
                        }

                        address = mem_info.base_address + mem_info.size;
                    } while (address != 0);

                    *out_count = count;
                    return ResultSuccess();
                }

                Result GetCheatProcessMappings(svc::MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    svc::MemoryInfo mem_info;
                    svc::PageInfo page_info;
                    u64 address = 0, total_count = 0, written_count = 0;
                    do {
                        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mem_info), std::addressof(page_info), this->GetCheatProcessHandle(), address))) {
                            break;
                        }

                        if (mem_info.permission != svc::MemoryPermission_None) {
                            if (offset <= total_count && written_count < max_count) {
                                mappings[written_count++] = mem_info;
                            }
                            total_count++;
                        }

                        address = mem_info.base_address + mem_info.size;
                    } while (address != 0 && written_count < max_count);

                    *out_count = written_count;
                    return ResultSuccess();
                }

                Result ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->ReadCheatProcessMemoryUnsafe(proc_addr, out_data, size);
                }

                Result WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->WriteCheatProcessMemoryUnsafe(proc_addr, data, size);
                }

                Result QueryCheatProcessMemory(svc::MemoryInfo *mapping, u64 address) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    svc::PageInfo page_info;
                    return svc::QueryDebugProcessMemory(mapping, std::addressof(page_info), this->GetCheatProcessHandle(), address);
                }

                Result PauseCheatProcess() {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->PauseCheatProcessUnsafe();
                }

                Result ResumeCheatProcess() {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->ResumeCheatProcessUnsafe();
                }

                Result GetCheatCount(u64 *out_count) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    size_t count = 0;
                    for (size_t i = 0; i < MaxCheatCount; i++) {
                        if (m_cheat_entries[i].definition.num_opcodes) {
                            count++;
                        }
                    }

                    *out_count = count;
                    return ResultSuccess();
                }

                Result GetCheats(CheatEntry *out_cheats, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    size_t count = 0, total_count = 0;
                    for (size_t i = 0; i < MaxCheatCount && count < max_count; i++) {
                        if (m_cheat_entries[i].definition.num_opcodes) {
                            total_count++;
                            if (total_count > offset) {
                                out_cheats[count++] = m_cheat_entries[i];
                            }
                        }
                    }

                    *out_count = count;
                    return ResultSuccess();
                }

                Result GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    R_UNLESS(entry != nullptr,                   dmnt::cheat::ResultCheatUnknownId());
                    R_UNLESS(entry->definition.num_opcodes != 0, dmnt::cheat::ResultCheatUnknownId());

                    *out_cheat = *entry;
                    return ResultSuccess();
                }

                Result ToggleCheat(u32 cheat_id) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    R_UNLESS(entry != nullptr,                   dmnt::cheat::ResultCheatUnknownId());
                    R_UNLESS(entry->definition.num_opcodes != 0, dmnt::cheat::ResultCheatUnknownId());

                    R_UNLESS(cheat_id != 0, dmnt::cheat::ResultCheatCannotDisable());

                    entry->enabled = !entry->enabled;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result AddCheat(u32 *out_id, const CheatDefinition &def, bool enabled) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    R_UNLESS(def.num_opcodes != 0,                       dmnt::cheat::ResultCheatInvalid());
                    R_UNLESS(def.num_opcodes <= util::size(def.opcodes), dmnt::cheat::ResultCheatInvalid());

                    CheatEntry *new_entry = this->GetFreeCheatEntry();
                    R_UNLESS(new_entry != nullptr, dmnt::cheat::ResultCheatOutOfResource());

                    new_entry->enabled = enabled;
                    new_entry->definition = def;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    /* Set output id. */
                    *out_id = new_entry->cheat_id;

                    return ResultSuccess();
                }

                Result RemoveCheat(u32 cheat_id) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(cheat_id < MaxCheatCount, dmnt::cheat::ResultCheatUnknownId());

                    this->ResetCheatEntry(cheat_id);

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result SetMasterCheat(const CheatDefinition &def) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    R_UNLESS(def.num_opcodes != 0,                       dmnt::cheat::ResultCheatInvalid());
                    R_UNLESS(def.num_opcodes <= util::size(def.opcodes), dmnt::cheat::ResultCheatInvalid());

                    CheatEntry *master_entry = m_cheat_entries + 0;

                    master_entry->enabled    = true;
                    master_entry->definition = def;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result ReadStaticRegister(u64 *out, size_t which) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(which < CheatVirtualMachine::NumStaticRegisters, dmnt::cheat::ResultCheatInvalid());

                    *out = m_cheat_vm.GetStaticRegister(which);
                    return ResultSuccess();
                }

                Result WriteStaticRegister(size_t which, u64 value) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(which < CheatVirtualMachine::NumStaticRegisters, dmnt::cheat::ResultCheatInvalid());

                    m_cheat_vm.SetStaticRegister(which, value);
                    return ResultSuccess();
                }

                Result ResetStaticRegisters() {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    m_cheat_vm.ResetStaticRegisters();
                    return ResultSuccess();
                }

                Result GetFrozenAddressCount(u64 *out_count) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    *out_count = std::distance(m_frozen_addresses_map.begin(), m_frozen_addresses_map.end());
                    return ResultSuccess();
                }

                Result GetFrozenAddresses(FrozenAddressEntry *frz_addrs, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    u64 total_count = 0, written_count = 0;
                    for (const auto &entry : m_frozen_addresses_map) {
                        if (written_count >= max_count) {
                            break;
                        }

                        if (offset <= total_count) {
                            frz_addrs[written_count].address = entry.GetAddress();
                            frz_addrs[written_count].value   = entry.GetValue();
                            written_count++;
                        }
                        total_count++;
                    }

                    *out_count = written_count;
                    return ResultSuccess();
                }

                Result GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = m_frozen_addresses_map.find_key(address);
                    R_UNLESS(it != m_frozen_addresses_map.end(), dmnt::cheat::ResultFrozenAddressNotFound());

                    frz_addr->address = it->GetAddress();
                    frz_addr->value   = it->GetValue();
                    return ResultSuccess();
                }

                Result EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = m_frozen_addresses_map.find_key(address);
                    R_UNLESS(it == m_frozen_addresses_map.end(), dmnt::cheat::ResultFrozenAddressAlreadyExists());

                    FrozenAddressValue value = {};
                    value.width = width;
                    R_TRY(this->ReadCheatProcessMemoryUnsafe(address, std::addressof(value.value), width));

                    FrozenAddressMapEntry *entry = AllocateFrozenAddress(address, value);
                    R_UNLESS(entry != nullptr, dmnt::cheat::ResultFrozenAddressOutOfResource());

                    m_frozen_addresses_map.insert(*entry);
                    *out_value = value.value;
                    return ResultSuccess();
                }

                Result DisableFrozenAddress(u64 address) {
                    std::scoped_lock lk(m_cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = m_frozen_addresses_map.find_key(address);
                    R_UNLESS(it != m_frozen_addresses_map.end(), dmnt::cheat::ResultFrozenAddressNotFound());

                    FrozenAddressMapEntry *entry = std::addressof(*it);
                    m_frozen_addresses_map.erase(it);
                    DeallocateFrozenAddress(entry);

                    return ResultSuccess();
                }

        };

        void CheatProcessManager::DetectLaunchThread(void *_this) {
            CheatProcessManager *manager = reinterpret_cast<CheatProcessManager *>(_this);
            Event hook;
            while (true) {
                eventLoadRemote(std::addressof(hook), manager->HookToCreateApplicationProcess(), true);
                if (R_SUCCEEDED(eventWait(std::addressof(hook), std::numeric_limits<u64>::max()))) {
                    manager->AttachToApplicationProcess(true);
                }
                eventClose(std::addressof(hook));
            }
        }

        void CheatProcessManager::DebugEventsThread(void *_this) {
            CheatProcessManager *manager = reinterpret_cast<CheatProcessManager *>(_this);
            while (true) {
                /* Atomically wait (and clear) signal for new process. */
                manager->m_debug_events_event.Wait();
                while (true) {
                    os::NativeHandle cheat_process_handle = manager->GetCheatProcessHandle();
                    s32 dummy;
                    while (cheat_process_handle != os::InvalidNativeHandle && R_SUCCEEDED(svc::WaitSynchronization(std::addressof(dummy), std::addressof(cheat_process_handle), 1, std::numeric_limits<u64>::max()))) {
                        manager->m_cheat_lock.Lock();
                        ON_SCOPE_EXIT { manager->m_cheat_lock.Unlock(); };
                        {
                            ON_SCOPE_EXIT { cheat_process_handle = manager->GetCheatProcessHandle(); };

                            /* If we did an unsafe break, wait until we're not broken. */
                            if (manager->m_broken_unsafe) {
                                manager->m_cheat_lock.Unlock();
                                manager->m_unsafe_break_event.Wait();
                                manager->m_cheat_lock.Lock();
                                if (manager->GetCheatProcessHandle() != os::InvalidNativeHandle) {
                                    continue;
                                } else {
                                    break;
                                }
                            }

                            /* Handle any pending debug events. */
                            if (manager->HasActiveCheatProcess()) {
                                R_TRY_CATCH(dmnt::cheat::impl::ContinueCheatProcess(manager->GetCheatProcessHandle())) {
                                    R_CATCH(svc::ResultProcessTerminated) {
                                        manager->CloseActiveCheatProcess();
                                        break;
                                    }
                                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                            }
                        }
                    }

                    /* WaitSynchronization failed. This means someone canceled our synchronization, possibly us. */
                    /* Let's check if we should quit! */
                    std::scoped_lock lk(manager->m_cheat_lock);
                    if (!manager->HasActiveCheatProcess()) {
                        break;
                    }
                }
            }
        }

        void CheatProcessManager::VirtualMachineThread(void *_this) {
            CheatProcessManager *manager = reinterpret_cast<CheatProcessManager *>(_this);
            while (true) {
                /* Apply cheats. */
                {
                    std::scoped_lock lk(manager->m_cheat_lock);

                    if (manager->HasActiveCheatProcess()) {
                        /* Execute VM. */
                        if (!manager->GetNeedsReloadVm() || manager->m_cheat_vm.LoadProgram(manager->m_cheat_entries, util::size(manager->m_cheat_entries))) {
                            manager->SetNeedsReloadVm(false);

                            /* Execute program only if it has opcodes. */
                            if (manager->m_cheat_vm.GetProgramSize()) {
                                manager->m_cheat_vm.Execute(std::addressof(manager->m_cheat_process_metadata));
                            }
                        }

                        /* Apply frozen addresses. */
                        for (const auto &entry : manager->m_frozen_addresses_map) {
                            const auto address = entry.GetAddress();
                            const auto &value  = entry.GetValue();

                            /* Use Write SVC directly, to avoid the usual frozen address update logic. */
                            svc::WriteDebugProcessMemory(manager->GetCheatProcessHandle(), reinterpret_cast<uintptr_t>(std::addressof(value.value)), address, value.width);
                        }
                    }
                }

                /* Sleep until next potential execution. */
                constexpr s64 TimesPerSecond   = 12;
                constexpr s64 DelayNanoSeconds = TimeSpan::FromSeconds(1).GetNanoSeconds() / TimesPerSecond;
                constexpr TimeSpan Delay       = TimeSpan::FromNanoSeconds(DelayNanoSeconds);
                os::SleepThread(Delay);
            }
        }

        #define R_ABORT_UNLESS_IF_NEW_PROCESS(res) \
            if (on_process_launch) { \
                R_ABORT_UNLESS(res); \
            } else { \
                R_TRY(res); \
            }

        Result CheatProcessManager::AttachToApplicationProcess(bool on_process_launch) {
            std::scoped_lock lk(m_cheat_lock);

            /* Close the active process, if needed. */
            {
                if (this->HasActiveCheatProcess()) {
                    /* When forcing attach, we're done. */
                    R_SUCCEED_IF(!on_process_launch);
                }

                /* Detach from the current process, if it's open. */
                this->CloseActiveCheatProcess();
            }

            /* Get the application process's ID. */
            R_ABORT_UNLESS_IF_NEW_PROCESS(pm::dmnt::GetApplicationProcessId(std::addressof(m_cheat_process_metadata.process_id)));
            auto proc_guard = SCOPE_GUARD {
                if (on_process_launch) {
                    this->StartProcess(m_cheat_process_metadata.process_id);
                }
                m_cheat_process_metadata.process_id = os::ProcessId{};
            };

            /* Get process handle, use it to learn memory extents. */
            {
                os::NativeHandle proc_h = os::InvalidNativeHandle;
                ncm::ProgramLocation loc = {};
                cfg::OverrideStatus status = {};

                R_ABORT_UNLESS_IF_NEW_PROCESS(pm::dmnt::AtmosphereGetProcessInfo(std::addressof(proc_h), std::addressof(loc), std::addressof(status), m_cheat_process_metadata.process_id));
                ON_SCOPE_EXIT { os::CloseNativeHandle(proc_h); };

                m_cheat_process_metadata.program_id = loc.program_id;

                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.heap_extents.base),  svc::InfoType_HeapRegionAddress,  proc_h, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.heap_extents.size),  svc::InfoType_HeapRegionSize,     proc_h, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.alias_extents.base), svc::InfoType_AliasRegionAddress, proc_h, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.alias_extents.size), svc::InfoType_AliasRegionSize,    proc_h, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.aslr_extents.base),  svc::InfoType_AslrRegionAddress,  proc_h, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_cheat_process_metadata.aslr_extents.size),  svc::InfoType_AslrRegionSize,     proc_h, 0));

                /* If new process launch, we may not want to actually attach. */
                if (on_process_launch) {
                    R_UNLESS(status.IsCheatEnabled(), dmnt::cheat::ResultCheatNotAttached());
                }
            }

            /* Get module information from loader. */
            {
                ldr::ModuleInfo proc_modules[2];
                s32 num_modules;

                /* TODO: ldr::dmnt:: */
                R_ABORT_UNLESS_IF_NEW_PROCESS(ldrDmntGetProcessModuleInfo(static_cast<u64>(m_cheat_process_metadata.process_id), reinterpret_cast<LoaderModuleInfo *>(proc_modules), util::size(proc_modules), std::addressof(num_modules)));

                /* All applications must have two modules. */
                /* Only accept one (which means we're attaching to HBL) */
                /* if we aren't auto-attaching. */
                const ldr::ModuleInfo *proc_module = nullptr;
                if (num_modules == 2) {
                    proc_module = std::addressof(proc_modules[1]);
                } else if (num_modules == 1 && !on_process_launch) {
                    proc_module = std::addressof(proc_modules[0]);
                } else {
                    return dmnt::cheat::ResultCheatNotAttached();
                }

                m_cheat_process_metadata.main_nso_extents.base = proc_module->address;
                m_cheat_process_metadata.main_nso_extents.size = proc_module->size;
                std::memcpy(m_cheat_process_metadata.main_nso_module_id, proc_module->module_id, sizeof(m_cheat_process_metadata.main_nso_module_id));
            }

            /* Read cheats off the SD. */
            if (!this->LoadCheats(m_cheat_process_metadata.program_id, m_cheat_process_metadata.main_nso_module_id) ||
                !this->LoadCheatToggles(m_cheat_process_metadata.program_id)) {
                /* If new process launch, require success. */
                R_UNLESS(!on_process_launch, dmnt::cheat::ResultCheatNotAttached());
            }

            /* Open a debug handle. */
            svc::Handle debug_handle = svc::InvalidHandle;
            R_ABORT_UNLESS_IF_NEW_PROCESS(svc::DebugActiveProcess(std::addressof(debug_handle), m_cheat_process_metadata.process_id.value));

            /* Set our debug handle. */
            m_cheat_process_debug_handle = debug_handle;

            /* Cancel process guard. */
            proc_guard.Cancel();

            /* Reset broken state. */
            m_broken_unsafe = false;
            m_unsafe_break_event.Signal();

            /* If new process, start the process. */
            if (on_process_launch) {
                this->StartProcess(m_cheat_process_metadata.process_id);
            }

            /* Signal to the debug events thread. */
            m_debug_events_event.Signal();

            /* Signal to our fans. */
            m_cheat_process_event.Signal();

            return ResultSuccess();
        }

        #undef R_ABORT_UNLESS_IF_NEW_PROCESS

        bool CheatProcessManager::ParseCheats(const char *s, size_t len) {
            /* Trigger a VM reload. */
            this->SetNeedsReloadVm(true);

            /* Parse the input string. */
            size_t i = 0;
            CheatEntry *cur_entry = nullptr;
            while (i < len) {
                if (std::isspace(static_cast<unsigned char>(s[i]))) {
                    /* Just ignore whitespace. */
                    i++;
                } else if (s[i] == '[') {
                    /* Parse a readable cheat name. */
                    cur_entry = this->GetFreeCheatEntry();
                    if (cur_entry == nullptr) {
                        return false;
                    }

                    /* Extract name bounds. */
                    size_t j = i + 1;
                    while (s[j] != ']') {
                        j++;
                        if (j >= len) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_entry->definition.readable_name));
                    std::memcpy(cur_entry->definition.readable_name, s + (i + 1), cheat_name_len);
                    cur_entry->definition.readable_name[cheat_name_len] = 0;

                    /* Skip onwards. */
                    i = j + 1;
                } else if (s[i] == '{') {
                    /* We're parsing a master cheat. */
                    cur_entry = std::addressof(m_cheat_entries[0]);

                    /* There can only be one master cheat. */
                    if (cur_entry->definition.num_opcodes > 0) {
                        return false;
                    }

                    /* Extract name bounds */
                    size_t j = i + 1;
                    while (s[j] != '}') {
                        j++;
                        if (j >= len) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_entry->definition.readable_name));
                    memcpy(cur_entry->definition.readable_name, s + (i + 1), cheat_name_len);
                    cur_entry->definition.readable_name[cheat_name_len] = 0;

                    /* Skip onwards. */
                    i = j + 1;
                } else if (std::isxdigit(static_cast<unsigned char>(s[i]))) {
                    /* Make sure that we have a cheat open. */
                    if (cur_entry == nullptr) {
                        return false;
                    }

                    /* Bounds check the opcode count. */
                    if (cur_entry->definition.num_opcodes >= util::size(cur_entry->definition.opcodes)) {
                        return false;
                    }

                    /* We're parsing an instruction, so validate it's 8 hex digits. */
                    for (size_t j = 1; j < 8; j++) {
                        /* Validate 8 hex chars. */
                        if (i + j >= len || !std::isxdigit(static_cast<unsigned char>(s[i+j]))) {
                            return false;
                        }
                    }

                    /* Parse the new opcode. */
                    char hex_str[9] = {0};
                    std::memcpy(hex_str, s + i, 8);
                    cur_entry->definition.opcodes[cur_entry->definition.num_opcodes++] = std::strtoul(hex_str, NULL, 16);

                    /* Skip onwards. */
                    i += 8;
                } else {
                    /* Unexpected character encountered. */
                    return false;
                }
            }

            /* Master cheat can't be disabled. */
            if (m_cheat_entries[0].definition.num_opcodes > 0) {
                m_cheat_entries[0].enabled = true;
            }

            /* Enable all entries we parsed. */
            for (size_t i = 1; i < MaxCheatCount; i++) {
                if (m_cheat_entries[i].definition.num_opcodes > 0) {
                    m_cheat_entries[i].enabled = m_enable_cheats_by_default;
                }
            }

            return true;
        }

        bool CheatProcessManager::ParseCheatToggles(const char *s, size_t len) {
            size_t i = 0;
            char cur_cheat_name[sizeof(CheatEntry::definition.readable_name)];
            char toggle[8];

            while (i < len) {
                if (std::isspace(static_cast<unsigned char>(s[i]))) {
                    /* Just ignore whitespace. */
                    i++;
                } else if (s[i] == '[') {
                    /* Extract name bounds. */
                    size_t j = i + 1;
                    while (s[j] != ']') {
                        j++;
                        if (j >= len) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_cheat_name));
                    std::memcpy(cur_cheat_name, s + (i + 1), cheat_name_len);
                    cur_cheat_name[cheat_name_len] = 0;

                    /* Skip onwards. */
                    i = j + 1;

                    /* Skip whitespace. */
                    while (std::isspace(static_cast<unsigned char>(s[i]))) {
                        i++;
                    }

                    /* Parse whether to toggle. */
                    j = i + 1;
                    while (!std::isspace(static_cast<unsigned char>(s[j]))) {
                        j++;
                        if (j >= len || (j - i) >= sizeof(toggle)) {
                            return false;
                        }
                    }

                    /* s[i:j] is toggle. */
                    const size_t toggle_len = (j - i);
                    std::memcpy(toggle, s + i, toggle_len);
                    toggle[toggle_len] = 0;

                    /* Allow specifying toggle for not present cheat. */
                    CheatEntry *entry = this->GetCheatEntryByReadableName(cur_cheat_name);
                    if (entry != nullptr) {
                        if (strcasecmp(toggle, "1") == 0 || strcasecmp(toggle, "true") == 0 || strcasecmp(toggle, "on") == 0) {
                            entry->enabled = true;
                        } else if (strcasecmp(toggle, "0") == 0 || strcasecmp(toggle, "false") == 0 || strcasecmp(toggle, "off") == 0) {
                            entry->enabled = false;
                        }
                    }

                    /* Skip onwards. */
                    i = j + 1;
                } else {
                    /* Unexpected character encountered. */
                    return false;
                }
            }

            return true;
        }

        bool CheatProcessManager::LoadCheats(const ncm::ProgramId program_id, const u8 *module_id) {
            /* Reset existing entries. */
            this->ResetAllCheatEntries();

            /* Open the file for program/module_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                util::SNPrintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/%02x%02x%02x%02x%02x%02x%02x%02x.txt", program_id.value,
                        module_id[0], module_id[1], module_id[2], module_id[3], module_id[4], module_id[5], module_id[6], module_id[7]);
                if (R_FAILED(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read))) {
                    return false;
                }
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Get file size. */
            s64 file_size;
            if (R_FAILED(fs::GetFileSize(std::addressof(file_size), file))) {
                return false;
            }
            if (file_size < 0 || file_size >= static_cast<s64>(sizeof(g_text_file_buffer))) {
                return false;
            }

            std::scoped_lock lk(g_text_file_buffer_lock);

            /* Read cheats into buffer. */
            if (R_FAILED(fs::ReadFile(file, 0, g_text_file_buffer, file_size))) {
                return false;
            }
            g_text_file_buffer[file_size] = '\x00';

            /* Parse cheat buffer. */
            return this->ParseCheats(g_text_file_buffer, std::strlen(g_text_file_buffer));
        }

        bool CheatProcessManager::LoadCheatToggles(const ncm::ProgramId program_id) {
            /* Unless we successfully parse, don't save toggles on close. */
            m_should_save_cheat_toggles = false;

            /* Open the file for program_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                util::SNPrintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/toggles.txt", program_id.value);
                if (R_FAILED(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read))) {
                    /* No file presence is allowed. */
                    return true;
                }
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Get file size. */
            s64 file_size;
            if (R_FAILED(fs::GetFileSize(std::addressof(file_size), file))) {
                return false;
            }
            if (file_size < 0 || file_size >= static_cast<s64>(sizeof(g_text_file_buffer))) {
                return false;
            }

            std::scoped_lock lk(g_text_file_buffer_lock);

            /* Read cheats into buffer. */
            if (R_FAILED(fs::ReadFile(file, 0, g_text_file_buffer, file_size))) {
                return false;
            }
            g_text_file_buffer[file_size] = '\x00';

            /* Parse toggle buffer. */
            m_should_save_cheat_toggles = this->ParseCheatToggles(g_text_file_buffer, std::strlen(g_text_file_buffer));
            return m_should_save_cheat_toggles;
        }

        void CheatProcessManager::SaveCheatToggles(const ncm::ProgramId program_id) {
            /* Open the file for program_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                util::SNPrintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/toggles.txt", program_id.value);
                fs::DeleteFile(path);
                fs::CreateFile(path, 0);
                if (R_FAILED(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                    return;
                }
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            s64 offset = 0;
            char buf[0x100];

            /* Save all non-master cheats. */
            for (size_t i = 1; i < MaxCheatCount; i++) {
                if (m_cheat_entries[i].definition.num_opcodes != 0) {
                    util::SNPrintf(buf, sizeof(buf), "[%s]\n", m_cheat_entries[i].definition.readable_name);
                    const size_t name_len = std::strlen(buf);
                    if (R_SUCCEEDED(fs::WriteFile(file, offset, buf, name_len, fs::WriteOption::Flush))) {
                        offset += name_len;
                    }

                    const char *entry = m_cheat_entries[i].enabled ? "true\n" : "false\n";
                    const size_t entry_len = std::strlen(entry);
                    if (R_SUCCEEDED(fs::WriteFile(file, offset, entry, entry_len, fs::WriteOption::Flush))) {
                        offset += entry_len;
                    }
                }
            }
        }


        /* Manager global. */
        util::TypedStorage<CheatProcessManager> g_cheat_process_manager;

    }

    void InitializeCheatManager() {
        /* Initialize the debug events manager (spawning its threads). */
        InitializeDebugEventsManager();

        /* Initialize the frozen address map heap. */
        g_frozen_address_map_heap = lmem::CreateUnitHeap(g_frozen_address_map_memory, sizeof(g_frozen_address_map_memory), sizeof(FrozenAddressMapEntry), lmem::CreateOption_ThreadSafe);

        /* Create the cheat process manager (spawning its threads). */
        util::ConstructAt(g_cheat_process_manager);
    }

    bool GetHasActiveCheatProcess() {
        return GetReference(g_cheat_process_manager).GetHasActiveCheatProcess();
    }

    os::NativeHandle GetCheatProcessEventHandle() {
        return GetReference(g_cheat_process_manager).GetCheatProcessEventHandle();
    }

    Result GetCheatProcessMetadata(CheatProcessMetadata *out) {
        return GetReference(g_cheat_process_manager).GetCheatProcessMetadata(out);
    }

    Result ForceOpenCheatProcess() {
        return GetReference(g_cheat_process_manager).ForceOpenCheatProcess();
    }

    Result PauseCheatProcess() {
        return GetReference(g_cheat_process_manager).PauseCheatProcess();
    }

    Result ResumeCheatProcess() {
        return GetReference(g_cheat_process_manager).ResumeCheatProcess();
    }

    Result ForceCloseCheatProcess() {
        return GetReference(g_cheat_process_manager).ForceCloseCheatProcess();
    }

    Result ReadCheatProcessMemoryUnsafe(u64 process_addr, void *out_data, size_t size) {
        return GetReference(g_cheat_process_manager).ReadCheatProcessMemoryUnsafe(process_addr, out_data, size);
    }

    Result WriteCheatProcessMemoryUnsafe(u64 process_addr, void *data, size_t size) {
        return GetReference(g_cheat_process_manager).WriteCheatProcessMemoryUnsafe(process_addr, data, size);
    }

    Result PauseCheatProcessUnsafe() {
        return GetReference(g_cheat_process_manager).PauseCheatProcessUnsafe();
    }

    Result ResumeCheatProcessUnsafe() {
        return GetReference(g_cheat_process_manager).ResumeCheatProcessUnsafe();
    }

    Result GetCheatProcessMappingCount(u64 *out_count) {
        return GetReference(g_cheat_process_manager).GetCheatProcessMappingCount(out_count);
    }

    Result GetCheatProcessMappings(svc::MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
        return GetReference(g_cheat_process_manager).GetCheatProcessMappings(mappings, max_count, out_count, offset);
    }

    Result ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
        return GetReference(g_cheat_process_manager).ReadCheatProcessMemory(proc_addr, out_data, size);
    }

    Result WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
        return GetReference(g_cheat_process_manager).WriteCheatProcessMemory(proc_addr, data, size);
    }

    Result QueryCheatProcessMemory(svc::MemoryInfo *mapping, u64 address) {
        return GetReference(g_cheat_process_manager).QueryCheatProcessMemory(mapping, address);
    }

    Result GetCheatCount(u64 *out_count) {
        return GetReference(g_cheat_process_manager).GetCheatCount(out_count);
    }

    Result GetCheats(CheatEntry *cheats, size_t max_count, u64 *out_count, u64 offset) {
        return GetReference(g_cheat_process_manager).GetCheats(cheats, max_count, out_count, offset);
    }

    Result GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
        return GetReference(g_cheat_process_manager).GetCheatById(out_cheat, cheat_id);
    }

    Result ToggleCheat(u32 cheat_id) {
        return GetReference(g_cheat_process_manager).ToggleCheat(cheat_id);
    }

    Result AddCheat(u32 *out_id, const CheatDefinition &def, bool enabled) {
        return GetReference(g_cheat_process_manager).AddCheat(out_id, def, enabled);
    }

    Result RemoveCheat(u32 cheat_id) {
        return GetReference(g_cheat_process_manager).RemoveCheat(cheat_id);
    }

    Result SetMasterCheat(const CheatDefinition &def) {
        return GetReference(g_cheat_process_manager).SetMasterCheat(def);
    }

    Result ReadStaticRegister(u64 *out, size_t which) {
        return GetReference(g_cheat_process_manager).ReadStaticRegister(out, which);
    }

    Result WriteStaticRegister(size_t which, u64 value) {
        return GetReference(g_cheat_process_manager).WriteStaticRegister(which, value);
    }

    Result ResetStaticRegisters() {
        return GetReference(g_cheat_process_manager).ResetStaticRegisters();
    }

    Result GetFrozenAddressCount(u64 *out_count) {
        return GetReference(g_cheat_process_manager).GetFrozenAddressCount(out_count);
    }

    Result GetFrozenAddresses(FrozenAddressEntry *frz_addrs, size_t max_count, u64 *out_count, u64 offset) {
        return GetReference(g_cheat_process_manager).GetFrozenAddresses(frz_addrs, max_count, out_count, offset);
    }

    Result GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
        return GetReference(g_cheat_process_manager).GetFrozenAddress(frz_addr, address);
    }

    Result EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
        return GetReference(g_cheat_process_manager).EnableFrozenAddress(out_value, address, width);
    }

    Result DisableFrozenAddress(u64 address) {
        return GetReference(g_cheat_process_manager).DisableFrozenAddress(address);
    }

}

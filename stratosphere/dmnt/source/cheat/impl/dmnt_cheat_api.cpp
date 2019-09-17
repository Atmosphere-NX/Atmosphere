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

#include <map>
#include "dmnt_cheat_api.hpp"
#include "dmnt_cheat_vm.hpp"
#include "dmnt_cheat_debug_events_manager.hpp"
#include <stratosphere/map.hpp>

namespace sts::dmnt::cheat::impl {

    namespace {

        /* Helper definitions. */
        constexpr size_t MaxCheatCount = 0x80;
        constexpr size_t MaxFrozenAddressCount = 0x80;

        /* Manager class. */
        class CheatProcessManager {
            private:
                HosMutex cheat_lock;
                HosSignal debug_events_signal;
                HosThread detect_thread, debug_events_thread;
                IEvent *cheat_process_event;
                Handle cheat_process_debug_handle = INVALID_HANDLE;
                CheatProcessMetadata cheat_process_metadata = {};

                HosThread vm_thread;
                bool needs_reload_vm = false;
                CheatVirtualMachine cheat_vm;

                bool enable_cheats_by_default = true;
                bool always_save_cheat_toggles = false;
                bool should_save_cheat_toggles = false;
                CheatEntry cheat_entries[MaxCheatCount];
                std::map<u64, FrozenAddressValue> frozen_addresses_map;
            private:
                static void DetectLaunchThread(void *_this);
                static void VirtualMachineThread(void *_this);
                static void DebugEventsThread(void *_this);

                Result AttachToApplicationProcess(bool on_process_launch);

                bool ParseCheats(const char *s, size_t len);
                bool LoadCheats(const ncm::TitleId title_id, const u8 *build_id);
                bool ParseCheatToggles(const char *s, size_t len);
                bool LoadCheatToggles(const ncm::TitleId title_id);
                void SaveCheatToggles(const ncm::TitleId title_id);

                bool GetNeedsReloadVm() const {
                    return this->needs_reload_vm;
                }

                void SetNeedsReloadVm(bool reload) {
                    this->needs_reload_vm = reload;
                }


                void ResetCheatEntry(size_t i) {
                    if (i < MaxCheatCount) {
                        std::memset(&this->cheat_entries[i], 0, sizeof(this->cheat_entries[i]));
                        this->cheat_entries[i].cheat_id = i;

                        this->SetNeedsReloadVm(true);
                    }
                }

                void ResetAllCheatEntries() {
                    for (size_t i = 0; i < MaxCheatCount; i++) {
                        this->ResetCheatEntry(i);
                    }
                }

                CheatEntry *GetCheatEntryById(size_t i) {
                    if (i < MaxCheatCount) {
                        return &this->cheat_entries[i];
                    }

                    return nullptr;
                }

                CheatEntry *GetCheatEntryByReadableName(const char *readable_name) {
                    /* Check all non-master cheats for match. */
                    for (size_t i = 1; i < MaxCheatCount; i++) {
                        if (std::strncmp(this->cheat_entries[i].definition.readable_name, readable_name, sizeof(this->cheat_entries[i].definition.readable_name)) == 0) {
                            return &this->cheat_entries[i];
                        }
                    }

                    return nullptr;
                }

                CheatEntry *GetFreeCheatEntry() {
                    /* Check all non-master cheats for availability. */
                    for (size_t i = 1; i < MaxCheatCount; i++) {
                        if (this->cheat_entries[i].definition.num_opcodes == 0) {
                            return &this->cheat_entries[i];
                        }
                    }

                    return nullptr;
                }

                void CloseActiveCheatProcess() {
                    if (this->cheat_process_debug_handle != INVALID_HANDLE) {
                        /* Knock out the debug events thread. */
                        R_ASSERT(this->debug_events_thread.CancelSynchronization());

                        /* Close resources. */
                        R_ASSERT(svcCloseHandle(this->cheat_process_debug_handle));
                        this->cheat_process_debug_handle = INVALID_HANDLE;

                        /* Save cheat toggles. */
                        if (this->always_save_cheat_toggles || this->should_save_cheat_toggles) {
                            this->SaveCheatToggles(this->cheat_process_metadata.title_id);
                            this->should_save_cheat_toggles = false;
                        }

                        /* Clear metadata. */
                        static_assert(std::is_pod<decltype(this->cheat_process_metadata)>::value, "CheatProcessMetadata definition!");
                        std::memset(&this->cheat_process_metadata, 0, sizeof(this->cheat_process_metadata));

                        /* Clear cheat list. */
                        this->ResetAllCheatEntries();

                        /* Clear frozen addresses. */
                        this->frozen_addresses_map.clear();

                        /* Signal to our fans. */
                        this->cheat_process_event->Signal();
                    }
                }

                bool HasActiveCheatProcess() {
                    /* Note: This function *MUST* be called only with the cheat lock held. */
                    u64 tmp;
                    bool has_cheat_process = this->cheat_process_debug_handle != INVALID_HANDLE;
                    has_cheat_process &= R_SUCCEEDED(svcGetProcessId(&tmp, this->cheat_process_debug_handle));
                    has_cheat_process &= R_SUCCEEDED(pm::dmnt::GetApplicationProcessId(&tmp));
                    has_cheat_process &= (tmp == this->cheat_process_metadata.process_id);

                    if (!has_cheat_process) {
                        this->CloseActiveCheatProcess();
                    }

                    return has_cheat_process;
                }

                Result EnsureCheatProcess() {
                    if (!this->HasActiveCheatProcess()) {
                        return ResultDmntCheatNotAttached;
                    }
                    return ResultSuccess;
                }

                Handle GetCheatProcessHandle() const {
                    return this->cheat_process_debug_handle;
                }

                Handle HookToCreateApplicationProcess() const {
                    Handle h = INVALID_HANDLE;
                    R_ASSERT(pm::dmnt::HookToCreateApplicationProcess(&h));
                    return h;
                }

                void StartProcess(u64 process_id) const {
                    R_ASSERT(pm::dmnt::StartProcess(process_id));
                }

            public:
                CheatProcessManager() {
                    /* Create cheat process detection event. */
                    this->cheat_process_event = CreateWriteOnlySystemEvent();

                    /* Learn whether we should enable cheats by default. */
                    {
                        u8 en;
                        if (R_SUCCEEDED(setsysGetSettingsItemValue("atmosphere", "dmnt_cheats_enabled_by_default", &en, sizeof(en)))) {
                            this->enable_cheats_by_default = (en != 0);
                        }

                        if (R_SUCCEEDED(setsysGetSettingsItemValue("atmosphere", "dmnt_always_save_cheat_toggles", &en, sizeof(en)))) {
                            this->always_save_cheat_toggles = (en != 0);
                        }
                    }

                    /* Spawn application detection thread, spawn cheat vm thread. */
                    R_ASSERT(this->detect_thread.Initialize(&CheatProcessManager::DetectLaunchThread, this, 0x4000, 39));
                    R_ASSERT(this->vm_thread.Initialize(&CheatProcessManager::VirtualMachineThread, this, 0x4000, 48));
                    R_ASSERT(this->debug_events_thread.Initialize(&CheatProcessManager::DebugEventsThread, this, 0x4000, 24));

                    /* Start threads. */
                    R_ASSERT(this->detect_thread.Start());
                    R_ASSERT(this->vm_thread.Start());
                    R_ASSERT(this->debug_events_thread.Start());
                }

                bool GetHasActiveCheatProcess() {
                    std::scoped_lock lk(this->cheat_lock);

                    return this->HasActiveCheatProcess();
                }

                Handle GetCheatProcessEventHandle() {
                    return this->cheat_process_event->GetHandle();
                }

                Result GetCheatProcessMetadata(CheatProcessMetadata *out) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    std::memcpy(out, &this->cheat_process_metadata, sizeof(*out));
                    return ResultSuccess;
                }

                Result ForceOpenCheatProcess() {
                    return this->AttachToApplicationProcess(false);
                }

                Result ReadCheatProcessMemoryUnsafe(u64 proc_addr, void *out_data, size_t size) {
                    return svcReadDebugProcessMemory(out_data, this->GetCheatProcessHandle(), proc_addr, size);
                }

                Result WriteCheatProcessMemoryUnsafe(u64 proc_addr, const void *data, size_t size) {
                    R_TRY(svcWriteDebugProcessMemory(this->GetCheatProcessHandle(), data, proc_addr, size));

                    for (auto& [address, value] : this->frozen_addresses_map) {
                        /* Map is ordered, so break when we can. */
                        if (address >= proc_addr + size) {
                            break;
                        }

                        /* Check if we need to write. */
                        if (proc_addr <= address && address < proc_addr + size) {
                            const size_t offset = (address - proc_addr);
                            const size_t copy_size = std::min(sizeof(value.value), size - offset);
                            std::memcpy(&value.value, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data) + offset), copy_size);
                        }
                    }

                    return ResultSuccess;
                }

                Result GetCheatProcessMappingCount(u64 *out_count) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    MemoryInfo mem_info;
                    u64 address = 0, count = 0;
                    do {
                        mem_info.perm = Perm_None;
                        u32 tmp;
                        if (R_FAILED(svcQueryDebugProcessMemory(&mem_info, &tmp, this->GetCheatProcessHandle(), address))) {
                            break;
                        }

                        if (mem_info.perm != Perm_None) {
                            count++;
                        }

                        address = mem_info.addr + mem_info.size;
                    } while (address != 0);

                    *out_count = count;
                    return ResultSuccess;
                }

                Result GetCheatProcessMappings(MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    MemoryInfo mem_info;
                    u64 address = 0, total_count = 0, written_count = 0;
                    do {
                        mem_info.perm = Perm_None;
                        u32 tmp;
                        if (R_FAILED(svcQueryDebugProcessMemory(&mem_info, &tmp, this->GetCheatProcessHandle(), address))) {
                            break;
                        }

                        if (mem_info.perm != Perm_None) {
                            if (offset <= total_count && written_count < max_count) {
                                mappings[written_count++] = mem_info;
                            }
                            total_count++;
                        }

                        address = mem_info.addr + mem_info.size;
                    } while (address != 0 && written_count < max_count);

                    *out_count = written_count;
                    return ResultSuccess;
                }

                Result ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->ReadCheatProcessMemoryUnsafe(proc_addr, out_data, size);
                }

                Result WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->WriteCheatProcessMemoryUnsafe(proc_addr, data, size);
                }

                Result QueryCheatProcessMemory(MemoryInfo *mapping, u64 address) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    u32 tmp;
                    return svcQueryDebugProcessMemory(mapping, &tmp, this->GetCheatProcessHandle(), address);
                }

                Result GetCheatCount(u64 *out_count) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    size_t count = 0;
                    for (size_t i = 0; i < MaxCheatCount; i++) {
                        if (this->cheat_entries[i].definition.num_opcodes) {
                            count++;
                        }
                    }

                    *out_count = count;
                    return ResultSuccess;
                }

                Result GetCheats(CheatEntry *out_cheats, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    size_t count = 0, total_count = 0;
                    for (size_t i = 0; i < MaxCheatCount && count < max_count; i++) {
                        if (this->cheat_entries[i].definition.num_opcodes) {
                            total_count++;
                            if (total_count > offset) {
                                out_cheats[count++] = this->cheat_entries[i];
                            }
                        }
                    }

                    *out_count = count;
                    return ResultSuccess;
                }

                Result GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    if (entry == nullptr || entry->definition.num_opcodes == 0) {
                        return ResultDmntCheatUnknownChtId;
                    }

                    *out_cheat = *entry;
                    return ResultSuccess;
                }

                Result ToggleCheat(u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    if (entry == nullptr || entry->definition.num_opcodes == 0) {
                        return ResultDmntCheatUnknownChtId;
                    }

                    if (cheat_id == 0) {
                        return ResultDmntCheatCannotDisableMasterCheat;
                    }

                    entry->enabled = !entry->enabled;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess;
                }

                Result AddCheat(u32 *out_id, const CheatDefinition *def, bool enabled) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    if (def->num_opcodes == 0 || def->num_opcodes > util::size(def->opcodes)) {
                        return ResultDmntCheatInvalidCheat;
                    }

                    CheatEntry *new_entry = this->GetFreeCheatEntry();
                    if (new_entry == nullptr) {
                        return ResultDmntCheatOutOfCheats;
                    }

                    new_entry->enabled = enabled;
                    new_entry->definition = *def;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess;
                }

                Result RemoveCheat(u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    if (cheat_id >= MaxCheatCount) {
                        return ResultDmntCheatUnknownChtId;
                    }

                    this->ResetCheatEntry(cheat_id);

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess;
                }

                Result GetFrozenAddressCount(u64 *out_count) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    *out_count = this->frozen_addresses_map.size();
                    return ResultSuccess;
                }

                Result GetFrozenAddresses(FrozenAddressEntry *frz_addrs, size_t max_count, u64 *out_count, u64 offset) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    u64 total_count = 0, written_count = 0;
                    for (auto const& [address, value] : this->frozen_addresses_map) {
                        if (written_count >= max_count) {
                            break;
                        }

                        if (offset <= total_count) {
                            frz_addrs[written_count].address = address;
                            frz_addrs[written_count].value = value;
                            written_count++;
                        }
                        total_count++;
                    }

                    *out_count = written_count;
                    return ResultSuccess;
                }

                Result GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = this->frozen_addresses_map.find(address);
                    if (it == this->frozen_addresses_map.end()) {
                        return ResultDmntCheatAddressNotFrozen;
                    }

                    frz_addr->address = it->first;
                    frz_addr->value   = it->second;
                    return ResultSuccess;
                }

                Result EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    if (this->frozen_addresses_map.size() >= MaxFrozenAddressCount) {
                        return ResultDmntCheatTooManyFrozenAddresses;
                    }

                    const auto it = this->frozen_addresses_map.find(address);
                    if (it != this->frozen_addresses_map.end()) {
                        return ResultDmntCheatAddressAlreadyFrozen;
                    }

                    FrozenAddressValue value = {};
                    value.width = width;
                    R_TRY(this->ReadCheatProcessMemoryUnsafe(address, &value.value, width));

                    this->frozen_addresses_map[address] = value;
                    *out_value = value.value;
                    return ResultSuccess;
                }

                Result DisableFrozenAddress(u64 address) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = this->frozen_addresses_map.find(address);
                    if (it == this->frozen_addresses_map.end()) {
                        return ResultDmntCheatAddressNotFrozen;
                    }

                    this->frozen_addresses_map.erase(it);
                    return ResultSuccess;
                }

        };

        void CheatProcessManager::DetectLaunchThread(void *_this) {
            CheatProcessManager *this_ptr = reinterpret_cast<CheatProcessManager *>(_this);
            Event hook;
            while (true) {
                eventLoadRemote(&hook, this_ptr->HookToCreateApplicationProcess(), true);
                if (R_SUCCEEDED(eventWait(&hook, U64_MAX))) {
                    this_ptr->AttachToApplicationProcess(true);
                }
                eventClose(&hook);
            }
        }

        void CheatProcessManager::DebugEventsThread(void *_this) {
            CheatProcessManager *this_ptr = reinterpret_cast<CheatProcessManager *>(_this);
            while (true) {
                /* Atomically wait (and clear) signal for new process. */
                this_ptr->debug_events_signal.Wait(true);
                while (true) {
                    while (R_SUCCEEDED(svcWaitSynchronizationSingle(this_ptr->GetCheatProcessHandle(), U64_MAX))) {
                        std::scoped_lock lk(this_ptr->cheat_lock);

                        /* Handle any pending debug events. */
                        if (this_ptr->HasActiveCheatProcess()) {
                            dmnt::cheat::impl::ContinueCheatProcess(this_ptr->GetCheatProcessHandle());
                        }
                    }

                    /* WaitSynchronization failed. This means someone canceled our synchronization, possibly us. */
                    /* Let's check if we should quit! */
                    std::scoped_lock lk(this_ptr->cheat_lock);
                    if (!this_ptr->HasActiveCheatProcess()) {
                        break;
                    }
                }
            }
        }

        void CheatProcessManager::VirtualMachineThread(void *_this) {
            CheatProcessManager *this_ptr = reinterpret_cast<CheatProcessManager *>(_this);
            while (true) {
                /* Apply cheats. */
                {
                    std::scoped_lock lk(this_ptr->cheat_lock);

                    if (this_ptr->HasActiveCheatProcess()) {
                        /* Execute VM. */
                        if (!this_ptr->GetNeedsReloadVm() || this_ptr->cheat_vm.LoadProgram(this_ptr->cheat_entries, util::size(this_ptr->cheat_entries))) {
                            this_ptr->SetNeedsReloadVm(false);

                            /* Execute program only if it has opcodes. */
                            if (this_ptr->cheat_vm.GetProgramSize()) {
                                this_ptr->cheat_vm.Execute(&this_ptr->cheat_process_metadata);
                            }
                        }

                        /* Apply frozen addresses. */
                        for (auto const& [address, value] : this_ptr->frozen_addresses_map) {
                            /* Use Write SVC directly, to avoid the usual frozen address update logic. */
                            svcWriteDebugProcessMemory(this_ptr->GetCheatProcessHandle(), &value.value, address, value.width);
                        }
                    }
                }

                /* Sleep until next potential execution. */
                constexpr u64 ONE_SECOND = 1'000'000'000ul;
                constexpr u64 TIMES_PER_SECOND = 12;
                constexpr u64 DELAY_TIME = ONE_SECOND / TIMES_PER_SECOND;
                svcSleepThread(DELAY_TIME);
            }
        }

        #define R_ASSERT_IF_NEW_PROCESS(res) \
            if (on_process_launch) { \
                R_ASSERT(res); \
            } else { \
                R_TRY(res); \
            }

        Result CheatProcessManager::AttachToApplicationProcess(bool on_process_launch) {
            std::scoped_lock lk(this->cheat_lock);

            /* Close the active process, if needed. */
            {
                if (this->HasActiveCheatProcess()) {
                    /* When forcing attach, we're done. */
                    if (!on_process_launch) {
                        return ResultSuccess;
                    }
                }

                /* Detach from the current process, if it's open. */
                this->CloseActiveCheatProcess();
            }

            /* Get the application process's ID. */
            R_ASSERT_IF_NEW_PROCESS(pm::dmnt::GetApplicationProcessId(&this->cheat_process_metadata.process_id));
            auto proc_guard = SCOPE_GUARD {
                if (on_process_launch) {
                    this->StartProcess(this->cheat_process_metadata.process_id);
                }
                this->cheat_process_metadata.process_id = 0;
            };

            /* Get process handle, use it to learn memory extents. */
            {
                Handle proc_h = INVALID_HANDLE;
                ncm::TitleLocation loc = {};
                ON_SCOPE_EXIT { if (proc_h != INVALID_HANDLE) { R_ASSERT(svcCloseHandle(proc_h)); } };

                R_ASSERT_IF_NEW_PROCESS(pm::dmnt::AtmosphereGetProcessInfo(&proc_h, &loc, this->cheat_process_metadata.process_id));
                this->cheat_process_metadata.title_id = loc.title_id;

                {
                    map::AddressSpaceInfo as_info;
                    R_ASSERT(map::GetProcessAddressSpaceInfo(&as_info, proc_h));
                    this->cheat_process_metadata.heap_extents.base  = as_info.heap_base;
                    this->cheat_process_metadata.heap_extents.size  = as_info.heap_size;
                    this->cheat_process_metadata.alias_extents.base = as_info.alias_base;
                    this->cheat_process_metadata.alias_extents.size = as_info.alias_size;
                    this->cheat_process_metadata.aslr_extents.base  = as_info.aslr_base;
                    this->cheat_process_metadata.aslr_extents.size  = as_info.aslr_size;
                }
            }

            /* If new process launch, we may not want to actually attach. */
            if (on_process_launch) {
                if (!cfg::IsCheatEnableKeyHeld(this->cheat_process_metadata.title_id)) {
                    return ResultDmntCheatNotAttached;
                }
            }

            /* Get module information from loader. */
            {
                LoaderModuleInfo proc_modules[2];
                u32 num_modules;

                /* TODO: ldr::dmnt:: */
                R_ASSERT_IF_NEW_PROCESS(ldrDmntGetModuleInfos(this->cheat_process_metadata.process_id, proc_modules, util::size(proc_modules), &num_modules));

                /* All applications must have two modules. */
                /* Only accept one (which means we're attaching to HBL) */
                /* if we aren't auto-attaching. */
                const LoaderModuleInfo *proc_module = nullptr;
                if (num_modules == 2) {
                    proc_module = &proc_modules[1];
                } else if (num_modules == 1 && !on_process_launch) {
                    proc_module = &proc_modules[0];
                } else {
                    return ResultDmntCheatNotAttached;
                }

                this->cheat_process_metadata.main_nso_extents.base = proc_module->base_address;
                this->cheat_process_metadata.main_nso_extents.size = proc_module->size;
                std::memcpy(this->cheat_process_metadata.main_nso_build_id, proc_module->build_id, sizeof(this->cheat_process_metadata.main_nso_build_id));
            }

            /* Read cheats off the SD. */
            if (!this->LoadCheats(this->cheat_process_metadata.title_id, this->cheat_process_metadata.main_nso_build_id) ||
                !this->LoadCheatToggles(this->cheat_process_metadata.title_id)) {
                /* If new process launch, require success. */
                if (on_process_launch) {
                    return ResultDmntCheatNotAttached;
                }
            }

            /* Open a debug handle. */
            R_ASSERT_IF_NEW_PROCESS(svcDebugActiveProcess(&this->cheat_process_debug_handle, this->cheat_process_metadata.process_id));

            /* Cancel process guard. */
            proc_guard.Cancel();

            /* If new process, start the process. */
            if (on_process_launch) {
                this->StartProcess(this->cheat_process_metadata.process_id);
            }

            /* Signal to the debug events thread. */
            this->debug_events_signal.Signal();

            /* Signal to our fans. */
            this->cheat_process_event->Signal();

            return ResultSuccess;
        }

        #undef R_ASSERT_IF_NEW_PROCESS

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
                        if (j >= len || (j - i - 1) >= sizeof(cur_entry->definition.readable_name)) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = (j - i - 1);
                    std::memcpy(cur_entry->definition.readable_name, &s[i+1], cheat_name_len);
                    cur_entry->definition.readable_name[cheat_name_len] = 0;

                    /* Skip onwards. */
                    i = j + 1;
                } else if (s[i] == '{') {
                    /* We're parsing a master cheat. */
                    cur_entry = &this->cheat_entries[0];

                    /* There can only be one master cheat. */
                    if (cur_entry->definition.num_opcodes > 0) {
                        return false;
                    }

                    /* Extract name bounds */
                    size_t j = i + 1;
                    while (s[j] != '}') {
                        j++;
                        if (j >= len || (j - i - 1) >= sizeof(cur_entry->definition.readable_name)) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = (j - i - 1);
                    memcpy(cur_entry->definition.readable_name, &s[i+1], cheat_name_len);
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
                    std::memcpy(hex_str, &s[i], 8);
                    cur_entry->definition.opcodes[cur_entry->definition.num_opcodes++] = std::strtoul(hex_str, NULL, 16);

                    /* Skip onwards. */
                    i += 8;
                } else {
                    /* Unexpected character encountered. */
                    return false;
                }
            }

            /* Master cheat can't be disabled. */
            if (this->cheat_entries[0].definition.num_opcodes > 0) {
                this->cheat_entries[0].enabled = true;
            }

            /* Enable all entries we parsed. */
            for (size_t i = 1; i < MaxCheatCount; i++) {
                if (this->cheat_entries[i].definition.num_opcodes > 0) {
                    this->cheat_entries[i].enabled = this->enable_cheats_by_default;
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
                        if (j >= len || (j - i - 1) >= sizeof(cur_cheat_name)) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = (j - i - 1);
                    std::memcpy(cur_cheat_name, &s[i+1], cheat_name_len);
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
                    std::memcpy(toggle, &s[i], toggle_len);
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

        bool CheatProcessManager::LoadCheats(const ncm::TitleId title_id, const u8 *build_id) {
            /* Reset existing entries. */
            this->ResetAllCheatEntries();

            /* Open the file for title/build_id. */
            FILE *f_cht = nullptr;
            {
                char path[FS_MAX_PATH+1] = {0};
                std::snprintf(path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/cheats/%02x%02x%02x%02x%02x%02x%02x%02x.txt", static_cast<u64>(title_id),
                        build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);

                f_cht = fopen(path, "rb");
            }

            /* Check for open failure. */
            if (f_cht == nullptr) {
                return false;
            }
            ON_SCOPE_EXIT { fclose(f_cht); };

            /* Get file size. */
            fseek(f_cht, 0L, SEEK_END);
            const size_t cht_sz = ftell(f_cht);
            fseek(f_cht, 0L, SEEK_SET);

            /* Allocate cheat txt buffer. */
            char *cht_txt = reinterpret_cast<char *>(std::malloc(cht_sz + 1));
            if (cht_txt == nullptr) {
                return false;
            }
            ON_SCOPE_EXIT { std::free(cht_txt); };

            /* Read cheats into buffer. */
            if (fread(cht_txt, 1, cht_sz, f_cht) != cht_sz) {
                return false;
            }
            cht_txt[cht_sz] = 0;

            /* Parse cheat buffer. */
            return this->ParseCheats(cht_txt, std::strlen(cht_txt));
        }

        bool CheatProcessManager::LoadCheatToggles(const ncm::TitleId title_id) {
            /* Open the file for title_id. */
            FILE *f_tg = nullptr;
            {
                char path[FS_MAX_PATH+1] = {0};
                std::snprintf(path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/cheats/toggles.txt", static_cast<u64>(title_id));
                f_tg = fopen(path, "rb");
            }

            /* Unless we successfully parse, don't save toggles on close. */
            this->should_save_cheat_toggles = false;

            /* Check for null, which is allowed. */
            if (f_tg == nullptr) {
                return true;
            }
            ON_SCOPE_EXIT { fclose(f_tg); };

            /* Get file size. */
            fseek(f_tg, 0L, SEEK_END);
            const size_t tg_sz = ftell(f_tg);
            fseek(f_tg, 0L, SEEK_SET);

            /* Allocate toggle txt buffer. */
            char *tg_txt = reinterpret_cast<char *>(std::malloc(tg_sz + 1));
            if (tg_txt == nullptr) {
                return false;
            }
            ON_SCOPE_EXIT { std::free(tg_txt); };

            /* Read cheats into buffer. */
            if (fread(tg_txt, 1, tg_sz, f_tg) != tg_sz) {
                return false;
            }
            tg_txt[tg_sz] = 0;

            /* Parse toggle buffer. */
            this->should_save_cheat_toggles = this->ParseCheatToggles(tg_txt, std::strlen(tg_txt));
            return this->should_save_cheat_toggles;
        }

        void CheatProcessManager::SaveCheatToggles(const ncm::TitleId title_id) {
            /* Open the file for title_id. */
            FILE *f_tg = nullptr;
            {
                char path[FS_MAX_PATH+1] = {0};
                std::snprintf(path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/cheats/toggles.txt", static_cast<u64>(title_id));
                if ((f_tg = fopen(path, "wb")) == nullptr) {
                    return;
                }
            }
            ON_SCOPE_EXIT { fclose(f_tg); };

            /* Save all non-master cheats. */
            for (size_t i = 1; i < MaxCheatCount; i++) {
                if (this->cheat_entries[i].definition.num_opcodes != 0) {
                    fprintf(f_tg, "[%s]\n", this->cheat_entries[i].definition.readable_name);
                    if (this->cheat_entries[i].enabled) {
                        fprintf(f_tg, "true\n");
                    } else {
                        fprintf(f_tg, "false\n");
                    }
                }
            }
        }


        /* Manager global. */
        CheatProcessManager g_cheat_process_manager;

    }

    bool GetHasActiveCheatProcess() {
        return g_cheat_process_manager.GetHasActiveCheatProcess();
    }

    Handle GetCheatProcessEventHandle() {
        return g_cheat_process_manager.GetCheatProcessEventHandle();
    }

    Result GetCheatProcessMetadata(CheatProcessMetadata *out) {
        return g_cheat_process_manager.GetCheatProcessMetadata(out);
    }

    Result ForceOpenCheatProcess() {
        return g_cheat_process_manager.ForceOpenCheatProcess();
    }

    Result ReadCheatProcessMemoryUnsafe(u64 process_addr, void *out_data, size_t size) {
        return g_cheat_process_manager.ReadCheatProcessMemoryUnsafe(process_addr, out_data, size);
    }

    Result WriteCheatProcessMemoryUnsafe(u64 process_addr, void *data, size_t size) {
        return g_cheat_process_manager.WriteCheatProcessMemoryUnsafe(process_addr, data, size);
    }

    Result GetCheatProcessMappingCount(u64 *out_count) {
        return g_cheat_process_manager.GetCheatProcessMappingCount(out_count);
    }

    Result GetCheatProcessMappings(MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
        return g_cheat_process_manager.GetCheatProcessMappings(mappings, max_count, out_count, offset);
    }

    Result ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
        return g_cheat_process_manager.ReadCheatProcessMemory(proc_addr, out_data, size);
    }

    Result WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
        return g_cheat_process_manager.WriteCheatProcessMemory(proc_addr, data, size);
    }

    Result QueryCheatProcessMemory(MemoryInfo *mapping, u64 address) {
        return g_cheat_process_manager.QueryCheatProcessMemory(mapping, address);
    }

    Result GetCheatCount(u64 *out_count) {
        return g_cheat_process_manager.GetCheatCount(out_count);
    }

    Result GetCheats(CheatEntry *cheats, size_t max_count, u64 *out_count, u64 offset) {
        return g_cheat_process_manager.GetCheats(cheats, max_count, out_count, offset);
    }

    Result GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
        return g_cheat_process_manager.GetCheatById(out_cheat, cheat_id);
    }

    Result ToggleCheat(u32 cheat_id) {
        return g_cheat_process_manager.ToggleCheat(cheat_id);
    }

    Result AddCheat(u32 *out_id, const CheatDefinition *def, bool enabled) {
        return g_cheat_process_manager.AddCheat(out_id, def, enabled);
    }

    Result RemoveCheat(u32 cheat_id) {
        return g_cheat_process_manager.RemoveCheat(cheat_id);
    }

    Result GetFrozenAddressCount(u64 *out_count) {
        return g_cheat_process_manager.GetFrozenAddressCount(out_count);
    }

    Result GetFrozenAddresses(FrozenAddressEntry *frz_addrs, size_t max_count, u64 *out_count, u64 offset) {
        return g_cheat_process_manager.GetFrozenAddresses(frz_addrs, max_count, out_count, offset);
    }

    Result GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
        return g_cheat_process_manager.GetFrozenAddress(frz_addr, address);
    }

    Result EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
        return g_cheat_process_manager.EnableFrozenAddress(out_value, address, width);
    }

    Result DisableFrozenAddress(u64 address) {
        return g_cheat_process_manager.DisableFrozenAddress(address);
    }

}

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
#include <stratosphere.hpp>
#include "dmnt_cheat_api.hpp"
#include "dmnt_cheat_vm.hpp"
#include "dmnt_cheat_debug_events_manager.hpp"

namespace ams::dmnt::cheat::impl {

    namespace {

        /* Helper definitions. */
        constexpr size_t MaxCheatCount = 0x80;
        constexpr size_t MaxFrozenAddressCount = 0x80;

        /* Manager class. */
        class CheatProcessManager {
            private:
                static constexpr size_t ThreadStackSize = 0x4000;
            private:
                os::Mutex cheat_lock;
                os::Event unsafe_break_event;
                os::Event debug_events_event; /* Autoclear. */
                os::ThreadType detect_thread, debug_events_thread;
                os::SystemEvent cheat_process_event;
                Handle cheat_process_debug_handle = svc::InvalidHandle;
                CheatProcessMetadata cheat_process_metadata = {};

                os::ThreadType vm_thread;
                bool broken_unsafe = false;
                bool needs_reload_vm = false;
                CheatVirtualMachine cheat_vm;

                bool enable_cheats_by_default = true;
                bool always_save_cheat_toggles = false;
                bool should_save_cheat_toggles = false;
                CheatEntry cheat_entries[MaxCheatCount] = {};
                std::map<u64, FrozenAddressValue> frozen_addresses_map;

                alignas(os::MemoryPageSize) u8 detect_thread_stack[ThreadStackSize] = {};
                alignas(os::MemoryPageSize) u8 debug_events_thread_stack[ThreadStackSize] = {};
                alignas(os::MemoryPageSize) u8 vm_thread_stack[ThreadStackSize] = {};
            private:
                static void DetectLaunchThread(void *_this);
                static void VirtualMachineThread(void *_this);
                static void DebugEventsThread(void *_this);

                Result AttachToApplicationProcess(bool on_process_launch);

                bool ParseCheats(const char *s, size_t len);
                bool LoadCheats(const ncm::ProgramId program_id, const u8 *build_id);
                bool ParseCheatToggles(const char *s, size_t len);
                bool LoadCheatToggles(const ncm::ProgramId program_id);
                void SaveCheatToggles(const ncm::ProgramId program_id);

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

                    this->cheat_vm.ResetStaticRegisters();
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
                    if (this->cheat_process_debug_handle != svc::InvalidHandle) {
                        /* We don't need to do any unsafe brekaing. */
                        this->broken_unsafe = false;
                        this->unsafe_break_event.Signal();

                        /* Knock out the debug events thread. */
                        os::CancelThreadSynchronization(std::addressof(this->debug_events_thread));

                        /* Close resources. */
                        R_ABORT_UNLESS(svc::CloseHandle(this->cheat_process_debug_handle));
                        this->cheat_process_debug_handle = svc::InvalidHandle;

                        /* Save cheat toggles. */
                        if (this->always_save_cheat_toggles || this->should_save_cheat_toggles) {
                            this->SaveCheatToggles(this->cheat_process_metadata.program_id);
                            this->should_save_cheat_toggles = false;
                        }

                        /* Clear metadata. */
                        static_assert(util::is_pod<decltype(this->cheat_process_metadata)>::value, "CheatProcessMetadata definition!");
                        std::memset(&this->cheat_process_metadata, 0, sizeof(this->cheat_process_metadata));

                        /* Clear cheat list. */
                        this->ResetAllCheatEntries();

                        /* Clear frozen addresses. */
                        this->frozen_addresses_map.clear();

                        /* Signal to our fans. */
                        this->cheat_process_event.Signal();
                    }
                }

                bool HasActiveCheatProcess() {
                    /* Note: This function *MUST* be called only with the cheat lock held. */
                    os::ProcessId pid;
                    bool has_cheat_process = this->cheat_process_debug_handle != svc::InvalidHandle;
                    has_cheat_process &= R_SUCCEEDED(os::TryGetProcessId(&pid, this->cheat_process_debug_handle));
                    has_cheat_process &= R_SUCCEEDED(pm::dmnt::GetApplicationProcessId(&pid));
                    has_cheat_process &= (pid == this->cheat_process_metadata.process_id);

                    if (!has_cheat_process) {
                        this->CloseActiveCheatProcess();
                    }

                    return has_cheat_process;
                }

                Result EnsureCheatProcess() {
                    R_UNLESS(this->HasActiveCheatProcess(), ResultCheatNotAttached());
                    return ResultSuccess();
                }

                Handle GetCheatProcessHandle() const {
                    return this->cheat_process_debug_handle;
                }

                Handle HookToCreateApplicationProcess() const {
                    Handle h = svc::InvalidHandle;
                    R_ABORT_UNLESS(pm::dmnt::HookToCreateApplicationProcess(&h));
                    return h;
                }

                void StartProcess(os::ProcessId process_id) const {
                    R_ABORT_UNLESS(pm::dmnt::StartProcess(process_id));
                }

            public:
                CheatProcessManager() : cheat_lock(false), unsafe_break_event(os::EventClearMode_ManualClear), debug_events_event(os::EventClearMode_AutoClear), cheat_process_event(os::EventClearMode_AutoClear, true) {
                    /* Learn whether we should enable cheats by default. */
                    {
                        u8 en = 0;
                        if (settings::fwdbg::GetSettingsItemValue(&en, sizeof(en), "atmosphere", "dmnt_cheats_enabled_by_default") == sizeof(en)) {
                            this->enable_cheats_by_default = (en != 0);
                        }

                        en = 0;
                        if (settings::fwdbg::GetSettingsItemValue( &en, sizeof(en), "atmosphere", "dmnt_always_save_cheat_toggles") == sizeof(en)) {
                            this->always_save_cheat_toggles = (en != 0);
                        }
                    }

                    /* Spawn application detection thread, spawn cheat vm thread. */
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(this->detect_thread), DetectLaunchThread, this, this->detect_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatDetect)));
                    os::SetThreadNamePointer(std::addressof(this->detect_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatDetect));
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(this->vm_thread), VirtualMachineThread, this, this->vm_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatVirtualMachine)));
                    os::SetThreadNamePointer(std::addressof(this->vm_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatVirtualMachine));
                    R_ABORT_UNLESS(os::CreateThread(std::addressof(this->debug_events_thread), DebugEventsThread, this, this->debug_events_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, CheatDebugEvents)));
                    os::SetThreadNamePointer(std::addressof(this->debug_events_thread), AMS_GET_SYSTEM_THREAD_NAME(dmnt, CheatDebugEvents));

                    /* Start threads. */
                    os::StartThread(std::addressof(this->detect_thread));
                    os::StartThread(std::addressof(this->vm_thread));
                    os::StartThread(std::addressof(this->debug_events_thread));
                }

                bool GetHasActiveCheatProcess() {
                    std::scoped_lock lk(this->cheat_lock);

                    return this->HasActiveCheatProcess();
                }

                Handle GetCheatProcessEventHandle() const {
                    return this->cheat_process_event.GetReadableHandle();
                }

                Result GetCheatProcessMetadata(CheatProcessMetadata *out) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    std::memcpy(out, &this->cheat_process_metadata, sizeof(*out));
                    return ResultSuccess();
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

                    return ResultSuccess();
                }

                Result PauseCheatProcessUnsafe() {
                    this->broken_unsafe = true;
                    this->unsafe_break_event.Clear();
                    return svcBreakDebugProcess(this->GetCheatProcessHandle());
                }

                Result ResumeCheatProcessUnsafe() {
                    this->broken_unsafe = false;
                    this->unsafe_break_event.Signal();
                    dmnt::cheat::impl::ContinueCheatProcess(this->GetCheatProcessHandle());
                    return ResultSuccess();
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
                    return ResultSuccess();
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
                    return ResultSuccess();
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

                Result PauseCheatProcess() {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->PauseCheatProcessUnsafe();
                }

                Result ResumeCheatProcess() {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    return this->ResumeCheatProcessUnsafe();
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
                    return ResultSuccess();
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
                    return ResultSuccess();
                }

                Result GetCheatById(CheatEntry *out_cheat, u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    R_UNLESS(entry != nullptr,                   ResultCheatUnknownId());
                    R_UNLESS(entry->definition.num_opcodes != 0, ResultCheatUnknownId());

                    *out_cheat = *entry;
                    return ResultSuccess();
                }

                Result ToggleCheat(u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    CheatEntry *entry = this->GetCheatEntryById(cheat_id);
                    R_UNLESS(entry != nullptr,                   ResultCheatUnknownId());
                    R_UNLESS(entry->definition.num_opcodes != 0, ResultCheatUnknownId());

                    R_UNLESS(cheat_id != 0, ResultCheatCannotDisable());

                    entry->enabled = !entry->enabled;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result AddCheat(u32 *out_id, const CheatDefinition &def, bool enabled) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    R_UNLESS(def.num_opcodes != 0,                       ResultCheatInvalid());
                    R_UNLESS(def.num_opcodes <= util::size(def.opcodes), ResultCheatInvalid());

                    CheatEntry *new_entry = this->GetFreeCheatEntry();
                    R_UNLESS(new_entry != nullptr, ResultCheatOutOfResource());

                    new_entry->enabled = enabled;
                    new_entry->definition = def;

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result RemoveCheat(u32 cheat_id) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(cheat_id < MaxCheatCount, ResultCheatUnknownId());

                    this->ResetCheatEntry(cheat_id);

                    /* Trigger a VM reload. */
                    this->SetNeedsReloadVm(true);

                    return ResultSuccess();
                }

                Result ReadStaticRegister(u64 *out, size_t which) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(which < CheatVirtualMachine::NumStaticRegisters, ResultCheatInvalid());

                    *out = this->cheat_vm.GetStaticRegister(which);
                    return ResultSuccess();
                }

                Result WriteStaticRegister(size_t which, u64 value) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());
                    R_UNLESS(which < CheatVirtualMachine::NumStaticRegisters, ResultCheatInvalid());

                    this->cheat_vm.SetStaticRegister(which, value);
                    return ResultSuccess();
                }

                Result ResetStaticRegisters() {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    this->cheat_vm.ResetStaticRegisters();
                    return ResultSuccess();
                }

                Result GetFrozenAddressCount(u64 *out_count) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    *out_count = this->frozen_addresses_map.size();
                    return ResultSuccess();
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
                    return ResultSuccess();
                }

                Result GetFrozenAddress(FrozenAddressEntry *frz_addr, u64 address) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = this->frozen_addresses_map.find(address);
                    R_UNLESS(it != this->frozen_addresses_map.end(), ResultFrozenAddressNotFound());

                    frz_addr->address = it->first;
                    frz_addr->value   = it->second;
                    return ResultSuccess();
                }

                Result EnableFrozenAddress(u64 *out_value, u64 address, u64 width) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    R_UNLESS(this->frozen_addresses_map.size() < MaxFrozenAddressCount, ResultFrozenAddressOutOfResource());

                    const auto it = this->frozen_addresses_map.find(address);
                    R_UNLESS(it == this->frozen_addresses_map.end(), ResultFrozenAddressAlreadyExists());

                    FrozenAddressValue value = {};
                    value.width = width;
                    R_TRY(this->ReadCheatProcessMemoryUnsafe(address, &value.value, width));

                    this->frozen_addresses_map[address] = value;
                    *out_value = value.value;
                    return ResultSuccess();
                }

                Result DisableFrozenAddress(u64 address) {
                    std::scoped_lock lk(this->cheat_lock);

                    R_TRY(this->EnsureCheatProcess());

                    const auto it = this->frozen_addresses_map.find(address);
                    R_UNLESS(it != this->frozen_addresses_map.end(), ResultFrozenAddressNotFound());

                    this->frozen_addresses_map.erase(it);
                    return ResultSuccess();
                }

        };

        void CheatProcessManager::DetectLaunchThread(void *_this) {
            CheatProcessManager *this_ptr = reinterpret_cast<CheatProcessManager *>(_this);
            Event hook;
            while (true) {
                eventLoadRemote(&hook, this_ptr->HookToCreateApplicationProcess(), true);
                if (R_SUCCEEDED(eventWait(&hook, std::numeric_limits<u64>::max()))) {
                    this_ptr->AttachToApplicationProcess(true);
                }
                eventClose(&hook);
            }
        }

        void CheatProcessManager::DebugEventsThread(void *_this) {
            CheatProcessManager *this_ptr = reinterpret_cast<CheatProcessManager *>(_this);
            while (true) {
                /* Atomically wait (and clear) signal for new process. */
                this_ptr->debug_events_event.Wait();
                while (true) {
                    Handle cheat_process_handle = this_ptr->GetCheatProcessHandle();
                    while (cheat_process_handle != svc::InvalidHandle && R_SUCCEEDED(svcWaitSynchronizationSingle(this_ptr->GetCheatProcessHandle(), std::numeric_limits<u64>::max()))) {
                        this_ptr->cheat_lock.Lock();
                        ON_SCOPE_EXIT { this_ptr->cheat_lock.Unlock(); };
                        {
                            ON_SCOPE_EXIT { cheat_process_handle = this_ptr->GetCheatProcessHandle(); };

                            /* If we did an unsafe break, wait until we're not broken. */
                            if (this_ptr->broken_unsafe) {
                                this_ptr->cheat_lock.Unlock();
                                this_ptr->unsafe_break_event.Wait();
                                this_ptr->cheat_lock.Lock();
                                if (this_ptr->GetCheatProcessHandle() != svc::InvalidHandle) {
                                    continue;
                                } else {
                                    break;
                                }
                            }

                            /* Handle any pending debug events. */
                            if (this_ptr->HasActiveCheatProcess()) {
                                R_TRY_CATCH(dmnt::cheat::impl::ContinueCheatProcess(this_ptr->GetCheatProcessHandle())) {
                                    R_CATCH(svc::ResultProcessTerminated) {
                                        this_ptr->CloseActiveCheatProcess();
                                        break;
                                    }
                                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                            }
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

        #define R_ABORT_UNLESS_IF_NEW_PROCESS(res) \
            if (on_process_launch) { \
                R_ABORT_UNLESS(res); \
            } else { \
                R_TRY(res); \
            }

        Result CheatProcessManager::AttachToApplicationProcess(bool on_process_launch) {
            std::scoped_lock lk(this->cheat_lock);

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
            R_ABORT_UNLESS_IF_NEW_PROCESS(pm::dmnt::GetApplicationProcessId(&this->cheat_process_metadata.process_id));
            auto proc_guard = SCOPE_GUARD {
                if (on_process_launch) {
                    this->StartProcess(this->cheat_process_metadata.process_id);
                }
                this->cheat_process_metadata.process_id = os::ProcessId{};
            };

            /* Get process handle, use it to learn memory extents. */
            {
                Handle proc_h = svc::InvalidHandle;
                ncm::ProgramLocation loc = {};
                cfg::OverrideStatus status = {};
                ON_SCOPE_EXIT { if (proc_h != svc::InvalidHandle) { R_ABORT_UNLESS(svcCloseHandle(proc_h)); } };

                R_ABORT_UNLESS_IF_NEW_PROCESS(pm::dmnt::AtmosphereGetProcessInfo(&proc_h, &loc, &status, this->cheat_process_metadata.process_id));
                this->cheat_process_metadata.program_id = loc.program_id;

                {
                    map::AddressSpaceInfo as_info;
                    R_ABORT_UNLESS(map::GetProcessAddressSpaceInfo(&as_info, proc_h));
                    this->cheat_process_metadata.heap_extents.base  = as_info.heap_base;
                    this->cheat_process_metadata.heap_extents.size  = as_info.heap_size;
                    this->cheat_process_metadata.alias_extents.base = as_info.alias_base;
                    this->cheat_process_metadata.alias_extents.size = as_info.alias_size;
                    this->cheat_process_metadata.aslr_extents.base  = as_info.aslr_base;
                    this->cheat_process_metadata.aslr_extents.size  = as_info.aslr_size;
                }

                /* If new process launch, we may not want to actually attach. */
                if (on_process_launch) {
                    R_UNLESS(status.IsCheatEnabled(), ResultCheatNotAttached());
                }
            }

            /* Get module information from loader. */
            {
                LoaderModuleInfo proc_modules[2];
                s32 num_modules;

                /* TODO: ldr::dmnt:: */
                R_ABORT_UNLESS_IF_NEW_PROCESS(ldrDmntGetProcessModuleInfo(static_cast<u64>(this->cheat_process_metadata.process_id), proc_modules, util::size(proc_modules), &num_modules));

                /* All applications must have two modules. */
                /* Only accept one (which means we're attaching to HBL) */
                /* if we aren't auto-attaching. */
                const LoaderModuleInfo *proc_module = nullptr;
                if (num_modules == 2) {
                    proc_module = &proc_modules[1];
                } else if (num_modules == 1 && !on_process_launch) {
                    proc_module = &proc_modules[0];
                } else {
                    return ResultCheatNotAttached();
                }

                this->cheat_process_metadata.main_nso_extents.base = proc_module->base_address;
                this->cheat_process_metadata.main_nso_extents.size = proc_module->size;
                std::memcpy(this->cheat_process_metadata.main_nso_build_id, proc_module->build_id, sizeof(this->cheat_process_metadata.main_nso_build_id));
            }

            /* Read cheats off the SD. */
            if (!this->LoadCheats(this->cheat_process_metadata.program_id, this->cheat_process_metadata.main_nso_build_id) ||
                !this->LoadCheatToggles(this->cheat_process_metadata.program_id)) {
                /* If new process launch, require success. */
                R_UNLESS(!on_process_launch, ResultCheatNotAttached());
            }

            /* Open a debug handle. */
            R_ABORT_UNLESS_IF_NEW_PROCESS(svcDebugActiveProcess(&this->cheat_process_debug_handle, static_cast<u64>(this->cheat_process_metadata.process_id)));

            /* Cancel process guard. */
            proc_guard.Cancel();

            /* Reset broken state. */
            this->broken_unsafe = false;
            this->unsafe_break_event.Signal();

            /* If new process, start the process. */
            if (on_process_launch) {
                this->StartProcess(this->cheat_process_metadata.process_id);
            }

            /* Signal to the debug events thread. */
            this->debug_events_event.Signal();

            /* Signal to our fans. */
            this->cheat_process_event.Signal();

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
                        if (j >= len) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_entry->definition.readable_name));
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
                        if (j >= len) {
                            return false;
                        }
                    }

                    /* s[i+1:j] is cheat name. */
                    const size_t cheat_name_len = std::min(j - i - 1, sizeof(cur_cheat_name));
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

        bool CheatProcessManager::LoadCheats(const ncm::ProgramId program_id, const u8 *build_id) {
            /* Reset existing entries. */
            this->ResetAllCheatEntries();

            /* Open the file for program/build_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                std::snprintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/%02x%02x%02x%02x%02x%02x%02x%02x.txt", program_id.value,
                        build_id[0], build_id[1], build_id[2], build_id[3], build_id[4], build_id[5], build_id[6], build_id[7]);
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

            /* Allocate cheat txt buffer. */
            char *cht_txt = static_cast<char *>(std::malloc(file_size + 1));
            if (cht_txt == nullptr) {
                return false;
            }
            ON_SCOPE_EXIT { std::free(cht_txt); };

            /* Read cheats into buffer. */
            if (R_FAILED(fs::ReadFile(file, 0, cht_txt, file_size))) {
                return false;
            }
            cht_txt[file_size] = '\x00';

            /* Parse cheat buffer. */
            return this->ParseCheats(cht_txt, std::strlen(cht_txt));
        }

        bool CheatProcessManager::LoadCheatToggles(const ncm::ProgramId program_id) {
            /* Unless we successfully parse, don't save toggles on close. */
            this->should_save_cheat_toggles = false;

            /* Open the file for program_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                std::snprintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/toggles.txt", program_id.value);
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

            /* Allocate toggle txt buffer. */
            char *tg_txt = static_cast<char *>(std::malloc(file_size + 1));
            if (tg_txt == nullptr) {
                return false;
            }
            ON_SCOPE_EXIT { std::free(tg_txt); };

            /* Read cheats into buffer. */
            if (R_FAILED(fs::ReadFile(file, 0, tg_txt, file_size))) {
                return false;
            }
            tg_txt[file_size] = '\x00';

            /* Parse toggle buffer. */
            this->should_save_cheat_toggles = this->ParseCheatToggles(tg_txt, std::strlen(tg_txt));
            return this->should_save_cheat_toggles;
        }

        void CheatProcessManager::SaveCheatToggles(const ncm::ProgramId program_id) {
            /* Open the file for program_id. */
            fs::FileHandle file;
            {
                char path[fs::EntryNameLengthMax + 1];
                std::snprintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/cheats/toggles.txt", program_id.value);
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
                if (this->cheat_entries[i].definition.num_opcodes != 0) {
                    std::snprintf(buf, sizeof(buf), "[%s]\n", this->cheat_entries[i].definition.readable_name);
                    const size_t name_len = std::strlen(buf);
                    if (R_SUCCEEDED(fs::WriteFile(file, offset, buf, name_len, fs::WriteOption::Flush))) {
                        offset += name_len;
                    }

                    const char *entry = this->cheat_entries[i].enabled ? "true\n" : "false\n";
                    const size_t entry_len = std::strlen(entry);
                    if (R_SUCCEEDED(fs::WriteFile(file, offset, entry, entry_len, fs::WriteOption::Flush))) {
                        offset += entry_len;
                    }
                }
            }
        }


        /* Manager global. */
        TYPED_STORAGE(CheatProcessManager) g_cheat_process_manager;

    }

    void InitializeCheatManager() {
        /* Initialize the debug events manager (spawning its threads). */
        InitializeDebugEventsManager();

        /* Create the cheat process manager (spawning its threads). */
        new (GetPointer(g_cheat_process_manager)) CheatProcessManager;
    }

    bool GetHasActiveCheatProcess() {
        return GetReference(g_cheat_process_manager).GetHasActiveCheatProcess();
    }

    Handle GetCheatProcessEventHandle() {
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

    Result GetCheatProcessMappings(MemoryInfo *mappings, size_t max_count, u64 *out_count, u64 offset) {
        return GetReference(g_cheat_process_manager).GetCheatProcessMappings(mappings, max_count, out_count, offset);
    }

    Result ReadCheatProcessMemory(u64 proc_addr, void *out_data, size_t size) {
        return GetReference(g_cheat_process_manager).ReadCheatProcessMemory(proc_addr, out_data, size);
    }

    Result WriteCheatProcessMemory(u64 proc_addr, const void *data, size_t size) {
        return GetReference(g_cheat_process_manager).WriteCheatProcessMemory(proc_addr, data, size);
    }

    Result QueryCheatProcessMemory(MemoryInfo *mapping, u64 address) {
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

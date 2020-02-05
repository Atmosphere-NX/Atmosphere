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

namespace ams::kern::arm64 {

    void KInterruptManager::Initialize(s32 core_id) {
        this->interrupt_controller.Initialize(core_id);
    }

    void KInterruptManager::Finalize(s32 core_id) {
        this->interrupt_controller.Finalize(core_id);
    }

    Result KInterruptManager::BindHandler(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->BindGlobal(handler, irq, core_id, priority, manual_clear, level);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->BindLocal(handler, irq, priority, manual_clear);
        }
    }

    Result KInterruptManager::UnbindHandler(s32 irq, s32 core_id) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->UnbindGlobal(irq);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->UnbindLocal(irq);
        }
    }

    Result KInterruptManager::ClearInterrupt(s32 irq) {
        R_UNLESS(KInterruptController::IsGlobal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;
        KScopedSpinLock lk(GetLock());
        return this->ClearGlobal(irq);
    }

    Result KInterruptManager::ClearInterrupt(s32 irq, s32 core_id) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->ClearGlobal(irq);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->ClearLocal(irq);
        }
    }

    Result KInterruptManager::BindGlobal(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level) {
        /* Ensure the priority level is valid. */
        R_UNLESS(KInterruptController::PriorityLevel_High <= priority, svc::ResultOutOfRange());
        R_UNLESS(priority <= KInterruptController::PriorityLevel_Low,  svc::ResultOutOfRange());

        /* Ensure we aren't already bound. */
        auto &entry = GetGlobalInterruptEntry(irq);
        R_UNLESS(entry.handler == nullptr, svc::ResultBusy());

        /* Set entry fields. */
        entry.needs_clear = false;
        entry.manually_cleared = manual_clear;
        entry.handler = handler;

        /* Configure the interrupt as level or edge. */
        if (level) {
            this->interrupt_controller.SetLevel(irq);
        } else {
            this->interrupt_controller.SetEdge(irq);
        }

        /* Configure the interrupt. */
        this->interrupt_controller.Clear(irq);
        this->interrupt_controller.SetTarget(irq, core_id);
        this->interrupt_controller.SetPriorityLevel(irq, priority);
        this->interrupt_controller.Enable(irq);

        return ResultSuccess();
    }

    Result KInterruptManager::BindLocal(KInterruptHandler *handler, s32 irq, s32 priority, bool manual_clear) {
        /* Ensure the priority level is valid. */
        R_UNLESS(KInterruptController::PriorityLevel_High <= priority, svc::ResultOutOfRange());
        R_UNLESS(priority <= KInterruptController::PriorityLevel_Low,  svc::ResultOutOfRange());

        /* Ensure we aren't already bound. */
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler == nullptr, svc::ResultBusy());

        /* Set entry fields. */
        entry.needs_clear = false;
        entry.manually_cleared = manual_clear;
        entry.handler = handler;
        entry.priority = static_cast<u8>(priority);

        /* Configure the interrupt. */
        this->interrupt_controller.Clear(irq);
        this->interrupt_controller.SetPriorityLevel(irq, priority);
        this->interrupt_controller.Enable(irq);

        return ResultSuccess();
    }

    Result KInterruptManager::UnbindGlobal(s32 irq) {
        for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
            this->interrupt_controller.ClearTarget(irq, static_cast<s32>(core_id));
        }
        this->interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        this->interrupt_controller.Disable(irq);

        GetGlobalInterruptEntry(irq).handler = nullptr;

        return ResultSuccess();
    }

    Result KInterruptManager::UnbindLocal(s32 irq) {
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        this->interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        this->interrupt_controller.Disable(irq);

        entry.handler = nullptr;

        return ResultSuccess();
    }

    Result KInterruptManager::ClearGlobal(s32 irq) {
        /* We can't clear an entry with no handler. */
        auto &entry = GetGlobalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        /* If auto-cleared, we can succeed immediately. */
        R_UNLESS(entry.manually_cleared,   ResultSuccess());
        R_UNLESS(entry.needs_clear,        ResultSuccess());

        /* Clear and enable. */
        entry.needs_clear = false;
        this->interrupt_controller.Enable(irq);
        return ResultSuccess();
    }

    Result KInterruptManager::ClearLocal(s32 irq) {
        /* We can't clear an entry with no handler. */
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        /* If auto-cleared, we can succeed immediately. */
        R_UNLESS(entry.manually_cleared,   ResultSuccess());
        R_UNLESS(entry.needs_clear,        ResultSuccess());

        /* Clear and set priority. */
        entry.needs_clear = false;
        this->interrupt_controller.SetPriorityLevel(irq, entry.priority);
        return ResultSuccess();
    }

}

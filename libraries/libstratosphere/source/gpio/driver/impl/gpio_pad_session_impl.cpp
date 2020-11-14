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

namespace ams::gpio::driver::impl {

    Result PadSessionImpl::Open(Pad *pad, ddsf::AccessMode access_mode) {
        /* Check if the pad has any open sessions. */
        const bool first_session = !pad->HasAnyOpenSession();

        /* Open the session. */
        R_TRY(ddsf::OpenSession(pad, this, access_mode));
        auto pad_guard = SCOPE_GUARD { ddsf::CloseSession(this); };

        /* If we're the first, we want to initialize the pad. */
        if (first_session) {
            R_TRY(pad->GetDriver().SafeCastTo<IGpioDriver>().InitializePad(pad));
        }

        /* We opened successfully. */
        pad_guard.Cancel();
        return ResultSuccess();
    }

    void PadSessionImpl::Close() {
        /* If the session isn't open, nothing to do. */
        if (!this->IsOpen()) {
            return;
        }

        /* Unbind the interrupt, if it's bound. */
        if (this->IsInterruptBound()) {
            this->UnbindInterrupt();
        }

        /* Get the pad we're a session for. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();

        /* Close the session. */
        ddsf::CloseSession(this);

        /* If we were the last session on the pad, finalize the pad. */
        if (!pad.HasAnyOpenSession()) {
            pad.GetDriver().SafeCastTo<IGpioDriver>().FinalizePad(std::addressof(pad));
        }
    }

    Result PadSessionImpl::BindInterrupt(os::SystemEventType *event) {
        /* Acquire exclusive access to the relevant interrupt control mutex. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();
        auto &mutex = pad.GetDriver().SafeCastTo<IGpioDriver>().GetInterruptControlMutex(pad);
        std::scoped_lock lk(mutex);

        /* Check that we're not already bound. */
        R_UNLESS(!this->IsInterruptBound(), gpio::ResultAlreadyBound());
        R_UNLESS(!this->GetDevice().SafeCastTo<Pad>().IsAnySessionBoundToInterrupt(), gpio::ResultAlreadyBound());

        /* Create the system event. */
        R_TRY(os::CreateSystemEvent(event, os::EventClearMode_ManualClear, true));
        auto ev_guard = SCOPE_GUARD { os::DestroySystemEvent(event); };

        /* Attach the event to our holder. */
        this->event_holder.AttachEvent(event);
        auto hl_guard = SCOPE_GUARD { this->event_holder.DetachEvent(); };

        /* Update interrupt needed. */
        R_TRY(this->UpdateDriverInterruptEnabled());

        /* We succeeded. */
        hl_guard.Cancel();
        ev_guard.Cancel();
        return ResultSuccess();
    }

    void PadSessionImpl::UnbindInterrupt() {
        /* Acquire exclusive access to the relevant interrupt control mutex. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();
        auto &mutex = pad.GetDriver().SafeCastTo<IGpioDriver>().GetInterruptControlMutex(pad);
        std::scoped_lock lk(mutex);

        /* If we're not bound, nothing to do. */
        if (!this->IsInterruptBound()) {
            return;
        }

        /* Detach and destroy the event */
        os::DestroySystemEvent(this->event_holder.DetachEvent());

        /* Update interrupt needed. */
        R_ABORT_UNLESS(this->UpdateDriverInterruptEnabled());
    }

    Result PadSessionImpl::UpdateDriverInterruptEnabled() {
        /* Check we have exclusive access to the relevant interrupt control mutex. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();
        auto &driver = pad.GetDriver().SafeCastTo<IGpioDriver>();
        AMS_ASSERT(driver.GetInterruptControlMutex(pad).IsLockedByCurrentThread());

        /* Set interrupt enabled. */
        return driver.SetInterruptEnabled(std::addressof(pad), pad.IsInterruptRequiredForDriver());
    }

    Result PadSessionImpl::GetInterruptEnabled(bool *out) const {
        *out = this->GetDevice().SafeCastTo<Pad>().IsInterruptEnabled();
        return ResultSuccess();
    }

    Result PadSessionImpl::SetInterruptEnabled(bool en) {
        /* Acquire exclusive access to the relevant interrupt control mutex. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();
        auto &mutex = pad.GetDriver().SafeCastTo<IGpioDriver>().GetInterruptControlMutex(pad);
        std::scoped_lock lk(mutex);

        /* Set the interrupt enable. */
        const bool prev = pad.IsInterruptEnabled();
        pad.SetInterruptEnabled(en);
        auto pad_guard = SCOPE_GUARD { pad.SetInterruptEnabled(prev); };

        /* Update interrupt needed. */
        R_TRY(this->UpdateDriverInterruptEnabled());

        pad_guard.Cancel();
        return ResultSuccess();
    }

    void PadSessionImpl::SignalInterruptBoundEvent() {
        /* Check we have exclusive access to the relevant interrupt control mutex. */
        auto &pad = this->GetDevice().SafeCastTo<Pad>();
        auto &driver = pad.GetDriver().SafeCastTo<IGpioDriver>();
        AMS_ASSERT(driver.GetInterruptControlMutex(pad).IsLockedByCurrentThread());

        if (auto *event = this->event_holder.GetSystemEvent(); event != nullptr) {
            os::SignalSystemEvent(event);
        }
    }

}

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
#include "gpio_suspend_handler.hpp"

namespace ams::gpio::driver::board::nintendo::nx::impl {

    void SuspendHandler::Initialize(uintptr_t gpio_vaddr) {
        /* Set our gpio virtual address. */
        this->gpio_virtual_address = gpio_vaddr;

        /* Ensure that we can use the wec library. */
        ams::wec::Initialize();
    }

    void SuspendHandler::SetValueForSleepState(TegraPad *pad, GpioValue value) {
        /* TODO */
        AMS_ABORT();
    }

    Result SuspendHandler::IsWakeEventActive(bool *out, TegraPad *pad) const {
        /* TODO */
        AMS_ABORT();
    }

    Result SuspendHandler::SetWakeEventActiveFlagSetForDebug(TegraPad *pad, bool en) {
        /* TODO */
        AMS_ABORT();
    }

    void SuspendHandler::SetWakePinDebugMode(WakePinDebugMode mode) {
        /* TODO */
        AMS_ABORT();
    }

    void SuspendHandler::Suspend() {
        /* TODO */
        AMS_ABORT();
    }

    void SuspendHandler::SuspendLow() {
        /* TODO */
        AMS_ABORT();
    }

    void SuspendHandler::Resume() {
        /* TODO */
        AMS_ABORT();
    }

    void SuspendHandler::ResumeLow() {
        /* TODO */
        AMS_ABORT();
    }

}

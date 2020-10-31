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
#include "gpio_driver_core.hpp"

namespace ams::gpio::driver::impl {

    namespace {

        os::ThreadType g_interrupt_thread;

        constexpr inline size_t InterruptThreadStackSize = os::MemoryPageSize;
        alignas(os::MemoryPageSize) u8 g_interrupt_thread_stack[InterruptThreadStackSize];

        gpio::driver::IGpioDriver::List &GetGpioDriverList() {
            static gpio::driver::IGpioDriver::List s_gpio_driver_list;
            return s_gpio_driver_list;
        }

        ddsf::EventHandlerManager &GetInterruptHandlerManager() {
            static ddsf::EventHandlerManager s_interrupt_handler_manager;
            return s_interrupt_handler_manager;
        }

        ddsf::DeviceCodeEntryManager &GetDeviceCodeEntryManager() {
            static ddsf::DeviceCodeEntryManager s_device_code_entry_manager(ddsf::GetDeviceCodeEntryHolderMemoryResource());
            return s_device_code_entry_manager;
        }

        void InterruptThreadFunction(void *arg) {
            AMS_UNUSED(arg);
            GetInterruptHandlerManager().LoopAuto();
        }

    }

    void InitializeDrivers() {
        /* Ensure the event handler manager is initialized. */
        GetInterruptHandlerManager().Initialize();

        /* Initialize all registered drivers. */
        for (auto &driver : GetGpioDriverList()) {
            driver.SafeCastTo<IGpioDriver>().InitializeDriver();
        }

        /* Create the interrupt thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_interrupt_thread), InterruptThreadFunction, nullptr, g_interrupt_thread_stack, InterruptThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(gpio, InterruptHandler)));
        os::SetThreadNamePointer(std::addressof(g_interrupt_thread), AMS_GET_SYSTEM_THREAD_NAME(gpio, InterruptHandler));
        os::StartThread(std::addressof(g_interrupt_thread));

        /* Wait for the interrupt thread to enter the loop. */
        GetInterruptHandlerManager().WaitLoopEnter();
    }

    void FinalizeDrivers() {
        /* Request the interrupt thread stop. */
        GetInterruptHandlerManager().RequestStop();
        os::WaitThread(std::addressof(g_interrupt_thread));
        os::DestroyThread(std::addressof(g_interrupt_thread));

        /* TODO: What else? */
        AMS_ABORT();
    }

    void RegisterDriver(IGpioDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        GetGpioDriverList().push_back(*driver);
    }

    void UnregisterDriver(IGpioDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        if (driver->IsLinkedToList()) {
            auto &list = GetGpioDriverList();
            list.erase(list.iterator_to(*driver));
        }
    }

    Result RegisterDeviceCode(DeviceCode device_code, Pad *pad) {
        AMS_ASSERT(pad != nullptr);
        R_TRY(GetDeviceCodeEntryManager().Add(device_code, pad));
        return ResultSuccess();
    }

    bool UnregisterDeviceCode(DeviceCode device_code) {
        return GetDeviceCodeEntryManager().Remove(device_code);
    }

    void RegisterInterruptHandler(ddsf::IEventHandler *handler) {
        AMS_ASSERT(handler != nullptr);
        GetInterruptHandlerManager().RegisterHandler(handler);
    }

    void UnregisterInterruptHandler(ddsf::IEventHandler *handler) {
        AMS_ASSERT(handler != nullptr);
        GetInterruptHandlerManager().UnregisterHandler(handler);
    }

    Result FindPad(Pad **out, DeviceCode device_code) {
        /* Validate output. */
        AMS_ASSERT(out != nullptr);

        /* Find the device. */
        ddsf::IDevice *device;
        R_TRY(GetDeviceCodeEntryManager().FindDevice(std::addressof(device), device_code));

        /* Set output. */
        *out = device->SafeCastToPointer<Pad>();
        return ResultSuccess();
    }

    Result FindPadByNumber(Pad **out, int pad_number) {
        /* Validate output. */
        AMS_ASSERT(out != nullptr);

        /* Find the pad. */
        bool found = false;
        GetDeviceCodeEntryManager().ForEachEntry([&](ddsf::DeviceCodeEntry &entry) -> bool {
            /* Convert the entry to a pad. */
            auto &pad = entry.GetDevice().SafeCastTo<Pad>();

            /* Check if the pad is the one we're looking for. */
            if (pad.GetPadNumber() == pad_number) {
                found = true;
                *out = std::addressof(pad);
                return false;
            }
            return true;
        });

        /* Check that we found the pad. */
        R_UNLESS(found, ddsf::ResultDeviceCodeNotFound());

        return ResultSuccess();
    }

}

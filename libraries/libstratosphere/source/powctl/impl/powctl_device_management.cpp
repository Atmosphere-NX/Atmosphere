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
#include "powctl_device_management.hpp"

namespace ams::powctl::impl {

    namespace {

        os::ThreadType g_interrupt_thread;

        constexpr inline size_t InterruptThreadStackSize = os::MemoryPageSize;
        alignas(os::MemoryPageSize) u8 g_interrupt_thread_stack[InterruptThreadStackSize];

        constinit u8 g_unit_heap_memory[2_KB];
        constinit lmem::HeapHandle g_unit_heap_handle;
        constinit sf::UnitHeapMemoryResource g_unit_heap_memory_resource;

        IPowerControlDriver::List &GetDriverList() {
            static IPowerControlDriver::List s_driver_list;
            return s_driver_list;
        }

        ddsf::EventHandlerManager &GetInterruptHandlerManager() {
            static ddsf::EventHandlerManager s_interrupt_handler_manager;
            return s_interrupt_handler_manager;
        }

        ddsf::DeviceCodeEntryManager &GetDeviceCodeEntryManager() {
            static ddsf::DeviceCodeEntryManager s_device_code_entry_manager = [] {
                /* Initialize the entry code heap. */
                g_unit_heap_handle = lmem::CreateUnitHeap(g_unit_heap_memory, sizeof(g_unit_heap_memory), sizeof(ddsf::DeviceCodeEntryHolder), lmem::CreateOption_ThreadSafe);

                /* Initialize the entry code memory resource. */
                g_unit_heap_memory_resource.Attach(g_unit_heap_handle);

                /* Make the entry manager using the newly initialized memory resource. */
                return ddsf::DeviceCodeEntryManager(std::addressof(g_unit_heap_memory_resource));
            }();

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
        for (auto &driver : GetDriverList()) {
            driver.SafeCastTo<IPowerControlDriver>().InitializeDriver();
        }

        /* Create the interrupt thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_interrupt_thread), InterruptThreadFunction, nullptr, g_interrupt_thread_stack, InterruptThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(powctl, InterruptHandler)));
        os::SetThreadNamePointer(std::addressof(g_interrupt_thread), AMS_GET_SYSTEM_THREAD_NAME(powctl, InterruptHandler));
        os::StartThread(std::addressof(g_interrupt_thread));

        /* Wait for the interrupt thread to enter the loop. */
        GetInterruptHandlerManager().WaitLoopEnter();
    }

    void FinalizeDrivers() {
        /* Request the interrupt thread stop. */
        GetInterruptHandlerManager().RequestStop();
        os::WaitThread(std::addressof(g_interrupt_thread));
        os::DestroyThread(std::addressof(g_interrupt_thread));

        /* Reset all device code entries. */
        GetDeviceCodeEntryManager().Reset();

        /* Finalize all registered drivers. */
        for (auto &driver : GetDriverList()) {
            driver.SafeCastTo<IPowerControlDriver>().FinalizeDriver();
        }

        /* Finalize the interrupt handler manager. */
        GetInterruptHandlerManager().Finalize();
    }

    void RegisterDriver(IPowerControlDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        GetDriverList().push_back(*driver);
    }

    void UnregisterDriver(IPowerControlDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        if (driver->IsLinkedToList()) {
            auto &list = GetDriverList();
            list.erase(list.iterator_to(*driver));
        }
    }

    Result RegisterDeviceCode(DeviceCode device_code, IDevice *device) {
        AMS_ASSERT(device != nullptr);
        R_TRY(GetDeviceCodeEntryManager().Add(device_code, device));
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

    Result FindDevice(powctl::impl::IDevice **out, DeviceCode device_code) {
        /* Validate output. */
        AMS_ASSERT(out != nullptr);

        /* Find the device. */
        ddsf::IDevice *device;
        R_TRY(GetDeviceCodeEntryManager().FindDevice(std::addressof(device), device_code));

        /* Set output. */
        *out = device->SafeCastToPointer<powctl::impl::IDevice>();
        return ResultSuccess();
    }

}
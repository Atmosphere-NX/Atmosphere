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
#include "impl/gpio_driver_impl.hpp"
#include "impl/gpio_initial_config.hpp"
#include "impl/gpio_tegra_pad.hpp"

namespace ams::gpio::driver::board::nintendo::nx {

    namespace {

        ams::gpio::driver::board::nintendo::nx::impl::DriverImpl *g_driver_impl = nullptr;

    }

    void Initialize(bool enable_interrupt_handlers) {
        /* Check that we haven't previously initialized. */
        AMS_ABORT_UNLESS(g_driver_impl == nullptr);

        /* Get the device driver subsystem framework memory resource. */
        auto *memory_resource = ddsf::GetMemoryResource();

        /* Allocate a new driver. */
        auto *driver_storage = static_cast<decltype(g_driver_impl)>(memory_resource->Allocate(sizeof(*g_driver_impl)));
        AMS_ABORT_UNLESS(driver_storage != nullptr);

        /* Construct the new driver. */
        g_driver_impl = new (driver_storage) ams::gpio::driver::board::nintendo::nx::impl::DriverImpl(impl::GpioRegistersPhysicalAddress, impl::GpioRegistersSize);

        /* Register the driver. */
        gpio::driver::RegisterDriver(g_driver_impl);

        /* Register interrupt handlers, if we should. */
        if (enable_interrupt_handlers) {
            for (size_t i = 0; i < util::size(impl::InterruptNameTable); ++i) {
                /* Allocate a handler. */
                impl::InterruptEventHandler *handler_storage = static_cast<impl::InterruptEventHandler *>(memory_resource->Allocate(sizeof(impl::InterruptEventHandler)));
                AMS_ABORT_UNLESS(handler_storage != nullptr);

                /* Initialize the handler. */
                impl::InterruptEventHandler *handler = new (handler_storage) impl::InterruptEventHandler;
                handler->Initialize(g_driver_impl, impl::InterruptNameTable[i], static_cast<int>(i));

                /* Register the handler. */
                gpio::driver::RegisterInterruptHandler(handler);
            }
        }

        /* Create and register all pads. */
        for (const auto &entry : impl::PadMapCombinationList) {
            /* Allocate a pad for our device. */
            impl::TegraPad *pad_storage = static_cast<impl::TegraPad *>(memory_resource->Allocate(sizeof(impl::TegraPad)));
            AMS_ABORT_UNLESS(pad_storage != nullptr);

            /* Create a pad for our device. */
            impl::TegraPad *pad = new (pad_storage) impl::TegraPad;
            pad->SetParameters(entry.internal_number, impl::PadInfo{entry.wake_event});

            /* Register the pad with our driver. */
            g_driver_impl->RegisterDevice(pad);

            /* Register the device code with our driver. */
            R_ABORT_UNLESS(gpio::driver::RegisterDeviceCode(entry.device_code, pad));
        }
    }

    void SetInitialGpioConfig() {
        return impl::SetInitialGpioConfig();
    }

    void SetInitialWakePinConfig() {
        return impl::SetInitialWakePinConfig();
    }

}

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
#include "gpio_driver_impl.hpp"
#include "gpio_initial_config.hpp"
#include "gpio_wake_pin_config.hpp"

namespace ams::gpio::driver::board::nintendo::nx::impl {

    namespace {

        spl::HardwareType GetHardwareType() {
            /* Acquire access to spl: */
            sm::ScopedServiceHolder<spl::Initialize, spl::Finalize> spl_holder;
            AMS_ABORT_UNLESS(static_cast<bool>(spl_holder));

            /* Get config. */
            return ::ams::spl::GetHardwareType();
        }

        #include "gpio_initial_wake_pin_config_icosa.inc"
     /* #include "gpio_initial_wake_pin_config_copper.inc" */
        #include "gpio_initial_wake_pin_config_hoag.inc"
        #include "gpio_initial_wake_pin_config_iowa.inc"
        #include "gpio_initial_wake_pin_config_calcio.inc"
        #include "gpio_initial_wake_pin_config_aula.inc"

        #include "gpio_initial_config_icosa.inc"
     /* #include "gpio_initial_config_copper.inc" */
        #include "gpio_initial_config_hoag.inc"
        #include "gpio_initial_config_iowa.inc"
        #include "gpio_initial_config_calcio.inc"
        #include "gpio_initial_config_aula.inc"

    }

    void SetInitialGpioConfig() {
        /* Set wake event levels, wake event enables. */
        const GpioInitialConfig *configs = nullptr;
        size_t num_configs = 0;

        /* Select the correct config. */
        switch (GetHardwareType()) {
            case spl::HardwareType::Icosa:
                configs     =    InitialGpioConfigsIcosa;
                num_configs = NumInitialGpioConfigsIcosa;
                break;
            case spl::HardwareType::Hoag:
                configs     =    InitialGpioConfigsHoag;
                num_configs = NumInitialGpioConfigsHoag;
                break;
            case spl::HardwareType::Iowa:
                configs     =    InitialGpioConfigsIowa;
                num_configs = NumInitialGpioConfigsIowa;
                break;
            case spl::HardwareType::Calcio:
                configs     =    InitialGpioConfigsCalcio;
                num_configs = NumInitialGpioConfigsCalcio;
                break;
            case spl::HardwareType::Aula:
                configs     =    InitialGpioConfigsAula;
                num_configs = NumInitialGpioConfigsAula;
                break;
            case spl::HardwareType::Copper:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Check we can use our config. */
        AMS_ABORT_UNLESS(configs != nullptr);

        /* Apply the configs. */
        {
            /* Create a driver to use for the duration of our application. */
            DriverImpl driver(GpioRegistersPhysicalAddress, GpioRegistersSize);
            driver.InitializeDriver();

            for (size_t i = 0; i < num_configs; ++i) {
                /* Find the internal pad number for our device. */
                bool found = false;
                for (const auto &entry : PadMapCombinationList) {
                    if (entry.device_code == configs[i].device_code) {
                        /* We found an entry. */
                        found = true;

                        /* Create a pad for our device. */
                        TegraPad pad;
                        pad.SetParameters(entry.internal_number, PadInfo{entry.wake_event});

                        /* Initialize the pad. */
                        R_ABORT_UNLESS(driver.InitializePad(std::addressof(pad)));

                        /* Set the direction. */
                        R_ABORT_UNLESS(driver.SetDirection(std::addressof(pad), configs[i].direction));

                        /* If the direction is output, set the value. */
                        if (configs[i].direction == Direction_Output) {
                            R_ABORT_UNLESS(driver.SetValue(std::addressof(pad), configs[i].value));
                        }

                        /* Finalize the pad we made. */
                        driver.FinalizePad(std::addressof(pad));
                        break;
                    }
                }

                /* Ensure that we applied the config for the pad we wanted. */
                AMS_ABORT_UNLESS(found);
            }

            /* Finalize the driver. */
            driver.FinalizeDriver();
        }
    }

    void SetInitialWakePinConfig() {
        /* Ensure the wec driver is initialized. */
        ams::wec::Initialize();

        /* Set wake event levels, wake event enables. */
        const WakePinConfig *configs = nullptr;
        size_t num_configs = 0;

        /* Select the correct config. */
        switch (GetHardwareType()) {
            case spl::HardwareType::Icosa:
                configs     =    InitialWakePinConfigsIcosa;
                num_configs = NumInitialWakePinConfigsIcosa;
                break;
            case spl::HardwareType::Hoag:
                configs     =    InitialWakePinConfigsHoag;
                num_configs = NumInitialWakePinConfigsHoag;
                break;
            case spl::HardwareType::Iowa:
                configs     =    InitialWakePinConfigsIowa;
                num_configs = NumInitialWakePinConfigsIowa;
                break;
            case spl::HardwareType::Calcio:
                configs     =    InitialWakePinConfigsCalcio;
                num_configs = NumInitialWakePinConfigsCalcio;
                break;
            case spl::HardwareType::Aula:
                configs     =    InitialWakePinConfigsAula;
                num_configs = NumInitialWakePinConfigsAula;
                break;
            case spl::HardwareType::Copper:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Check we can use our config. */
        AMS_ABORT_UNLESS(configs != nullptr);

        /* Apply the config. */
        for (size_t i = 0; i < num_configs; ++i) {
            wec::SetWakeEventLevel(configs[i].wake_event, configs[i].level);
            wec::SetWakeEventEnabled(configs[i].wake_event, configs[i].enable);
        }
    }

}

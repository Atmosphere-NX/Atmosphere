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
#include "fatal_task_sound.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Task definition. */
        class StopSoundTask : public ITaskWithDefaultStack {
            private:
                void StopSound();
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "SoundTask";
                }
        };

        /* Task global. */
        StopSoundTask g_stop_sound_task;

        /* Task implementation. */
        void StopSoundTask::StopSound() {
            /* Talk to the ALC5639 over I2C, and disable audio output. */
            {
                I2cSession audio;
                if (R_SUCCEEDED(i2cOpenSession(&audio, I2cDevice_Alc5639))) {
                    ON_SCOPE_EXIT { i2csessionClose(&audio); };

                    struct {
                        u8 reg;
                        u16 val;
                    } __attribute__((packed)) cmd;
                    static_assert(sizeof(cmd) == 3, "I2C command definition!");

                    cmd.reg = 0x01;
                    cmd.val = 0xC8C8;
                    i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);

                    cmd.reg = 0x02;
                    cmd.val = 0xC8C8;
                    i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);

                    for (u8 reg = 97; reg <= 102; reg++) {
                        cmd.reg = reg;
                        cmd.val = 0;
                        i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);
                    }
                }
            }

            /* Talk to the ALC5639 over GPIO, and disable audio output */
            {
                gpio::GpioPadSession audio;
                if (R_SUCCEEDED(gpio::OpenSession(std::addressof(audio), gpio::DeviceCode_CodecLdoEnTemp))) {
                    ON_SCOPE_EXIT { gpio::CloseSession(std::addressof(audio)); };

                    /* Set direction output, sleep 200 ms so it can take effect. */
                    gpio::SetDirection(std::addressof(audio), gpio::Direction_Output);
                    os::SleepThread(TimeSpan::FromMilliSeconds(200));

                    /* Pull audio codec low. */
                    gpio::SetValue(std::addressof(audio), gpio::GpioValue_Low);
                }
            }
        }

        Result StopSoundTask::Run() {
            StopSound();
            return ResultSuccess();
        }

    }

    ITask *GetStopSoundTask(const ThrowContext *ctx) {
        g_stop_sound_task.Initialize(ctx);
        return &g_stop_sound_task;
    }

}

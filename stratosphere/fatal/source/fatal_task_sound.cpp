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
                        u16 dev;
                        u8 val;
                    } __attribute__((packed)) cmd;
                    static_assert(sizeof(cmd) == 3, "I2C command definition!");

                    cmd.dev = 0xC801;
                    cmd.val = 200;
                    i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);

                    cmd.dev = 0xC802;
                    cmd.val = 200;
                    i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);

                    cmd.dev = 0xC802;
                    cmd.val = 200;
                    i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);

                    for (u16 dev = 97; dev <= 102; dev++) {
                        cmd.dev = dev;
                        cmd.val = 0;
                        i2csessionSendAuto(&audio, &cmd, sizeof(cmd), I2cTransactionOption_All);
                    }
                }
            }

            /* Talk to the ALC5639 over GPIO, and disable audio output */
            {
                GpioPadSession audio;
                if (R_SUCCEEDED(gpioOpenSession(&audio, GpioPadName_AudioCodec))) {
                    ON_SCOPE_EXIT { gpioPadClose(&audio); };

                    /* Set direction output, sleep 200 ms so it can take effect. */
                    gpioPadSetDirection(&audio, GpioDirection_Output);
                    svcSleepThread(200000000UL);

                    /* Pull audio codec low. */
                    gpioPadSetValue(&audio, GpioValue_Low);
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

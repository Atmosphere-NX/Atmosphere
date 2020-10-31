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

namespace ams::clkrst {

    namespace {

        constexpr inline dd::PhysicalAddress ClkRstRegistersPhysicalAddress = 0x60006000;
        constexpr inline size_t ClkRstRegistersSize = 0x1000;

        uintptr_t g_clkrst_registers = dd::QueryIoMapping(ClkRstRegistersPhysicalAddress, ClkRstRegistersSize);

        struct ClkRstDefinition {
            u32 clk_src_ofs;
            u32 clk_en_ofs;
            u32 rst_ofs;
            u32 clk_en_index;
            u32 rst_index;
            u8 clk_src;
            u8 clk_divisor;
        };

        constexpr inline const ClkRstDefinition Definitions[] = {
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C1, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_ENB_I2C1_INDEX, CLK_RST_CONTROLLER_RST_I2C1_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C2, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_ENB_I2C2_INDEX, CLK_RST_CONTROLLER_RST_I2C2_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C3, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_ENB_I2C3_INDEX, CLK_RST_CONTROLLER_RST_I2C3_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C4, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_ENB_I2C4_INDEX, CLK_RST_CONTROLLER_RST_I2C4_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_ENB_I2C5_INDEX, CLK_RST_CONTROLLER_RST_I2C5_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_I2C6, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_ENB_I2C6_INDEX, CLK_RST_CONTROLLER_RST_I2C6_INDEX, 0x00 /* PLLP_OUT0 */, 0x04 },
            { CLK_RST_CONTROLLER_CLK_SOURCE_PWM,  CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_ENB_PWM_INDEX,  CLK_RST_CONTROLLER_RST_PWM_INDEX,  0x00 /* PLLP_OUT0 */, 0x10 },
        };

        constexpr inline const struct {
            const ClkRstDefinition *definition;
            DeviceCode device_code;
        } ClkRstDefinitionMap[] = {
            { std::addressof(Definitions[0]), i2c::DeviceCode_I2c1         },
            { std::addressof(Definitions[1]), i2c::DeviceCode_I2c2         },
            { std::addressof(Definitions[2]), i2c::DeviceCode_I2c3         },
            { std::addressof(Definitions[3]), i2c::DeviceCode_I2c4         },
            { std::addressof(Definitions[4]), i2c::DeviceCode_I2c5         },
            { std::addressof(Definitions[5]), i2c::DeviceCode_I2c6         },
            { std::addressof(Definitions[6]), pwm::DeviceCode_LcdBacklight },
        };

        ALWAYS_INLINE const ClkRstDefinition *GetDefinition(DeviceCode device_code) {
            const ClkRstDefinition *def = nullptr;

            for (const auto &entry : ClkRstDefinitionMap) {
                if (entry.device_code == device_code) {
                    def = entry.definition;
                    break;
                }
            }

            AMS_ABORT_UNLESS(def != nullptr);
            return def;
        }

        ALWAYS_INLINE const ClkRstDefinition &GetDefinition(ClkRstSession *session) {
            const ClkRstDefinition *def = nullptr;

            for (const auto &entry : ClkRstDefinitionMap) {
                if (session->_session == entry.definition) {
                    def = entry.definition;
                    break;
                }
            }

            AMS_ABORT_UNLESS(def != nullptr);
            return *def;
        }

    }

    void Initialize() {
        /* ... */
    }

    void Finalize() {
        /* ... */
    }

    Result OpenSession(ClkRstSession *out, DeviceCode device_code) {
        /* Get the relevant definition. */
        out->_session = const_cast<ClkRstDefinition *>(GetDefinition(device_code));
        return ResultSuccess();
    }

    void CloseSession(ClkRstSession *session) {
        /* Clear the session. */
        session->_session = nullptr;
    }

    void SetResetAsserted(ClkRstSession *session) {
        /* Get the definition. */
        const auto &def = GetDefinition(session);

        /* Assert reset. */
        reg::ReadWrite(g_clkrst_registers + def.rst_ofs, 1u << def.rst_index, 1u << def.rst_index);
    }

    void SetResetDeasserted(ClkRstSession *session) {
        /* Get the definition. */
        const auto &def = GetDefinition(session);

        /* Clear reset. */
        reg::ReadWrite(g_clkrst_registers + def.rst_ofs, 0, 1u << def.rst_index);
    }

    void SetClockRate(ClkRstSession *session, u32 hz) {
        /* Get the definition. */
        const auto &def = GetDefinition(session);

        /* Enable clock. */
        reg::ReadWrite(g_clkrst_registers + def.clk_en_ofs, 1u << def.clk_en_index, 1u << def.clk_en_index);

        /* Set the clock divisor. */
        reg::ReadWrite(g_clkrst_registers + def.clk_src_ofs, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_CLK_DIVISOR, def.clk_divisor));

        /* Wait for 2us for clock setting to take. */
        os::SleepThread(TimeSpan::FromMicroSeconds(2));

        /* Set the clock source. */
        reg::ReadWrite(g_clkrst_registers + def.clk_src_ofs, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_CLK_SOURCE, def.clk_src));

        /* Wait for 2us for clock setting to take. */
        os::SleepThread(TimeSpan::FromMicroSeconds(2));
    }

    void SetClockDisabled(ClkRstSession *session) {
        AMS_ABORT("SetClockDisabled not implemented for boot system module");
    }

}

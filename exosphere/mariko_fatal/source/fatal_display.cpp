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
#include <exosphere.hpp>
#include "fatal_device_page_table.hpp"
#include "fatal_registers_di.hpp"
#include "fatal_display.hpp"
#include "fatal_print.hpp"

namespace ams::secmon::fatal {

    namespace {

        #include "fatal_display_config.inc"

    }

    namespace {

        /* Helpful defines. */
        constexpr int DsiWaitForCommandMilliSecondsMax        = 250;
        constexpr int DsiWaitForCommandCompletionMilliSeconds = 5;
        constexpr int DsiWaitForHostControlMilliSecondsMax    = 150;

        constexpr inline int I2cAddressMax77620Pmic     = 0x3C;

        constexpr size_t GPIO_PORT3_CNF_0 = 0x200;
        constexpr size_t GPIO_PORT3_OE_0  = 0x210;
        constexpr size_t GPIO_PORT3_OUT_0 = 0x220;

        constexpr size_t GPIO_PORT6_CNF_1 = 0x504;
        constexpr size_t GPIO_PORT6_OE_1  = 0x514;
        constexpr size_t GPIO_PORT6_OUT_1 = 0x524;

        /* Globals. */
        constexpr inline const uintptr_t PMC             = secmon::MemoryRegionVirtualDevicePmc    .GetAddress();
        constexpr inline const uintptr_t g_disp1_regs    = secmon::MemoryRegionVirtualDeviceDisp1  .GetAddress();
        constexpr inline const uintptr_t g_dsi_regs      = secmon::MemoryRegionVirtualDeviceDsi    .GetAddress();
        constexpr inline const uintptr_t g_clk_rst_regs  = secmon::MemoryRegionVirtualDeviceClkRst .GetAddress();
        constexpr inline const uintptr_t g_gpio_regs     = secmon::MemoryRegionVirtualDeviceGpio   .GetAddress();
        constexpr inline const uintptr_t g_apb_misc_regs = secmon::MemoryRegionVirtualDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t g_mipi_cal_regs = secmon::MemoryRegionVirtualDeviceMipiCal.GetAddress();

        constinit u32 *g_frame_buffer = nullptr;

        inline void DoRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes, size_t num_writes) {
            for (size_t i = 0; i < num_writes; i++) {
                reg::Write(base_address + reg_writes[i].offset, reg_writes[i].value);
            }
        }

        inline void DoSocDependentRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes_erista, size_t num_writes_erista, const RegisterWrite *reg_writes_mariko, size_t num_writes_mariko) {
            switch (GetSocType()) {
                case fuse::SocType_Erista: DoRegisterWrites(base_address, reg_writes_erista, num_writes_erista); break;
                case fuse::SocType_Mariko: DoRegisterWrites(base_address, reg_writes_mariko, num_writes_mariko); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        inline void DoSleepOrRegisterWrites(uintptr_t base_address, const SleepOrRegisterWrite *reg_writes, size_t num_writes) {
            for (size_t i = 0; i < num_writes; i++) {
                switch (reg_writes[i].kind) {
                    case SleepOrRegisterWriteKind_Write:
                        reg::Write(base_address + sizeof(u32) * reg_writes[i].offset, reg_writes[i].value);
                        break;
                    case SleepOrRegisterWriteKind_Sleep:
                        util::WaitMicroSeconds(reg_writes[i].offset * UINT64_C(1000));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
        }

        void WaitDsiTrigger() {
            const u32 timeout = util::GetMicroSeconds() + (DsiWaitForCommandMilliSecondsMax * 1000u);

            while (true) {
                if (util::GetMicroSeconds() >= timeout) {
                    break;
                }
                if (reg::Read(g_dsi_regs + sizeof(u32) * DSI_TRIGGER) == 0) {
                    break;
                }
            }

            util::WaitMicroSeconds(DsiWaitForCommandCompletionMilliSeconds * 1000u);
        }

        void WaitDsiHostControl() {
            const u32 timeout = util::GetMicroSeconds() + (DsiWaitForHostControlMilliSecondsMax * 1000u);

            while (true) {
                if (util::GetMicroSeconds() >= timeout) {
                    break;
                }
                if ((reg::Read(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL) & DSI_HOST_CONTROL_IMM_BTA) == 0) {
                    break;
                }
            }
        }

        void EnableBacklightForVendor2050ForHardwareTypeFive(int brightness) {
            /* Enable FRAME_END_INT */
            reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_INT_ENABLE, 2);

            /* Configure DSI_LINE_TYPE as FOUR */
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 1);
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 9);

            /* Set and wait for FRAME_END_INT */
            reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_INT_STATUS, 2);
            while ((reg::Read(g_disp1_regs + sizeof(u32) * DC_CMD_INT_STATUS) & 2) != 0) { /* ... */ }

            /* Configure display brightness. */
            const u32 brightness_val = ((0x7FF * brightness) / 100);
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x339);
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, (brightness_val & 0x700) | ((brightness_val & 0xFF) << 16) | 0x51);

            /* Set and wait for FRAME_END_INT */
            reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_INT_STATUS, 2);
            while ((reg::Read(g_disp1_regs + sizeof(u32) * DC_CMD_INT_STATUS) & 2) != 0) { /* ... */ }

            /* Set client sync point block reset. */
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_INCR_SYNCPT_CNTRL, 1);
            util::WaitMicroSeconds(300'000ul);

            /* Clear client sync point block resest. */
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_INCR_SYNCPT_CNTRL, 0);
            util::WaitMicroSeconds(300'000ul);

            /* Clear DSI_LINE_TYPE config. */
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 0);

            /* Disable FRAME_END_INT */
            reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_INT_ENABLE, 0);
            reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_INT_STATUS, 2);
        }

        void EnableBacklightForGeneric(int brightness) {
            AMS_UNUSED(brightness);

            reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);
        }

        #define DO_REGISTER_WRITES(base_address, writes) DoRegisterWrites(base_address, writes, util::size(writes))
        #define DO_SOC_DEPENDENT_REGISTER_WRITES(base_address, writes) DoSocDependentRegisterWrites(base_address, writes##Erista, util::size(writes##Erista), writes##Mariko, util::size(writes##Mariko))
        #define DO_SLEEP_OR_REGISTER_WRITES(base_address, writes) DoSleepOrRegisterWrites(base_address, writes, util::size(writes))

        void InitializeFrameBuffer() {
            if (g_frame_buffer != nullptr) {
                std::memset(g_frame_buffer, 0, FrameBufferSize);
                hw::FlushDataCache(g_frame_buffer, FrameBufferSize);
            } else {
                /* Clear the frame buffer. */
                g_frame_buffer = secmon::MemoryRegionDramDcFramebuffer.GetPointer<u32>();
                std::memset(g_frame_buffer, 0, FrameBufferSize);
                hw::FlushDataCache(g_frame_buffer, FrameBufferSize);

                /* Attach the frame buffer to DC. */
                InitializeDevicePageTableForDc();
            }
        }

        [[maybe_unused]] void FinalizeFrameBuffer() {
            /* We don't actually support finalizing the framebuffer, so do nothing here. */
        }

        constexpr const char *GetErrorDescription(u32 error_desc) {
            switch (error_desc) {
                case 0x100:
                    return "Instruction Abort";
                case 0x101:
                    return "Data Abort";
                case 0x102:
                    return "PC Misalignment";
                case 0x103:
                    return "SP Misalignment";
                case 0x104:
                    return "Trap";
                case 0x106:
                    return "SError";
                case 0x301:
                    return "Bad SVC";
                case 0xF00:
                    return "Kernel Panic";
                case 0xFFD:
                    return "Stack overflow";
                case 0xFFE:
                    return "std::abort() called";
                default:
                    return "Unknown";
            }
        }

        void PrintSuggestedErrorFix(const ams::impl::FatalErrorContext *f_ctx) {
            /* Try to recognize certain errors automatically, and suggest fixes for them. */
            const char *suggestion = nullptr;

            constexpr u64 ProgramIdAmsMitm = UINT64_C(0x010041544D530000);
            constexpr u64 ProgramIdBoot    = UINT64_C(0x0100000000000005);
            if (f_ctx->error_desc == 0xFFE) {
                if (f_ctx->program_id == ProgramIdAmsMitm) {
                    /* When a user has archive bits set improperly, attempting to create an automatic backup will fail */
                    /* to create the file path with error 0x202 */
                    if (f_ctx->gprs[0] == fs::ResultPathNotFound().GetValue()) {
                        /* When the archive bit error is occurring, it manifests as failure to create automatic backup. */
                        /* Thus, we can search the stack for the automatic backups path. */
                        const char * const automatic_backups_prefix = "automatic_backups/X" /* ..... */;
                        const int prefix_len = std::strlen(automatic_backups_prefix);

                        for (size_t i = 0; i + prefix_len < f_ctx->stack_dump_size; ++i) {
                            if (std::memcmp(&f_ctx->stack_dump[i], automatic_backups_prefix, prefix_len) == 0) {
                                suggestion = "The atmosphere directory may improperly have archive bits set.\n"
                                             "Please try running an archive bit fixer tool (for example, the one in Hekate).\n";
                                break;
                            }
                        }
                    } else if (f_ctx->gprs[0] == fs::ResultExFatUnavailable().GetValue()) {
                        /* When a user installs non-exFAT firm but has an exFAT formatted SD card, this error will */
                        /* be returned on attempt to access the SD card. */
                        suggestion = "Your console has non-exFAT firmware installed, but your SD card\n"
                                     "is formatted as exFAT. Format your SD card as FAT32, or manually\n"
                                     "flash exFAT firmware to package2.\n";
                    }
                } else if (f_ctx->program_id == ProgramIdBoot) {
                    /* 9.x -> 10.x updated the API for SvcQueryIoMapping. */
                    /* This can cause the kernel to reject incorrect-ABI calls by boot when a partial update is applied */
                    /* (older kernel in package2, for some reason). */
                    for (size_t i = 0; i < 8; ++i) {
                        if (f_ctx->gprs[i] == svc::ResultNotFound().GetValue()) {
                            suggestion = "A partial update may have been improperly performed.\n"
                                         "To fix, try manually flashing latest package2 to MMC.\n"
                                         "\n"
                                         "For help doing this, seek support in the ReSwitched or\n"
                                         "Nintendo Homebrew discord servers.\n";
                            break;
                        }
                    }
                }
            } else if (f_ctx->error_desc == 0xF00) { /* Kernel Panic */
                suggestion = "Please contact SciresM#0524 on Discord, or create an issue on the Atmosphere\n"
                             "GitHub issue tracker. Thank you very much for helping to test mesosphere.\n";
            }

            /* If we found a suggestion, print it. */
            if (suggestion != nullptr) {
                Print("%s", suggestion);
            }
        }

        void FinalizeDisplay() {
            /* TODO: What other configuration is needed, if any? */

            /* Configure LCD pinmux tristate + passthrough. */
            reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_EN,     reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
            reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_INT,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
            reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_PWM, reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
            reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_EN,  reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
            reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_RST,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));

            if (fuse::GetHardwareType() == fuse::HardwareType_Five) {
                /* Configure LCD backlight. */
                reg::SetBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x4);
                reg::SetBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x4);
            } else {
                /* Configure LCD power, VDD. */
                reg::SetBits(g_gpio_regs + GPIO_PORT3_CNF_0, 0x3);
                reg::SetBits(g_gpio_regs + GPIO_PORT3_OE_0,  0x3);
                reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
                util::WaitMicroSeconds(10'000ul);

                reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
                util::WaitMicroSeconds(10'000ul);

                /* Configure LCD backlight. */
                reg::SetBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x7);
                reg::SetBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x7);
                reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x2);
            }

            /* Disable the LCD backlight. */
            if (GetLcdVendor() == 0x2050) {
                /* TODO: We're not sure display is alive. How to manage this? */
                /* This is probably incorrect backlight disable for hw-type 5. */
                reg::ClearBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);
            } else {
                reg::ClearBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);
            }

            /* Disable backlight RST/Voltage. */
            reg::ClearBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x4);
            if (GetLcdVendor() == 0x2050) {
                util::WaitMicroSeconds(30'000ul);
            } else {
                util::WaitMicroSeconds(10'000ul);
                reg::ClearBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
                util::WaitMicroSeconds(10'000ul);
                reg::ClearBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
                util::WaitMicroSeconds(10'000ul);
            }

            /* Cut clock to DSI. */
            reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_H_SET_SET_MIPI_CAL_RST, ENABLE),
                                                                          CLK_RST_REG_BITS_ENUM(RST_DEV_H_SET_SET_DSI_RST,      ENABLE));

            reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLR_CLR_CLK_ENB_MIPI_CAL, ENABLE),
                                                                          CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLR_CLR_CLK_ENB_DSI,      ENABLE));

            reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SET_SET_HOST1X_RST, ENABLE),
                                                                          CLK_RST_REG_BITS_ENUM(RST_DEV_L_SET_SET_DISP1_RST,  ENABLE));

            reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLR_CLR_CLK_ENB_HOST1X, ENABLE),
                                                                          CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLR_CLR_CLK_ENB_DISP1,  ENABLE));

            reg::Write(g_dsi_regs + sizeof(u32) * DSI_PAD_CONTROL_0, (DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF)));
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_POWER_CONTROL, 0);
        }

    }

    void InitializeDisplay() {
        /* Ensure that the display is finalized. */
        FinalizeDisplay();

        /* Setup the framebuffer. */
        InitializeFrameBuffer();

        /* Get the hardware type. */
        const auto hw_type = fuse::GetHardwareType();

        /* Turn on DSI/voltage rail. */
        {
            if (GetSocType() == fuse::SocType_Mariko) {
                i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, 0x18, 0x3A);
                i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, 0x18, 0x3A);
            }
            i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, 0x23, 0xD0);
        }

        /* Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks. */
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_H_CLR_CLR_MIPI_CAL_RST, ENABLE),
                                                                      CLK_RST_REG_BITS_ENUM(RST_DEV_H_CLR_CLR_DSI_RST,      ENABLE));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_SET_SET_CLK_ENB_MIPI_CAL, ENABLE),
                                                                      CLK_RST_REG_BITS_ENUM(CLK_ENB_H_SET_SET_CLK_ENB_DSI,      ENABLE));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_CLR_CLR_HOST1X_RST, ENABLE),
                                                                      CLK_RST_REG_BITS_ENUM(RST_DEV_L_CLR_CLR_DISP1_RST,  ENABLE));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_SET_SET_CLK_ENB_HOST1X, ENABLE),
                                                                      CLK_RST_REG_BITS_ENUM(CLK_ENB_L_SET_SET_CLK_ENB_DISP1,  ENABLE));


        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_X_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_SET_SET_CLK_ENB_UART_FST_MIPI_CAL, ENABLE));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_UART_FST_MIPI_CAL_UART_FST_MIPI_CAL_CLK_DIVISOR,        10),
                                                                                     CLK_RST_REG_BITS_ENUM (CLK_SOURCE_UART_FST_MIPI_CAL_UART_FST_MIPI_CAL_CLK_SRC,     PLLP_OUT3));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_W_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_SET_SET_CLK_ENB_DSIA_LP, ENABLE));

        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP, CLK_RST_REG_BITS_VALUE(CLK_SOURCE_DSIA_LP_DSIA_LP_CLK_DIVISOR,        10),
                                                                           CLK_RST_REG_BITS_ENUM (CLK_SOURCE_DSIA_LP_DSIA_LP_CLK_SRC,     PLLP_OUT0));

        /* Set IO_DPD_REQ to DPD_OFF. */
        reg::ReadWrite(PMC + APBDEV_PMC_IO_DPD_REQ,  PMC_REG_BITS_ENUM(IO_DPD_REQ_CODE,  DPD_OFF));
        reg::ReadWrite(PMC + APBDEV_PMC_IO_DPD2_REQ, PMC_REG_BITS_ENUM(IO_DPD2_REQ_CODE, DPD_OFF));

        /* Configure LCD pinmux tristate + passthrough. */
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_EN,     reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_INT,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_PWM, reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_EN,  reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_RST,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));

        if (hw_type == fuse::HardwareType_Five) {
            /* Configure LCD backlight. */
            reg::SetBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x4);
            reg::SetBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x4);
        } else {
            /* Configure LCD power, VDD. */
            reg::SetBits(g_gpio_regs + GPIO_PORT3_CNF_0, 0x3);
            reg::SetBits(g_gpio_regs + GPIO_PORT3_OE_0,  0x3);
            reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
            util::WaitMicroSeconds(10'000ul);

            reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
            util::WaitMicroSeconds(10'000ul);

            /* Configure LCD backlight. */
            reg::SetBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x7);
            reg::SetBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x7);
            reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x2);
        }

        /* Configure display interface and display. */
        reg::Write(g_mipi_cal_regs + MIPI_CAL_MIPI_BIAS_PAD_CFG2, 0);
        if (GetSocType() == fuse::SocType_Mariko) {
            reg::Write(g_mipi_cal_regs + MIPI_CAL_MIPI_BIAS_PAD_CFG0, 0);
            reg::Write(g_apb_misc_regs + APB_MISC_GP_DSI_PAD_CONTROL, 0);
        }

        /* Execute configs. */
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld01);
        DO_SLEEP_OR_REGISTER_WRITES(g_disp1_regs, DisplayConfigDc01);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init01);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init02);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init03);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init04);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init05);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init06);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init07);
        util::WaitMicroSeconds(10'000ul);

        /* Enable backlight reset. */
        reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x4);
        util::WaitMicroSeconds(60'000ul);

        if (hw_type == fuse::HardwareType_Five) {
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_BTA_TIMING, 0x40103);
        } else {
            reg::Write(g_dsi_regs + sizeof(u32) * DSI_BTA_TIMING, 0x50204);
        }

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x337);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
        WaitDsiTrigger();

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x406);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
        WaitDsiTrigger();

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL, DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC);
        WaitDsiHostControl();
        util::WaitMicroSeconds(5'000ul);

        /* Parse LCD vendor. */
        {
            u32 host_response[3];
            for (size_t i = 0; i < util::size(host_response); i++) {
                host_response[i] = reg::Read(g_dsi_regs + sizeof(u32) * DSI_RD_DATA);
            }

            /* The last word from host response is:
                Bits 0-7: FAB
                Bits 8-15: REV
                Bits 16-23: Minor REV
            */
            u32 lcd_vendor;
            if ((host_response[2] & 0xFF) == 0x10) {
                lcd_vendor = 0;
            } else {
                lcd_vendor = (host_response[2] >> 8) & 0xFF00;
            }
            lcd_vendor = (lcd_vendor & 0xFFFFFF00) | (host_response[2] & 0xFF);

            AMS_ASSERT(lcd_vendor == GetLcdVendor());
        }

        /* LCD vendor specific configuration. */
        switch (GetLcdVendor()) {
            case 0x10: /* Japan Display Inc screens. */
                DO_SLEEP_OR_REGISTER_WRITES(g_dsi_regs, DisplayConfigJdiSpecificInit01);
                break;
            case 0xF20: /* Innolux first revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(180'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(5'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x751548B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(5'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
            case 0xF30: /* AUO first revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(180'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(5'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x711148B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(5'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
            case 0x2050: /* Unknown (hardware type 5) screen. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(180'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0xA015);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x205315);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x339);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x51);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(5'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
            case 0x1020: /* Innolux second revision screen. */
            case 0x1030: /* AUO second revision screen. */
            case 0x1040: /* Unknown second revision screen. */
            default:
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                util::WaitMicroSeconds(120'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
        }
        util::WaitMicroSeconds(20'000ul);

        DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld02);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init08);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_SLEEP_OR_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init09);

        reg::Write(g_disp1_regs + sizeof(u32) * DC_DISP_DISP_CLOCK_CONTROL, SHIFT_CLK_DIVIDER(4));
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init10);
        util::WaitMicroSeconds(10'000ul);

        /* Configure MIPI CAL. */
        DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal01);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal02);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init11);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal03);
        DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal04);
        if (GetSocType() == fuse::SocType_Mariko) {
            /* On Mariko the above configurations are executed twice, for some reason. */
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal02);
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init11);
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal03);
            DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal04);
        }
        util::WaitMicroSeconds(10'000ul);

        /* Write DISP1, FrameBuffer config. */
        DO_SLEEP_OR_REGISTER_WRITES(g_disp1_regs, DisplayConfigDc02);
        DO_SLEEP_OR_REGISTER_WRITES(g_disp1_regs, DisplayConfigFrameBuffer);
        if (GetLcdVendor() != 0x2050) {
            util::WaitMicroSeconds(35'000ul);
        }
    }

    void ShowDisplay(const ams::impl::FatalErrorContext *f_ctx, const Result save_result) {
        /* Initialize the console. */
        InitializeConsole(g_frame_buffer);
        {
            Print("%s\n", "A fatal error occurred when running Atmosph\xe8re.");
            Print("Program ID: %016" PRIx64 "\n", f_ctx->program_id);
            Print("Error Desc: %s (0x%x)\n", GetErrorDescription(f_ctx->error_desc), f_ctx->error_desc);
            Print("\n");

            if (R_SUCCEEDED(save_result)) {
                Print("Report saved to /atmosphere/fatal_errors/report_%016" PRIx64 ".bin", f_ctx->report_identifier);
            } else {
                Print("Failed to save report to the SD card! (%08x)\n", save_result.GetValue());
            }

            PrintSuggestedErrorFix(f_ctx);

            Print("\nPress POWER to reboot.\n");
        }

        /* Ensure the device will see consistent data. */
        hw::FlushDataCache(g_frame_buffer, FrameBufferSize);

        /* Enable backlight. */
        constexpr auto DisplayBrightness = 100;
        if (GetLcdVendor() == 0x2050) {
            EnableBacklightForVendor2050ForHardwareTypeFive(DisplayBrightness);
        } else {
            EnableBacklightForGeneric(DisplayBrightness);
        }
    }

}

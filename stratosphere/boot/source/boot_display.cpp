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
#include "boot_display.hpp"
#include "boot_i2c_utils.hpp"

#include "boot_registers_di.hpp"

namespace ams::boot {

    /* Display configuration included into anonymous namespace. */
    namespace {

        #include "boot_display_config.inc"

    }

    namespace {

        /* Helpful defines. */
        constexpr size_t DeviceAddressSpaceAlignSize = 4_MB;

        constexpr dd::DeviceVirtualAddress FrameBufferDeviceAddress = DisplayConfigFrameBufferAddress;

        constexpr size_t FrameBufferWidth  = 768;
        constexpr size_t FrameBufferHeight = 1280;
        constexpr size_t FrameBufferSize   = FrameBufferHeight * FrameBufferWidth * sizeof(u32);

        constexpr dd::PhysicalAddress PmcBase     = 0x7000E400ul;
        constexpr dd::PhysicalAddress Disp1Base   = 0x54200000ul;
        constexpr dd::PhysicalAddress DsiBase     = 0x54300000ul;
        constexpr dd::PhysicalAddress ClkRstBase  = 0x60006000ul;
        constexpr dd::PhysicalAddress GpioBase    = 0x6000D000ul;
        constexpr dd::PhysicalAddress ApbMiscBase = 0x70000000ul;
        constexpr dd::PhysicalAddress MipiCalBase = 0x700E3000ul;

        constexpr size_t Disp1Size   = 3 * os::MemoryPageSize;
        constexpr size_t DsiSize     =     os::MemoryPageSize;
        constexpr size_t ClkRstSize  =     os::MemoryPageSize;
        constexpr size_t GpioSize    =     os::MemoryPageSize;
        constexpr size_t ApbMiscSize =     os::MemoryPageSize;
        constexpr size_t MipiCalSize =     os::MemoryPageSize;

        constexpr int DsiWaitForCommandMilliSecondsMax        = 250;
        constexpr int DsiWaitForCommandCompletionMilliSeconds = 5;
        constexpr int DsiWaitForHostControlMilliSecondsMax    = 150;

        constexpr size_t GPIO_PORT3_CNF_0 = 0x200;
        constexpr size_t GPIO_PORT3_OE_0  = 0x210;
        constexpr size_t GPIO_PORT3_OUT_0 = 0x220;

        constexpr size_t GPIO_PORT6_CNF_1 = 0x504;
        constexpr size_t GPIO_PORT6_OE_1  = 0x514;
        constexpr size_t GPIO_PORT6_OUT_1 = 0x524;

        /* Globals. */
        constinit bool g_is_display_intialized = false;
        constinit spl::SocType g_soc_type = spl::SocType_Erista;

        constinit u32 g_lcd_vendor = 0;
        constinit int g_display_brightness = 100;

        constinit dd::DeviceAddressSpaceType g_device_address_space;

        constinit u32 *g_frame_buffer = nullptr;
        constinit u8 g_frame_buffer_storage[DeviceAddressSpaceAlignSize + FrameBufferSize];

        constinit uintptr_t g_disp1_regs    = 0;
        constinit uintptr_t g_dsi_regs      = 0;
        constinit uintptr_t g_clk_rst_regs  = 0;
        constinit uintptr_t g_gpio_regs     = 0;
        constinit uintptr_t g_apb_misc_regs = 0;
        constinit uintptr_t g_mipi_cal_regs = 0;

        /* Helper functions. */
        void InitializeRegisterVirtualAddresses() {
            g_disp1_regs    = dd::QueryIoMapping(Disp1Base,   Disp1Size);
            g_dsi_regs      = dd::QueryIoMapping(DsiBase,     DsiSize);
            g_clk_rst_regs  = dd::QueryIoMapping(ClkRstBase,  ClkRstSize);
            g_gpio_regs     = dd::QueryIoMapping(GpioBase,    GpioSize);
            g_apb_misc_regs = dd::QueryIoMapping(ApbMiscBase, ApbMiscSize);
            g_mipi_cal_regs = dd::QueryIoMapping(MipiCalBase, MipiCalSize);

            AMS_ABORT_UNLESS(g_disp1_regs != 0);
            AMS_ABORT_UNLESS(g_dsi_regs != 0);
            AMS_ABORT_UNLESS(g_clk_rst_regs != 0);
            AMS_ABORT_UNLESS(g_gpio_regs != 0);
            AMS_ABORT_UNLESS(g_apb_misc_regs != 0);
            AMS_ABORT_UNLESS(g_mipi_cal_regs != 0);
        }

        inline void DoRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes, size_t num_writes) {
            for (size_t i = 0; i < num_writes; i++) {
                reg::Write(base_address + reg_writes[i].offset, reg_writes[i].value);
            }
        }

        inline void DoSocDependentRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes_erista, size_t num_writes_erista, const RegisterWrite *reg_writes_mariko, size_t num_writes_mariko) {
            switch (g_soc_type) {
                case spl::SocType_Erista: DoRegisterWrites(base_address, reg_writes_erista, num_writes_erista); break;
                case spl::SocType_Mariko: DoRegisterWrites(base_address, reg_writes_mariko, num_writes_mariko); break;
            }
        }

        inline void DoDsiSleepOrRegisterWrites(const DsiSleepOrRegisterWrite *reg_writes, size_t num_writes) {
            for (size_t i = 0; i < num_writes; i++) {
                switch (reg_writes[i].kind) {
                    case DsiSleepOrRegisterWriteKind_Write:
                        reg::Write(g_dsi_regs + sizeof(u32) * reg_writes[i].offset, reg_writes[i].value);
                        break;
                    case DsiSleepOrRegisterWriteKind_Sleep:
                        svcSleepThread(1'000'000ul * u64(reg_writes[i].offset));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
        }

        #define DO_REGISTER_WRITES(base_address, writes) DoRegisterWrites(base_address, writes, util::size(writes))
        #define DO_SOC_DEPENDENT_REGISTER_WRITES(base_address, writes) DoSocDependentRegisterWrites(base_address, writes##Erista, util::size(writes##Erista), writes##Mariko, util::size(writes##Mariko))
        #define DO_DSI_SLEEP_OR_REGISTER_WRITES(writes) DoDsiSleepOrRegisterWrites(writes, util::size(writes))

        void InitializeFrameBuffer() {
            if (g_frame_buffer != nullptr) {
                std::memset(g_frame_buffer, 0x00, FrameBufferSize);
                dd::FlushDataCache(g_frame_buffer, FrameBufferSize);
            } else {
                const uintptr_t frame_buffer_aligned = util::AlignUp(reinterpret_cast<uintptr_t>(g_frame_buffer_storage), DeviceAddressSpaceAlignSize);
                g_frame_buffer = reinterpret_cast<u32 *>(frame_buffer_aligned);

                std::memset(g_frame_buffer, 0x00, FrameBufferSize);
                dd::FlushDataCache(g_frame_buffer, FrameBufferSize);

                /* Create Address Space. */
                R_ABORT_UNLESS(dd::CreateDeviceAddressSpace(std::addressof(g_device_address_space), 0, (UINT64_C(1) << 32)));

                /* Attach it to the DC. */
                R_ABORT_UNLESS(dd::AttachDeviceAddressSpace(std::addressof(g_device_address_space), svc::DeviceName_Dc));

                /* Map the framebuffer for the DC as read-only. */
                R_ABORT_UNLESS(dd::MapDeviceAddressSpaceAligned(std::addressof(g_device_address_space), dd::GetCurrentProcessHandle(), frame_buffer_aligned, FrameBufferSize, FrameBufferDeviceAddress, dd::MemoryPermission_ReadOnly));
            }
        }

        void FinalizeFrameBuffer() {
            if (g_frame_buffer != nullptr) {
                const uintptr_t frame_buffer_aligned = util::AlignUp(reinterpret_cast<uintptr_t>(g_frame_buffer), DeviceAddressSpaceAlignSize);

                /* Unmap the framebuffer from the DC. */
                dd::UnmapDeviceAddressSpace(std::addressof(g_device_address_space), dd::GetCurrentProcessHandle(), frame_buffer_aligned, FrameBufferSize, FrameBufferDeviceAddress);

                /* Detach address space from the DC. */
                dd::DetachDeviceAddressSpace(std::addressof(g_device_address_space), svc::DeviceName_Dc);

                /* Destroy the address space. */
                dd::DestroyDeviceAddressSpace(std::addressof(g_device_address_space));

                g_frame_buffer = nullptr;
            }
        }

        void WaitDsiTrigger() {
            os::Tick timeout = os::GetSystemTick() + os::ConvertToTick(TimeSpan::FromMilliSeconds(DsiWaitForCommandMilliSecondsMax));

            while (true) {
                if (os::GetSystemTick() >= timeout) {
                    break;
                }
                if (reg::Read(g_dsi_regs + sizeof(u32) * DSI_TRIGGER) == 0) {
                    break;
                }
            }

            os::SleepThread(TimeSpan::FromMilliSeconds(DsiWaitForCommandCompletionMilliSeconds));
        }

        void WaitDsiHostControl() {
            os::Tick timeout = os::GetSystemTick() + os::ConvertToTick(TimeSpan::FromMilliSeconds(DsiWaitForHostControlMilliSecondsMax));

            while (true) {
                if (os::GetSystemTick() >= timeout) {
                    break;
                }
                if ((reg::Read(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL) & DSI_HOST_CONTROL_IMM_BTA) == 0) {
                    break;
                }
            }
        }

    }

    void InitializeDisplay() {
        /* Setup globals. */
        InitializeRegisterVirtualAddresses();
        g_soc_type = spl::GetSocType();
        InitializeFrameBuffer();

        /* Turn on DSI/voltage rail. */
        {
            i2c::driver::I2cSession i2c_session;
            R_ABORT_UNLESS(i2c::driver::OpenSession(std::addressof(i2c_session), i2c::DeviceCode_Max77620Pmic));

            if (g_soc_type == spl::SocType_Mariko) {
                WriteI2cRegister(i2c_session, 0x18, 0x3A);
                WriteI2cRegister(i2c_session, 0x1F, 0x71);
            }
            WriteI2cRegister(i2c_session, 0x23, 0xD0);
        }

        /* Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks. */
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_CLR, 0x1010000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_SET, 0x1010000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_CLR, 0x18000000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_SET, 0x18000000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_X_SET, 0x20000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL, 0xA);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_W_SET, 0x80000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP, 0xA);

        /* DPD idle. */
        dd::WriteIoRegister(PmcBase + APBDEV_PMC_IO_DPD_REQ,  0x40000000);
        dd::WriteIoRegister(PmcBase + APBDEV_PMC_IO_DPD2_REQ, 0x40000000);

        /* Configure LCD pinmux tristate + passthrough. */
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_EN,     reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_NFC_INT,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_PWM, reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_EN,  reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ClearBits(g_apb_misc_regs + PINMUX_AUX_LCD_RST,    reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));

        /* Configure LCD power, VDD. */
        reg::SetBits(g_gpio_regs + GPIO_PORT3_CNF_0, 0x3);
        reg::SetBits(g_gpio_regs + GPIO_PORT3_OE_0,  0x3);
        reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
        svcSleepThread(10'000'000ul);
        reg::SetBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
        svcSleepThread(10'000'000ul);

        /* Configure LCD backlight. */
        reg::SetBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x7);
        reg::SetBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x7);
        reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x2);

        /* Configure display interface and display. */
        reg::Write(g_mipi_cal_regs + 0x060, 0);
        if (g_soc_type == spl::SocType_Mariko) {
            reg::Write(g_mipi_cal_regs + 0x058, 0);
            reg::Write(g_apb_misc_regs + 0xAC0, 0);
        }

        /* Execute configs. */
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld01);
        DO_REGISTER_WRITES(g_disp1_regs, DisplayConfigDc01);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init01);
        /* NOTE: Nintendo bug here. */
        /* As of 8.0.0, Nintendo writes this list to CAR instead of DSI */
        /* This results in them zeroing CLK_SOURCE_UARTA... */
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init02);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init03);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init04);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init05);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init06);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init07);

        svcSleepThread(10'000'000ul);

        /* Enable backlight reset. */
        reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x4);
        svcSleepThread(60'000'000ul);

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_BTA_TIMING, 0x50204);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x337);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
        WaitDsiTrigger();

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x406);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
        WaitDsiTrigger();

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL, DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC);
        WaitDsiHostControl();
        svcSleepThread(5'000'000ul);

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
            if ((host_response[2] & 0xFF) == 0x10) {
                g_lcd_vendor = 0;
            } else {
                g_lcd_vendor = (host_response[2] >> 8) & 0xFF00;
            }
            g_lcd_vendor = (g_lcd_vendor & 0xFFFFFF00) | (host_response[2] & 0xFF);
        }

        /* LCD vendor specific configuration. */
        switch (g_lcd_vendor) {
            case 0xF30: /* AUO first revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(180'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x711148B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
            case 0xF20: /* Innolux first revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(180'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x751548B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                break;
            case 0x10: /* Japan Display Inc screens. */
                DO_DSI_SLEEP_OR_REGISTER_WRITES(DisplayConfigJdiSpecificInit01);
                break;
            default:
                /* Innolux and AUO second revision screens. */
                if ((g_lcd_vendor | 0x10) == 0x1030) {
                    reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                    reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                    svcSleepThread(120'000'000ul);
                    reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                    reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                }
                break;
        }
        svcSleepThread(20'000'000ul);

        DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld02);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init08);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init09);

        reg::Write(g_disp1_regs + sizeof(u32) * DC_DISP_DISP_CLOCK_CONTROL, SHIFT_CLK_DIVIDER(4));
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init10);
        svcSleepThread(10'000'000ul);

        /* Configure MIPI CAL. */
        DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal01);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal02);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init11);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal03);
        DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal04);
        if (g_soc_type == spl::SocType_Mariko) {
            /* On Mariko the above configurations are executed twice, for some reason. */
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal02);
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init11);
            DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal03);
            DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal04);
        }
        svcSleepThread(10'000'000ul);

        /* Write DISP1, FrameBuffer config. */
        DO_REGISTER_WRITES(g_disp1_regs, DisplayConfigDc02);
        DO_REGISTER_WRITES(g_disp1_regs, DisplayConfigFrameBuffer);
        svcSleepThread(35'000'000ul);
        g_is_display_intialized = true;
    }

    void ShowDisplay(size_t x, size_t y, size_t width, size_t height, const u32 *img) {
        if (!g_is_display_intialized) {
            return;
        }

        /* Draw the image to the screen. */
        std::memset(g_frame_buffer, 0, FrameBufferSize);
        {
            for (size_t cur_y = 0; cur_y < height; cur_y++) {
                for (size_t cur_x = 0; cur_x < width; cur_x++) {
                    g_frame_buffer[(FrameBufferHeight - (x + cur_x)) * FrameBufferWidth + y + cur_y] = img[cur_y * width + cur_x];
                }
            }
        }
        armDCacheFlush(g_frame_buffer, FrameBufferSize);

        /* Enable backlight. */
        reg::SetBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);
    }

    void FinalizeDisplay() {
        if (!g_is_display_intialized) {
            return;
        }

        /* Disable backlight. */
        reg::ClearBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);

        reg::Write(g_disp1_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 1);
        reg::Write(g_disp1_regs + sizeof(u32) * DSI_WR_DATA, 0x2805);

        /* Nintendo waits 5 frames before continuing. */
        {
            const uintptr_t host1x_vaddr = dd::GetIoMapping(0x500030a4, 4);
            const u32 start_val = reg::Read(host1x_vaddr);
            while (reg::Read(host1x_vaddr) < start_val + 5) {
                /* spinlock here. */
            }
        }

        reg::Write(g_disp1_regs + sizeof(u32) * DC_CMD_STATE_ACCESS, (READ_MUX | WRITE_MUX));
        reg::Write(g_disp1_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 0);


        DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld01);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Fini01);
        DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
        DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Fini02);

        svcSleepThread(10'000'000ul);

        /* Vendor specific shutdown. */
        switch (g_lcd_vendor) {
            case 0x10:   /* Japan Display Inc screens. */
                DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigJdiSpecificFini01);
                break;
            case 0xF30:  /* AUO first revision screens. */
                DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigAuoRev1SpecificFini01);
                svcSleepThread(5'000'000ul);
                break;
            case 0x1020: /* Innolux second revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0xB39);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x751548B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x71143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x115631);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                break;
            case 0x1030: /* AUO second revision screens. */
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0xB39);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x711148B1);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x71143209);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x114D31);
                reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(5'000'000ul);
                break;
            default:
                break;
        }
        svcSleepThread(5'000'000ul);

        reg::Write(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1005);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
        svcSleepThread(50'000'000ul);

        /* Disable backlight RST/Voltage. */
        reg::ClearBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x4);
        svcSleepThread(10'000'000ul);
        reg::ClearBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
        svcSleepThread(10'000'000ul);
        reg::ClearBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
        svcSleepThread(10'000'000ul);

        /* Cut clock to DSI. */
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_SET, 0x1010000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_CLR, 0x1010000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_SET, 0x18000000);
        reg::Write(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, 0x18000000);
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_PAD_CONTROL_0, (DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF)));
        reg::Write(g_dsi_regs + sizeof(u32) * DSI_POWER_CONTROL, 0);

        /* Final LCD config for PWM */
        reg::ClearBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x1);
        reg::SetBits(g_apb_misc_regs + PINMUX_AUX_LCD_BL_PWM, reg::EncodeMask(PINMUX_REG_BITS_MASK(AUX_TRISTATE)));
        reg::ReadWrite(g_apb_misc_regs + PINMUX_AUX_LCD_BL_PWM, 1, 0x3);

        /* Unmap framebuffer from DC virtual address space. */
        FinalizeFrameBuffer();
        g_is_display_intialized = false;
    }

    void SetDisplayBrightness(int percentage) {
        g_display_brightness = percentage;
    }

}

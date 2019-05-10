/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "boot_functions.hpp"
#include "boot_display_config.hpp"
#include "i2c_driver/i2c_api.hpp"

/* Helpful defines. */
constexpr size_t DeviceAddressSpaceAlignSize = 0x400000;
constexpr size_t DeviceAddressSpaceAlignMask = DeviceAddressSpaceAlignSize - 1;
constexpr uintptr_t FrameBufferPaddr = DisplayConfigFrameBufferAddress;
constexpr size_t FrameBufferWidth = 768;
constexpr size_t FrameBufferHeight = 1280;
constexpr size_t FrameBufferSize = FrameBufferHeight * FrameBufferWidth * sizeof(u32);

constexpr uintptr_t Disp1Base = 0x54200000ul;
constexpr uintptr_t DsiBase = 0x54300000ul;
constexpr uintptr_t ClkRstBase = 0x60006000ul;
constexpr uintptr_t GpioBase = 0x6000D000ul;
constexpr uintptr_t ApbMiscBase = 0x70000000ul;
constexpr uintptr_t MipiCalBase = 0x700E3000ul;
constexpr size_t Disp1Size = 0x3000;
constexpr size_t DsiSize = 0x1000;
constexpr size_t ClkRstSize = 0x1000;
constexpr size_t GpioSize = 0x1000;
constexpr size_t ApbMiscSize = 0x1000;
constexpr size_t MipiCalSize = 0x1000;

/* Types. */

/* Globals. */
static bool g_is_display_intialized = false;
static u32 *g_frame_buffer = nullptr;
static bool g_is_mariko = false;
static u32 g_lcd_vendor = 0;
static Handle g_dc_das_hnd = INVALID_HANDLE;
static u8 g_frame_buffer_storage[DeviceAddressSpaceAlignSize + FrameBufferSize];

static uintptr_t g_disp1_regs = 0;
static uintptr_t g_dsi_regs = 0;
static uintptr_t g_clk_rst_regs = 0;
static uintptr_t g_gpio_regs = 0;
static uintptr_t g_apb_misc_regs = 0;
static uintptr_t g_mipi_cal_regs = 0;

static inline uintptr_t QueryVirtualAddress(uintptr_t phys, size_t size) {
    uintptr_t aligned_phys = phys & ~0xFFFul;
    size_t aligned_size = size + (phys - aligned_phys);
    uintptr_t aligned_virt;
    if (R_FAILED(svcQueryIoMapping(&aligned_virt, aligned_phys, aligned_size))) {
        std::abort();
    }
    return aligned_virt + (phys - aligned_phys);
}

static inline void WriteRegister(volatile u32 *reg, u32 val) {
    *reg = val;
}

static inline void WriteRegister(uintptr_t reg, u32 val) {
    WriteRegister(reinterpret_cast<volatile u32 *>(reg), val);
}

static inline u32 ReadRegister(volatile u32 *reg) {
    u32 val = *reg;
    return val;
}

static inline u32 ReadRegister(uintptr_t reg) {
    return ReadRegister(reinterpret_cast<volatile u32 *>(reg));
}

static inline void SetRegisterBits(volatile u32 *reg, u32 mask) {
    *reg |= mask;
}

static inline void SetRegisterBits(uintptr_t reg, u32 mask) {
    SetRegisterBits(reinterpret_cast<volatile u32 *>(reg), mask);
}

static inline void ClearRegisterBits(volatile u32 *reg, u32 mask) {
    *reg &= mask;
}

static inline void ClearRegisterBits(uintptr_t reg, u32 mask) {
    ClearRegisterBits(reinterpret_cast<volatile u32 *>(reg), mask);
}

static inline void ReadWriteRegisterBits(volatile u32 *reg, u32 val, u32 mask) {
    *reg = (*reg & (~mask)) | (val & mask);
}

static inline void ReadWriteRegisterBits(uintptr_t reg, u32 val, u32 mask) {
    ReadWriteRegisterBits(reinterpret_cast<volatile u32 *>(reg), val, mask);
}

static void InitializeRegisterBaseAddresses() {
    g_disp1_regs    = QueryVirtualAddress(Disp1Base, Disp1Size);
    g_dsi_regs      = QueryVirtualAddress(DsiBase, DsiSize);
    g_clk_rst_regs  = QueryVirtualAddress(ClkRstBase, ClkRstSize);
    g_gpio_regs     = QueryVirtualAddress(GpioBase, GpioSize);
    g_apb_misc_regs = QueryVirtualAddress(ApbMiscBase, ApbMiscSize);
    g_mipi_cal_regs = QueryVirtualAddress(MipiCalBase, MipiCalSize);
}

static inline void DoRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes, size_t num_writes) {
    for (size_t i = 0; i < num_writes; i++) {
        *(reinterpret_cast<volatile u32 *>(base_address + reg_writes[i].offset)) = reg_writes[i].value;
    }
}

static inline void DoSocDependentRegisterWrites(uintptr_t base_address, const RegisterWrite *reg_writes_erista, size_t num_writes_erista, const RegisterWrite *reg_writes_mariko, size_t num_writes_mariko) {
    if (g_is_mariko) {
        DoRegisterWrites(base_address, reg_writes_mariko, num_writes_mariko);
    } else {
        DoRegisterWrites(base_address, reg_writes_erista, num_writes_erista);
    }
}

static inline void DoDsiSleepOrRegisterWrites(const DsiSleepOrRegisterWrite *reg_writes, size_t num_writes) {
    for (size_t i = 0; i < num_writes; i++) {
        if (reg_writes[i].kind == DsiSleepOrRegisterWriteKind_Write) {
            *(reinterpret_cast<volatile u32 *>(g_dsi_regs + sizeof(u32) * reg_writes[i].offset)) = reg_writes[i].value;
        } else if (reg_writes[i].kind == DsiSleepOrRegisterWriteKind_Sleep) {
            svcSleepThread(1'000'000ul * u64(reg_writes[i].offset));
        } else {
            std::abort();
        }
    }
}

#define DO_REGISTER_WRITES(base_address, writes) DoRegisterWrites(base_address, writes, sizeof(writes) / sizeof(writes[0]))
#define DO_SOC_DEPENDENT_REGISTER_WRITES(base_address, writes) DoSocDependentRegisterWrites(base_address, writes##Erista, sizeof(writes##Erista) / sizeof(writes##Erista[0]), writes##Mariko, sizeof(writes##Mariko) / sizeof(writes##Mariko[0]))
#define DO_DSI_SLEEP_OR_REGISTER_WRITES(writes) DoDsiSleepOrRegisterWrites(writes, sizeof(writes) / sizeof(writes[0]))

static void InitializeFrameBuffer() {
    if (g_frame_buffer != nullptr) {
        std::memset(g_frame_buffer, 0x00, FrameBufferSize);
        armDCacheFlush(g_frame_buffer, FrameBufferSize);
    } else {
        const uintptr_t frame_buffer_aligned = ((reinterpret_cast<uintptr_t>(g_frame_buffer_storage) + DeviceAddressSpaceAlignMask) & ~uintptr_t(DeviceAddressSpaceAlignMask));
        g_frame_buffer = reinterpret_cast<u32 *>(frame_buffer_aligned);
        std::memset(g_frame_buffer, 0x00, FrameBufferSize);
        armDCacheFlush(g_frame_buffer, FrameBufferSize);

        constexpr u64 DeviceName_DC = 2;

        /* Create Address Space. */
        if (R_FAILED(svcCreateDeviceAddressSpace(&g_dc_das_hnd, 0, (1ul << 32)))) {
            std::abort();
        }
        /* Attach it to the DC. */
        if (R_FAILED(svcAttachDeviceAddressSpace(DeviceName_DC, g_dc_das_hnd))) {
            std::abort();
        }

        /* Map the framebuffer for the DC as read-only. */
        if (R_FAILED(svcMapDeviceAddressSpaceAligned(g_dc_das_hnd, CUR_PROCESS_HANDLE, frame_buffer_aligned, FrameBufferSize, FrameBufferPaddr, 1))) {
            std::abort();
        }
    }
}

static void FinalizeFrameBuffer() {
    if (g_frame_buffer != nullptr) {
        const uintptr_t frame_buffer_aligned = reinterpret_cast<uintptr_t>(g_frame_buffer);
        constexpr u64 DeviceName_DC = 2;

        /* Unmap the framebuffer from the DC. */
        if (R_FAILED(svcUnmapDeviceAddressSpace(g_dc_das_hnd, CUR_PROCESS_HANDLE, frame_buffer_aligned, FrameBufferSize, FrameBufferPaddr))) {
            std::abort();
        }
        /* Detach address space from the DC. */
        if (R_FAILED(svcDetachDeviceAddressSpace(DeviceName_DC, g_dc_das_hnd))) {
            std::abort();
        }
        /* Close the address space. */
        if (R_FAILED(svcCloseHandle(g_dc_das_hnd))) {
            std::abort();
        }
        g_dc_das_hnd = INVALID_HANDLE;
        g_frame_buffer = nullptr;
    }
}

static void WaitDsiTrigger() {
    TimeoutHelper timeout_helper(250'000'000ul);

    while (true) {
        if (timeout_helper.TimedOut()) {
            break;
        }
        if (ReadRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER) == 0) {
            break;
        }
    }

    svcSleepThread(5'000'000ul);
}

static void WaitDsiHostControl() {
    TimeoutHelper timeout_helper(150'000'000ul);

    while (true) {
        if (timeout_helper.TimedOut()) {
            break;
        }
        if ((ReadRegister(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL) & DSI_HOST_CONTROL_IMM_BTA) == 0) {
            break;
        }
    }
}

void Boot::InitializeDisplay() {
    /* Setup globals. */
    InitializeRegisterBaseAddresses();
    g_is_mariko = Boot::IsMariko();
    InitializeFrameBuffer();

    /* Turn on DSI/voltage rail. */
    {
        I2cSessionImpl i2c_session;
        I2cDriver::Initialize();
        I2cDriver::OpenSession(&i2c_session, I2cDevice_Max77620Pmic);

        if (g_is_mariko) {
            Boot::WriteI2cRegister(i2c_session, 0x18, 0x3A);
            Boot::WriteI2cRegister(i2c_session, 0x1F, 0x71);
        }
        Boot::WriteI2cRegister(i2c_session, 0x23, 0xD0);

        I2cDriver::Finalize();
    }

    /* Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks. */
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_CLR, 0x1010000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_SET, 0x1010000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_CLR, 0x18000000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_SET, 0x18000000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_X_SET, 0x20000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL, 0xA);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_W_SET, 0x80000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP, 0xA);

    /* DPD idle. */
    WritePmcRegister(PmcBase + APBDEV_PMC_IO_DPD_REQ, 0x40000000);
    WritePmcRegister(PmcBase + APBDEV_PMC_IO_DPD2_REQ, 0x40000000);

    /* Configure LCD pinmux tristate + passthrough. */
    ClearRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_NFC_EN, ~PINMUX_TRISTATE);
    ClearRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_NFC_INT, ~PINMUX_TRISTATE);
    ClearRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_LCD_BL_PWM, ~PINMUX_TRISTATE);
    ClearRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_LCD_BL_EN, ~PINMUX_TRISTATE);
    ClearRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_LCD_RST, ~PINMUX_TRISTATE);

    /* Configure LCD power, VDD. */
    SetRegisterBits(g_gpio_regs + GPIO_PORT3_CNF_0, 0x3);
    SetRegisterBits(g_gpio_regs + GPIO_PORT3_OE_0,  0x3);
    SetRegisterBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x1);
    svcSleepThread(10'000'000ul);
    SetRegisterBits(g_gpio_regs + GPIO_PORT3_OUT_0, 0x2);
    svcSleepThread(10'000'000ul);

    /* Configure LCD backlight. */
    SetRegisterBits(g_gpio_regs + GPIO_PORT6_CNF_1, 0x7);
    SetRegisterBits(g_gpio_regs + GPIO_PORT6_OE_1,  0x7);
    SetRegisterBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x2);

    /* Configure display interface and display. */
    WriteRegister(g_mipi_cal_regs + 0x060, 0);
    if (g_is_mariko) {
        WriteRegister(g_mipi_cal_regs + 0x058, 0);
        WriteRegister(g_apb_misc_regs + 0xAC0, 0);
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
    SetRegisterBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x4);
    svcSleepThread(60'000'000ul);

    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_BTA_TIMING, 0x50204);
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x337);
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
    WaitDsiTrigger();

    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x406);
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
    WaitDsiTrigger();

    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_HOST_CONTROL, DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC);
    WaitDsiHostControl();
    svcSleepThread(5'000'000ul);

    /* Parse LCD vendor. */
    {
        u32 host_response[3];
        for (size_t i = 0; i < sizeof(host_response) / sizeof(host_response[0]); i++) {
            host_response[i] = ReadRegister(g_dsi_regs + sizeof(u32) * DSI_RD_DATA);
        }

        if ((host_response[2] & 0xFF) == 0x10) {
            g_lcd_vendor = 0;
        } else {
            g_lcd_vendor = (host_response[2] >> 8) & 0xFF00;
        }
        g_lcd_vendor = (g_lcd_vendor & 0xFFFFFF00) | (host_response[2] & 0xFF);
    }

    /* LCD vendor specific configuration. */
    switch (g_lcd_vendor) {
        case 0xF30: /* TODO: What's this? */
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(180'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x711148B1);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            break;
        case 0xF20: /* TODO: What's this? */
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(180'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x739);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x751548B1);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x143209);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            break;
        case 0x10: /* Japan Display Inc screens. */
            DO_DSI_SLEEP_OR_REGISTER_WRITES(DisplayConfigJdiSpecificInit01);
            break;
        default:
            if ((g_lcd_vendor | 0x10) == 0x1030) {
                WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1105);
                WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
                svcSleepThread(120'000'000ul);
                WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x2905);
                WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            }
            break;
    }
    svcSleepThread(20'000'000ul);

    DO_SOC_DEPENDENT_REGISTER_WRITES(g_clk_rst_regs, DisplayConfigPlld02);
    DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init08);
    DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsiPhyTiming);
    DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init09);

    WriteRegister(g_disp1_regs + sizeof(u32) * DC_DISP_DISP_CLOCK_CONTROL, SHIFT_CLK_DIVIDER(4));
    DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init10);
    svcSleepThread(10'000'000ul);

    /* Configure MIPI CAL. */
    DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal01);
    DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal02);
    DO_SOC_DEPENDENT_REGISTER_WRITES(g_dsi_regs, DisplayConfigDsi01Init11);
    DO_SOC_DEPENDENT_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal03);
    DO_REGISTER_WRITES(g_mipi_cal_regs, DisplayConfigMipiCal04);
    if (g_is_mariko) {
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

void Boot::ShowDisplay(size_t x, size_t y, size_t width, size_t height, const u32 *img) {
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
    SetRegisterBits(g_gpio_regs + GPIO_PORT6_OUT_1, 0x1);
}

void Boot::FinalizeDisplay() {
    if (!g_is_display_intialized) {
        return;
    }

    /* Disable backlight. */
    ClearRegisterBits(g_gpio_regs + GPIO_PORT6_OUT_1, ~0x1);

    WriteRegister(g_disp1_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 1);
    WriteRegister(g_disp1_regs + sizeof(u32) * DSI_WR_DATA, 0x2805);

    /* Nintendo waits 5 frames before continuing. */
    {
        const uintptr_t host1x_vaddr = QueryVirtualAddress(0x500030a4, 4);
        const u32 start_val = ReadRegister(host1x_vaddr);
        while (ReadRegister(host1x_vaddr) < start_val + 5) {
            /* spinlock here. */
        }
    }

    WriteRegister(g_disp1_regs + sizeof(u32) * DC_CMD_STATE_ACCESS, (READ_MUX | WRITE_MUX));
    WriteRegister(g_disp1_regs + sizeof(u32) * DSI_VIDEO_MODE_CONTROL, 0);


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
        case 0xF30:  /* TODO: What's this? */
            DO_REGISTER_WRITES(g_dsi_regs, DisplayConfigF30SpecificFini01);
            svcSleepThread(5'000'000ul);
            break;
        case 0x1020: /* TODO: What's this? */
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0xB39);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x751548B1);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x71143209);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x115631);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            break;
        case 0x1030: /* TODO: What's this? */
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x439);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x9483FFB9);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0xB39);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x711148B1);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x71143209);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x114D31);
            WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
            svcSleepThread(5'000'000ul);
            break;
        default:
            break;
    }
    svcSleepThread(5'000'000ul);

    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_WR_DATA, 0x1005);
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST);
    svcSleepThread(50'000'000ul);

    /* Disable backlight RST/Voltage. */
    ClearRegisterBits(g_gpio_regs + GPIO_PORT6_OUT_1, ~0x4);
    svcSleepThread(10'000'000ul);
    ClearRegisterBits(g_gpio_regs + GPIO_PORT3_OUT_0, ~0x2);
    svcSleepThread(10'000'000ul);
    ClearRegisterBits(g_gpio_regs + GPIO_PORT3_OUT_0, ~0x1);
    svcSleepThread(10'000'000ul);

    /* Cut clock to DSI. */
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_H_SET, 0x1010000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_H_CLR, 0x1010000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_RST_DEV_L_SET, 0x18000000);
    WriteRegister(g_clk_rst_regs + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, 0x18000000);
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_PAD_CONTROL_0, (DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF)));
    WriteRegister(g_dsi_regs + sizeof(u32) * DSI_POWER_CONTROL, 0);

    /* Final LCD config for PWM */
    ClearRegisterBits(g_gpio_regs + GPIO_PORT6_CNF_1, ~0x1);
    SetRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_LCD_BL_PWM, PINMUX_TRISTATE);
    ReadWriteRegisterBits(g_apb_misc_regs + 0x3000 + PINMUX_AUX_LCD_BL_PWM, 1, 0x3);

    /* Unmap framebuffer from DC virtual address space. */
    FinalizeFrameBuffer();
    g_is_display_intialized = false;
}
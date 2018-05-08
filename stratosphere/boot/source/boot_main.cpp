#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#define GPIO_BASE 0x6000D000
#define APB_MISC_BASE 0x70000000
#define PINMUX_BASE (APB_MISC_BASE + 0x3000)
#define PMC_BASE 0x7000E400
#define MAX_GPIOS 0x3D
#define MAX_PINMUX 0xA2

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x200000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    Result rc;

    /* Initialize services we need (TODO: NCM) */
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    
    rc = splInitialize();
    if (R_FAILED(rc))
        fatalSimple(0xCAFE << 4 | 1);
    
    rc = pmshellInitialize();
    if (R_FAILED(rc))
        fatalSimple(0xCAFE << 4 | 2);
    
    fsdevInit();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevExit();
    pmshellExit(); 
    splExit();
    fsExit();
    smExit();
}

static const std::tuple<u32, bool, bool> g_gpio_map[] = {
    /* Icosa, Copper, Hoag and Mariko  */
    {0xFFFFFFFF, false, false},  /* Invalid */
    {0x000000CC, true, false},   /* Port Z, Pin 4 */
    {0x00000024, true, false},   /* Port E, Pin 4 */
    {0x0000003C, true, false},   /* Port H, Pin 4 */
    {0x000000DA, false, true},   /* Port BB, Pin 2 */
    {0x000000DB, true, false},   /* Port BB, Pin 3 */
    {0x000000DC, false, false},  /* Port BB, Pin 4 */
    {0x00000025, true, false},   /* Port E, Pin 5 */
    {0x00000090, false, false},  /* Port S, Pin 0 */
    {0x00000091, false, false},  /* Port S, Pin 1 */
    {0x00000096, true, false},   /* Port S, Pin 6 */
    {0x00000097, false, true},   /* Port S, Pin 7 */
    {0x00000026, false, false},  /* Port E, Pin 6 */
    {0x00000005, true, false},   /* Port A, Pin 5 */
    {0x00000078, false, false},  /* Port P, Pin 0 */
    {0x00000093, false, true},   /* Port S, Pin 3 */
    {0x0000007D, false, false},  /* Port P, Pin 5 */
    {0x0000007C, false, false},  /* Port P, Pin 4 */
    {0x0000007B, false, false},  /* Port P, Pin 3 */
    {0x0000007A, false, false},  /* Port P, Pin 2 */
    {0x000000BC, false, true},   /* Port X, Pin 4 */
    {0x000000AE, false, false},  /* Port V, Pin 6 */
    {0x000000BA, false, false},  /* Port X, Pin 2 */
    {0x000000B9, false, true},   /* Port X, Pin 1 */
    {0x000000BD, false, false},  /* Port X, Pin 5 */
    {0x000000BE, false, true},   /* Port X, Pin 6 */
    {0x000000BF, false, true},   /* Port X, Pin 7 */
    {0x000000C0, false, true},   /* Port Y, Pin 0 */
    {0x000000C1, false, false},  /* Port Y, Pin 1 */
    {0x000000A9, true, false},   /* Port V, Pin 1 */
    {0x000000AA, true, false},   /* Port V, Pin 2 */
    {0x00000055, true, false},   /* Port K, Pin 5 */
    {0x000000AD, true, false},   /* Port V, Pin 5 */
    {0x000000C8, false, true},   /* Port Z, Pin 0 */
    {0x000000CA, false, false},  /* Port Z, Pin 2 */
    {0x000000CB, false, true},   /* Port Z, Pin 3 */
    {0x0000004F, true, false},   /* Port J, Pin 7 */
    {0x00000050, false, false},  /* Port K, Pin 0 */
    {0x00000051, false, false},  /* Port K, Pin 1 */
    {0x00000052, false, false},  /* Port K, Pin 2 */
    {0x00000054, false, true},   /* Port K, Pin 4 */
    {0x00000056, false, true},   /* Port K, Pin 6 */
    {0x00000057, false, true},   /* Port K, Pin 7 */
    {0x00000053, true, false},   /* Port K, Pin 3 */
    {0x000000E3, true, false},   /* Port CC, Pin 3 */
    {0x00000038, true, false},   /* Port H, Pin 0 */
    {0x00000039, true, false},   /* Port H, Pin 1 */
    {0x0000003B, true, false},   /* Port H, Pin 3 */
    {0x0000003D, false, false},  /* Port H, Pin 5 */
    {0x0000003F, true, false},   /* Port H, Pin 7 */
    {0x00000040, true, false},   /* Port I, Pin 0 */
    {0x00000041, true, false},   /* Port I, Pin 1 */
    {0x0000003E, false, false},  /* Port H, Pin 6 */
    {0x000000E2, false, true},   /* Port CC, Pin 2 */
    {0x000000E4, true, false},   /* Port CC, Pin 4 */
    {0x0000003A, false, false},  /* Port H, Pin 2 */
    {0x000000C9, false, true},   /* Port Z, Pin 1 */
    {0x0000004D, true, false},   /* Port J, Pin 5 */
    {0x00000058, true, false},   /* Port L, Pin 0 */
    {0x0000003E, false, false},  /* Port H, Pin 6 */
    {0x00000026, false, false},  /* Port E, Pin 6 */
    
    /* Copper only */
    {0xFFFFFFFF, false, false},  /* Invalid */
    {0x00000033, false, false},  /* Port G, Pin 3 */
    {0x0000001C, false, false},  /* Port D, Pin 4 */
    {0x000000D9, true, false},   /* Port BB, Pin 1 */
    {0x0000000C, false, false},  /* Port B, Pin 4 */
    {0x0000000D, false, false},  /* Port B, Pin 5 */
    {0x00000021, true, false},   /* Port E, Pin 1 */
    {0x00000027, false, false},  /* Port E, Pin 7 */
    {0x00000092, false, false},  /* Port S, Pin 2 */
    {0x00000095, true, false},   /* Port S, Pin 5 */
    {0x00000098, true, false},   /* Port T, Pin 0 */
    {0x00000010, true, false},   /* Port C, Pin 0 */
    {0x00000011, true, false},   /* Port C, Pin 1 */
    {0x00000012, true, false},   /* Port C, Pin 2 */
    {0x00000042, true, false},   /* Port I, Pin 2 */
    {0x000000E6, false, false},  /* Port CC, Pin 6 */
    
    /* 2.0.0+ Copper only */
    {0x000000AC, true, false},   /* Port V, Pin 4 */
    {0x000000E1, false, false},  /* Port CC, Pin 1 */
    
    /* 5.0.0+ Copper only (unused) */
    {0x00000056, false, false},  /* Port K, Pin 6 */
};

static int gpio_configure(u64 gpio_base_vaddr, unsigned int gpio_pad_name) {
    /* Fetch this GPIO's pad descriptor */
    u32 gpio_pad_desc = std::get<0>(g_gpio_map[gpio_pad_name]);
    
    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));
    
    /* Extract the bit and lock values from the GPIO pad descriptor */
    u32 gpio_cnf_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (0x01 << (gpio_pad_desc & 0x07)));
    
    /* Write to the appropriate GPIO_CNF_x register (upper offset) */
    *((u32 *)gpio_base_vaddr + gpio_reg_offset + 0x80) = gpio_cnf_val;
    
    /* Do a dummy read from GPIO_CNF_x register (lower offset) */
    gpio_cnf_val = *((u32 *)gpio_base_vaddr + gpio_reg_offset);
    
    return gpio_cnf_val;
}

static int gpio_set_direction(u64 gpio_base_vaddr, unsigned int gpio_pad_name) {
    /* Fetch this GPIO's pad descriptor */
    u32 gpio_pad_desc = std::get<0>(g_gpio_map[gpio_pad_name]);
    
    /* Fetch this GPIO's direction */
    bool is_out = std::get<1>(g_gpio_map[gpio_pad_name]);
    
    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));
    
    /* Set the direction bit and lock values */
    u32 gpio_oe_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (is_out << (gpio_pad_desc & 0x07)));
    
    /* Write to the appropriate GPIO_OE_x register (upper offset) */
    *((u32 *)gpio_base_vaddr + gpio_reg_offset + 0x90) = gpio_oe_val;
    
    /* Do a dummy read from GPIO_OE_x register (lower offset) */
    gpio_oe_val = *((u32 *)gpio_base_vaddr + gpio_reg_offset);
    
    return gpio_oe_val;
}

static int gpio_set_value(u64 gpio_base_vaddr, unsigned int gpio_pad_name) {
    /* Fetch this GPIO's pad descriptor */
    u32 gpio_pad_desc = std::get<0>(g_gpio_map[gpio_pad_name]);
    
    /* Fetch this GPIO's output value */
    bool is_high = std::get<2>(g_gpio_map[gpio_pad_name]);
    
    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));
    
    /* Set the output bit and lock values */
    u32 gpio_out_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (is_high << (gpio_pad_desc & 0x07)));
    
    /* Write to the appropriate GPIO_OUT_x register (upper offset) */
    *((u32 *)gpio_base_vaddr + gpio_reg_offset + 0xA0) = gpio_out_val;
    
    /* Do a dummy read from GPIO_OUT_x register (lower offset) */
    gpio_out_val = *((u32 *)gpio_base_vaddr + gpio_reg_offset);
    
    return gpio_out_val;
}

static const std::tuple<u32, u32, u8> g_pinmux_map[] = {
    {0x00003000, 0x72FF, 0x01},   /* Sdmmc1Clk */
    {0x00003004, 0x72FF, 0x02},   /* Sdmmc1Cmd */
    {0x00003008, 0x72FF, 0x02},   /* Sdmmc1Dat3 */
    {0x0000300C, 0x72FF, 0x02},   /* Sdmmc1Dat2 */
    {0x00003010, 0x72FF, 0x02},   /* Sdmmc1Dat1 */
    {0x00003014, 0x72FF, 0x01},   /* Sdmmc1Dat0 */
    {0x0000301C, 0x72FF, 0x01},   /* Sdmmc3Clk */
    {0x00003020, 0x72FF, 0x01},   /* Sdmmc3Cmd */
    {0x00003024, 0x72FF, 0x01},   /* Sdmmc3Dat0 */
    {0x00003028, 0x72FF, 0x01},   /* Sdmmc3Dat1 */
    {0x0000302C, 0x72FF, 0x01},   /* Sdmmc3Dat2 */
    {0x00003030, 0x72FF, 0x01},   /* Sdmmc3Dat3 */
    {0x00003038, 0x1DFF, 0x01},   /* PexL0RstN */
    {0x0000303C, 0x1DFF, 0x01},   /* PexL0ClkreqN */
    {0x00003040, 0x1DFF, 0x01},   /* PexWakeN */
    {0x00003044, 0x1DFF, 0x01},   /* PexL1RstN */
    {0x00003048, 0x1DFF, 0x01},   /* PexL1ClkreqN */
    {0x0000304C, 0x19FF, 0x01},   /* SataLedActive */
    {0x00003050, 0x1F2FF, 0x01},  /* Spi1Mosi */
    {0x00003054, 0x1F2FF, 0x01},  /* Spi1Miso */
    {0x00003058, 0x1F2FF, 0x01},  /* Spi1Sck */
    {0x0000305C, 0x1F2FF, 0x01},  /* Spi1Cs0 */
    {0x00003060, 0x1F2FF, 0x01},  /* Spi1Cs1 */
    {0x00003064, 0x72FF, 0x02},   /* Spi2Mosi */
    {0x00003068, 0x72FF, 0x02},   /* Spi2Miso */
    {0x0000306C, 0x72FF, 0x02},   /* Spi2Sck */
    {0x00003070, 0x72FF, 0x02},   /* Spi2Cs0 */
    {0x00003074, 0x72FF, 0x01},   /* Spi2Cs1 */
    {0x00003078, 0x1F2FF, 0x01},  /* Spi4Mosi */
    {0x0000307C, 0x1F2FF, 0x01},  /* Spi4Miso */
    {0x00003080, 0x1F2FF, 0x01},  /* Spi4Sck */
    {0x00003084, 0x1F2FF, 0x01},  /* Spi4Cs0 */
    {0x00003088, 0x72FF, 0x01},   /* QspiSck */
    {0x0000308C, 0x72FF, 0x01},   /* QspiCsN */
    {0x00003090, 0x72FF, 0x01},   /* QspiIo0 */
    {0x00003094, 0x72FF, 0x01},   /* QspiIo1 */
    {0x00003098, 0x72FF, 0x01},   /* QspiIo2 */
    {0x0000309C, 0x72FF, 0x01},   /* QspiIo3 */
    {0x000030A4, 0x19FF, 0x02},   /* Dmic1Clk */
    {0x000030A8, 0x19FF, 0x02},   /* Dmic1Dat */
    {0x000030AC, 0x19FF, 0x02},   /* Dmic2Clk */
    {0x000030B0, 0x19FF, 0x02},   /* Dmic2Dat */
    {0x000030B4, 0x19FF, 0x02},   /* Dmic3Clk */
    {0x000030B8, 0x19FF, 0x02},   /* Dmic3Dat */
    {0x000030BC, 0x1DFF, 0x01},   /* Gen1I2cScl */
    {0x000030C0, 0x1DFF, 0x01},   /* Gen1I2cSda */
    {0x000030C4, 0x1DFF, 0x01},   /* Gen2I2cScl */
    {0x000030C8, 0x1DFF, 0x01},   /* Gen2I2cSda */
    {0x000030CC, 0x1DFF, 0x01},   /* Gen3I2cScl */
    {0x000030D0, 0x1DFF, 0x01},   /* Gen3I2cSda */
    {0x000030D4, 0x1DFF, 0x02},   /* CamI2cScl */
    {0x000030D8, 0x1DFF, 0x02},   /* CamI2cSda */
    {0x000030DC, 0x1DFF, 0x01},   /* PwrI2cScl */
    {0x000030E0, 0x1DFF, 0x01},   /* PwrI2cSda */
    {0x000030E4, 0x19FF, 0x01},   /* Uart1Tx */
    {0x000030E8, 0x19FF, 0x01},   /* Uart1Rx */
    {0x000030EC, 0x19FF, 0x01},   /* Uart1Rts */
    {0x000030F0, 0x19FF, 0x01},   /* Uart1Cts */
    {0x000030F4, 0x19FF, 0x00},   /* Uart2Tx */
    {0x000030F8, 0x19FF, 0x00},   /* Uart2Rx */
    {0x000030FC, 0x19FF, 0x02},   /* Uart2Rts */
    {0x00003100, 0x19FF, 0x02},   /* Uart2Cts */
    {0x00003104, 0x19FF, 0x02},   /* Uart3Tx */
    {0x00003108, 0x19FF, 0x02},   /* Uart3Rx */
    {0x0000310C, 0x19FF, 0x02},   /* Uart3Rts */
    {0x00003110, 0x19FF, 0x02},   /* Uart3Cts */
    {0x00003114, 0x19FF, 0x02},   /* Uart4Tx */
    {0x00003118, 0x19FF, 0x02},   /* Uart4Rx */
    {0x0000311C, 0x19FF, 0x02},   /* Uart4Rts */
    {0x00003120, 0x19FF, 0x02},   /* Uart4Cts */
    {0x00003124, 0x72FF, 0x01},   /* Dap1Fs */
    {0x00003128, 0x72FF, 0x01},   /* Dap1Din */
    {0x0000312C, 0x72FF, 0x01},   /* Dap1Dout */
    {0x00003130, 0x72FF, 0x01},   /* Dap1Sclk */
    {0x00003134, 0x72FF, 0x01},   /* Dap2Fs */
    {0x00003138, 0x72FF, 0x01},   /* Dap2Din */
    {0x0000313C, 0x72FF, 0x01},   /* Dap2Dout */
    {0x00003140, 0x72FF, 0x01},   /* Dap2Sclk */
    {0x00003144, 0x72FF, 0x01},   /* Dap4Fs */
    {0x00003148, 0x72FF, 0x01},   /* Dap4Din */
    {0x0000314C, 0x72FF, 0x01},   /* Dap4Dout */
    {0x00003150, 0x72FF, 0x01},   /* Dap4Sclk */
    {0x00003154, 0x72FF, 0x01},   /* Cam1Mclk */
    {0x00003158, 0x72FF, 0x01},   /* Cam2Mclk */
    {0x0000315C, 0x72FF, 0x01},   /* JtagRtck */
    {0x00003160, 0x118C, 0xFF},   /* Clk32kIn */
    {0x00003164, 0x72FF, 0x02},   /* Clk32kOut */
    {0x00003168, 0x1DFF, 0x01},   /* BattBcl */
    {0x0000316C, 0x11CC, 0xFF},   /* ClkReq */
    {0x00003170, 0x11CC, 0xFF},   /* CpuPwrReq */
    {0x00003174, 0x11CC, 0xFF},   /* PwrIntN */
    {0x00003178, 0x11CC, 0xFF},   /* Shutdown */
    {0x0000317C, 0x11CC, 0xFF},   /* CorePwrReq */
    {0x00003180, 0x19FF, 0x01},   /* AudMclk */
    {0x00003184, 0x19FF, 0x00},   /* DvfsPwm */
    {0x00003188, 0x19FF, 0x00},   /* DvfsClk */
    {0x0000318C, 0x19FF, 0x00},   /* GpioX1Aud */
    {0x00003190, 0x19FF, 0x00},   /* GpioX3Aud */
    {0x00003194, 0x1DFF, 0x00},   /* GpioPcc7 */
    {0x00003198, 0x1DFF, 0x01},   /* HdmiCec */
    {0x0000319C, 0x1DFF, 0x01},   /* HdmiIntDpHpd */
    {0x000031A0, 0x19FF, 0x01},   /* SpdifOut */
    {0x000031A4, 0x19FF, 0x01},   /* SpdifIn */
    {0x000031A8, 0x1DFF, 0x01},   /* UsbVbusEn0 */
    {0x000031AC, 0x1DFF, 0x01},   /* UsbVbusEn1 */
    {0x000031B0, 0x19FF, 0x01},   /* DpHpd0 */
    {0x000031B4, 0x19FF, 0x00},   /* WifiEn */
    {0x000031B8, 0x19FF, 0x00},   /* WifiRst */
    {0x000031BC, 0x19FF, 0x00},   /* WifiWakeAp */
    {0x000031C0, 0x19FF, 0x00},   /* ApWakeBt */
    {0x000031C4, 0x19FF, 0x00},   /* BtRst */
    {0x000031C8, 0x19FF, 0x00},   /* BtWakeAp */
    {0x000031CC, 0x19FF, 0x00},   /* ApWakeNfc */
    {0x000031D0, 0x19FF, 0x00},   /* NfcEn */
    {0x000031D4, 0x19FF, 0x00},   /* NfcInt */
    {0x000031D8, 0x19FF, 0x00},   /* GpsEn */
    {0x000031DC, 0x19FF, 0x00},   /* GpsRst */
    {0x000031E0, 0x19FF, 0x01},   /* CamRst */
    {0x000031E4, 0x19FF, 0x02},   /* CamAfEn */
    {0x000031E8, 0x19FF, 0x02},   /* CamFlashEn */
    {0x000031EC, 0x19FF, 0x01},   /* Cam1Pwdn */
    {0x000031F0, 0x19FF, 0x01},   /* Cam2Pwdn */
    {0x000031F4, 0x19FF, 0x01},   /* Cam1Strobe */
    {0x000031F8, 0x19FF, 0x01},   /* LcdTe */
    {0x000031FC, 0x19FF, 0x03},   /* LcdBlPwm */
    {0x00003200, 0x19FF, 0x00},   /* LcdBlEn */
    {0x00003204, 0x19FF, 0x00},   /* LcdRst */
    {0x00003208, 0x19FF, 0x01},   /* LcdGpio1 */
    {0x0000320C, 0x19FF, 0x02},   /* LcdGpio2 */
    {0x00003210, 0x19FF, 0x00},   /* ApReady */
    {0x00003214, 0x19FF, 0x00},   /* TouchRst */
    {0x00003218, 0x19FF, 0x01},   /* TouchClk */
    {0x0000321C, 0x19FF, 0x00},   /* ModemWakeAp */
    {0x00003220, 0x19FF, 0x00},   /* TouchInt */
    {0x00003224, 0x19FF, 0x00},   /* MotionInt */
    {0x00003228, 0x19FF, 0x00},   /* AlsProxInt */
    {0x0000322C, 0x19FF, 0x00},   /* TempAlert */
    {0x00003230, 0x19FF, 0x00},   /* ButtonPowerOn */
    {0x00003234, 0x19FF, 0x00},   /* ButtonVolUp */
    {0x00003238, 0x19FF, 0x00},   /* ButtonVolDown */
    {0x0000323C, 0x19FF, 0x00},   /* ButtonSlideSw */
    {0x00003240, 0x19FF, 0x00},   /* ButtonHome */
    {0x00003244, 0x19FF, 0x01},   /* GpioPa6 */
    {0x00003248, 0x19FF, 0x00},   /* GpioPe6 */
    {0x0000324C, 0x19FF, 0x00},   /* GpioPe7 */
    {0x00003250, 0x19FF, 0x00},   /* GpioPh6 */
    {0x00003254, 0x72FF, 0x02},   /* GpioPk0 */
    {0x00003258, 0x72FF, 0x02},   /* GpioPk1 */
    {0x0000325C, 0x72FF, 0x02},   /* GpioPk2 */
    {0x00003260, 0x72FF, 0x02},   /* GpioPk3 */
    {0x00003264, 0x72FF, 0x01},   /* GpioPk4 */
    {0x00003268, 0x72FF, 0x01},   /* GpioPk5 */
    {0x0000326C, 0x72FF, 0x01},   /* GpioPk6 */
    {0x00003270, 0x72FF, 0x01},   /* GpioPk7 */
    {0x00003274, 0x72FF, 0x00},   /* GpioPl0 */
    {0x00003278, 0x72FF, 0x01},   /* GpioPl1 */
    {0x0000327C, 0x72FF, 0x01},   /* GpioPz0 */
    {0x00003280, 0x72FF, 0x02},   /* GpioPz1 */
    {0x00003284, 0x72FF, 0x02},   /* GpioPz2 */
    {0x00003288, 0x72FF, 0x01},   /* GpioPz3 */
    {0x0000328C, 0x72FF, 0x01},   /* GpioPz4 */
    {0x00003290, 0x72FF, 0x01},   /* GpioPz5 */
};

static int pinmux_update_park(u64 pinmux_base_vaddr, unsigned int pinmux_idx) {
    /* Fetch this PINMUX's register offset */
    u32 pinmux_reg_offset = std::get<0>(g_pinmux_map[pinmux_idx]);
    
    /* Fetch this PINMUX's mask value */
    u32 pinmux_mask_val = std::get<1>(g_pinmux_map[pinmux_idx]);
    
    /* Read from the PINMUX register */
    u32 pinmux_val = *((u32 *)pinmux_base_vaddr + pinmux_reg_offset);
    
    /* This PINMUX supports park change */
    if (pinmux_mask_val & 0x20) {
        /* Clear park status if set */
        if (pinmux_val & 0x20) {
            pinmux_val &= ~(0x20);
        }
    }
    
    /* Write to the appropriate PINMUX register */
    *((u32 *)pinmux_base_vaddr + pinmux_reg_offset) = pinmux_val;
    
    /* Do a dummy read from the PINMUX register */
    pinmux_val = *((u32 *)pinmux_base_vaddr + pinmux_reg_offset);

    return pinmux_val;
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    
    Result rc;
    u64 pinmux_base_vaddr = 0;
    u64 gpio_base_vaddr = 0;
    u64 pmc_base_vaddr = 0;
    
    /* Map the APB MISC registers for PINMUX */
    rc = svcQueryIoMapping(&pinmux_base_vaddr, APB_MISC_BASE, 0x4000);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* IO mapping failed */
    if (!pinmux_base_vaddr)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
    
    /* Map the GPIO registers */
    rc = svcQueryIoMapping(&gpio_base_vaddr, GPIO_BASE, 0x1000);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* IO mapping failed */
    if (!gpio_base_vaddr)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
    
    /* Change GPIO voltage to 1.8v */
    if (kernelAbove200()) {
        u32 rw_reg_val = 0;
        
        /* Use svcReadWriteRegister to write APBDEV_PMC_PWR_DET_0 */
        rc = svcReadWriteRegister(&rw_reg_val, PMC_BASE + 0x48, 0xA42000, 0xA42000);
        if (R_FAILED(rc)) {
            return rc;
        }
        
        /* Use svcReadWriteRegister to write APBDEV_PMC_PWR_DET_VAL_0 */
        rc = svcReadWriteRegister(&rw_reg_val, PMC_BASE + 0xE4, 0, 0xA42000);
        if (R_FAILED(rc)) {
            return rc;
        }
    } else {        
        /* Map the PMC registers directly */
        rc = svcQueryIoMapping(&pmc_base_vaddr, PMC_BASE, 0x3000);
        if (R_FAILED(rc)) {
            return rc;
        }
        
        /* IO mapping failed */
        if (!pmc_base_vaddr)
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
        
        /* Write to APBDEV_PMC_PWR_DET_0 */
        *((u32 *)pmc_base_vaddr + 0x48) |= 0xA42000;
        
        /* Write to APBDEV_PMC_PWR_DET_VAL_0 */
        *((u32 *)pmc_base_vaddr + 0xE4) &= ~(0xA42000);
    }
    
    /* Wait for changes to take effect */
    svcSleepThread(100000);
    
    /* Setup all GPIOs from 0x01 to 0x3C */
    for (unsigned int i = 1; i < MAX_GPIOS; i++) {
        gpio_configure(gpio_base_vaddr, i);
        gpio_set_direction(gpio_base_vaddr, i);
        gpio_set_value(gpio_base_vaddr, i);
    }
    
    u64 hardware_type = 0;
    
    /* Get the hardware type from SPL */
    rc = splGetConfig(SplConfigItem_HardwareType, &hardware_type);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* This is ignored in Copper hardware */
    if (hardware_type != 0x01) {
        /* TODO: Battery charge check */
    }
    
    /* Update PINMUX park status */
    for (unsigned int i = 0; i < MAX_PINMUX; i++) {
        pinmux_update_park(pinmux_base_vaddr, i);
    }
    
    /* TODO: Set initial PINMUX configuration */
    /* TODO: Set initial PINMUX drive pad configuration */
    /* TODO: Set initial wake pin configuration */
    
    /* This is ignored in Copper hardware */
    if (hardware_type != 0x01) {
        /* Configure PMC clock out */
        if (kernelAbove200()) {
            u32 rw_reg_val = 0;
            
            /* Use svcReadWriteRegister to write APBDEV_PMC_CLK_OUT_CNTRL_0 */
            rc = svcReadWriteRegister(&rw_reg_val, PMC_BASE + 0x1A8, 0xC4, 0xC4);
            if (R_FAILED(rc)) {
                return rc;
            }
        } else {
            /* Write to APBDEV_PMC_CLK_OUT_CNTRL_0 */
            *((u32 *)pmc_base_vaddr + 0x1A8) |= 0xC4;
        }
    }
    
    /* Setup GPIO 0x4B for Copper hardware only */
    if (hardware_type == 0x01) {
        gpio_configure(gpio_base_vaddr, 0x4B);
        gpio_set_direction(gpio_base_vaddr, 0x4B);
        gpio_set_value(gpio_base_vaddr, 0x4B);
    }
    
    /* TODO: NAND repair */
    
    /* pmshellNotifyBootFinished(); */
    
    rc = 0;
    return rc;
}

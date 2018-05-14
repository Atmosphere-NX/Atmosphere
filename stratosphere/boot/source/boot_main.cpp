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
#define MAX_PINMUX_5X 0xAF

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
    
    fsdevMountSdmc();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
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
    
static const std::tuple<u32, u32, u32> g_pinmux_map[] = {
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
    
    /* 5.0.0+ only */
    {0x00003294, 0x1F2FF, 0x02},   /* Sdmmc2Dat0 */
    {0x00003298, 0x1F2FF, 0x02},   /* Sdmmc2Dat1 */
    {0x0000329C, 0x1F2FF, 0x02},   /* Sdmmc2Dat2 */
    {0x000032A0, 0x1F2FF, 0x02},   /* Sdmmc2Dat3 */
    {0x000032A4, 0x1F2FF, 0x02},   /* Sdmmc2Dat4 */
    {0x000032A8, 0x1F2FF, 0x02},   /* Sdmmc2Dat5 */
    {0x000032AC, 0x1F2FF, 0x02},   /* Sdmmc2Dat6 */
    {0x000032B0, 0x1F2FF, 0x02},   /* Sdmmc2Dat7 */
    {0x000032B4, 0x1F2FF, 0x02},   /* Sdmmc2Clk */
    {0x000032B8, 0x1F2FF, 0x00},   /* Sdmmc2Clkb */
    {0x000032BC, 0x1F2FF, 0x02},   /* Sdmmc2Cmd */
    {0x000032C0, 0x1F2FF, 0x00},   /* Sdmmc2Dqs */
    {0x000032C4, 0x1F2FF, 0x00},   /* Sdmmc2Dqsb */
};

static const std::tuple<u32, u32, u32> g_pinmux_config_map_icosa[] = {
    {0x5D, 0x00, 0x67},
    {0x47, 0x28, 0x7F},
    {0x48, 0x00, 0x67},
    {0x46, 0x00, 0x67},
    {0x49, 0x00, 0x67},
    {0x30, 0x40, 0x27F},
    {0x31, 0x40, 0x27F},
    {0x0D, 0x20, 0x27F},
    {0x0C, 0x00, 0x267},
    {0x10, 0x20, 0x27F},
    {0x0F, 0x00, 0x267},
    {0x0E, 0x20, 0x27F},
    {0x00, 0x48, 0x7F},
    {0x01, 0x50, 0x7F},
    {0x05, 0x50, 0x7F},
    {0x04, 0x50, 0x7F},
    {0x03, 0x50, 0x7F},
    {0x02, 0x50, 0x7F},
    {0x5B, 0x00, 0x78},
    {0x7C, 0x01, 0x67},
    {0x80, 0x01, 0x7F},
    {0x34, 0x40, 0x27F},
    {0x35, 0x40, 0x27F},
    {0x55, 0x20, 0x78},
    {0x56, 0x20, 0x7F},
    {0xA1, 0x30, 0x7F},
    {0x5C, 0x00, 0x78},
    {0x59, 0x00, 0x60},
    {0x5A, 0x30, 0x78},
    {0x2C, 0x40, 0x27F},
    {0x2D, 0x40, 0x27F},
    {0x2E, 0x40, 0x27F},
    {0x2F, 0x40, 0x27F},
    {0x3B, 0x20, 0x7F},
    {0x3C, 0x00, 0x67},
    {0x3D, 0x20, 0x7F},
    {0x36, 0x00, 0x67},
    {0x37, 0x30, 0x7F},
    {0x38, 0x00, 0x67},
    {0x39, 0x28, 0x7F},
    {0x54, 0x00, 0x67},
    {0x9B, 0x30, 0x7F},
    {0x1C, 0x00, 0x67},
    {0x1D, 0x30, 0x7F},
    {0x1E, 0x00, 0x67},
    {0x1F, 0x00, 0x67},
    {0x3F, 0x20, 0x7F},
    {0x40, 0x00, 0x67},
    {0x41, 0x20, 0x7F},
    {0x42, 0x00, 0x67},
    {0x43, 0x28, 0x7F},
    {0x44, 0x00, 0x67},
    {0x45, 0x28, 0x7F},
    {0x22, 0x00, 0x67},
    {0x23, 0x28, 0x7F},
    {0x20, 0x00, 0x67},
    {0x21, 0x00, 0x67},
    {0x4B, 0x28, 0x7F},
    {0x4C, 0x00, 0x67},
    {0x4A, 0x00, 0x67},
    {0x4D, 0x00, 0x67},
    {0x64, 0x20, 0x27F},
    {0x5F, 0x34, 0x7F},
    {0x60, 0x04, 0x67},
    {0x61, 0x2C, 0x7F},
    {0x2A, 0x04, 0x67},
    {0x2B, 0x04, 0x67},
    {0x8F, 0x24, 0x7F},
    {0x33, 0x34, 0x27F},
    {0x52, 0x2C, 0x7F},
    {0x53, 0x24, 0x7F},
    {0x77, 0x04, 0x67},
    {0x78, 0x34, 0x7F},
    {0x11, 0x04, 0x67},
    {0x06, 0x2C, 0x7F},
    {0x08, 0x24, 0x7F},
    {0x09, 0x24, 0x7F},
    {0x0A, 0x24, 0x7F},
    {0x0B, 0x24, 0x7F},
    {0x88, 0x34, 0x7F},
    {0x86, 0x2C, 0x7F},
    {0x82, 0x24, 0x7F},
    {0x85, 0x34, 0x7F},
    {0x89, 0x24, 0x7F},
    {0x8A, 0x34, 0x7F},
    {0x8B, 0x34, 0x7F},
    {0x8C, 0x34, 0x7F},
    {0x8D, 0x24, 0x7F},
    {0x7D, 0x04, 0x67},
    {0x7E, 0x04, 0x67},
    {0x81, 0x04, 0x67},
    {0x9C, 0x34, 0x7F},
    {0x9D, 0x34, 0x7F},
    {0x9E, 0x2C, 0x7F},
    {0x9F, 0x34, 0x7F},
    {0xA0, 0x04, 0x67},
    {0x4F, 0x04, 0x67},
    {0x51, 0x04, 0x67},
    {0x3A, 0x24, 0x7F},
    {0x92, 0x4C, 0x7F},
    {0x93, 0x4C, 0x7F},
    {0x94, 0x44, 0x7F},
    {0x95, 0x04, 0x67},
    {0x96, 0x34, 0x7F},
    {0x97, 0x04, 0x67},
    {0x98, 0x34, 0x7F},
    {0x99, 0x34, 0x7F},
    {0x9A, 0x04, 0x67},
    {0x3E, 0x24, 0x7F},
    {0x6A, 0x04, 0x67},
    {0x6B, 0x04, 0x67},
    {0x6C, 0x2C, 0x7F},
    {0x6D, 0x04, 0x67},
    {0x6E, 0x04, 0x67},
    {0x6F, 0x24, 0x7F},
    {0x91, 0x24, 0x7F},
    {0x70, 0x04, 0x7F},
    {0x71, 0x04, 0x67},
    {0x72, 0x04, 0x67},
    {0x65, 0x34, 0x7F},
    {0x66, 0x04, 0x67},
    {0x67, 0x04, 0x267},
    {0x5E, 0x05, 0x07},
    {0x17, 0x05, 0x07},
    {0x18, 0x05, 0x07},
    {0x19, 0x05, 0x07},
    {0x1A, 0x05, 0x07},
    {0x1B, 0x05, 0x07},
    {0x26, 0x05, 0x07},
    {0x27, 0x05, 0x07},
    {0x28, 0x05, 0x07},
    {0x29, 0x05, 0x07},
    {0x90, 0x05, 0x07},
    {0x32, 0x05, 0x07},
    {0x75, 0x05, 0x07},
    {0x76, 0x05, 0x07},
    {0x79, 0x05, 0x07},
    {0x7A, 0x05, 0x07},
    {0x8E, 0x05, 0x07},
    {0x07, 0x05, 0x07},
    {0x87, 0x05, 0x07},
    {0x83, 0x05, 0x07},
    {0x84, 0x05, 0x07},
    {0x7B, 0x05, 0x07},
    {0x7F, 0x05, 0x07},
    {0x58, 0x00, 0x00},
    {0x50, 0x05, 0x07},
    {0x4E, 0x05, 0x07},
    {0x12, 0x05, 0x07},
    {0x13, 0x05, 0x07},
    {0x14, 0x05, 0x07},
    {0x15, 0x05, 0x07},
    {0x16, 0x05, 0x07},
    {0x73, 0x05, 0x07},
    {0x74, 0x05, 0x07},
    {0x24, 0x05, 0x07},
    {0x25, 0x05, 0x07},
    {0x62, 0x05, 0x07},
    {0x68, 0x05, 0x07},
    {0x69, 0x05, 0x07},
    {0x63, 0x05, 0x07},
};

static const std::tuple<u32, u32, u32> g_pinmux_config_map_copper[] = {
    {0x10, 0x20, 0x27F},
    {0x0F, 0x00, 0x267},
    {0x0E, 0x20, 0x27F},
    {0x5B, 0x00, 0x00},
    {0x80, 0x01, 0x7F},
    {0x34, 0x40, 0x267},
    {0x35, 0x40, 0x267},
    {0x55, 0x00, 0x18},
    {0x56, 0x01, 0x67},
    {0x5C, 0x00, 0x00},
    {0x59, 0x00, 0x00},
    {0x5A, 0x10, 0x18},
    {0x2C, 0x40, 0x267},
    {0x2D, 0x40, 0x267},
    {0x2E, 0x40, 0x267},
    {0x2F, 0x40, 0x267},
    {0x36, 0x00, 0x67},
    {0x37, 0x30, 0x7F},
    {0x38, 0x00, 0x67},
    {0x39, 0x28, 0x7F},
    {0x54, 0x00, 0x67},
    {0x9B, 0x30, 0x7F},
    {0x42, 0x00, 0x67},
    {0x43, 0x28, 0x7F},
    {0x44, 0x00, 0x67},
    {0x45, 0x28, 0x7F},
    {0x4B, 0x28, 0x7F},
    {0x4C, 0x00, 0x67},
    {0x4A, 0x00, 0x67},
    {0x4D, 0x00, 0x67},
    {0x64, 0x20, 0x27F},
    {0x63, 0x40, 0x267},
    {0x5E, 0x04, 0x67},
    {0x60, 0x04, 0x67},
    {0x17, 0x24, 0x7F},
    {0x18, 0x24, 0x7F},
    {0x27, 0x04, 0x67},
    {0x2A, 0x04, 0x67},
    {0x2B, 0x04, 0x67},
    {0x90, 0x24, 0x7F},
    {0x32, 0x24, 0x27F},
    {0x33, 0x34, 0x27F},
    {0x76, 0x04, 0x67},
    {0x79, 0x04, 0x67},
    {0x08, 0x24, 0x7F},
    {0x09, 0x24, 0x7F},
    {0x0A, 0x24, 0x7F},
    {0x0B, 0x24, 0x7F},
    {0x88, 0x34, 0x7F},
    {0x89, 0x24, 0x7F},
    {0x8A, 0x34, 0x7F},
    {0x8B, 0x34, 0x7F},
    {0x8D, 0x34, 0x7F},
    {0x81, 0x04, 0x67},
    {0x9D, 0x34, 0x7F},
    {0x9F, 0x34, 0x7F},
    {0xA1, 0x34, 0x7F},
    {0x92, 0x4C, 0x7F},
    {0x93, 0x4C, 0x7F},
    {0x94, 0x44, 0x7F},
    {0x96, 0x34, 0x7F},
    {0x98, 0x34, 0x7F},
    {0x99, 0x34, 0x7F},
    {0x12, 0x04, 0x7F},
    {0x13, 0x04, 0x67},
    {0x14, 0x04, 0x7F},
    {0x6A, 0x04, 0x67},
    {0x6B, 0x04, 0x67},
    {0x6C, 0x2C, 0x7F},
    {0x6D, 0x04, 0x67},
    {0x6E, 0x04, 0x67},
    {0x6F, 0x24, 0x7F},
    {0x70, 0x04, 0x7F},
    {0x73, 0x04, 0x67},
    {0x69, 0x24, 0x7F},
    {0x5D, 0x05, 0x07},
    {0x5F, 0x05, 0x07},
    {0x61, 0x05, 0x07},
    {0x47, 0x05, 0x07},
    {0x48, 0x05, 0x07},
    {0x46, 0x05, 0x07},
    {0x49, 0x05, 0x07},
    {0x19, 0x05, 0x07},
    {0x1A, 0x05, 0x07},
    {0x1B, 0x05, 0x07},
    {0x26, 0x05, 0x07},
    {0x28, 0x05, 0x07},
    {0x29, 0x05, 0x07},
    {0x8F, 0x05, 0x07},
    {0x30, 0x05, 0x07},
    {0x31, 0x05, 0x07},
    {0x52, 0x05, 0x07},
    {0x53, 0x05, 0x07},
    {0x75, 0x05, 0x07},
    {0x77, 0x05, 0x07},
    {0x78, 0x05, 0x07},
    {0x7A, 0x05, 0x07},
    {0x0D, 0x05, 0x07},
    {0x0C, 0x05, 0x07},
    {0x11, 0x05, 0x07},
    {0x8E, 0x05, 0x07},
    {0x00, 0x05, 0x07},
    {0x01, 0x05, 0x07},
    {0x05, 0x05, 0x07},
    {0x04, 0x05, 0x07},
    {0x03, 0x05, 0x07},
    {0x02, 0x05, 0x07},
    {0x06, 0x05, 0x07},
    {0x07, 0x05, 0x07},
    {0x87, 0x05, 0x07},
    {0x86, 0x05, 0x07},
    {0x82, 0x05, 0x07},
    {0x83, 0x05, 0x07},
    {0x85, 0x05, 0x07},
    {0x84, 0x05, 0x07},
    {0x8C, 0x05, 0x07},
    {0x7B, 0x05, 0x07},
    {0x7C, 0x05, 0x07},
    {0x7D, 0x05, 0x07},
    {0x7E, 0x05, 0x07},
    {0x7F, 0x05, 0x07},
    {0x9C, 0x05, 0x07},
    {0x9E, 0x05, 0x07},
    {0xA0, 0x05, 0x07},
    {0x58, 0x00, 0x00},
    {0x4F, 0x05, 0x07},
    {0x50, 0x05, 0x07},
    {0x4E, 0x05, 0x07},
    {0x51, 0x05, 0x07},
    {0x3A, 0x05, 0x07},
    {0x3B, 0x05, 0x07},
    {0x3C, 0x05, 0x07},
    {0x3D, 0x05, 0x07},
    {0x95, 0x05, 0x07},
    {0x97, 0x05, 0x07},
    {0x9A, 0x05, 0x07},
    {0x15, 0x05, 0x07},
    {0x16, 0x05, 0x07},
    {0x1C, 0x05, 0x07},
    {0x1D, 0x05, 0x07},
    {0x1E, 0x05, 0x07},
    {0x1F, 0x05, 0x07},
    {0x3E, 0x05, 0x07},
    {0x3F, 0x05, 0x07},
    {0x40, 0x05, 0x07},
    {0x41, 0x05, 0x07},
    {0x91, 0x05, 0x07},
    {0x71, 0x05, 0x07},
    {0x72, 0x05, 0x07},
    {0x74, 0x05, 0x07},
    {0x22, 0x05, 0x07},
    {0x23, 0x05, 0x07},
    {0x20, 0x05, 0x07},
    {0x21, 0x05, 0x07},
    {0x24, 0x05, 0x07},
    {0x25, 0x05, 0x07},
    {0x62, 0x05, 0x07},
    {0x65, 0x05, 0x07},
    {0x66, 0x05, 0x07},
    {0x67, 0x05, 0x07},
    {0x68, 0x05, 0x07},
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

static int pinmux_update_pad(u64 pinmux_base_vaddr, unsigned int pinmux_idx, unsigned int pinmux_config_val, unsigned int pinmux_config_mask_val) {
    /* Fetch this PINMUX's register offset */
    u32 pinmux_reg_offset = std::get<0>(g_pinmux_map[pinmux_idx]);
    
    /* Fetch this PINMUX's mask value */
    u32 pinmux_mask_val = std::get<1>(g_pinmux_map[pinmux_idx]);
        
    /* Read from the PINMUX register */
    u32 pinmux_val = *((u32 *)pinmux_base_vaddr + pinmux_reg_offset);
    
    /* This PINMUX register is locked */
    if (pinmux_val & 0x80)
        return pinmux_val;
    
    u32 pm_config_val = (pinmux_config_val & 0x07);
    u32 pm_val = pm_config_val;
    
    /* Adjust PM */
    if (pinmux_config_mask_val & 0x07) {
        /* Default to safe value */
        if (pm_config_val >= 0x06)
            pm_val = 0x04;
        
        /* Apply additional changes first */
        if (pm_config_val == 0x05) {
            /* This pin supports PUPD change */
            if (pinmux_mask_val & 0x0C) {
                /* Change PUPD */
                if ((pinmux_val & 0x0C) != 0x04) {
                    pinmux_val &= 0xFFFFFFF3;
                    pinmux_val |= 0x04;
                }
            }
            
            /* This pin supports Tristate change */
            if (pinmux_mask_val & 0x10) {
                /* Change Tristate */
                if (!(pinmux_val & 0x10)) {
                    pinmux_val |= 0x10;
                }
            }
            
            /* This pin supports EInput change */
            if (pinmux_mask_val & 0x40) {
                /* Change EInput */
                if (pinmux_val & 0x40) {
                    pinmux_val &= 0xFFFFFFBF;
                }
            }
            
            /* Default to safe value */
            pm_val = 0x04;
        }
        
        /* Translate PM value if necessary */
        if ((pm_val & 0xFF) == 0x04)
            pm_val = std::get<2>(g_pinmux_map[pinmux_idx]);
        
        /* This pin supports PM change */
        if (pinmux_mask_val & 0x03) {
            /* Change PM */
            if ((pinmux_val & 0x03) != (pm_val & 0x03)) {
                pinmux_val &= 0xFFFFFFFC;
                pinmux_val |= (pm_val & 0x03);
            }
        }
    }
    
    u32 pupd_config_val = (pinmux_config_val & 0x18);
    
    /* Adjust PUPD */
    if (pinmux_config_mask_val & 0x18) {
        if (pupd_config_val < 0x11) {
            /* This pin supports PUPD change */
            if (pinmux_mask_val & 0x0C) {
                /* Change PUPD */
                if ((pinmux_val & 0x0C) != (pupd_config_val >> 0x03)) {
                    pinmux_val &= 0xFFFFFFF3;
                    pinmux_val |= (pupd_config_val >> 0x01);
                }
            }
        }
    }
    
    u32 eod_config_val = (pinmux_config_val & 0x60);
    
    /* Adjust EOd field */
    if (pinmux_config_mask_val & 0x60) {
        if (eod_config_val == 0x20) {
            /* This pin supports Tristate change */
            if (pinmux_mask_val & 0x10) {
                /* Change Tristate */
                if (!(pinmux_val & 0x10)) {
                    pinmux_val |= 0x10;
                }
            }
            
            /* This pin supports EInput change */
            if (pinmux_mask_val & 0x40) {
                /* Change EInput */
                if (!(pinmux_val & 0x40)) {
                    pinmux_val |= 0x40;
                }
            }
            
            /* This pin supports EOd change */
            if (pinmux_mask_val & 0x800) {
                /* Change EOd */
                if (pinmux_val & 0x800) {
                    pinmux_val &= 0xFFFFF7FF;
                }
            }
        } else if (eod_config_val == 0x40) {
            /* This pin supports Tristate change */
            if (pinmux_mask_val & 0x10) {
                /* Change Tristate */
                if (pinmux_val & 0x10) {
                    pinmux_val &= 0xFFFFFFEF;
                }
            }
            
            /* This pin supports EInput change */
            if (pinmux_mask_val & 0x40) {
                /* Change EInput */
                if (!(pinmux_val & 0x40)) {
                    pinmux_val |= 0x40;
                }
            }
            
            /* This pin supports EOd change */
            if (pinmux_mask_val & 0x800) {
                /* Change EOd */
                if (pinmux_val & 0x800) {
                    pinmux_val &= 0xFFFFF7FF;
                }
            }
        } else if (eod_config_val == 0x60) {
            /* This pin supports Tristate change */
            if (pinmux_mask_val & 0x10) {
                /* Change Tristate */
                if (pinmux_val & 0x10) {
                    pinmux_val &= 0xFFFFFFEF;
                }
            }
            
            /* This pin supports EInput change */
            if (pinmux_mask_val & 0x40) {
                /* Change EInput */
                if (!(pinmux_val & 0x40)) {
                    pinmux_val |= 0x40;
                }
            }
            
            /* This pin supports EOd change */
            if (pinmux_mask_val & 0x800) {
                /* Change EOd */
                if (!(pinmux_val & 0x800)) {
                    pinmux_val |= 0x800;
                }
            }
        } else {
            /* This pin supports Tristate change */
            if (pinmux_mask_val & 0x10) {
                /* Change Tristate */
                if (pinmux_val & 0x10) {
                    pinmux_val &= 0xFFFFFFEF;
                }
            }
            
            /* This pin supports EInput change */
            if (pinmux_mask_val & 0x40) {
                /* Change EInput */
                if (pinmux_val & 0x40) {
                    pinmux_val &= 0xFFFFFFBF;
                }
            }
            
            /* This pin supports EOd change */
            if (pinmux_mask_val & 0x800) {
                /* Change EOd */
                if (pinmux_val & 0x800) {
                    pinmux_val &= 0xFFFFF7FF;
                }
            }
        }
    }
    
    u32 lock_config_val = (pinmux_config_val & 0x80);
    
    /* Adjust Lock */
    if (pinmux_config_mask_val & 0x80) {
        /* This pin supports Lock change */
        if (pinmux_mask_val & 0x80) {
            /* Change Lock */
            if ((pinmux_val ^ pinmux_config_val) & 0x80) {
                pinmux_val &= 0xFFFFFF7F;
                pinmux_val |= lock_config_val;
            }
        }
    }
    
    u32 ioreset_config_val = ((pinmux_config_val >> 0x08) & 0x10000);
    
    /* Adjust IoReset */
    if (pinmux_config_mask_val & 0x100) {
        /* This pin supports IoReset change */
        if (pinmux_mask_val & 0x10000) {
            /* Change IoReset */
            if (((pinmux_val >> 0x10) ^ (pinmux_config_val >> 0x08)) & 0x01) {
                pinmux_val |= ioreset_config_val;
            }
        }
    }
    
    u32 park_config_val = ((pinmux_config_val >> 0x0A) & 0x20);
    
    /* Adjust Park */
    if (pinmux_config_mask_val & 0x400) {
        /* This pin supports Park change */
        if (pinmux_mask_val & 0x20) {
            /* Change Park */
            if (((pinmux_val >> 0x05) ^ (pinmux_config_val >> 0x0A)) & 0x01) {
                pinmux_val |= park_config_val;
            }
        }
    }
    
    u32 elpdr_config_val = ((pinmux_config_val >> 0x0B) & 0x100);
    
    /* Adjust ELpdr */
    if (pinmux_config_mask_val & 0x800) {
        /* This pin supports ELpdr change */
        if (pinmux_mask_val & 0x100) {
            /* Change ELpdr */
            if (((pinmux_val >> 0x08) ^ (pinmux_config_val >> 0x0B)) & 0x01) {
                pinmux_val |= elpdr_config_val;
            }
        }
    }
    
    u32 ehsm_config_val = ((pinmux_config_val >> 0x0C) & 0x200);
    
    /* Adjust EHsm */
    if (pinmux_config_mask_val & 0x1000) {
        /* This pin supports EHsm change */
        if (pinmux_mask_val & 0x200) {
            /* Change EHsm */
            if (((pinmux_val >> 0x09) ^ (pinmux_config_val >> 0x0C)) & 0x01) {
                pinmux_val |= ehsm_config_val;
            }
        }
    }
    
    u32 eiohv_config_val = ((pinmux_config_val >> 0x09) & 0x400);
    
    /* Adjust EIoHv */
    if (pinmux_config_mask_val & 0x200) {
        /* This pin supports EIoHv change */
        if (pinmux_mask_val & 0x400) {
            /* Change EIoHv */
            if (((pinmux_val >> 0x0A) ^ (pinmux_config_val >> 0x09)) & 0x01) {
                pinmux_val |= eiohv_config_val;
            }
        }
    }
    
    u32 eschmt_config_val = ((pinmux_config_val >> 0x0D) & 0x1000);
    
    /* Adjust ESchmt */
    if (pinmux_config_mask_val & 0x2000) {
        /* This pin supports ESchmt change */
        if (pinmux_mask_val & 0x1000) {
            /* Change ESchmt */
            if (((pinmux_val >> 0x0C) ^ (pinmux_config_val >> 0x0D)) & 0x01) {
                pinmux_val |= eschmt_config_val;
            }
        }
    }
    
    u32 preemp_config_val = ((pinmux_config_val >> 0x0D) & 0x8000);
    
    /* Adjust Preemp */
    if (pinmux_config_mask_val & 0x10000) {
        
        /* This pin supports Preemp change */
        if (pinmux_mask_val & 0x8000) {
            /* Change Preemp */
            if (((pinmux_val >> 0x0F) ^ (pinmux_config_val >> 0x10)) & 0x01) {
                pinmux_val |= preemp_config_val;
            }
        }
    }
    
    u32 drvtype_config_val = ((pinmux_config_val >> 0x0E) & 0x6000);
    
    /* Adjust DrvType */
    if (pinmux_config_mask_val & 0xC000) {
        /* This pin supports DrvType change */
        if (pinmux_mask_val & 0x6000) {
            /* Change DrvType */
            if (((pinmux_val >> 0x0D) ^ (pinmux_config_val >> 0x0E)) & 0x03) {
                pinmux_val |= drvtype_config_val;
            }
        }
    }
    
    /* Write to the appropriate PINMUX register */
    *((u32 *)pinmux_base_vaddr + pinmux_reg_offset) = pinmux_val;
    
    /* Do a dummy read from the PINMUX register */
    pinmux_val = *((u32 *)pinmux_base_vaddr + pinmux_reg_offset);

    return pinmux_val;
}

static const std::tuple<u32, u32, u32> g_drivepad_map[] = {
    {0x000008E4, 0x01F1F000},   /* AlsProxInt */
    {0x000008E8, 0x01F1F000},   /* ApReady */
    {0x000008EC, 0x01F1F000},   /* ApWakeBt */
    {0x000008F0, 0x01F1F000},   /* ApWakeNfc */
    {0x000008F4, 0x01F1F000},   /* AudMclk */
    {0x000008F8, 0x01F1F000},   /* BattBcl */
    {0x000008FC, 0x01F1F000},   /* BtRst */
    {0x00000900, 0x01F1F000},   /* BtWakeAp */
    {0x00000904, 0x01F1F000},   /* ButtonHome */
    {0x00000908, 0x01F1F000},   /* ButtonPowerOn */
    {0x0000090C, 0x01F1F000},   /* ButtonSlideSw */
    {0x00000910, 0x01F1F000},   /* ButtonVolDown */
    {0x00000914, 0x01F1F000},   /* ButtonVolUp */
    {0x00000918, 0x01F1F000},   /* Cam1Mclk */
    {0x0000091C, 0x01F1F000},   /* Cam1Pwdn */
    {0x00000920, 0x01F1F000},   /* Cam1Strobe */
    {0x00000924, 0x01F1F000},   /* Cam2Mclk */
    {0x00000928, 0x01F1F000},   /* Cam2Pwdn */
    {0x0000092C, 0x01F1F000},   /* CamAfEn */
    {0x00000930, 0x01F1F000},   /* CamFlashEn */
    {0x00000934, 0x01F1F000},   /* CamI2cScl */
    {0x00000938, 0x01F1F000},   /* CamI2cSda */
    {0x0000093C, 0x01F1F000},   /* CamRst */
    {0x00000940, 0x01F1F000},   /* Clk32kIn */
    {0x00000944, 0x01F1F000},   /* Clk32kOut */
    {0x00000948, 0x01F1F000},   /* ClkReq */
    {0x0000094C, 0x01F1F000},   /* CorePwrReq */
    {0x00000950, 0x01F1F000},   /* CpuPwrReq */
    {0x00000954, 0xF0000000},   /* Dap1Din */
    {0x00000958, 0xF0000000},   /* Dap1Dout */
    {0x0000095C, 0xF0000000},   /* Dap1Fs */
    {0x00000960, 0xF0000000},   /* Dap1Sclk */
    {0x00000964, 0xF0000000},   /* Dap2Din */
    {0x00000968, 0xF0000000},   /* Dap2Dout */
    {0x0000096C, 0xF0000000},   /* Dap2Fs */
    {0x00000970, 0xF0000000},   /* Dap2Sclk */
    {0x00000974, 0x01F1F000},   /* Dap4Din */
    {0x00000978, 0x01F1F000},   /* Dap4Dout */
    {0x0000097C, 0x01F1F000},   /* Dap4Fs */
    {0x00000980, 0x01F1F000},   /* Dap4Sclk */
    {0x00000984, 0x01F1F000},   /* Dmic1Clk */
    {0x00000988, 0x01F1F000},   /* Dmic1Dat */
    {0x0000098C, 0x01F1F000},   /* Dmic2Clk */
    {0x00000990, 0x01F1F000},   /* Dmic2Dat */
    {0x00000994, 0x01F1F000},   /* Dmic3Clk */
    {0x00000998, 0x01F1F000},   /* Dmic3Dat */
    {0x0000099C, 0x01F1F000},   /* DpHpd */
    {0x000009A0, 0x01F1F000},   /* DvfsClk */
    {0x000009A4, 0x01F1F000},   /* DvfsPwm */
    {0x000009A8, 0x01F1F000},   /* Gen1I2cScl */
    {0x000009AC, 0x01F1F000},   /* Gen1I2cSda */
    {0x000009B0, 0x01F1F000},   /* Gen2I2cScl */
    {0x000009B4, 0x01F1F000},   /* Gen2I2cSda */
    {0x000009B8, 0x01F1F000},   /* Gen3I2cScl */
    {0x000009BC, 0x01F1F000},   /* Gen3I2cSda */
    {0x000009C0, 0x01F1F000},   /* GpioPa6 */
    {0x000009C4, 0x01F1F000},   /* GpioPcc7 */
    {0x000009C8, 0x01F1F000},   /* GpioPe6 */
    {0x000009CC, 0x01F1F000},   /* GpioPe7 */
    {0x000009D0, 0x01F1F000},   /* GpioPh6 */
    {0x000009D4, 0xF0000000},   /* GpioPk0 */
    {0x000009D8, 0xF0000000},   /* GpioPk1 */
    {0x000009DC, 0xF0000000},   /* GpioPk2 */
    {0x000009E0, 0xF0000000},   /* GpioPk3 */
    {0x000009E4, 0xF0000000},   /* GpioPk4 */
    {0x000009E8, 0xF0000000},   /* GpioPk5 */
    {0x000009EC, 0xF0000000},   /* GpioPk6 */
    {0x000009F0, 0xF0000000},   /* GpioPk7 */
    {0x000009F4, 0xF0000000},   /* GpioPl0 */
    {0x000009F8, 0xF0000000},   /* GpioPl1 */
    {0x000009FC, 0x07F7F000},   /* GpioPz0 */
    {0x00000A00, 0x07F7F000},   /* GpioPz1 */
    {0x00000A04, 0x07F7F000},   /* GpioPz2 */
    {0x00000A08, 0x07F7F000},   /* GpioPz3 */
    {0x00000A0C, 0x07F7F000},   /* GpioPz4 */
    {0x00000A10, 0x07F7F000},   /* GpioPz5 */
    {0x00000A14, 0x01F1F000},   /* GpioX1Aud */
    {0x00000A18, 0x01F1F000},   /* GpioX3Aud */
    {0x00000A1C, 0x01F1F000},   /* GpsEn */
    {0x00000A20, 0x01F1F000},   /* GpsRst */
    {0x00000A24, 0x01F1F000},   /* HdmiCec */
    {0x00000A28, 0x01F1F000},   /* HdmiIntDpHpd */
    {0x00000A2C, 0x01F1F000},   /* JtagRtck */
    {0x00000A30, 0x01F1F000},   /* LcdBlEn */
    {0x00000A34, 0x01F1F000},   /* LcdBlPwm */
    {0x00000A38, 0x01F1F000},   /* LcdGpio1 */
    {0x00000A3C, 0x01F1F000},   /* LcdGpio2 */
    {0x00000A40, 0x01F1F000},   /* LcdRst */
    {0x00000A44, 0x01F1F000},   /* LcdTe */
    {0x00000A48, 0x01F1F000},   /* ModemWakeAp */
    {0x00000A4C, 0x01F1F000},   /* MotionInt */
    {0x00000A50, 0x01F1F000},   /* NfcEn */
    {0x00000A54, 0x01F1F000},   /* NfcInt */
    {0x00000A58, 0x01F1F000},   /* PexL0ClkReqN */
    {0x00000A5C, 0x01F1F000},   /* PexL0RstN */
    {0x00000A60, 0x01F1F000},   /* PexL1ClkreqN */
    {0x00000A64, 0x01F1F000},   /* PexL1RstN */
    {0x00000A68, 0x01F1F000},   /* PexWakeN */
    {0x00000A6C, 0x01F1F000},   /* PwrI2cScl */
    {0x00000A70, 0x01F1F000},   /* PwrI2cSda */
    {0x00000A74, 0x01F1F000},   /* PwrIntN */
    {0x00000A78, 0x07F7F000},   /* QspiComp */
    {0x00000A90, 0xF0000000},   /* QspiSck */
    {0x00000A94, 0x01F1F000},   /* SataLedActive */
    {0x00000A98, 0xF7F7F000},   /* Sdmmc1Pad */
    {0x00000AB0, 0xF7F7F000},   /* Sdmmc3Pad */
    {0x00000AC8, 0x01F1F000},   /* Shutdown */
    {0x00000ACC, 0x01F1F000},   /* SpdifIn */
    {0x00000AD0, 0x01F1F000},   /* SpdifOut */
    {0x00000AD4, 0xF0000000},   /* Spi1Cs0 */
    {0x00000AD8, 0xF0000000},   /* Spi1Cs1 */
    {0x00000ADC, 0xF0000000},   /* Spi1Miso */
    {0x00000AE0, 0xF0000000},   /* Spi1Mosi */
    {0x00000AE4, 0xF0000000},   /* Spi1Sck */
    {0x00000AE8, 0xF0000000},   /* Spi2Cs0 */
    {0x00000AEC, 0xF0000000},   /* Spi2Cs1 */
    {0x00000AF0, 0xF0000000},   /* Spi2Miso */
    {0x00000AF4, 0xF0000000},   /* Spi2Mosi */
    {0x00000AF8, 0xF0000000},   /* Spi2Sck */
    {0x00000AFC, 0xF0000000},   /* Spi4Cs0 */
    {0x00000B00, 0xF0000000},   /* Spi4Miso */
    {0x00000B04, 0xF0000000},   /* Spi4Mosi */
    {0x00000B08, 0xF0000000},   /* Spi4Sck */
    {0x00000B0C, 0x01F1F000},   /* TempAlert */
    {0x00000B10, 0x01F1F000},   /* TouchClk */
    {0x00000B14, 0x01F1F000},   /* TouchInt */
    {0x00000B18, 0x01F1F000},   /* TouchRst */
    {0x00000B1C, 0x01F1F000},   /* Uart1Cts */
    {0x00000B20, 0x01F1F000},   /* Uart1Rts */
    {0x00000B24, 0x01F1F000},   /* Uart1Rx */
    {0x00000B28, 0x01F1F000},   /* Uart1Tx */
    {0x00000B2C, 0x01F1F000},   /* Uart2Cts */
    {0x00000B30, 0x01F1F000},   /* Uart2Rts */
    {0x00000B34, 0x01F1F000},   /* Uart2Rx */
    {0x00000B38, 0x01F1F000},   /* Uart2Tx */
    {0x00000B3C, 0x01F1F000},   /* Uart3Cts */
    {0x00000B40, 0x01F1F000},   /* Uart3Rts */
    {0x00000B44, 0x01F1F000},   /* Uart3Rx */
    {0x00000B48, 0x01F1F000},   /* Uart3Tx */
    {0x00000B4C, 0x01F1F000},   /* Uart4Cts */
    {0x00000B50, 0x01F1F000},   /* Uart4Rts */
    {0x00000B54, 0x01F1F000},   /* Uart4Rx */
    {0x00000B58, 0x01F1F000},   /* Uart4Tx */
    {0x00000B5C, 0x01F1F000},   /* UsbVbusEn0 */
    {0x00000B60, 0x01F1F000},   /* UsbVbusEn1 */
    {0x00000B64, 0x01F1F000},   /* WifiEn */
    {0x00000B68, 0x01F1F000},   /* WifiRst */
    {0x00000B6C, 0x01F1F000},   /* WifiWakeAp */
};

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
    
    if ((hardware_type == 0x00) || (hardware_type == 0x02)) {
        /* Configure all PINMUX pads (minus BattBcl) for Icosa or Hoag */
        for (unsigned int i = 0; i < (MAX_PINMUX - 1); i++) {
            pinmux_update_pad(pinmux_base_vaddr, std::get<0>(g_pinmux_config_map_icosa[i]), std::get<1>(g_pinmux_config_map_icosa[i]), std::get<2>(g_pinmux_config_map_icosa[i]));
        }
    } else if (hardware_type == 0x01) {
        /* Configure all PINMUX pads (minus BattBcl) for Copper */
        for (unsigned int i = 0; i < (MAX_PINMUX - 1); i++) {
            pinmux_update_pad(pinmux_base_vaddr, std::get<0>(g_pinmux_config_map_copper[i]), std::get<1>(g_pinmux_config_map_copper[i]), std::get<2>(g_pinmux_config_map_copper[i]));
        }
    } else if (hardware_type == 0x03) {
        /* TODO: Configure PINMUX pads for Mariko */
    } else {
        /* Invalid */
    }
    
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

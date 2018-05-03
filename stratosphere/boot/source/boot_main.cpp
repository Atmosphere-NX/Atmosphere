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

    /* Initialize services we need (TODO: SPL, NCM) */
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    
    rc = pmshellInitialize();
    if (R_FAILED(rc))
        fatalSimple(0xCAFE << 4 | 1);
    
    fsdevInit();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevExit();
    pmshellExit(); 
    fsExit();
    smExit();
}

static const std::tuple<u32, bool, bool> g_gpio_map[] = {
    std::make_tuple(0xFFFFFFFF, false, false),  /* Invalid */
    std::make_tuple(0x000000CC, true, false),   /* Port Z, Pin 4 */
    std::make_tuple(0x00000024, true, false),   /* Port E, Pin 4 */
    std::make_tuple(0x0000003C, true, false),   /* Port H, Pin 4 */
    std::make_tuple(0x000000DA, false, true),   /* Port BB, Pin 2 */
    std::make_tuple(0x000000DB, true, false),   /* Port BB, Pin 3 */
    std::make_tuple(0x000000DC, false, false),  /* Port BB, Pin 4 */
    std::make_tuple(0x00000025, true, false),   /* Port E, Pin 5 */
    std::make_tuple(0x00000090, false, false),  /* Port S, Pin 0 */
    std::make_tuple(0x00000091, false, false),  /* Port S, Pin 1 */
    std::make_tuple(0x00000096, true, false),   /* Port S, Pin 6 */
    std::make_tuple(0x00000097, false, true),   /* Port S, Pin 7 */
    std::make_tuple(0x00000026, false, false),  /* Port E, Pin 6 */
    std::make_tuple(0x00000005, true, false),   /* Port A, Pin 5 */
    std::make_tuple(0x00000078, false, false),  /* Port P, Pin 0 */
    std::make_tuple(0x00000093, false, true),   /* Port S, Pin 3 */
    std::make_tuple(0x0000007D, false, false),  /* Port P, Pin 5 */
    std::make_tuple(0x0000007C, false, false),  /* Port P, Pin 4 */
    std::make_tuple(0x0000007B, false, false),  /* Port P, Pin 3 */
    std::make_tuple(0x0000007A, false, false),  /* Port P, Pin 2 */
    std::make_tuple(0x000000BC, false, true),   /* Port X, Pin 4 */
    std::make_tuple(0x000000AE, false, false),  /* Port V, Pin 6 */
    std::make_tuple(0x000000BA, false, false),  /* Port X, Pin 2 */
    std::make_tuple(0x000000B9, false, true),   /* Port X, Pin 1 */
    std::make_tuple(0x000000BD, false, false),  /* Port X, Pin 5 */
    std::make_tuple(0x000000BE, false, true),   /* Port X, Pin 6 */
    std::make_tuple(0x000000BF, false, true),   /* Port X, Pin 7 */
    std::make_tuple(0x000000C0, false, true),   /* Port Y, Pin 0 */
    std::make_tuple(0x000000C1, false, false),  /* Port Y, Pin 1 */
    std::make_tuple(0x000000A9, true, false),   /* Port V, Pin 1 */
    std::make_tuple(0x000000AA, true, false),   /* Port V, Pin 2 */
    std::make_tuple(0x00000055, true, false),   /* Port K, Pin 5 */
    std::make_tuple(0x000000AD, true, false),   /* Port V, Pin 5 */
    std::make_tuple(0x000000C8, false, true),   /* Port Z, Pin 0 */
    std::make_tuple(0x000000CA, false, false),  /* Port Z, Pin 2 */
    std::make_tuple(0x000000CB, false, true),   /* Port Z, Pin 3 */
    std::make_tuple(0x0000004F, true, false),   /* Port J, Pin 7 */
    std::make_tuple(0x00000050, false, false),  /* Port K, Pin 0 */
    std::make_tuple(0x00000051, false, false),  /* Port K, Pin 1 */
    std::make_tuple(0x00000052, false, false),  /* Port K, Pin 2 */
    std::make_tuple(0x00000054, false, true),   /* Port K, Pin 4 */
    std::make_tuple(0x00000056, false, true),   /* Port K, Pin 6 */
    std::make_tuple(0x00000057, false, true),   /* Port K, Pin 7 */
    std::make_tuple(0x00000053, true, false),   /* Port K, Pin 3 */
    std::make_tuple(0x000000E3, true, false),   /* Port CC, Pin 3 */
    std::make_tuple(0x00000038, true, false),   /* Port H, Pin 0 */
    std::make_tuple(0x00000039, true, false),   /* Port H, Pin 1 */
    std::make_tuple(0x0000003B, true, false),   /* Port H, Pin 3 */
    std::make_tuple(0x0000003D, false, false),  /* Port H, Pin 5 */
    std::make_tuple(0x0000003F, true, false),   /* Port H, Pin 7 */
    std::make_tuple(0x00000040, true, false),   /* Port I, Pin 0 */
    std::make_tuple(0x00000041, true, false),   /* Port I, Pin 1 */
    std::make_tuple(0x0000003E, false, false),  /* Port H, Pin 6 */
    std::make_tuple(0x000000E2, false, true),   /* Port CC, Pin 2 */
    std::make_tuple(0x000000E4, true, false),   /* Port CC, Pin 4 */
    std::make_tuple(0x0000003A, false, false),  /* Port H, Pin 2 */
    std::make_tuple(0x000000C9, false, true),   /* Port Z, Pin 1 */
    std::make_tuple(0x0000004D, true, false),   /* Port J, Pin 5 */
    std::make_tuple(0x00000058, true, false),   /* Port L, Pin 0 */
    std::make_tuple(0x0000003E, false, false),  /* Port H, Pin 6 */
    std::make_tuple(0x00000026, false, false),  /* Port E, Pin 6 */
    std::make_tuple(0xFFFFFFFF, false, false),  /* Invalid */
    std::make_tuple(0x00000033, false, false),  /* Port G, Pin 3 */
    std::make_tuple(0x0000001C, false, false),  /* Port D, Pin 4 */
    std::make_tuple(0x000000D9, false, false),  /* Port BB, Pin 1 */
    std::make_tuple(0x0000000C, false, false),  /* Port B, Pin 4 */
    std::make_tuple(0x0000000D, false, false),  /* Port B, Pin 5 */
    std::make_tuple(0x00000021, false, false),  /* Port E, Pin 1 */
    std::make_tuple(0x00000027, false, false),  /* Port E, Pin 7 */
    std::make_tuple(0x00000092, false, false),  /* Port S, Pin 2 */
    std::make_tuple(0x00000095, false, false),  /* Port S, Pin 5 */
    std::make_tuple(0x00000098, false, false),  /* Port T, Pin 0 */
    std::make_tuple(0x00000010, false, false),  /* Port C, Pin 0 */
    std::make_tuple(0x00000011, false, false),  /* Port C, Pin 1 */
    std::make_tuple(0x00000012, false, false),  /* Port C, Pin 2 */
    std::make_tuple(0x00000042, false, false),  /* Port I, Pin 2 */
    std::make_tuple(0x000000E6, false, false),  /* Port CC, Pin 6 */
};

int gpio_configure(unsigned int gpio_pad_name) {
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

int gpio_set_direction(unsigned int gpio_pad_name) {
    /* Fetch this GPIO's pad descriptor */
    u32 gpio_pad_desc = std::get<0>(g_gpio_map[gpio_pad_name]);
    
    /* Fetch this GPIO's direction */
    bool is_out = std::get<1>(g_gpio_map[gpio_pad_name]);
    
    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));
    
    /* Set the direction bit and lock values (bug?) */
    u32 gpio_oe_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (is_out << (gpio_pad_desc & 0x07)));
    
    /* Write to the appropriate GPIO_OE_x register (upper offset) */
    *((u32 *)gpio_base_vaddr + gpio_reg_offset + 0x90) = gpio_oe_val;
    
    /* Do a dummy read from GPIO_OE_x register (lower offset) */
    gpio_oe_val = *((u32 *)gpio_base_vaddr + gpio_reg_offset);
    
    return gpio_oe_val;
}

int gpio_set_output(unsigned int gpio_pad_name) {
    /* Fetch this GPIO's pad descriptor */
    u32 gpio_pad_desc = std::get<0>(g_gpio_map[gpio_pad_name]);
    
    /* Fetch this GPIO's output value */
    bool is_high = std::get<2>(g_gpio_map[gpio_pad_name]);
    
    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));
    
    /* Set the output bit and lock values (bug?) */
    u32 gpio_out_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (is_high << (gpio_pad_desc & 0x07)));
    
    /* Write to the appropriate GPIO_OUT_x register (upper offset) */
    *((u32 *)gpio_base_vaddr + gpio_reg_offset + 0xA0) = gpio_out_val;
    
    /* Do a dummy read from GPIO_OUT_x register (lower offset) */
    gpio_out_val = *((u32 *)gpio_base_vaddr + gpio_reg_offset);
    
    return gpio_out_val;
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    
    Result rc;
    u64* pinmux_base_vaddr = NULL;
    u64* gpio_base_vaddr = NULL;
    u64* pmc_base_vaddr = NULL;
    
    /* Map the APB MISC registers for PINMUX */
    rc = svcQueryIoMapping(pinmux_base_vaddr, APB_MISC_BASE, 0x4000);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* IO mapping failed */
    if (!pinmux_base_vaddr)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
    
    /* Map the GPIO registers */
    rc = svcQueryIoMapping(gpio_base_vaddr, GPIO_BASE, 0x1000);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* IO mapping failed */
    if (!gpio_base_vaddr)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
    
    /* Change GPIO voltage to 1.8v */
    if (kernelAbove200()) {
        /* TODO: svcReadWriteRegister */
    } else {        
        /* Map the PMC registers directly */
        rc = svcQueryIoMapping(pmc_base_vaddr, PMC_BASE, 0x3000);
        if (R_FAILED(rc)) {
            return rc;
        }
        
        /* IO mapping failed */
        if (!pmc_base_vaddr)
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_IoError));
        
        /* Write to APBDEV_PMC_PWR_DET_0 */
        *((u32 *)pmc_base_vaddr + 0x48) |= 0x00A42000;
        
        /* Write to APBDEV_PMC_PWR_DET_VAL_0 */
        *((u32 *)pmc_base_vaddr + 0xE4) &= 0xFF5BDFFF;
    }
    
    /* Wait for changes to take effect */
    svcSleepThread(100000);
    
    /* Setup all GPIOs from 0x01 to 0x3C */
    for (unsigned int i = 1; i < MAX_GPIOS; i++) {
        gpio_configure(i);
        gpio_set_direction(i);
        gpio_set_output(i);
    }
    
    /* TODO: Hardware setup, NAND repair, NotifyBootFinished */
    
    rc = 0;
    return rc;
}

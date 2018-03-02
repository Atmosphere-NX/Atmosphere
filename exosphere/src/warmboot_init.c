#include "utils.h"
#include "memory_map.h"
#include "arm.h"

/* start.s */
void __set_memory_registers(uintptr_t ttbr0, uintptr_t vbar, uint64_t cpuectlr, uint32_t scr,
                            uint32_t tcr, uint32_t cptr, uint64_t mair, uint32_t sctlr);

uintptr_t get_warmboot_crt0_stack_address(void) {
    return TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE012_STACK) + 0x800;
}

void set_memory_registers_enable_mmu(void) {
    static const uintptr_t vbar  = TZRAM_GET_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800;
    static const uintptr_t ttbr0 = vbar - 64;

    /*
        - Disable table walk descriptor access prefetch.
        - L2 instruction fetch prefetch distance = 3 (reset value)
        - L2 load/store data prefetch distance = 8 (reset value)
        - Enable the processor to receive instruction cache and TLB maintenance operations broadcast from other processors in the cluster
    */
    static const uint64_t cpuectlr = 0x1B00000040ull;

    /*
        - The next lower level is Aarch64
        - Secure instruction fetch (when the PE is in Secure state, this bit disables instruction fetch from Non-secure memory)
        - External Abort/SError taken to EL3
        - FIQ taken to EL3
        - NS (EL0 and EL1 are nonsecure)
    */
    static const uint32_t scr = 0x63D;

    /*
        - PA size: 36-bit (64 GB)
        - Granule size: 4KB
        - Shareability attribute for memory associated with translation table walks using TTBR0_EL3: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - T0SZ = 31 (33-bit address space)
    */
    static const uint32_t tcr = TCR_EL3_RSVD | TCR_PS(1) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(33);

    /* Nothing trapped */
    static const uint32_t cptr = 0;

    /*
        - Attribute 0: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
        - Attribute 1: Device-nGnRE memory
        - Other attributes: Device-nGnRnE memory
    */
    static const uint64_t mair = 0x4FFull;

    /*
        - Cacheability control, for EL3 instruction accesses DISABLED
        (- SP Alignment check bit NOT SET)
        - Cacheability control, for EL3 data accesses DISABLED (normal memory accesses from EL3 are cacheable)
        (- Alignement check bit NOT SET)
        - MMU enabled for EL3 stage 1 address translation
    */
    static const uint32_t sctlr = 0x30C51835ull;

    __set_memory_registers(ttbr0, vbar, cpuectlr, scr, tcr, cptr, mair, sctlr);
}

void warmboot_init(warmboot_func_list_t *func_list) {
    (void)func_list;
}

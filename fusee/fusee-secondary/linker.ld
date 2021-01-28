OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
    crt0        PT_LOAD;
    chainloader PT_LOAD;
    nxboot      PT_LOAD;
    main        PT_LOAD;
}

/* Mostly copied from https://github.com/devkitPro/buildscripts/blob/master/dkarm-eabi/crtls/3dsx.ld */
MEMORY
{
    main        : ORIGIN = 0xF0000000, LENGTH = 0x10000000
    high_iram   : ORIGIN = 0x40010000, LENGTH = 0x8000
    low_iram    : ORIGIN = 0x40002000, LENGTH = 0x6000
}

SECTIONS
{
    PROVIDE(__start__           = 0xF0000000);
    PROVIDE(__stack_top__       = 0x40010000);
    PROVIDE(__stack_bottom__    = 0x40008000);
    PROVIDE(__heap_start__      = 0x90020000);
    PROVIDE(__heap_end__        = 0xA0020000);

    . = __start__;

    .crt0 :
    {
        KEEP( *(.text.start) )
        KEEP( *(.init) )
        . = ALIGN(32);
    } >main :crt0

    .chainloader_loadable :
    {
        . = ALIGN(32);
        PROVIDE (__chainloader_start__  = ABSOLUTE(.));
        PROVIDE (__chainloader_lma__    = LOADADDR(.chainloader_loadable));
        KEEP(*(.chainloader.text.start))
        chainloader.o(.text*)
        chainloader.o(.rodata*)
        chainloader.o(.data*)
        . = ALIGN(32);
    } >low_iram AT>main :chainloader

    .chainloader_bss (NOLOAD) :
    {
        . = ALIGN(32);
        PROVIDE (__chainloader_bss_start__ = ABSOLUTE(.));
        chainloader.o(.bss* COMMON)
        . = ALIGN(32);
        PROVIDE (__chainloader_end__ = ABSOLUTE(.));
    } >low_iram :NONE

    .nxboot_loadable :
    {
        . = ALIGN(32);
        PROVIDE (__nxboot_start__  = ABSOLUTE(.));
        PROVIDE (__nxboot_lma__    = LOADADDR(.nxboot_loadable));
        KEEP(*(.nxboot.text.start))
        nxboot_iram.o(.text*)
        cluster.o(.text*)
        car.o(.text*)
        nxboot_iram.o(.rodata*)
        cluster.o(.rodata*)
        car.o(.rodata*)
        nxboot_iram.o(.data*)
        cluster.o(.data*)
        car.o(.data*)
        . = ALIGN(32);
    } >high_iram AT>main :nxboot

    .nxboot_bss (NOLOAD) :
    {
        . = ALIGN(32);
        PROVIDE (__nxboot_bss_start__ = ABSOLUTE(.));
        nxboot_iram.o(.bss* COMMON)
        cluster.o(.bss* COMMON)
        car.o(.bss* COMMON)
        . = ALIGN(32);
        PROVIDE (__nxboot_end__ = ABSOLUTE(.));
    } >high_iram :NONE

    .text :
    {
        . = ALIGN(32);
        /* .text */
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t*)

        /* .fini */
        KEEP( *(.fini) )
        . = ALIGN(8);
    } >main :main

    .rodata :
    {
        *(.rodata)
        *(.roda)
        *(.rodata.*)
        *all.rodata*(*)
        *(.gnu.linkonce.r*)
        SORT(CONSTRUCTORS)
        . = ALIGN(8);
    } >main

    .preinit_array :
    {
        PROVIDE (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE (__preinit_array_end = .);
    } >main

    .init_array ALIGN(4) :
    {
        PROVIDE (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = .);
    } >main

    .fini_array ALIGN(4) :
    {
        PROVIDE (__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE (__fini_array_end = .);
    } >main

    .ctors ALIGN(4) :
    {
        KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
    } >main

    .dtors ALIGN(4) :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
    } >main

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) __exidx_start = ABSOLUTE(.);} >main
    ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*)  __exidx_end = ABSOLUTE(.);} >main

    .data :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
        CONSTRUCTORS
        . = ALIGN(32);
    } >main

    __data_end__ = ABSOLUTE(.);
    PROVIDE (__total_size__ = (__data_end__ - __start__));

    .bss (NOLOAD) :
    {
        . = ALIGN(32);
        PROVIDE (__bss_start__ = ABSOLUTE(.));
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b*)
        *(COMMON)
        . = ALIGN(32);
        PROVIDE (__bss_end__ = ABSOLUTE(.));
    } >main :NONE
    __end__ = ABSOLUTE(.) ;

    /* ==================
       ==== Metadata ====
       ================== */

    /* Discard sections that difficult post-processing */
    /DISCARD/ : { *(.group .comment .note) }

    /* Stabs debugging sections. */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }

    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0. */

    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }

    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }

    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }

    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

    /* =======================
       ==== Embedded Data ====
       ======================= */
    PROVIDE(__ams_mitm_kip_start__ = ams_mitm_kip - __start__);
    PROVIDE(__ams_mitm_kip_size__ = ams_mitm_kip_end - ams_mitm_kip);
    PROVIDE(__boot_kip_start__ = boot_kip - __start__);
    PROVIDE(__boot_kip_size__ = boot_kip_end - boot_kip);
    PROVIDE(__exosphere_bin_start__ = exosphere_bin - __start__);
    PROVIDE(__exosphere_bin_size__ = exosphere_bin_end - exosphere_bin);
    PROVIDE(__fusee_primary_bin_start__ = fusee_primary_bin - __start__);
    PROVIDE(__fusee_primary_bin_size__ = fusee_primary_bin_end - fusee_primary_bin);
    PROVIDE(__loader_kip_start__ = loader_kip - __start__);
    PROVIDE(__loader_kip_size__ = loader_kip_end - loader_kip);
    PROVIDE(__warmboot_bin_start__ = warmboot_bin - __start__);
    PROVIDE(__warmboot_bin_size__ = warmboot_bin_end - warmboot_bin);
    PROVIDE(__ncm_kip_start__ = ncm_kip - __start__);
    PROVIDE(__ncm_kip_size__ = ncm_kip_end - ncm_kip);
    PROVIDE(__pm_kip_start__ = pm_kip - __start__);
    PROVIDE(__pm_kip_size__ = pm_kip_end - pm_kip);
    PROVIDE(__rebootstub_bin_start__ = rebootstub_bin - __start__);
    PROVIDE(__rebootstub_bin_size__ = rebootstub_bin_end - rebootstub_bin);
    PROVIDE(__sept_primary_bin_start__ = sept_primary_bin - __start__);
    PROVIDE(__sept_primary_bin_size__ = sept_primary_bin_end - sept_primary_bin);
    PROVIDE(__sept_secondary_00_enc_start__ = sept_secondary_00_enc - __start__);
    PROVIDE(__sept_secondary_00_enc_size__ = sept_secondary_00_enc_end - sept_secondary_00_enc);
    PROVIDE(__sept_secondary_01_enc_start__ = sept_secondary_01_enc - __start__);
    PROVIDE(__sept_secondary_01_enc_size__ = sept_secondary_01_enc_end - sept_secondary_01_enc);
    PROVIDE(__sm_kip_start__ = sm_kip - __start__);
    PROVIDE(__sm_kip_size__ = sm_kip_end - sm_kip);
    PROVIDE(__spl_kip_start__ = spl_kip - __start__);
    PROVIDE(__spl_kip_size__ = spl_kip_end - spl_kip);
    PROVIDE(__splash_screen_bin_start__ = splash_screen_bin - __start__);
    PROVIDE(__splash_screen_bin_size__ = splash_screen_bin_end - splash_screen_bin);
    PROVIDE(__thermosphere_bin_start__ = thermosphere_bin - __start__);
    PROVIDE(__thermosphere_bin_size__ = thermosphere_bin_end - thermosphere_bin);
    PROVIDE(__emummc_kip_start__ = emummc_kip - __start__);
    PROVIDE(__emummc_kip_size__ = emummc_kip_end - emummc_kip);
    PROVIDE(__kernel_ldr_bin_start__ = kernel_ldr_bin - __start__);
    PROVIDE(__kernel_ldr_bin_size__ = kernel_ldr_bin_end - kernel_ldr_bin);
    PROVIDE(__mesosphere_bin_start__ = mesosphere_bin - __start__);
    PROVIDE(__mesosphere_bin_size__ = mesosphere_bin_end - mesosphere_bin);
    PROVIDE(__mariko_fatal_bin_start__ = mariko_fatal_bin - __start__);
    PROVIDE(__mariko_fatal_bin_size__ = mariko_fatal_bin_end - mariko_fatal_bin);
}

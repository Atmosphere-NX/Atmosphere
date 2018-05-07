OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

/* Mostly copied from https://github.com/devkitPro/buildscripts/blob/master/dkarm-eabi/crtls/3dsx.ld */
MEMORY
{
    NULL        : ORIGIN = 0x00000000, LENGTH = 0x1000
    main        : ORIGIN = 0xFFF00000, LENGTH = 0x00100000
    low_iram    : ORIGIN = 0x40003000, LENGTH = 0x8000
}

SECTIONS
{
    PROVIDE(__start__           = 0xF0000000);
    PROVIDE(__stack_top__       = 0x40010000);
    PROVIDE(__stack_bottom__    = 0x4000C000);
    PROVIDE(__heap_start__      = 0xE0000000);
    PROVIDE(__heap_end__        = 0xF0000000);

    . = __start__;
    . = ALIGN(32);

    .crt0 :
    {
        . = ALIGN(32);
        KEEP( *(.text.start) )
        KEEP( *(.init) )
        . = ALIGN(4);
    } >main

    .chainloader_loadable :
    {
        . = ALIGN(32);
        PROVIDE (__chainloader_start__  = .);
        PROVIDE (__chainloader_lma__    = LOADADDR(.chainloader_loadable));
        KEEP(*(.chainloader.text.start))
        build/chainloader.o(.text*)
        build/chainloader.o(.rodata*)
        build/chainloader.o(.data*)
        . = ALIGN(8);

    } >low_iram AT>main

    .chainloader_bss :
    {
        . = ALIGN(8);
        PROVIDE (__chainloader_bss_start__ = .);
        build/chainloader.o(.bss* COMMON)
        . = ALIGN(8);
        PROVIDE (__chainloader_end__ = .);
    } >low_iram AT>main

    .text :
    {
        . = ALIGN(4);

        /* .text */
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t*)
        . = ALIGN(4);

        /* .fini */
        KEEP( *(.fini) )
        . = ALIGN(4);
    } >main

    .rodata :
    {
        *(.rodata)
        *(.roda)
        *(.rodata.*)
        *all.rodata*(*)
        *(.gnu.linkonce.r*)
        SORT(CONSTRUCTORS)
        . = ALIGN(4);
    } >main

    .preinit_array ALIGN(4) :
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
    } >main

    .dtors ALIGN(4) :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } >main

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >main
    __exidx_start = .;
    ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >main
    __exidx_end = .;

    .data :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
        CONSTRUCTORS
        . = ALIGN(4);
    } >main

    .bss :
    {
        __bss_start__ = ALIGN(32);
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
    } >main
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
}

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
    crt0        PT_LOAD;
    chainloader PT_LOAD;
    main        PT_LOAD;
}

/* Mostly copied from https://github.com/devkitPro/buildscripts/blob/master/dkarm-eabi/crtls/3dsx.ld */
MEMORY
{
    NULL        : ORIGIN = 0x00000000, LENGTH = 0x1000
    main        : ORIGIN = 0x40008000, LENGTH = 0x38000
    low_iram    : ORIGIN = 0x40000000, LENGTH = 0x6000
}

SECTIONS
{
    PROVIDE(__start__           = 0x40008000);
    PROVIDE(__stack_top__       = 0x40008000);
    PROVIDE(__stack_bottom__    = 0x40006000);
    PROVIDE(__heap_start__      = 0);
    PROVIDE(__heap_end__        = 0);

    . = __start__;

    .crt0 :
    {
        KEEP( *(.text.start) )
        KEEP( *(.init) )
        . = ALIGN(32);
        PROVIDE (__reboot_start__ = ABSOLUTE(.));
        KEEP( *(.reboot*) )
        . = ALIGN(4);
        PROVIDE (__reboot_end__   = ABSOLUTE(.));
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

    .text :
    {
        . = ALIGN(32);
        /* .text */
        *(.text.reboot_to_self)
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
}

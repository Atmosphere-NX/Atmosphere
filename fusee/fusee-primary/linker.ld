OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
    crt0        PT_LOAD;
    main        PT_LOAD;
    loader_stub PT_LOAD;
}

/* Mostly copied from https://github.com/devkitPro/buildscripts/blob/master/dkarm-eabi/crtls/3dsx.ld */
MEMORY
{
    NULL        : ORIGIN = 0x00000000, LENGTH = 0x1000
    main        : ORIGIN = 0x40010000, LENGTH = 0x20000
    loader_stub : ORIGIN = 0x40030000, LENGTH = 0x4000
}

SECTIONS
{
    PROVIDE(__start__           = 0x40008000);
    PROVIDE(__stack_top__       = 0x40038000);
    PROVIDE(__stack_bottom__    = 0x40034000);
    PROVIDE(__heap_start__      = 0);
    PROVIDE(__heap_end__        = 0);

    . = __start__;

    .crt0 :
    {
        KEEP( *(.text.start) )
        KEEP( *(.init) )
        . = ALIGN(32);
    } >main :crt0

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

    .loader_stub :
    {
        . = ALIGN(32);
        PROVIDE (__loader_stub_start__  = ABSOLUTE(.));
        PROVIDE (__loader_stub_lma__    = LOADADDR(.loader_stub));
        loader_stub.o(.text*)
        lz4.o(.text*)
        utils.o(.text*)
        loader_stub.o(.rodata*)
        lz4.o(.rodata*)
        utils.o(.rodata*)
        loader_stub.o(.data*)
        lz4.o(.data*)
        utils.o(data)
        . = ALIGN(32);
        PROVIDE (__loader_stub_end__ = ABSOLUTE(.));
    } >loader_stub AT>main : loader_stub

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

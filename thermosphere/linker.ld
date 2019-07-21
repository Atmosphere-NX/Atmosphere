OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
    PROVIDE(__start__ = ORIGIN(main));
    . = __start__;

    .text :
    {
        . = ALIGN(8);
        KEEP(*(.crt0*));
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        . = ALIGN(0x800);
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.vectors*));
        . = ALIGN(8);
    } >main

    .init :
    {
        KEEP( *(.init) )
        . = ALIGN(8);
    } >main

    .plt :
    {
        *(.plt)
        *(.iplt)
        . = ALIGN(8);
    } >main


    .fini :
    {
        KEEP( *(.fini) )
        . = ALIGN(8);
    } >main

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
        . = ALIGN(8);
    } >main

    .got            : { __got_start__ = ABSOLUTE(.); *(.got) *(.igot) } >main
    .got.plt        : { *(.got.plt)  *(.igot.plt)  __got_end__ = ABSOLUTE(.);} >main

    .preinit_array :
    {
        . = ALIGN(8);
        PROVIDE (__preinit_array_start = ABSOLUTE(.));
        KEEP (*(.preinit_array))
        PROVIDE (__preinit_array_end = ABSOLUTE(.));
        . = ALIGN(8);
    } >main

    .init_array :
    {
        PROVIDE (__init_array_start = ABSOLUTE(.));
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = ABSOLUTE(.));
    } >main

    .fini_array :
    {
        . = ALIGN(8);
        PROVIDE (__fini_array_start = ABSOLUTE(.));
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE (__fini_array_end = ABSOLUTE(.));
        . = ALIGN(8);
    } >main

    .ctors :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
    } >main

    .dtors ALIGN(8) :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(8);
    } >main

    .data ALIGN(8) :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(8);
    } >main


    .eh_frame_hdr     : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } >main
    .eh_frame         : { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main
    .gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) } >main
    .gnu_extab        : { *(.gnu_extab*) } >main
    .exception_ranges : { *(.exception_ranges .exception_ranges*) } >main

    .dynamic           : { *(.dynamic) } >main
    .interp            : { *(.interp) } >main
    .note.gnu.build-id : { *(.note.gnu.build-id) } >main
    .hash              : { *(.hash) } >main
    .gnu.hash          : { *(.gnu.hash) } >main
    .gnu.version       : { *(.gnu.version) } >main
    .gnu.version_d     : { *(.gnu.version_d) } >main
    .gnu.version_r     : { *(.gnu.version_r) } >main
    .dynsym            : { *(.dynsym) } >main
    .dynstr            : { *(.dynstr) } >main
    .rela.dyn          : { *(.rela.*); __main_end__ = ABSOLUTE(.);} >main

    .bss :
    {
        . = ALIGN(8);
        __bss_start__  = ABSOLUTE(.);
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        __end__ = ABSOLUTE(.);
    } >main

    __end__ = ABSOLUTE(.) ;

    __stacks_top__ = ABSOLUTE(. + 0x1000);

    . = ALIGN(8);

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

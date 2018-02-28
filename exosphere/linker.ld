OUTPUT_ARCH(aarch64)
ENTRY(__start_cold)

MEMORY
{
    fake  : ORIGIN = 0,           LENGTH = 4096M
    tzram : ORIGIN = 0x07C010000, LENGTH = 64K
    /*
        The warmboot crt0 is preceeded by the exception vector page and the L2 and L3 translation tables.
        Normally the main code immediately follows the warmboot crt0, aligned to 256 bytes.
        We can't ensure or replicate that behavior properly so we'll just give 2K to the warmboot crt0.
    */
    warmboot_crt0 : ORIGIN = ORIGIN(tzram) + 12K, LENGTH = 2K

    /* 8K are the MMU L2 and L3 tables & 2K from the evt page */ 
    main    : ORIGIN = 0x1F01E0000                          + LENGTH(warmboot_crt0),    LENGTH = LENGTH(tzram) - LENGTH(pk2ldr) - LENGTH(evt) - LENGTH(warmboot_crt0) - 10K
    pk2ldr  : ORIGIN = ORIGIN(main) - LENGTH(warmboot_crt0) + LENGTH(tzram),            LENGTH = 8K
    /* The first half of the page are exception entry stacks, the other half are the vectors themselves */
    evt     : ORIGIN = ORIGIN(pk2ldr)                       + 40K + 2K,                 LENGTH = 2K
}

SECTIONS
{
    PROVIDE(__start__ = 0x0);
    . = __start__;

    .cold_crt0 :
    {
        . = ALIGN(64);
        KEEP (*(.cold_crt0.text.start)) /* MUST be first */
        KEEP (*(.cold_crt0.text*))
        KEEP (build/coldboot_init.o(.text*))
        *(.cold_crt0.rodata*)
        build/coldboot_init.o(.rodata*)
        *(.cold_crt0.data*)
        build/coldboot_init.o(.data*)
        . = ALIGN(8);
        *(.cold_crt0.bss*)
        build/coldboot_init.o(.bss*)
        . = ALIGN(64);
    } >fake AT>fake

    .warm_crt0 :
    {
        . = ALIGN(64);
        __warmboot_crt0_lma__   = LOADADDR(.warm_crt0);
        __warmboot_crt0_start__ = ABSOLUTE(.);
        KEEP (*(.warm_crt0.text.start)) /* Should be first */
        KEEP (*(.warm_crt0.text*))
        KEEP (build/warmboot_init.o(.text*))
        *(.warm_crt0.rodata*)
        build/warmboot_init.o(.rodata*)
        *(.warm_crt0.data*)
        build/warmboot_init.o(.data*)
        . = ALIGN(8);
        *(.warm_crt0.bss*)
        build/warmboot_init.o(.bss*)
        . = ALIGN(64);
        __warmboot_crt0_end__ = ABSOLUTE(.);
    } >warmboot_crt0 AT>fake

    .pk2ldr :
    {
        . = ALIGN(4096);
        __pk2ldr_lma__   = LOADADDR(.pk2ldr);
        __pk2ldr_start__ = ABSOLUTE(.);
        KEEP (build/package2.o(.text*))
        build/package2.o(.rodata*)
        build/package2.o(.data*)
        . = ALIGN(8);
    } >pk2ldr AT>fake

    .pk2ldr.bss :
    {
        . = ALIGN(8);
        __pk2ldr_bss_start__ = ABSOLUTE(.);
        build/package2.o(.bss*)
        . = ALIGN(8);
        __pk2ldr_end__ = ABSOLUTE(.);
    } >pk2ldr AT>fake

    .vectors :
    {
        . = ALIGN(2048);
        __vectors_lma__   = LOADADDR(.vectors);
        __vectors_start__ = ABSOLUTE(.);
        KEEP (*(.vectors*))
        . = ALIGN(8);
        __vectors_end__ = ABSOLUTE(.);
    } >evt AT>fake

    .text :
    {
        . = ALIGN(256);
        __main_lma__    = LOADADDR(.text);
        __main_start__  = ABSOLUTE(.);
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        . = ALIGN(8);
    } >main AT>fake

    .init :
    {
        KEEP( *(.init) )
        . = ALIGN(8);
    } >main AT>fake

    .plt :
    {
        *(.plt)
        *(.iplt)
        . = ALIGN(8);
    } >main AT>fake


    .fini :
    {
        KEEP( *(.fini) )
        . = ALIGN(8);
    } >main AT>fake

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
        . = ALIGN(8);
    } >main AT>fake

    .got            : { __got_start__ = ABSOLUTE(.); *(.got) *(.igot) } >main AT>fake
    .got.plt        : { *(.got.plt)  *(.igot.plt)  __got_end__ = ABSOLUTE(.);} >main AT>fake

    .preinit_array :
    {
        . = ALIGN(8);
        PROVIDE (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE (__preinit_array_end = .);
        . = ALIGN(8);
    } >main AT>fake

    .init_array :
    {
        PROVIDE (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = .);
    } >main AT>fake

    .fini_array :
    {
        . = ALIGN(8);
        PROVIDE (__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE (__fini_array_end = .);
        . = ALIGN(8);
    } >main AT>fake

    .ctors :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
    } >main AT>fake

    .dtors ALIGN(8) :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(8);
    } >main AT>fake

    .data ALIGN(8) :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(8);
    } >main AT>fake


    .eh_frame_hdr     : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } >main AT>fake
    .eh_frame         : { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>fake
    .gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) } >main AT>fake
    .gnu_extab        : { *(.gnu_extab*) } >main AT>fake
    .exception_ranges : { *(.exception_ranges .exception_ranges*) } >main AT>fake

    .dynamic           : { *(.dynamic) } >main AT>fake
    .interp            : { *(.interp) } >main AT>fake
    .note.gnu.build-id : { *(.note.gnu.build-id) } >main AT>fake
    .hash              : { *(.hash) } >main AT>fake
    .gnu.hash          : { *(.gnu.hash) } >main AT>fake
    .gnu.version       : { *(.gnu.version) } >main AT>fake
    .gnu.version_d     : { *(.gnu.version_d) } >main AT>fake
    .gnu.version_r     : { *(.gnu.version_r) } >main AT>fake
    .dynsym            : { *(.dynsym) } >main AT>fake
    .dynstr            : { *(.dynstr) } >main AT>fake
    .rela.dyn          : { *(.rela.*); __main_end__ = ABSOLUTE(.);} >main AT>fake

    .bss :
    {
        . = ALIGN(8);
        __main_bss_start__ = ABSOLUTE(.);
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        __main_end__ = ABSOLUTE(.);
    } >main AT>fake

    __end__ = ABSOLUTE(.) ;

    . = ALIGN(0x1000);
    __argdata__ = ABSOLUTE(.) ;

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

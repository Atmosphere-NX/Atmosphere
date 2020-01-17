OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS
{
    main PT_LOAD;
}

MEMORY
{
    mainVa  : ORIGIN = 0x7FFFE10000, LENGTH = 2M - 64K
}

SECTIONS
{
    __start_pa__        = ABSOLUTE(ORIGIN(main));
    __temp_pa__         = ABSOLUTE(ORIGIN(temp));
    __max_image_size__  = ABSOLUTE(LENGTH(main));
    __max_temp_size__   = ABSOLUTE(LENGTH(temp) - 0x1000);

    .text :
    {
        . = ALIGN(8);
        __start__ = ABSOLUTE(.);
        KEEP(*(.crt0*));
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        . = ALIGN(0x800);
        __vectors_start__ = ABSOLUTE(.);
        KEEP(*(.vectors*));
        __vectors_end__   = ABSOLUTE(.);
        ASSERT(__vectors_end__ - __vectors_start__ <= 0x800, "Exception vectors section should be max 0x800 in size!");
        . = ALIGN(8);
    } >mainVa AT>main :main

    .init :
    {
        KEEP( *(.init) )
        . = ALIGN(8);
    } >mainVa AT>main :main

    .plt :
    {
        *(.plt)
        *(.iplt)
        . = ALIGN(8);
    } >mainVa AT>main :main


    .fini :
    {
        KEEP( *(.fini) )
        . = ALIGN(8);
    } >mainVa AT>main :main

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
        . = ALIGN(8);
    } >mainVa AT>main :main

    .got            : { __got_start__ = ABSOLUTE(.); *(.got) *(.igot) } >mainVa AT>main :main
    .got.plt        : { *(.got.plt)  *(.igot.plt)  __got_end__ = ABSOLUTE(.);} >mainVa AT>main :main

    .preinit_array :
    {
        . = ALIGN(8);
        PROVIDE (__preinit_array_start = ABSOLUTE(.));
        KEEP (*(.preinit_array))
        PROVIDE (__preinit_array_end = ABSOLUTE(.));
        ASSERT(__preinit_array_end == __preinit_array_start, ".preinit_array not empty!");
        . = ALIGN(8);
    } >mainVa AT>main :main

    .init_array :
    {
        PROVIDE (__init_array_start = ABSOLUTE(.));
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = ABSOLUTE(.));
        ASSERT(__init_array_end == __init_array_start, ".init_array not empty!");
    } >mainVa AT>main :main

    .fini_array :
    {
        . = ALIGN(8);
        PROVIDE (__fini_array_start = ABSOLUTE(.));
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE (__fini_array_end = ABSOLUTE(.));
        . = ALIGN(8);
        ASSERT(__fini_array_end == __fini_array_start, ".fini_array not empty!");
    } >mainVa AT>main :main

    .ctors :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
    } >mainVa AT>main :main

    .dtors ALIGN(8) :
    {
        . = ALIGN(8);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(8);
    } >mainVa AT>main :main

    .data ALIGN(8) :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(8);
    } >mainVa AT>main :main


    .eh_frame_hdr     : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } >mainVa AT>main :main
    .eh_frame         : { KEEP (*(.eh_frame)) *(.eh_frame.*) } >mainVa AT>main :main
    .gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) } >mainVa AT>main :main
    .gnu_extab        : { *(.gnu_extab*) } >mainVa AT>main :main
    .exception_ranges : { *(.exception_ranges .exception_ranges*) } >mainVa AT>main :main

    .dynamic           : { *(.dynamic) } >mainVa AT>main :main
    .interp            : { *(.interp) } >mainVa AT>main :main
    .note.gnu.build-id : { *(.note.gnu.build-id) } >mainVa AT>main :main
    .hash              : { *(.hash) } >mainVa AT>main :main
    .gnu.hash          : { *(.gnu.hash) } >mainVa AT>main :main
    .gnu.version       : { *(.gnu.version) } >mainVa AT>main :main
    .gnu.version_d     : { *(.gnu.version_d) } >mainVa AT>main :main
    .gnu.version_r     : { *(.gnu.version_r) } >mainVa AT>main :main
    .dynsym            : { *(.dynsym) } >mainVa AT>main :main
    .dynstr            : { *(.dynstr) } >mainVa AT>main :main
    .rela.dyn          : { *(.rela.*); __main_end__ = ABSOLUTE(.);} >mainVa AT>main :main

    .bss (NOLOAD) :
    {
        . = ALIGN(0x1000);
        __bss_start__  = ABSOLUTE(.);
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        __real_bss_end__ = ABSOLUTE(.);
        __image_size__ = ABSOLUTE(__real_bss_end__ - __start__);
        ASSERT(__image_size__ <= __max_image_size__, "Image too big!");
        /*
            The logic here: tempbss *additional pages* are at a very different PA, but
            we can allow .tempbss to use unused "non-temporary" BSS space. Their VAs are
            contiguous.
        */
        *(.tempbss .tempbss.*)
        __bss_end__ = ABSOLUTE(.);
        __temp_size__ = ABSOLUTE(__bss_end__ - __real_bss_end__);
    } >mainVa :NONE



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

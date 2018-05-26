OUTPUT_ARCH(aarch64)
ENTRY(__start_cold)

PHDRS
{
    ccrt0   PT_LOAD FLAGS(7) /* Read | Write | Execute */;
    pk2ldr  PT_LOAD FLAGS(7) /* Read | Write | Execute */;
    wcrt0   PT_LOAD FLAGS(7) /* Read | Write | Execute */;

    code    PT_LOAD FLAGS(5) /* Read | Execute */;
    rodata  PT_LOAD FLAGS(4) /* Read */;
    data    PT_LOAD FLAGS(6) /* Read | Write */;

    evt     PT_LOAD FLAGS(5) /* Read | Execute */;
}

MEMORY
{
    ccrt0 : ORIGIN = 0x040006000, LENGTH = 4K
    glob  : ORIGIN = 0x040020000, LENGTH = 128K
    tzram : ORIGIN = 0x07C010000, LENGTH = 64K
    /*
        The warmboot crt0 is preceeded by the exception vector page and the L2 and L3 translation tables.
        Normally the main code immediately follows the warmboot crt0, aligned to 256 bytes.
        We can't ensure or replicate that behavior properly so we'll just give 2K to the warmboot crt0.
    */
    wcrt0   : ORIGIN = ORIGIN(tzram) + 12K, LENGTH = 2K

    /* 8K are the MMU L2 and L3 tables & 2K from the evt page */
    main    : ORIGIN = 0x1F01E0000                  + LENGTH(wcrt0),            LENGTH = LENGTH(tzram) - LENGTH(pk2ldr) - LENGTH(evt) - LENGTH(wcrt0) - 10K
    pk2ldr  : ORIGIN = ORIGIN(main) - LENGTH(wcrt0) + LENGTH(tzram),            LENGTH = 8K
    /* The first half of the page are exception entry stacks, the other half are the vectors themselves */
    evt     : ORIGIN = ORIGIN(pk2ldr)               + 40K + 2K,                 LENGTH = 2K
}

SECTIONS
{
    PROVIDE(__start__ = 0x040006000);
    . = ABSOLUTE(__start__);

    .cold_crt0 :
    {
        . = ALIGN(64);
        __cold_crt0_start__ = ABSOLUTE(.);
        __glob_origin__ = ORIGIN(glob);
        KEEP (*(.cold_crt0.text.start)) /* MUST be first */
        KEEP (*(.cold_crt0.text*))
        KEEP (coldboot_init.o(.text*))
        *(.cold_crt0.rodata*)
        coldboot_init.o(.rodata*)
        *(.cold_crt0.data*)
        coldboot_init.o(.data*)
        . = ALIGN(8);
        *(.cold_crt0.bss*)
        coldboot_init.o(.bss* COMMON)
        . = ALIGN(64);
        __cold_crt0_end__ = ABSOLUTE(.);
    } >ccrt0 AT>glob :ccrt0

    .pk2ldr :
    {
        . = ALIGN(4096);
        __pk2ldr_lma__   = LOADADDR(.pk2ldr);
        __pk2ldr_start__ = ABSOLUTE(.);
        KEEP (package2.o(.text*))
        package2.o(.rodata*)
        package2.o(.data*)
        . = ALIGN(32);
    } >pk2ldr AT>glob :pk2ldr

    .pk2ldr.bss (NOLOAD) :
    {
        . = ALIGN(32);
        __pk2ldr_bss_start__ = ABSOLUTE(.);
        package2.o(.bss* COMMON)
        . = ALIGN(32);
        __pk2ldr_end__ = ABSOLUTE(.);
    } >pk2ldr :NONE

    .vectors :
    {
        . = ALIGN(2048);
        __vectors_lma__   = LOADADDR(.vectors);
        __vectors_start__ = ABSOLUTE(.);
        KEEP (*(.vectors*))
        . = ALIGN(8);
        __vectors_end__ = ABSOLUTE(.);
    } >evt AT>glob :evt

    .warm_crt0 :
    {
        . = ALIGN(64);
        __warmboot_crt0_lma__   = LOADADDR(.warm_crt0);
        __warmboot_crt0_start__ = ABSOLUTE(.);
        KEEP (*(.warm_crt0.text.start)) /* Should be first */
        KEEP (*(.warm_crt0.text*))
        KEEP (warmboot_init.o(.text*))
        *(.warm_crt0.rodata*)
        warmboot_init.o(.rodata*)
        *(.warm_crt0.data*)
        warmboot_init.o(.data*)
        . = ALIGN(8);
        *(.warm_crt0.bss*)
        warmboot_init.o(.bss*)
        . = ALIGN(64);
        __warmboot_crt0_end__ = ABSOLUTE(.);
    } >wcrt0 AT>glob :wcrt0

    /* =========== CODE section =========== */

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
        . = ALIGN(32);
    } >main AT>glob :code

    .init :
    {
        KEEP( *(.init) )
        . = ALIGN(8);
    } >main AT>glob :code

    .plt :
    {
        *(.plt)
        *(.iplt)
        . = ALIGN(8);
    } >main AT>glob :code

    .fini :
    {
        KEEP( *(.fini) )
        . = ALIGN(8);
    } >main AT>glob :code

    /* =========== RODATA section =========== */

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(8);
    } >main AT>glob :rodata

    .eh_frame_hdr     : ONLY_IF_RO { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } >main AT>glob :rodata
    .eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob :rodata
    .gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob :rodata
    .gnu_extab        : ONLY_IF_RO { *(.gnu_extab*) } >main AT>glob :rodata
    .exception_ranges : ONLY_IF_RO { *(.exception_ranges .exception_ranges*) } >main AT>glob :rodata

    .note.gnu.build-id : { *(.note.gnu.build-id) } >main AT>glob :rodata
    .hash              : { *(.hash) } >main AT>glob :rodata
    .gnu.hash          : { *(.gnu.hash) } >main AT>glob :rodata
    .gnu.version       : { *(.gnu.version) } >main AT>glob :rodata
    .gnu.version_d     : { *(.gnu.version_d) } >main AT>glob :rodata
    .gnu.version_r     : { *(.gnu.version_r) } >main AT>glob :rodata

    /* =========== DATA section =========== */

    .eh_frame_hdr     : ONLY_IF_RW { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } >main AT>glob :data
    .eh_frame         : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob :data
    .gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob :data
    .gnu_extab        : ONLY_IF_RW { *(.gnu_extab*) } >main AT>glob :data
    .exception_ranges : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) } >main AT>glob :data

    .preinit_array ALIGN(8) :
    {
        PROVIDE (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE (__preinit_array_end = .);
        . = ALIGN(8);
    } >main AT>glob :data

    .init_array :
    {
        PROVIDE (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = .);
    } >main AT>glob :data

    .fini_array :
    {
        PROVIDE (__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE (__fini_array_end = .);
        . = ALIGN(8);
    } >main AT>glob :data

    .ctors :
    {
        KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
    } >main AT>glob :data

    .dtors :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        . = ALIGN(8);
    } >main AT>glob :data

    .got            : { __got_start__ = ABSOLUTE(.); *(.got) *(.igot) } >main AT>glob :data
    .got.plt        : { *(.got.plt)  *(.igot.plt)  __got_end__ = ABSOLUTE(.);} >main AT>glob :data

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
        . = ALIGN(32);
    } >main AT>glob :data

    .bss (NOLOAD) :
    {
        . = ALIGN(32);
        __main_bss_start__  = ABSOLUTE(.);
        __loaded_end_lma__  = LOADADDR(.bss);
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(32);
        __main_end__ = ABSOLUTE(.);
    } >main :NONE

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

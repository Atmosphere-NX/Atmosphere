OUTPUT_ARCH(aarch64)
ENTRY(_start)

MEMORY
{
    NULL           : ORIGIN = 0,           LENGTH = 4K
    unused_region  : ORIGIN =      0x1000, LENGTH = 4K
    iram_boot_code : ORIGIN = 0x040032000, LENGTH = 4K
    iram_boot_keys : ORIGIN = 0x040033000, LENGTH = 4K
    tzram          : ORIGIN = 0x07C010000, LENGTH = 64K

    /* Warmboot code follows the vectors in memory. */
    /* However, we can't know for sure how big warmboot is, so we'll just say it's 2K. */
    warmboot_text : ORIGIN = ORIGIN(tzram) + 10K, LENGTH = 2K

    main       : ORIGIN = 0x1F00C0000, LENGTH = 48K
    debug_code : ORIGIN = 0x1F0150000, LENGTH = 16K
    tzram_boot : ORIGIN = 0x1F01C0800, LENGTH = 6K

    glob       : ORIGIN = 0x040032000, LENGTH = 128K
}

SECTIONS
{
	.metadata :
	{
        . = ALIGN(8);
		KEEP (*(.metadata .metadata.*))
		. = ALIGN(8);
	} >unused_region AT>glob

    PROVIDE(__glob_start__ = ORIGIN(glob));
    . = __glob_start__;

    __bootcode_start__ = ABSOLUTE(.);

	.crt0 :
	{
		KEEP (*(.crt0 .crt0.*))
        KEEP (secmon_crt0_cpp.o(.text*))
        KEEP (secmon_make_page_table.o(.text*))
		*(.crt0.rodata*)
        secmon_crt0_cpp.o(.rodata*)
        secmon_make_page_table.o(.rodata*)
		*(.crt0.data*)
        secmon_crt0_cpp.o(.data*)
        secmon_make_page_table.o(.data*)
		. = ALIGN(8);
	} >iram_boot_code AT>glob

	.preinit_array ALIGN(8) :
	{
		PROVIDE (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE (__preinit_array_end = .);
	} >iram_boot_code AT>glob

	.init_array ALIGN(8) :
	{
		PROVIDE (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		PROVIDE (__init_array_end = .);
	} >iram_boot_code AT>glob

	.fini_array ALIGN(8) :
	{
		PROVIDE (__fini_array_start = .);
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		PROVIDE (__fini_array_end = .);
	} >iram_boot_code AT>glob

	.ctors ALIGN(8) :
	{
		KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
	} >iram_boot_code AT>glob

	.dtors ALIGN(8) :
	{
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
	} >iram_boot_code AT>glob

    .boot_code.fill : {
        FILL(0x00000000);
        . = ORIGIN(iram_boot_code) + 0xFFF;
        BYTE(0x00);
    } >iram_boot_code AT>glob

    .boot_code_volatile_keys : {
		KEEP (*(.volatile_keys .volatile_keys.*))
    } >iram_boot_keys AT>glob

    .boot_keys.fill : {
        FILL(0x00000000);
        . = ORIGIN(iram_boot_keys) + 0xFFF;
        BYTE(0x00);
    } >iram_boot_keys AT>glob

    .debug_code : {
        KEEP (*(.text._ZN3ams3log6PrintfEPKcz .text._ZN3ams3log7VPrintfEPKcSt9__va_list .text._ZN3ams3log4DumpEPKvm))
        KEEP (*(.text._ZN3ams4util10TVSNPrintfEPcmPKcSt9__va_list .text._ZN3ams4util12_GLOBAL__N_114TVSNPrintfImplEPcmPKcSt9__va_list .text._ZZN3ams4util12_GLOBAL__N_114TVSNPrintfImplEPcmPKcSt9__va_listENKUlbmE3_clEbm))
        KEEP (*(.text._ZN3ams4util12_GLOBAL__N_1L14TVSNPrintfImplEPcmPKcSt9__va_list .text._ZZN3ams4util12_GLOBAL__N_1L14TVSNPrintfImplEPcmPKcSt9__va_listENKUlbmE_clEbm))
        KEEP(secmon_exception_handler.o(.text*))
        secmon_exception_handler.o(.rodata*)
        secmon_exception_handler.o(.data*)
    } >debug_code AT>glob

    .debug_code.bss_fill :
    {
        FILL(0x00000000);
        secmon_exception_handler.o(.bss* COMMON)
        . = ORIGIN(debug_code) + LENGTH(debug_code) - 1;
        BYTE(0x00);
    } >debug_code AT>glob

    __bootcode_end__ = ABSOLUTE(.) - ORIGIN(debug_code) + 0x40034000;

    __program_start__ = __bootcode_end__;

    .tzram_boot_code :
    {
        KEEP(secmon_main.o(.text*))
        KEEP(secmon_boot_functions.o(.text*))
        KEEP(secmon_boot_cache.o(.text*))
        KEEP(secmon_boot_config.o(.text*))
        KEEP(secmon_boot_setup.o(.text*))
        KEEP(secmon_boot_rsa.o(.text*))
        KEEP(secmon_package2.o(.text*))
        secmon_main.o(.rodata*)
        secmon_boot_functions.o(.rodata*)
        secmon_boot_cache.o(.rodata*)
        secmon_boot_config.o(.rodata*)
        secmon_boot_setup.o(.rodata*)
        secmon_boot_rsa.o(.rodata*)
        secmon_package2.o(.rodata*)
        secmon_main.o(.data*)
        secmon_boot_functions.o(.data*)
        secmon_boot_cache.o(.data*)
        secmon_boot_config.o(.data*)
        secmon_boot_setup.o(.data*)
        secmon_boot_rsa.o(.data*)
        secmon_package2.o(.data*)
        . = ALIGN(8);
    } >tzram_boot AT>glob

    .tzram_boot_code.bss_fill :
    {
        FILL(0x00000000);
        __boot_bss_start__ = ABSOLUTE(.);
        secmon_main.o(.bss* COMMON)
        secmon_boot_functions.o(.bss* COMMON)
        secmon_boot_cache.o(.bss* COMMON)
        secmon_boot_config.o(.bss* COMMON)
        secmon_boot_setup.o(.bss* COMMON)
        secmon_boot_rsa.o(.bss* COMMON)
        secmon_package2.o(.bss* COMMON)
        __boot_bss_end__ = ABSOLUTE(.);
        . = ORIGIN(tzram_boot) + LENGTH(tzram_boot) - 1;
        BYTE(0x00);
    } > tzram_boot AT>glob

    .vectors :
    {
        KEEP (*(.vectors*))
        . = ALIGN(0x100);
    } >main AT>glob

    .warmboot :
    {
        KEEP (*(.warmboot.text.start)) /* Should be first */
        KEEP (*(.warmboot.text*))
        KEEP(secmon_setup_warm.o(.text*))
        KEEP(*(.text._ZN3ams4tsec4LockEv))
        KEEP (*(.warmboot.rodata*))
        KEEP(secmon_setup_warm.o(.rodata*))
        KEEP (*(.warmboot.data*))
        KEEP(secmon_setup_warm.o(.data*))
    } >warmboot_text AT>glob

	.text ORIGIN(main) + SIZEOF(.vectors) + SIZEOF(.warmboot) :
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		. = ALIGN(8);
	} >main AT>glob

	.init :
	{
		KEEP( *(.init) )
		. = ALIGN(8);
	} >main AT>glob

	.plt :
	{
		*(.plt)
		*(.iplt)
		. = ALIGN(8);
	} >main AT>glob

	.fini :
	{
		KEEP( *(.fini) )
		. = ALIGN(8);
	} >main AT>glob


	/* =========== RODATA section =========== */
	. = ALIGN(8);
	__rodata_start = . ;

	.rodata :
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(8);
	} >main AT>glob

	.eh_frame_hdr     : { __eh_frame_hdr_start = .; *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) __eh_frame_hdr_end = .; } >main AT>glob
	.eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob
	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob
	.gnu_extab        : ONLY_IF_RO { *(.gnu_extab*) } >main AT>glob

	.hash              : { *(.hash) } >main AT>glob

	/* =========== DATA section =========== */
	. = ALIGN(8);
	__data_start = . ;

	.eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob
	.gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob
	.gnu_extab : ONLY_IF_RW { *(.gnu_extab*) } >main AT>glob
	.exception_ranges : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) } >main AT>glob

	__got_start__ = .;

	.got            : { *(.got) *(.igot) } >main AT>glob
	.got.plt        : { *(.got.plt)  *(.igot.plt) } >main AT>glob

	__got_end__ = .;

	.data ALIGN(8) :
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	} >main AT>glob

	.bss ALIGN(8) (NOLOAD) :
	{
        __bss_start__ = ABSOLUTE(.);
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(16);
        __bss_end__ = ABSOLUTE(.);
	} >main AT>glob

    __program_end__ = ABSOLUTE(.);

	/* ==================
	   ==== Metadata ====
	   ================== */

	/* Discard sections that difficult post-processing */
	/DISCARD/ : { *(.group .comment .note .interp) }

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
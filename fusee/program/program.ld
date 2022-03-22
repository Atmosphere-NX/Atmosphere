OUTPUT_ARCH(arm)
ENTRY(_ZN3ams6nxboot5StartEv)

SECTIONS
{
	/* =========== CODE section =========== */

	PROVIDE(__start__ = ORIGIN(main));
	. = __start__;
	__main_start__ = . ;

	.crt0 :
	{
        FILL(0x00000000)
		KEEP (*(.crt0 .crt0.*))
        . = ORIGIN(main) + 0xC0 - 1;
        BYTE(00);
	} >main AT>glob

	.text :
	{
        FILL(0x00000000)
        KEEP(*(.text._ZN3ams4util15GetMicroSecondsEv))
        KEEP(*(.text._ZN3ams4util16WaitMicroSecondsEi))
        KEEP(*(.text.memcpy))
        KEEP(*(.text.memset))
        KEEP(*(.text.memcmp))
        KEEP(*(.text._ZN3ams6nxboot14ShowFatalErrorEPKcz))
        KEEP(*(.text._ZN3ams6nxboot10UncompressEPvjPKvj))
        KEEP(*(.text._ZN3ams6nxboot12RebootToSelfEv))
        _*.o(SORT(.text*))
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
        . = ALIGN(16);
        . = . + 15;
        BYTE(0x00);
	} >main AT>glob

	.init :
	{
		KEEP( *(.init) )
	} >main AT>glob

	.plt :
	{
		*(.plt)
		*(.iplt)
	} >main AT>glob

	.fini :
	{
		KEEP( *(.fini) )
	} >main AT>glob


	/* =========== RODATA section =========== */
	__rodata_start = . ;

	.rodata :
	{
        FILL(0x00000000)
		*(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(16);
        . = . + 15;
        BYTE(0x00);
	} >main AT>glob

	.eh_frame_hdr     : { __eh_frame_hdr_start = .; *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) __eh_frame_hdr_end = .; } >main AT>glob
	.eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob
	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob
	.gnu_extab        : ONLY_IF_RO { *(.gnu_extab*) } >main AT>glob

	.hash              : { *(.hash) } >main AT>glob

	/* =========== DATA section =========== */
	__data_start = . ;

	.eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } >main AT>glob
	.gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } >main AT>glob
	.gnu_extab : ONLY_IF_RW { *(.gnu_extab*) } >main AT>glob
	.exception_ranges : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) } >main AT>glob

	.preinit_array :
	{
		PROVIDE (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE (__preinit_array_end = .);
	} >main AT>glob

	.init_array :
	{
		PROVIDE (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		PROVIDE (__init_array_end = .);
	} >main AT>glob

	.fini_array :
	{
		PROVIDE (__fini_array_start = .);
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		PROVIDE (__fini_array_end = .);
	} >main AT>glob

	.ctors :
	{
		KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
	} >main AT>glob

	.dtors :
	{
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
	} >main AT>glob

	__got_start__ = .;

	.got            : { *(.got) *(.igot) } >main
	.got.plt        : { *(.got.plt)  *(.igot.plt) } >main

	__got_end__ = .;

	.data :
	{
        FILL(0x00000000)
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
        . = ALIGN(16);
        . = . + 15;
        BYTE(0x00);
	} >main AT>glob

	__bss_start__ = .;
	.main.fill :
	{
        FILL(0x00000000)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
        . = ORIGIN(main) + LENGTH(main) - 1;
        BYTE(0x00);
	} >main AT>glob

	__bss_end__ = .;
	__main_end__ = ABSOLUTE(.) ;

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
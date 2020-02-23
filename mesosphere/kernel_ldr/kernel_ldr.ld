OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS
{
	krnlldr PT_LOAD FLAGS(7) /* Read | Write | Execute */;
	dyn     PT_DYNAMIC;
}

SECTIONS
{
	/* =========== CODE section =========== */
	PROVIDE(__start__ = 0x0);
	. = __start__;
	__code_start = . ;

	.crt0 :
	{
		KEEP (*(.crt0 .crt0.*))
		. = ALIGN(8);
	} :krnlldr

	.init :
	{
		KEEP( *(.init) )
		. = ALIGN(8);
	} :krnlldr

	.plt :
	{
		*(.plt)
		*(.iplt)
		. = ALIGN(8);
	} :krnlldr

	.text :
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		. = ALIGN(8);
	} :krnlldr

	.fini :
	{
		KEEP( *(.fini) )
		. = ALIGN(8);
	} :krnlldr

	/* .vectors. */
	. = ALIGN(2K);
	__vectors_start__ = . ;
    .vectors :
	{
		KEEP( *(.vectors) )
		. = ALIGN(8);
	} :krnlldr

	/* =========== RODATA section =========== */
	. = ALIGN(8);
	__rodata_start = . ;

	.rodata :
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(8);
	} :krnlldr

	.eh_frame_hdr     : { __eh_frame_hdr_start = .; *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) __eh_frame_hdr_end = .; } :krnlldr
	.eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } :krnlldr
	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } :krnlldr
	.gnu_extab        : ONLY_IF_RO { *(.gnu_extab*) } : rodata

	.dynamic           : { *(.dynamic) } :krnlldr :dyn
	.dynsym            : { *(.dynsym) } :krnlldr
	.dynstr            : { *(.dynstr) } :krnlldr
	.rela.dyn          : { *(.rela.*) } :krnlldr
	.hash              : { *(.hash) } :krnlldr
	.gnu.hash          : { *(.gnu.hash) } :krnlldr
	.gnu.version       : { *(.gnu.version) } :krnlldr
	.gnu.version_d     : { *(.gnu.version_d) } :krnlldr
	.gnu.version_r     : { *(.gnu.version_r) } :krnlldr

	/* =========== DATA section =========== */
	. = ALIGN(8);
	__data_start = . ;

	.eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } :krnlldr
	.gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } :krnlldr
	.gnu_extab : ONLY_IF_RW { *(.gnu_extab*) } : data
	.exception_ranges : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) } :krnlldr

	.preinit_array ALIGN(8) :
	{
		PROVIDE (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE (__preinit_array_end = .);
	} :krnlldr

	.init_array ALIGN(8) :
	{
		PROVIDE (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		PROVIDE (__init_array_end = .);
	} :krnlldr

	.fini_array ALIGN(8) :
	{
		PROVIDE (__fini_array_start = .);
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		PROVIDE (__fini_array_end = .);
	} :krnlldr

	.ctors ALIGN(8) :
	{
		KEEP (*crtbegin.o(.ctors)) /* MUST be first -- GCC requires it */
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
	} :krnlldr

	.dtors ALIGN(8) :
	{
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
	} :krnlldr

	__got_start__ = .;

	.got            : { *(.got) *(.igot) } :krnlldr
	.got.plt        : { *(.got.plt)  *(.igot.plt) } :krnlldr

	__got_end__ = .;

	.data ALIGN(8) :
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	} :krnlldr

	__bss_start__ = .;
	.bss ALIGN(8) :
	{
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(16);

		/* Reserve space for the stack */
		__stack_start = .;
		. += 0x1000;
		__stack_end = .;
	} :krnlldr
	__bss_end__ = .;

	__end__ = ABSOLUTE(.) ;

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
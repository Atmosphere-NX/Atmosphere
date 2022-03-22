OUTPUT_ARCH(arm)
ENTRY(_ZN3ams6nxboot6loader5StartEv)

MEMORY
{
    NULL        : ORIGIN = 0,          LENGTH = 4K
    data        : ORIGIN = 0x40010000, LENGTH = 0x28000
    loader_stub : ORIGIN = 0x4003D000, LENGTH = 4K
}


SECTIONS
{
	/* =========== CODE section =========== */

	. = ORIGIN(data);
	__data_start__ = . ;

    .crt0 :
    {
        KEEP(*(.crt0 .crt0.*))
        . = ALIGN(8);
    } >data

	/* =========== RODATA section =========== */
	. = ALIGN(8);

	.rodata :
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(8);
	} >data

	.eh_frame_hdr     : { __eh_frame_hdr_start = .; *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) __eh_frame_hdr_end = .; } >data
	.eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } >data
	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } >data
	.gnu_extab        : ONLY_IF_RO { *(.gnu_extab*) } >data

	.hash              : { *(.hash) } >data

	/* =========== DATA section =========== */
	. = ALIGN(8);

	.eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } >data
	.gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } >data
	.gnu_extab : ONLY_IF_RW { *(.gnu_extab*) } >data
	.exception_ranges : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) } >data

	.data ALIGN(8) :
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	} >data

	__bss_start__ = .;
	.bss (NOLOAD) :
	{
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
        . = ALIGN(32);
	} >data :NONE
	__bss_end__ = .;

	__data_end__ = ABSOLUTE(.);


	.loader_stub :
	{
        . = ALIGN(32);
        PROVIDE (__loader_stub_start__  = ABSOLUTE(.));
        PROVIDE (__loader_stub_lma__    = LOADADDR(.loader_stub));
        fusee_loader_main.o(.text*)
        fusee_loader_uncompress.o(.text*)
        fusee_loader_error.o(.text*)
        *(.text.memcpy)
        fusee_loader_main.o(.rodata*)
        fusee_loader_uncompress.o(.rodata*)
        fusee_loader_error.o(.rodata*)
        fusee_loader_main.o(.data*)
        fusee_loader_uncompress.o(.data*)
        fusee_loader_error.o(.data*)
        . = ALIGN(32);
        PROVIDE (__loader_stub_end__ = ABSOLUTE(.));
	} >loader_stub AT>data

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
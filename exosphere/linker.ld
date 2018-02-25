ENTRY(__start_cold)
SECTIONS
{
    /* DUMMY LINKER SCRIPT, REPLACE ASAP */
    . = 0xF0000000;
    
    __start__ = ABSOLUTE(.);

    .text   : ALIGN(4) { *(.text*); . = ALIGN(4); }
    .rodata : ALIGN(4) { *(.rodata*); . = ALIGN(4); }
    .data   : ALIGN(4) { *(.data*); . = ALIGN(8); *(.bss* COMMON); . = ALIGN(8); }

    . = ALIGN(4);

    __end__ = ABSOLUTE(.);
}

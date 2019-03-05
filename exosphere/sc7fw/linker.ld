OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)
SECTIONS
{
    . = 0x40003000;

    __start__ = ABSOLUTE(.);

    .text   : ALIGN(4) { *(.text.start) *(.text*); . = ALIGN(4); }
    .rodata : ALIGN(4) { *(.rodata*); . = ALIGN(4); }
    .bss    : ALIGN(8) { __bss_start__ = .; *(.bss* COMMON); . = ALIGN(8); __bss_end__ = .; }

    . = ALIGN(4);

    __end__ = ABSOLUTE(.);

    __stack_top__    = 0x40005000;
    __stack_bottom__ = 0x40004000;
}
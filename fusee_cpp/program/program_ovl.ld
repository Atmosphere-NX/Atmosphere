MEMORY
{
    NULL  : ORIGIN = 0,          LENGTH = 4K
    glob  : ORIGIN = 0x40001000, LENGTH = 0x3F000
    main  : ORIGIN = 0x40001000, LENGTH = 0x2B000
    ovl   : ORIGIN = 0x4002C000, LENGTH = 0x14000
}

SECTIONS {
    OVERLAY : NOCROSSREFS {
        .ovl_sein {
            fusee_secure_initialize.o(.text*);
            fusee_sdram.o(.text*);
            fusee_secure_initialize.o(.rodata*);
            fusee_sdram.o(.rodata*);
            fusee_secure_initialize.o(.data*);
            fusee_sdram.o(.data*);
            fusee_secure_initialize.o(.bss*);
            fusee_sdram.o(.bss*);
            FILL(0x00000000)
            . = ORIGIN(ovl) + LENGTH(ovl) - 1;
            BYTE(0x00);
        }
    } >ovl AT>glob
}
INSERT AFTER .main.fill
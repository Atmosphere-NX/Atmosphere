MEMORY
{
    NULL  : ORIGIN = 0,          LENGTH = 4K
    main  : ORIGIN = 0x40001000, LENGTH = 0x2B000
    ovl   : ORIGIN = 0x4002C000, LENGTH = 0x14000

    glob  : ORIGIN = ORIGIN(main), LENGTH = LENGTH(main) + LENGTH(ovl) * 3
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
        .ovl_mtc_erista {
            KEEP(*(.text._ZN3ams6nxboot22DoMemoryTrainingEristaEiPv))
            fusee_mtc_erista.o(.text*);
            fusee_mtc_erista.o(SORT(.rodata*));
            fusee_mtc_erista.o(SORT(.data*));
            fusee_mtc_erista.o(SORT(.bss*));
            FILL(0x00000000)
            . = ORIGIN(ovl) + LENGTH(ovl) - 1;
            BYTE(0x00);
        }
        .ovl_mtc_mariko {
            KEEP(*(.text._ZN3ams6nxboot22DoMemoryTrainingMarikoEPbiPv))
            fusee_mtc_mariko.o(.text*);
            fusee_mtc_mariko.o(SORT(.rodata*));
            fusee_mtc_mariko.o(SORT(.data*));
            fusee_mtc_mariko.o(SORT(.bss*));
            FILL(0x00000000)
            . = ORIGIN(ovl) + LENGTH(ovl) - 1;
            BYTE(0x00);
        }
    } >ovl AT>glob
}
INSERT AFTER .main.fill
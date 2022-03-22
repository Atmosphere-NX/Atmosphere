#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <switch.h>
#include "ams_bpc.h"

#define IRAM_PAYLOAD_MAX_SIZE 0x24000
static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE];

void userAppExit(void)
{
    amsBpcExit();
    setsysExit();
    spsmExit();
}

static void reboot_to_payload(void) {
    Result rc = amsBpcSetRebootPayload(g_reboot_payload, IRAM_PAYLOAD_MAX_SIZE);
    if (R_FAILED(rc)) {
        printf("Failed to set reboot payload: 0x%x\n", rc);
    }
    else {
        spsmShutdown(true);
    }
}

int main(int argc, char **argv)
{
    consoleInit(NULL);

    padConfigureInput(8, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeAny(&pad);

    Result rc = 0;
    bool can_reboot = true;

    if (R_FAILED(rc = setsysInitialize())) {
        printf("Failed to initialize set:sys: 0x%x\n", rc);
        can_reboot = false;
    }
    else {
        SetSysProductModel model;
        setsysGetProductModel(&model);
        if (model != SetSysProductModel_Nx && model != SetSysProductModel_Copper) {
            printf("Reboot to payload cannot be used on a Mariko system\n");
            can_reboot = false;
        }
    }

    if (can_reboot && R_FAILED(rc = spsmInitialize())) {
        printf("Failed to initialize spsm: 0x%x\n", rc);
        can_reboot = false;
    }

    if (can_reboot) {
        smExit(); //Required to connect to ams:bpc
        if R_FAILED(rc = amsBpcInitialize()) {
            printf("Failed to initialize ams:bpc: 0x%x\n", rc);
            can_reboot = false;
        }
    }

    if (can_reboot) {
        FILE *f = fopen("sdmc:/atmosphere/reboot_payload.bin", "rb");
        if (f == NULL) {
            printf("Failed to open atmosphere/reboot_payload.bin!\n");
            can_reboot = false;
        } else {
            fread(g_reboot_payload, 1, sizeof(g_reboot_payload), f);
            fclose(f);
            printf("Press [-] to reboot to payload\n");
        }
    }

    printf("Press [L] to exit\n");

    // Main loop
    while(appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (can_reboot && (kDown & HidNpadButton_Minus)) {
            reboot_to_payload();
        }
        if (kDown & HidNpadButton_L)  { break; } // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}

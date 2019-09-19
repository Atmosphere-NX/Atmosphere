#include <stdio.h>
#include <switch.h>

int main(int argc, char **argv)
{
    consoleInit(NULL);
    
    bool can_reboot = true;
    Result rc = bpcInitialize();
    if (R_FAILED(rc)) {
        printf("Failed to initialize bpc: 0x%x\n", rc);
        can_reboot = false;
    } else {
        printf("Press [-] to reboot to payload\n");
    }
         
    printf("Press [L] to exit\n");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        u64 kDown = 0;

        for (int controller = 0; controller < 10; controller++) {
            // hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
            kDown |= hidKeysDown((HidControllerID) controller);
        }

        if (can_reboot && kDown & KEY_MINUS) {
            bpcRebootSystem();
        }
        if (kDown & KEY_L)  { break; } // break in order to return to hbmenu 

        consoleUpdate(NULL);
    }

    if (can_reboot) {
        bpcExit();
    }
    
    consoleExit(NULL);
    return 0;
}

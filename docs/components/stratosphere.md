# Stratosphère
Stratosphère allows customization of the Horizon OS and Switch kernel. It includes custom sysmodules that extend the kernel and provide new features. It also includes a reimplementation of the loader sysmodules to hook important system actions.

The sysmodules that Stratosphère includes are:
+ [boot](../modules/boot.md): This module boots the system and initalizes hardware.
+ [creport](../modules/creport.md): Reimplementation of Nintendo’s crash report system. Dumps all error logs to the SD card instead of saving them to the NAND and sending them to Nintendo.
+ [fs_mitm](../modules/fs_mitm.md): This module can log, deny, delay, replace, and redirect any request made to the File System.
+ [loader](../modules/loader.md): Enables modifying the code of binaries that are not stored inside the kernel.
+ [pm](../modules/pm.md): Reimplementation of Nintendo’s Process Manager.
+ [sm](../modules/sm.md): Reimplementation of Nintendo’s Service Manager.

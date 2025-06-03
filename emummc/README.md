# emuMMC
*A SDMMC driver replacement for Nintendo's Filesystem Services, by **m4xw***  

### Supported Horizon Versions
**1.0.0 - 20.1.0**

## Features
* Arbitrary SDMMC backend selection  
  **This allows loading eMMC from SD or even SD from eMMC**
* On the fly hooking / patching, fully self-infesting  
  **Only one payload required for all versions!**
* File-based SDMMC backend support (from SD)  
  **This allows loading eMMC images from hekate-backups (split or not)**
* SDMMC device based sector offset (*currently eMMC only*)  
  **Raw partition support for eMMC from SD with less performance overhead**
* Full support for `/Nintendo` folder redirection to a arbitrary path  
  **No 8 char length restriction!**
* exosphere based context configuration  
  **This includes full support for multiple emuMMC images**

## Compiling
### hekate
Run `./build.sh` and copy the produced kipm (Kernel Initial Process Modification) file to `/bootloader/sys/`

### Atmosphere
Run `make`, the resulting kip can be used for code injection via fusee (place at `/atmosphere/emummc.kip`)

## License
**emuMMC is released as GPLv2**

## Credits
* **CTCaer** - The CTCaer hekate fork, file-based emuMMC support, SDMMC driver fixes among other things
* **SciresM, hexkyz** - The Atmosphere project, FS offsets, additional research related to newer FS versions
* **naehrwert** - The hekate project, its SDMMC driver and being very helpful in the early research phase
* **jakibaki** - KIP Inject PoC, used in the early dev phase
* **switchbrew/devkitPro** - devkitA64 and libnx sources

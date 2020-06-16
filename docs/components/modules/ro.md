# ro
This module is a reimplementation of the Horizon OS's `ro` system module, which is responsible for loading dynamic libraries and was introduced by firmware version `3.0.0`.

Atmosphère's reimplementation backports this module's functionalities to firmware versions lower than `3.0.0` where said functionalities were provided by the `ldr` system module instead.

## Extensions
Atmosphère extends this module to allow libraries to be patched by files stored on the SD card.

### NRO Patching
When an NRO is loaded, Atmosphère's reimplementation will search for IPS patch files on the SD card in the following locations.
```
/atmosphere/nro_patches/<patchset name>/<nro build id>.ips
```
This organization allows patch sets affecting multiple NROs to be distributed as a single directory. Patches will be searched for in each patch set directory. The name of each patch file should match the hexadecimal build ID of the NRO to affect, except that trailing zero bytes may be left off. Because the NRO build ID is unique for every NRO, this means patches will only apply to the files they are meant to apply to.

Patch files are accepted in either IPS format or IPS32 format.

# loader

loader is a reimplementation of the loader sysmodule. This module is responsible for creating processes from executable NSO images and registering their access control with the kernel, sm, and fs.

## Atmosphère Extensions

Atmosphère extends this module to allow executables to be replaced or patched by files stored on the SD card. Note that a few services are required for SD card access and therefore cannot be replaced or patched in this manner. This includes psc, bus, and pcv.

### Exefs Replacement

TODO: details on buttons affecting this.

When a process is created, loader will search for several NSO filenames in the title's exefs directory.
These filenames are, in this order:

  - rtld
  - main
  - subsdk0
  - subsdk1
  - ...
  - subsdk9
  - sdk

Each NSO that is found will be loaded into the process contiguously. The process's entrypoint is at the first NSO to be loaded, usually `rtld` or `main`.

Additionally, when a process is loaded, loader will search for a `main.npdm` file in the exefs directory specifying the title's permissions.

Atmosphère extends this functionality by also searching for these files on the SD card. When searching for a file, loader will first check if it exists on the SD card. If it does, that file will be used instead. Otherwise, it will use the copy located in the exefs, if that is present. The following directory will be searched.

```
sdmc:/atmosphere/titles/<title id>/exefs/
```

This allows the replacement of applets, sysmodules, or even games with homebrew versions.

In order to prevent an NSO from being loaded even if it exists in the exefs, loader will also check if a stub file exists. If such a file exists, the NSO will not be loaded. The files should be named like `rtld.stub`, `main.stub`, etc. and may be empty.

### NSO Patching

TODO: details on buttons affecting this.

When an NSO is loaded, the stratosphere implementatin of loader will search for IPS patch files on the SD card in the following locations.
```
sdmc:/atmosphere/exefs_patches/<patchset name>/<nso build id>.ips
```
This organization allows patchsets affecting multiple NSOs to be distributed as a single directory. Patches will be searched for in each patchset directory. The name of each patch file should match the hexadecimal build ID of the NSO to affect, except that trailing zero bytes may be left off. Because the NSO build ID is unique for every NSO, this means patches will only apply to the files they are meant to apply to.

Patch files are accepted in either IPS format or IPS32 format.

Because NSO files are compressed, patch files are not made between the original version of a compressed NSO and the modified version of such an NSO. Instead, they are made between the uncompressed version of an NSO and the modified (and still uncompressed) version of that NSO. This also means that a patch file cannot be manually applied to the compressed version of an NSO; it must be applied to the uncompressed version. The Stratosphere implementation of loader will correctly apply these patches while loading the process regardless of whether the NSO it finds is compressed or not.

When authoring patches, [hactool](https://github.com/SciresM/hactool) can be used to find an NSO's build ID and to uncompress NSOs. Recent versions of the [ReSwitched IDA loaders](https://github.com/reswitched/loaders) can be used to load uncompressed NSOs into IDA in such a way that you can [apply patches to the input file](https://www.hex-rays.com/products/ida/support/idadoc/1618.shtml). From there, any IPS tool can be used to create the patch between the original NSO and the patched NSO. Note that if the NSO you are patching is larger than 16 MiB, you will have to use a tool that supports IPS32.

### HBL Support

TODO

### Button Overrides

TODO

### SM MITM Integration

When the Stratosphere implementation of loader creates a new process, it notifies [sm](sm.md) through the `AtmosphereAssociatePidTidForMitm` command to notify any MITM services of new processes' identities.

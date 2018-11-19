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

Atmosphère can use the loader module in order to turn any game on your Switch's home menu into a launchpoint for the Homebrew Menu, rather than launching it through the album applet. This allows one to launch the Homebrew Menu with access to the ~3.2GB of RAM that the Switch reserves for games and applications, as opposed to the 442MB of RAM we are limited to when launching the Homebrew Menu from the album. This also means that it is no longer necessary to install homebrew as `.nsp` files on your Switch so long as you are using this method, as the only reason to do so is to allow the homebrew to access all of the Switch's available memory.

In order to setup this method you will need the latest release of [hbmenu](https://github.com/switchbrew/nx-hbmenu/releases), and the latest release of [hbloader](https://github.com/switchbrew/nx-hbloader/releases). Place `hbmenu.nro` on the root of your Switch's SD Card, and place `hbl.nsp` in the atmosphere folder. From there, simply configure `loader.ini` in the atmosphere folder by replacing the Title ID in the ini (hbl_tid) (it is the Title ID for the album by default) with the Title ID of whatever game you wish to use to launch the Homebrew Menu. A list of Title IDs for Switch Games can be found [here](https://switchbrew.org/wiki/Title_list/Games). Afterwards you may reinsert your SD Card into your Switch and boot into Atmosphère as you normally would. You should now be able to boot into the Homebrew Menu by launching your designated game of choice.

### Button Overrides

By default `loader.ini` is configured to launch the Homebrew Menu when launching the game normally, and launching the game when selecting the game while holding down R. If you wish to change this, you can modify the override_key section of `loader.ini`. Placing an exclamation point in front of whatever button you wish to use will make it so that you will only launch the actual game while holding down that button, otherwise you will go into the Homebrew Menu. Removing the exclamation point will reverse this, meaning that you will boot into the Homebrew Menu only while holding down the assigned button when launching the game.

For example, `override_key=!R` will run the game only while holding down R when launching it, otherwise it will boot into the Homebrew Menu. `override_key=R` will only boot into the Homebrew Menu while holding down R when launching the game, otherwise it will launch the game as normal.

### SM MITM Integration

When the Stratosphere implementation of loader creates a new process, it notifies [sm](sm.md) through the `AtmosphereAssociatePidTidForMitm` command to notify any MITM services of new processes' identities.

### IPC: AtmosphereSetExternalContentSource

An additional command is added to the [`ldr:shel`](https://reswitched.github.io/SwIPC/ifaces.html#nn::ro::detail::ILdrShellInterface) interface, called `AtmosphereSetExternalContentSource`. It's command ID is `65000` on all system firmware versions. It takes a `u64 tid` and returns a server-side session handle. The client is expected to implement the `IFileSystem` interface on the returned handle. The next time the title specified by the given title ID is launched, its ExeFS contents will be loaded from the custom `IFileSystem` instead of from SD card or original ExeFS. NSOs loaded from external content source may still be subject to exefs IPS patches. After the title is launched, the `IFileSystem` is closed and the external content source override is removed. If `AtmosphereSetExternalContentSource` is called on a title that already has an external content source set for it, the existing one will be removed and replaced with the new one. It is illegal to call `AtmosphereSetExternalContentSource` while the title is being launched.

The `IFileSystem` only needs to implement `OpenFile`. The paths received by the `IFileSystem`'s `OpenFile` command begin with slashes, as in `/main`, `/rtld`, and `/main.npdm`. A result code of 0x202 should be returned if the file does not exist. The `IFile`s returned from `OpenFile` only need to implement `Read` and `GetSize`.

The SwIPC definition for the `AtmosphereSetExternalContentSource` command follows.
```
interface nn::ldr::detail::IShellInterface is ldr:shel {
  ...
  [65000] AtmosphereSetExternalContentSource(u64 tid) -> handle<copy, session_server> ifilesystem_handle;
}
```

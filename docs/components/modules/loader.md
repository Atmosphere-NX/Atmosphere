# loader
This module is a reimplementation of the Horizon OS's `ldr` system module, which is responsible for creating processes from executable NSO images and registering their access control.

## Extensions
Atmosphère extends this module to allow executables to be replaced or patched by files stored on the SD card. Note that a few services are required for SD card access and therefore cannot be replaced or patched in this manner.

### Exefs Replacement
Atmosphère's reimplementation allows replacing executable files in the file system.

#### Partition Replacement
It is possible to replace the full exefs partition at once with a PFS0 file. In that case, Atmosphère will load the following file:
```
/atmosphere/contents/<program id>/exefs.nsp
```

#### File Replacement
When a process is created, loader will search for several NSO filenames in the program's exefs directory.
These filenames are, in this order:
  - rtld
  - main
  - subsdk0
  - subsdk1
  - ...
  - subsdk9
  - sdk

Each NSO that is found will be loaded into the process contiguously. The process's entrypoint is at the first NSO to be loaded, usually `rtld` or `main`.

Additionally, when a process is loaded, loader will search for a `main.npdm` file in the exefs directory specifying the program's permissions.

Atmosphère extends this functionality by also searching for these files on the SD card. When searching for a file, loader will first check if it exists on the SD card. If it does, that file will be used instead. Otherwise, it will use the copy located in the exefs, if that is present. The following directory will be searched:
```
/atmosphere/contents/<program id>/exefs/
```

This allows the replacement of applets, system modules, or even games with homebrew versions.

##### File Stubbing
In order to prevent an NSO from being loaded even if it exists in the exefs, loader will also check if a stub file exists. If such a file exists, the NSO will not be loaded. The files should be named like `rtld.stub`, `main.stub`, etc. and may be empty.

##### Technical Semantics

loader's semantics for content override can (as you may observe from reading the above) be complicated to understand. The following is an abbreviated description of the very technical semantics by which loader decides what content to read when trying to read a file for a program id.

* If an external content filesystem exists for the program id, the external content filesystem is used directly with no further redirection.
* Otherwise, if the program ID is being overridden with [nx-hbloader](https://github.com/switchbrew/nx-hbloader/releases) (see Homebrew Support below), the nsp filesystem for hbl is used directly with no further redirection.
* Otherwise, if content redirection is enabled for the program ID (controlled by a configurable button combination) and a loose file exists on the SD card, the loose file is used.
* Otherwise, if a stub file exists, a "Not Found" error is returned.
* Otherwise, if an SD card executable filesystem ("exefs.nsp") exists, it is used without further redirection.
* Finally, the "real"/base code file system is used without further redirection.

In addition, there are a few other technical details relevant to Atmosphere's redirection:
* When overriding with nx-hbloader, the real code filesystem must exist. When "main.npdm" (a program capabilities descriptor file) is read, the content from the real code filesystem is read in order to determine whether an applet or an application is being overridden. This allows nx-hbloader to automatically support both applet and application environments.
* When overriding applications, the real code filesystem must exist and contain valid content. This is required to perform accurate-to-Nintendo content verification procedures.
* When programs are launched, both a program id and a "storage id" are specified by the launch requester. When the storage id specified is "none" (normally always invalid), Atmosphere assumes that a custom system module is attempting to be launched. This removes the aforementioned requirement on base content validity; the above procedure is still used to determine how to redirect content, however reads to the "real"/base code file system may return "Not Found" errors if the real/base code file system does not exist.

### NSO Patching
When an NSO is loaded, Atmosphère's reimplementation will search for IPS patch files on the SD card in the following locations.
```
/atmosphere/exefs_patches/<patchset name>/<nso build id>.ips
```

This organization allows patch sets affecting multiple NSOs to be distributed as a single directory and also allows patches from multiple patch sets to be stacked. Patches will be searched for in each patch set directory. The name of each patch file should match the hexadecimal build ID of the NSO to affect, except that trailing zero bytes may be left off. Because the NSO build ID is unique for every NSO, this means patches will only apply to the files they are meant to apply to.

Patch files are accepted in either IPS format or IPS32 format.

Because NSO files are compressed, patch files are not made between the original version of a compressed NSO and the modified version of such an NSO. Instead, they are made between the uncompressed version of an NSO and the modified (and still uncompressed) version of that NSO. This also means that a patch file cannot be manually applied to the compressed version of an NSO; it must be applied to the uncompressed version. Atmosphère's reimplementation will correctly apply these patches while loading the process regardless of whether the NSO it finds is compressed or not.

When authoring patches, [hactool](https://github.com/SciresM/hactool) can be used to find an NSO's build ID and to uncompress NSOs. Recent versions of the [ReSwitched IDA loaders](https://github.com/reswitched/loaders) can be used to load uncompressed NSOs into IDA in such a way that you can [apply patches to the input file](https://www.hex-rays.com/products/ida/support/idadoc/1618.shtml). From there, any IPS tool can be used to create the patch between the original NSO and the patched NSO. Note that if the NSO you are patching is larger than 16 MiB, you will have to use a tool that supports IPS32.

### Homebrew Support
Atmosphère provides first class support for [nx-hbloader](https://github.com/switchbrew/nx-hbloader/releases) and [nx-hbmenu](https://github.com/switchbrew/nx-hbmenu/releases).

Launching of the nx-hbloader process is controlled by configurable button inputs. See [here](../../features/configurations.md) for more detailed information.

In addition, loader has extensions to enable homebrew to launch web applets. This normally requires the application launching the applet to have HTML Manual content inside an installed NCA. Atmosphère's reimplementation will automatically ensure that the commands used to check this succeed, and will redirect the relevant file system to the `/atmosphere/hbl_html/` subdirectory.

### IPC Commands
Atmosphère's reimplementation extends the HIPC loader services' API with several custom commands.

The SwIPC definition for the `ldr:pm` extension commands follows:
```
interface ams::ldr::pm::ProcessManagerInterface is ldr:pm {
  ...
  [65000] AtmosphereHasLaunchedProgram(ncm::ProgramId program_id) -> sf::Out<bool> out;
  [65001] AtmosphereGetProgramInfo(ncm::ProgramLocation &loc) -> sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status;
  [65002] AtmospherePinProgram(ncm::ProgramLocation &loc, cfg::OverrideStatus &override_status) -> sf::Out<PinId> out_id;
}
```

The SwIPC definition for the `ldr:dmnt` extension commands follows:
```
interface ams::ldr::dmnt::DebugMonitorInterface is ldr:dmnt {
  ...
  [65000] AtmosphereHasLaunchedProgram(ncm::ProgramId program_id) -> sf::Out<bool> out;
}
```

The SwIPC definition for the `ldr:shel` extension commands follows:
```
interface ams::ldr::shell::ShellInterface is ldr:shel {
  ...
  [65000] AtmosphereRegisterExternalCode(ncm::ProgramId program_id) -> sf::OutMoveHandle out;
  [65001] AtmosphereUnregisterExternalCode(ncm::ProgramId program_id);
}
```

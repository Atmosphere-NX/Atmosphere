# Changelog
## 0.8.4
+ Support for 7.0.0/7.0.1 was added.
  + This is facilitated through a new payload, `sept`, which can be signed, encrypted, and then loaded by Nintendo's TSEC firmware.
  + `sept` will derive the keys needed to boot new firmware, and then load `sept/payload.bin` off the SD card and jump to it.
+ Recognition of applications for override/mitm has been improved.
  + Nintendo's official Title ID range (`0x0100000000000000`-`0x01FFFFFFFFFFFFFF`) is now enforced.
+ A deadlock condition was fixed involving libstratosphere mitm sysmodules.
+ Kernel patches for JIT support were added (Thanks, @m4xw!).
  + These loosen restrictions on caller process in svcControlCodeMemory.
+ `set.mitm` and `fs.mitm` were merged into a single `ams_mitm` sysmodule.
  + This saves a process ID, allowing users to run one additional process up to the 0x40 process limit.
+ A `bpc.mitm` component was added, performing custom behavior on shutdown/reboot requests from `am` or applications.
  + Performing a reboot from the reboot menu now reboots to atmosphere. This can be configured via `system_settings.ini`.
  + Performing a shutdown from the reboot menu now works properly with AutoRCM, and does a real shutdown.
+ General system stability improvements to enhance the user's experience.
## 0.8.3
+ A custom warmboot firmware was implemented, which does not perform anti-downgrade fuse checks.
  + This fixes sleep mode when using a downgraded NAND.
  + This also removes Atmosphère's final dependency on Nintendo's encrypted PK11 binary; all components are now re-implemented.
+ The ExternalContentSource API was changed to not clear on failure.
+ Content override now supports an "app" setting, that causes all applications to be overridden with HBL instead of a specific title.
  + Note: because override keys are system-wide, using this setting will prevent using mods in games (as every game will be HBL).
+ A bug was fixed causing incorrect fatal-error output when svcBreak was called on 5.0.0+.
+ An extension was added to set.mitm to support customization of system settings.
  + These are controlled by `atmosphere/system_settings.ini`, see [here](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/modules/set_mitm.md) for documentation.
+ An extension was added to sm, adding a new `sm:dmnt` service.
  + This can be used by a debug monitor in order to debug the registration state of various other services.
+ A bug was fixed in the MitM API that could sometimes cause a system hang during boot.
+ A change was made to the MitM API: in cases where sm would have returned 0xE15 when installing a mitm service, it now defers the result (following GetService semantics).
+ Support for booting into maintenance mode by holding +/- was added to PM.
+ An extension was added to exosphere, adding a custom SMC that allows for DMA to IRAM.
+ In addition, smcGetConfig was extended to reboot to a payload in IRAM at 0x40010000 when ConfigItem 65001 is set to 2.
  + Fatal will now use this to reboot to sdmc:/atmosphere/reboot_payload.bin if present, when a vol button is pressed.
  + An example homebrew ("reboot_to_payload") was also written and is now included with Atmosphère.
+ General system stability improvements to enhance the user's experience.
## 0.8.2
+ A number of bugs were fixed causing users to sometimes see `Key Derivation Failed!`.
  + KFUSE clock enable timings have been adjusted to allow time to stabilize before TSEC is granted access.
  + A race condition was fixed that could cause wrong key data to be used on 6.2.0
  + The TSEC firmware is now retried on failure, fixing a failure affecting ~1/50 boots on 6.2.0.
+ A bug was fixed causing some modules to not work on firmware 1.0.0.
+ A bug was fixed causing sleep mode to not work with debugmode enabled.
  + As a result, debugmode is now enabled in the default BCT.ini.
+ General system stability improvements to enhance the user's experience.
## 0.8.1
+ A bug was fixed causing users to see `Failed to enable SMMU!` if fusee had previously rebooted.
  + This message will still occur sporadically if fusee is not launched from coldboot, but it can never happen twice in a row.
+ A race condition was fixed in Atmosphere `bis_protect` functionality that could cause NS to be able to overwrite BCT public keys.
  + This sometimes broke AutoRCM protection, the current fix has been tested on hardware and verified to work.
+ Support was added for enabling `debugmode` based on the `exosphere` section of `BCT.ini`:
  + Setting `debugmode = 1` will cause exosphere to tell the kernel that debugmode is active.
  + Setting `debugmode_user = 1` will cause exosphere to tell userland that debugmode is active.
  + These are completely independent of one another, allowing fine control of system behavior.
+ Support was added for `nogc` functionality; thanks to @rajkosto for the patches.
  + By default, `nogc` patches will automatically apply if the user is booting into 4.0.0+ with fuses from <= 3.0.2.
  + Users can override this functionality via the `nogc` entry in the `stratosphere` section of `BCT.ini`:
    + Setting `nogc = 1` will force enable `nogc` patches.
    + Setting `nogc = 0` will force disable `nogc` patches.
  + If patches are enabled but not found for the booting system, a fatal error will be thrown.
    + This should prevent running FS without `nogc` patches after updating to an unsupported system version.
+ An extension was added to `exosphere` allowing userland applications to cause the system to reboot into RCM:
  + This is done by calling smcSetConfig(id=65001, value=<nonzero>); user homebrew can use splSetConfig for this.
+ On fatal error, the user can now choose to perform a standard reboot via the power button, or a reboot into RCM via either volume button. 
+ A custom message was added to `fatal` for when an Atmosphère API version mismatch is detected (2495-1623).
+ General system stability improvements to enhance the user's experience.
## 0.8.0
+ A custom `fatal` system module was added.
  + This re-implements and extends Nintendo's fatal module, with the following features:
    + Atmosphère's `fatal` does not create error reports.
    + Atmosphère's `fatal` draws a custom error screen, showing registers and a backtrace.
    + Atmosphère's `fatal` attempts to gather debugging info for all crashes, and not just ones that include info.
    + Atmosphère's `fatal` will attempt saving reports to the SD, if a crash report was not generated by `creport`.
+ Title flag handling was changed to prevent folder clutter.
  + Instead of living in `atmosphere/titles/<tid>/%s.flag`, flags are now located in `atmosphere/titles/<tid>/flags/%s.flag`
    + The old format will continue to be supported for some time, but is deprecated.
  + Flags can now be applied to HBL by placing them at `atmosphere/flags/hbl_%s.flag`.
+ Changes were made to the mitm API, greatly improving caller semantics.
  + `sm` now informs mitm services of a new session's process id, enabling custom handling based on title id/process id.
+ smhax is no longer enabled, because it is no longer needed and breaks significant functionality.
  + Users with updated HBL/homebrew should see no observable differences due to this change.
+ Functionality was added implementing basic protections for NAND from userland homebrew:
  + BOOT0 now has write protection for the BCT public key and keyblob regions.
    + The `ns` sysmodule is no longer allowed to write the BCT public keys; all other processes can.
      + This should prevent system updates from removing AutoRCM.
    + No processes should be allowed to write to the keyblob region.
  + By default, BIS partitions other than BOOT0 are now read-only, and CAL0 is neither readable nor writable.
    + Adding a `bis_write` flag for a title will allow it to write to BIS.
    + Adding a `cal_read` flag for a title will allow it to read CAL0.
  + An automatic backup is now made of CAL0 on boot.
    + `fs.mitm` maintains a file handle to this backup, so userland software cannot read it.
  + To facilitate this, `fs.mitm` now mitms all sessions for non-system modules; content overriding has been made separate from service interception.
  + Please note: these protections are basic, and sufficiently malicious homebrew ++can defeat them++.
    + Please be careful to only run homebrew software from sources that you trust.
+ A bug involving HDCP titles crashing on newer firmwares was fixed.
+ Support was added for system version 6.2.0; our thanks to @motezazer for his invaluable help.
  + By default, new keys will automatically be derived without user input.
  + Support is also present for loading new keys from `atmosphere/prod.keys` or `atmosphere/dev.keys`
+ General system stability improvements to enhance the user's experience.
## 0.7.5
+ DRAM training was added to fusee-secondary, courtesy @hexkyz.
  + This greatly improves the speed of memory accesses during boot, resulting in a boot time that is ~200-400% faster.
+ creport has had its code region detection improved.
  + Instead of only checking one of the crashing thread's PC/LR for code region presence, creport now checks both + every address in the stacktrace. This is also now done for every thread.
    + This matches the improvement Nintendo added to official creport in 6.1.0.
  + The code region detection heuristic was further improved by checking whether an address points to .rodata or .rwdata, instead of just .text.
  + This means that a crash appears in a loaded NRO (or otherwise discontiguous) code region, creport will be able to detect all active code regions, and not just that one. 
## 0.7.4
+ [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere) has been completely refactored/rewritten, and split into its own, separate submodule.
  + While this is mostly "under the hood" for end-users, the refactor is faster (improving both boot-time and runtime performance), more accurate (many of the internal IPC structures are now bug-for-bug compatible with Nintendo's implementations), and significantly more stable (it fixes a large number of bugs present in the old library).
  + The refactored API is significantly cleaner and easier to write system module code for, which should improve/speed up development of stratosphere.
  + Developers looking to write their own custom system modules for the Switch can now easily include libstratosphere as a submodule in their projects.
+ Loader was extended to add a new generic way to redirect content (ExternalContentSources), courtesy @misson20000:
  + A new command was added to ldr:shel, taking in a tid to redirect and returning a session handle.
  + When the requested TID is loading, Loader will query the handle as though it were an IFileSystem.
    + This allows clients to generically define their own filesystems, and override content with them in loader.
+ fs.mitm has gotten several optimizations that should improve its performance and stability:
  + RomFS redirection now only occurs when there is content to redirect, even if the title is being mitm'd elsewhere.
  + A cache is now maintained of the active data storage, if any, for all opened title IDs. This means if two processes both try to open the same archive, fs.mitm won't duplicate any of its work.
  + RomFS metadata is now cached to the SD card on build instead of being persisted in memory -- this greatly reduces memory footprint and allows fs.mitm to redirect more titles simultaneously than before.
+ A number of bugs were fixed, including:
  + A resource leak was fixed in process creation. This fixes crashes that occur when a large number (>32) games have been launched since the last reboot.
  + fs.mitm no longer errors when receiving a zero-sized buffer. This fixes crashes in some games, including The Messenger.
  + Multi-threaded server semantics should no longer cause deadlocks in certain circumstances. This fixes crashes in some games, including NES Classics.
  + PM now only gives full FS permissions to the active KIPs. This fixes a potential crash where new processes might be unable to be registered with FS.
+ The `make dist` target now includes the branch in the generated zip name.
+ General system stability improvements to enhance the user's experience.
## 0.7.3
+ Loader and fs.mitm now try to reload loader.ini before reading it. This allows for changing the override button combination/HBL title id at runtime.
+ Added a MitM between set:sys and qlaunch, used to override the system version string displayed in system settings.
  + The displayed system version will now display `<Actual version> (AMS <x>.<y>.<z>)`.
+ General system stability improvements to enhance the user's experience.
## 0.7.2
+ Fixed a bug in fs.mitm's LayeredFS read implementation that caused some games to crash when trying to read files.
+ Fixed a bug affecting 1.0.0 that caused games to crash with fatal error 2001-0106 on boot.
+ Improved filenames output by the make dist target.
+ General system stability improvements to enhance the user's experience.

## 0.7.1
+ Fixed a bug preventing consoles on 4.0.0-4.1.0 from going to sleep and waking back up.
+ Fixed a bug preventing consoles on < 4.0.0 from booting without specific KIPs on the SD card.
+ An API was added to Atmosphère's Service Manager for deferring acquisition of all handles for specific services until after early initialization is completed.
+ General system stability improvements to enhance the user's experience.

## 0.7.0
+ First official release of Atmosphère.
+ Supports the following featureset:
  + Fusée, a custom bootloader.
    + Supports loading/customizing of arbitrary KIPs from the SD card.
    + Supports loading a custom kernel from the SD card ("/atmosphere/kernel.bin").
    + Supports compile-time defined kernel patches on a per-firmware basis.
    + All patches at paths like /atmosphere/kip_patches/<user-defined patch name>/<SHA256 of KIP>.ips will be applied to the relevant KIPs, allowing for easy distribution of patches supporting multiple versions.
      + Both the IPS and IPS32 formats are supported.
    + All patches at paths like /atmosphere/kernel_patches/<user-defined patch name>/<SHA256 of Kernel>.ips will be applied to the kernel, allowing for easy distribution of patches supporting multiple versions.
      + Both the IPS and IPS32 formats are supported.
    + Configurable by editing BCT.ini on the SD card.
    + Atmosphère should also be launchable by the alternative hekate bootloader, for those who prefer it.
  + Exosphère, a fully-featured custom secure monitor.
    + Exosphere is a re-implementation of Nintendo's TrustZone firmware, fully replicating all of its features.
    + In addition, it has been extended to provide information on current Atmosphere API version, for homebrew wishing to make use of it.
  + Stratosphère, a set of custom system modules. This includes:
    + A loader system module.
      + Reimplementation of Nintendo's loader, fully replicating all original functionality.
      + Configurable by editing /atmosphere/loader.ini
      + First class support for the Homebrew Loader.
        + An exefs NSP (default "/atmosphere/hbl.nsp") will be used in place of the victim title's exefs.
        + By default, HBL will replace the album applet, but any application should also be supported.
      + Extended to support arbitrary redirection of executable content to the SD card.
        + Files will be preferentially loaded from /atmosphere/titles/<titleid>/exefs/, if present.
        + Files present in the original exefs a user wants to mark as not present may be "stubbed" by creating a .stub file on the SD.
        + If present, a PFS0 at /atmosphere/titles/<titleid>/exefs.nsp will fully replace the original exefs.
        + Redirection is optionally toggleable by holding down certain buttons (by default, holding R disables redirection).
      + Full support for patching NSO content is implemented.
        + All patches at paths like /atmosphere/exefs_patches/<user-defined patch name>/<Hex Build-ID for NSO to patch>.ips will be applied, allowing for easy distribution of patches supporting multiple firmware versions and/or titles.
        + Both the IPS and IPS32 formats are supported.
      + Extended to support launching content from loose executable files on the SD card, without requiring any official installation.
        + This is done by specifying FsStorageId_None on launch.
    + A service manager system module.
      + Reimplementation of Nintendo's service manager, fully replicating all original functionality.
      + Compile-time support for reintroduction of "smhax", allowing clients to optionally skip service access verification by skipping initialization.
      + Extended to allow homebrew to acquire more handles to privileged services than Nintendo natively allows.
      + Extended to add a new API for installing Man-In-The-Middle listeners for arbitrary services.
        + API can additionally be used to safely detect whether a service has been registered in a non-blocking way with no side-effects.
        + Full API documentation to come.
    + A process manager system module.
      + Reimplementation of Nintendo's process manager, fully replicating all original functionality.
      + Extended to allow homebrew to acquire handles to arbitrary processes, and thus read/modify system memory without blocking execution.
      + Extended to allow homebrew to retrieve information about system resource limits.
      + Extended by embedding a full, extended implementation of Nintendo's boot2 system module.
        + Title launch order has been optimized in order to grant access to the SD card faster.
        + The error-collection system module is intentionally not launched, preventing many system telemetry error reports from being generated at all.
        + Users may place their own custom sysmodules on the SD card and flag them for automatic boot2 launch by creating a /atmosphere/titles/<title ID>/boot2.flag file on their SD card.
    + A custom fs.mitm system module.
      + Uses Atmosphère's MitM API in order to provide an easy means for users to modify game content.
      + Intercepts all FS commands sent by games, with special handling for commands used to mount RomFS/DLC content to enable easy creation and distribution of game/DLC mods.
        + fs.mitm will parse the base RomFS image for a game, a RomFS image located at /atmosphere/titles/<title ID>/romfs.bin, and all loose files in /atmosphere/titles/<title ID>/romfs/, and merge them together into a single RomFS image.
        + When merging, loose files are preferred to content in the SD card romfs.bin image, and files from the SD card image are preferred to those in the base image.
      + Can additionally be used to intercept commands sent by arbitrary system titles (excepting those launched before SD card is active), by creating a /atmosphere/titles/<title ID>/fsmitm.flag file on the SD card.
      + Can be forcibly disabled for any title, by creating a /atmosphere/titles/<title ID>/fsmitm_disable.flag file on the SD card.
      + Redirection is optionally toggleable by holding down certain buttons (by default, holding R disables redirection).
    + A custom crash report system module.
      + Serves as a drop-in replacement for Nintendo's own creport system module.
      + Generates detailed, human-readable reports on system crashes, saving to /atmosphere/crash_reports/<timestamp>_<title ID>.log.
      + Because reports are not sent to the erpt sysmodule, this disables all crash report related telemetry.
  + General system stability improvements to enhance the user's experience.

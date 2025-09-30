# Changelog
## 1.9.5
+ Basic support was added for 20.5.0.
+ General system stability improvements to enhance the user's experience.
## 1.9.4
+ Basic support was added for 20.4.0.
+ An issue was fixed in `exosphère`'s register accessilibity tables (thanks @CTCaer).
  + I believe this had no impact on official code, though it would have prevented some homebrew from interacting correctly with the MC0/MC1 registers.
+ An issue was fixed that could cause a deadlock when building multiple romfs images simultaneously (thanks @Ereza).
  + This fixes support for certain mods, e.g. system language translations overriding content for both overlayDisp and qlaunch.
+ General system stability improvements to enhance the user's experience.
## 1.9.3
+ Basic support was added for 20.3.0.
+ Compatibility was fixed for loading mods with KOTOR 2 (star wars).
+ General system stability improvements to enhance the user's experience.
## 1.9.2
+ Basic support was added for 20.2.0.
+ USB 3.0 support force-enable was fixed for 20.1.0+.
+ General system stability improvements to enhance the user's experience.
## 1.9.1
+ Basic support was added for 20.1.0.
+ General system stability improvements to enhance the user's experience.
## 1.9.0
+ Basic support was added for 20.0.0.
  + The console should boot and atmosphère should be fully functional. However, not all modules have been fully updated to reflect the latest changes.
    + There shouldn't be anything user visible resulting from this, but it will be addressed in a future atmosphère update.
    + The same action item from 18.0.0 remains, and I believe in my heart of hearts that it will be addressed eventually. Someone has told me they're working on it.
    + There aren't (to my knowledge) outstanding 19.0.0 items any more.
  + **Please note**: As a result of changes made to nintendo's software in 20.0.0, there is roughly 10MB less memory available for custom system modules.
    + We can only steal a maximum of 14MB from the applet pool, down from 40MB.
    + To compensate for this, `ams.mitm`'s heap usage has been reduced by 20MB.
      + To facilitate this, a new helper module (`memlet`) was added, so that memory may be temporarily stolen during the romfs building process.
      + Hopefully, this results in relatively little breakage, however it is possible that user mods which replace extremely large numbers of files in The Legend of Zelda: Tears of the Kingdom may no longer function.
        + If you are affected by this, you will see "Data abort (0x101)" when trying to launch the game with mods.
        + Please reach out to `sciresm` on discord if this occurs to share your error report binary. However, some issues may be impossible to fix.
        + I apologize sincerely if the issue is impossible to resolve, but I have been forced unavoidably to make compromises here, and I think this is the best balance to be struck.
  + `exosphère` was updated to reflect the latest official secure monitor behavior.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `loader` was updated to reflect the latest official behavior.
  + `pm` was updated to reflect the latest official behavior.
  + `ncm` was partially updated to reflect the latest official behavior.
  + `erpt` was updated to reflect the latest official behavior.
+ Atmosphère was updated to use GCC 15/newlib (latest devkitA64/devkitARM releases).
+ A number of improvements were made to the dmnt cheat engine.
  + New instructions were added, and instructions were updated for improved/new functionality.
  + Please see the documents for details -- thanks @tomvita!
+ General system stability improvements to enhance the user's experience.
## 1.8.0
+ Basic support was added for 19.0.0.
  + The console should boot and atmosphère should be fully functional. However, not all modules have been fully updated to reflect the latest changes.
    + There shouldn't be anything user visible resulting from this, but it will be addressed in a future atmosphère update. There is still one action item from 18.0.0 to be addressed, as well.
  + `exosphère` was updated to reflect the latest official secure monitor behavior.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `loader` was updated to reflect the latest official behavior.
  + `pm` was updated to reflect the latest official behavior.
  + `ro` was updated to reflect the latest official behavior.
+ `creport`'s file acces patterns were optimized, greatly improving performance when generating a crash report.
+ Atmosphère now uses `relr` relocations where possible.
  + This reduces the filesize of a number of atmosphère's modules.
+ A number of minor issues were fixed and improvements were made, including:
  + Support was fixed for running Atmosphère on newer units with specific Hynix/Micron DRAM chips.
+ General system stability improvements to enhance the user's experience.
## 1.7.1
+ Support was added for 18.1.0.
+ Atmosphère was updated to use GCC 14/newlib (latest devkitA64/devkitARM releases).
+ Further changes were for 18.0.0:
  + `loader` was updated to reflect the latest official behavior.
+ General system stability improvements to enhance the user's experience.
## 1.7.0
+ Basic support was added for 18.0.0.
  + The console should boot and atmosphère should be fully functional. However, not all modules have been fully updated to reflect the latest changes.
    + There shouldn't be anything user visible resulting from this, but it will be addressed in a future atmosphère update, once I am not traveling so much.
  + `exosphère` was updated to reflect the latest official secure monitor behavior.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `spl` was updated to reflect the latest official behavior.
+ `fusee`'s no longer supports applying IPS patches to KIPs.
  + The only KIPs that are ever present are a) atmosphère modules, b) custom system modules, or c) FS.
  + The IPS subsystem was originally designed to make nogc patches work for FS, but these are now internal, and it appears the literal only kip patches that exist are for piracy.
    + I could not find any kip patches posted anywhere made for any other purpose.
  + It fundamentally does not make sense to slow down boot for every normal user for a feature that has no actual use-case, especially when `fusee` seeks to be a minimal bootloader.
+ Minor improvements were made to atmosphere's gdbstub, including:
  + Support was added for QStartNoAckMode.
  + An issue was fixed that could cause a fatal error when creating too many breakpoints.
+ A number of minor issues were fixed and improvements were made, including:
  + `pt-BR` (`PortugueseBr`) is now accepted as a valid language when overriding game locales.
  + A bug was fixed that could cause atmosphere to incorrectly serialize output object IDs over IPC when using domain objects.
  + A bug was fixed in `pm`'s resource limit boost logic that could potentially cause legitimate boosts to fail in certain circumstances.
  + `loader`/`ro` will now throw a fatal error when using invalid IPS patches that go out of bounds, instead of corrupting memory.
  + Support was fixed for booting using a memory configuration of half of the true available memory (e.g. forcing a 4GB configuration on an 8GB board).
+ General system stability improvements to enhance the user's experience.
## 1.6.2
+ Support was finished for 17.0.0.
  + `erpt` was updated to support the latest official behavior.
  + `jpegdec` was updated to support the latest official behavior.
  + `pm` was updated to support the latest official behavior.
+ General system stability improvements to enhance the user's experience.
## 1.6.1
+ An improved solution to [the problem that would cause consoles which had previously re-built their SYSTEM partition to brick on update-to-17.0.0](https://gist.github.com/SciresM/2ddb708c812ed585c4d99f54e25205ff) was added.
  + In particular, booting atmosphère will now automatically detect the problem and unbrick any consoles which have fallen into this state.
+ Some improvements were made to `haze`, including:
  + Performance was greatly improved:
    + Support was added for GetObjectPropList, which decreases the amount of requests made by ~8x.
    + Haze now performs rendering on the GPU, freeing up the CPU to respond to requests in a more timely manner.
  + An issue was fixed with how `haze` configures `bMaxPacketSize0` which improves support for USB3.
+ General system stability improvements to enhance the user's experience.
## 1.6.0
+ Basic support was added for 17.0.0.
  + The console should boot and atmosphère should be fully functional. However, not all modules have been fully updated to reflect the latest changes.
    + There shouldn't be anything user visible resulting from this, but it will be addressed in a soon-to-come atmosphère update.
  + `exosphère` was updated to reflect the latest official secure monitor behavior.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `ncm` was updated to reflect the latest official behavior.
  + `erpt` was partially updated to support the latest official behavior.
+ Atmosphere's gdbstub now supports waiting to attach to a specific program id on launch (as opposed to any application).
  + The monitor command for this is `monitor wait <hex program id>`, where program id can optionally have an `0x` prefix.
+ Support was added to `haze` for editing files in-place and performing 64-bit transfers (files larger than 4 GB).
+ `bpc.mitm` was enabled on Mariko units, and now triggers pmic-based shutdowns/reboots (thanks @CTCaer).
  + This should cause the console to no longer wake ~15 seconds after shutdown on Mariko.
+ A number of minor issues were fixed and improvements were made, including:
  + A workaround was added for a change in 17.0.0 that would cause consoles which had previously re-built their SYSTEM partition to brick on update-to-17.0.0.
+ General system stability improvements to enhance the user's experience.
## 1.5.5
+ Support was added for 16.1.0.
+ General system stability improvements to enhance the user's experience.
## 1.5.4
+ Experimental new functionality was implemented to prevent crashing when building romfs for certain games with obscene file counts.
  + This includes both Fire Emblem: Engage (~190000 files), and The Legend of Zelda: Tears of the Kingdom (~300000) files.
  + The solution involved adding functionality to ams.mitm/pm to dynamically steal memory from the application (and system) pool as needed when the games have romfs mods.
    + No memory is taken, and there is no cost to this functionality when playing without mods (or with overrides disabled).
  + The Legend of Zelda: Tears of the Kingdom is currently the absolute worst case game, requiring ~48 MB of memory to build a romfs image to play with mods.
    + Right now, the memory is sourced as follows: 32 MB (base ams.mitm heap), 10 MB (stolen from application pool), 8 MB (dynamically stolen from system pool).
    + This is 50 MB, which allows a little overhead in the worst case (prevents crashing due to exhausting the heap for other allocations in ams.mitm).
    + Zelda is remarkably sensitive to memory being stolen from the application pool, tolerating no more than 16 MB on 1.0.0 and 12 MB on 1.1.0. I have chosen to steal 10 MB, to be safe, for now.
      + This may break on a future game update, but I will fix it if and when that happens. There is no perfect solution; the game simply requires too much memory to support mods flawlessly, and I am forced to compromise.
  + As usual, if you encounter a game that exhausts ams.mitm's memory (crashing it) when loading layeredfs mods, please contact `SciresM#0524`.
    "I am jinxing myself by saying this, but it's really hard to imagine any game being worse than The Legend of Zelda: Tears of the Kingdom, but if it happens again I will drop everything to fix it as usual".
+ General system stability improvements to enhance the user's experience.
## 1.5.3
+ Support was added for 16.0.3.
+ Atmosphère was updated to use GCC 13/newlib (latest devkitA64/devkitARM releases).
  + **Please note**: This introduces a known issue, which is currently being worked on.
    + As you may recall from the 1.4.1 changelog, Fire Emblem: Engage requires enormous amounts of memory to support using layeredfs mods with the game.
    + Latest GCC/newlib slightly increases malloc overhead size, which makes the previous memory increase insufficient.
    + A general-case solution to this is in the works, which should hopefully fix the problem in a way that doesn't jinx me for the future.
+ A number of minor issues were fixed and improvements were made, including:
  + An issue was fixed that caused system font replacement to not work on 16.0.0+.
  + An minor accuracy issue was addressed in mesosphere's management of certain memory ranges; this issue would have had zero visible impact to the end-user.
+ General system stability improvements to enhance the user's experience.
## 1.5.2
+ A homebrew application (`haze`) was added for performing USB file transfer (with thanks to @liamwhite for both design and implementation).
  + `haze` is included with atmosphère, and provides access to the SD card via the PTP/MTP protocol.
    + **Please note**: haze will show inside the homebrew menu under the name "USB File Transfer".
  + **Please note**: Atmosphère cannot be updated at runtime, and trying to install an atmosphère update via haze will fail as usual.
+ General system stability improvements to enhance the user's experience.
## 1.5.1
+ `fatal` was updated to reduce memory footprint.
  + Starting in 16.0.0, official `fatal` has no framebuffer or rendering logic, and instead calls other system service commands to draw the screen.
  + However, these commands aren't usable by atmosphère (too small rendering window, bad color support).
  + To reduce the relative memory footprint differential between atmosphère and official code, the framebuffer (2 MB) is now dynamically allocated when needed.
    + This will try to allocate from multiple pools (preferring System > System_NonSecure > Application).
    + This technically requires that 2 MB be available in at least one of these pools for the fatal screen to render (otherwise, a reboot-to-black-and-white-fatal will occur), but this should be a non-issue in almost all cases.
+ A feature was added to optionally mirror the bluetooth pairing database to the SD card (thanks @ndeadly).
  + This allows device pairings to be automatically kept in-sync across sysmmc/all emummcs.
  + This is opt-in, and can be controlled by setting `atmosphere!enable_external_bluetooth_db = u8!0x1`.
  + When enabled, the pairing database will be synchronized to `/atmosphere/bluetooth_devices.db`.
+ General system stability improvements to enhance the user's experience.
## 1.5.0
+ Support was added for 16.0.0
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `ncm` was updated to reflect the latest official behavior.
  + Many FS apis were updated under the hood to reflect the latest official behavior.
  + **Please Note**: 16.0.0 made breaking changes to a number of system APIs, including in FS/NCM/Shared Font commands that some homebrew programs may use.
    + These programs may encounter strange errors, and may need to be recompiled with a libnx updated to support 16.0.0's changes to function properly.
+ A number of minor issues were fixed and improvements were made, including:
  + An issue was fixed that could cause GPIO outputs to be misconfigured under certain circumstances.
+ General system stability improvements to enhance the user's experience.
## 1.4.1
+ A number of minor issues were fixed and improvements were made, including:
  + `dmnt` cheat toggle files are no longer ignored when they are missing a trailing newline.
  + The mechanism for automatically cleaning up `erpt_reports` added in 1.3.0 was fixed.
    + This was actually just very fundamentally broken and has never worked, but it is verified working now.
  + Minor fixes were made in `mesosphère` to match official kernel behavior (spin lock assembly was corrected, wrong result on failure in in GetProcessId was corrected).
  + A missing call to GetSdStatus when initializing SD cards at non uhs-i mode was added in the sdmmc driver.
+ `ams.mitm`'s memory usage was increased by 16 MB, to prevent crashing when building romfs for games with obscene file counts.
  + To quote the changelog for 1.2.3: "Animal Crossing's 2.0.0 update contains >99000 files [...] It's really hard to imagine any game being worse than Animal Crossing".
  + As it turns out, Fire Emblem: Engage has ~186000 files, and is approximately twice as bad as animal crossing.
  + The additional memory here is taken from the applet pool; no issues are expected to arise from this, but please report anything you may run into.
  + As usual, if you encounter a game that exhausts ams.mitm's memory (crashing it) when loading layeredfs mods, please contact `SciresM#0524`.
    + I am jinxing myself by saying this, but it's really hard to imagine any game being worse than Fire Emblem: Engage, but if it happens again I will drop everything to fix it as usual.
+ General system stability improvements to enhance the user's experience.
## 1.4.0
+ Support was added for 15.0.0.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `ncm` was updated to reflect the latest official behavior.
+ A number of minor issues were fixed and improvements were made, including:
  + The capacity limit on registered add-on contents was fixed in NCM to reflect the increase that occurred in 12.0.0.
  + An off-by-one was fixed in mesosphere when computing the new value for an address arbiter signaled with ModifyByWaitingCountIfEqual.
  + dmnt.gen2's gdbstub now sanitizes thread names to prevent invalid characters from breaking gdb.
  + dmnt.gen2's gdbstub now reports the architecture tag correctly when attached to 32-bit processes.
  + Support for program-specific html manual content overrides was added for non-hbl takeover context.
  + A bug was fixed in how emummc constructed the alternate Nintendo directory path.
    + Previously, this was using `/*/Nintendo/Nintendo` instead of `/*/Nintendo`.
    + Code was added to automatically move the old folders to the new ones when booting into emummc.
  + A bug was fixed in boot that caused an incorrectly low input voltage limit to be set.
+ General system stability improvements to enhance the user's experience.
## 1.3.2
+ Support was improved for 14.0.0+.
  + `loader` was updated to reflect the latest official behaviors.
  + `ro` was  updated to reflect the latest official behaviors.
+ A number of minor issues were fixed and improvements were made, including:
  + A memory leak was fixed in filesystem path management; this could cause a crash when launching games ~100 times, or when deleting/re-downloading games.
  + A bug was fixed that could cause threads to not see a newly signaled semaphore.
  + A number of minor inaccuracies were fixed in the updated FileSystem APIs.
+ General system stability improvements to enhance the user's experience.
## 1.3.1
+ Support was added for 14.1.0.
+ A number of minor under the hood improvements to accuracy were made to better reflect latest official system module behavior, particularly around FS apis.
+ General system stability improvements to enhance the user's experience.
## 1.3.0
+ Support was added for 14.0.0.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `erpt` was  updated to reflect the latest official behaviors.
  + `pm` was  updated to reflect the latest official behaviors.
  + `fatal` was  updated to reflect the latest official behaviors.
+ A mechanism for automatically cleaning up `erpt_reports` was added.
  + When booting, if the console has more than 1000 reports inside `/atmosphere/erpt_reports`, the folder will be cleaned to empty.
  + This behavior can be disabled by setting `erpt!disable_automatic_report_cleanup` = u8!0x1 in system_settings.ini.
+ Atmosphère's build system was re-written, and now allows globally building for various builds/configs.
  + All boards now automatically support release/debugging/auditing targets; it is now possible to build a full debugging/auditing build of atmosphère for the first time.
+ Support was added for compiling libstratosphère to run on PC.
  + The currently implemented/tested targets are Windows (x64), Linux (x64, arm64), macOS (x64, arm64).
    + If you are a developer interested in adding support for another target, please reach out to `SciresM#0524` on discord.
  + This is intended to finally allow sanely testing Atmosphère's code, by allowing most of it to run on a PC (with access to a debugger) instead of on game console hardware.
  + In addition, this will allow making PC tools which reuse code written for Atmosphère directly..
  + **Please Note**: This has no relation to interacting with official software on PC whatsoever. This really allows for making tests and self-contained atmosphère-based command-line tools; the Atmosphère project continues to have zero interest in attempting to run official software of any kind.
  + In the course of adding this support (and working on tooling using it), a number of fairly major revisions were made to stratosphere (particularly surrounding filesystem code).
    + **Please Note**: A number of changes made for this (and ones necessary in the process of adding support for 14.0.0) are api-breaking.
      + If you're a developer and any of this caused your code to break, please feel free to contact `SciresM#0524` for help updating your program.
+ General system stability improvements to enhance the user's experience.
## 1.2.6
+ Support was added for 13.2.1.
+ A number of minor issues were fixed and improvements were made, including:
  + A minor performance improvement was implemented in service table dispatch by sorting and binary-searching the service command table instead of using linear search.
  + Static initialization logic in Atmosphere was made much more regular.
+ General system stability improvements to enhance the user's experience.
## 1.2.5
+ Support was added for 13.2.0.
+ A number of minor issues were fixed and improvements were made, including:
  + A bug was fixed that caused `mesosphère` to underreport the total memory size by 8MB for certain games which use newer system-resource-size memory management.
    + This caused FIFA 19 to crash, and possibly other issues.
  + Memory management changes were made to `sm` that save 0x5000 of memory.
  + A microoptimization was made to the way `mesosphère` manages updating the debug register for hardware single-step support.
  + Support was fixed for enabling `usb!usb30_force_enabled` on 13.0.0+.
  + The work-in-progress unit testing framework was updated to use doctest instead of catch2.
+ General system stability improvements to enhance the user's experience.
## 1.2.4
+ Changes were made to the way fs.mitm builds images when providing a layeredfs romfs.
  + Cache management (to avoid unnecessary rebuild) was revised, to add a grace period of ~500ms-1s between process closing romfs image and ams.mitm needing to rebuild if romfs is re-opened.
    + This makes our cache much more effective, previously we were re-building romfs several times.
  + RomFS image ownership was overhauled, with a new reference-counting implementation added (used to implement the above grace period).
    + Certain games (e.g. Puyo Puyo Tetris 2, probably others) were sensitive to this timing, and could use access patterns which would trigger creation of romfs image while previous romfs image was in the middle of destructor.
    + This could cause a fatal error, because the destructor for the old image could run simultaneously with building the new image.
  + This also provides a speedup versus the 1.2.3 code, with Animal Crossing now taking ~8 fewer seconds to get past the Nintendo Switch logo.
+ General system stability improvements to enhance the user's experience.
## 1.2.3
+ Because ams.TMA is taking longer to develop than expected, experimental support for Atmosphère's gdbstub as a standalone is now available.
  + To enable it, set `atmosphere!enable_standalone_gdbstub` = u8!0x1 in system_settings.ini.
    + The standalone also requires `atmosphere!enable_htc` = u8!0x0, but this should be the case for everyone since ams.TMA isn't actually usable yet.
  + Once enabled, open the devkitPro provided-gdb (`aarch64-none-elf-gdb` for 64-bit or `arm-none-eabi-gdb` for 32-bit).
    + The standalone stub exposes itself on port 22225 -- so the command to connect is `target extended-remote <ip address>:22225`.
    + Type `info os processes` to get a list of process IDs that can be attached to.
      + The stub should work on both system programs, games, and homebrew -- but please note that debugging certain processes (like sockets) can cause hang due to the stub using them itself.
  + Software break-points, hardware break-points, hardware watch-points, and hardware single-step are all supported/implemented.
  + The following monitor commands are currently supported:
    + `monitor get info`: Get process info, address space layout, and information on modules.
    + `monitor get mappings`: Get all memory mappings.
    + `monitor get mapping <addr>`: Get the memory mapping for a specific address.
    + `monitor wait application`: Causes the stub to wait for an application to be launched. The next application will be started suspended.
      + User is expected to send `attach <pid>` after launching, which will cause attach-on-first-instruction. Failure to attach may cause system instability, this probably needs work.
  + **Please Note**: The GDBstub is new and may have bugs/need work. If you find issues, please report them to SciresM#0524 -- all help finding/fixing bugs is appreciated, here.
  + Generally speaking, if you would like to report information about fixes needed/discuss development of the gdbstub, join ReSwitched's #dev-support channel.
+ Changes were made to the way fs.mitm builds images when providing a layeredfs romfs.
  + Animal Crossing's 2.0.0 update contains >99000 files, and has tables so big that we ran out of memory even after the optimizations made in 0.10.5.
    + Previously, we used fixed-sized 0x40000 work buffers for file/directory tables and simultaneously built hash/content tables in one loop over files/directories.
    + We now iterate over the file/directory tables multiple times, first once to determine the hash table indices, then repeatedly to build hash tables, then once to build content tables.
    + We also now allow smaller-than-0x40000 work buffers, trying half-as-big buffers until allocation succeeds (or work buffer would be <0x4000, which is a safeguard against truly horrible performance).
  + There is a slight speed penalty to these changes, but it's on the order of seconds for the worst case (Animal Crossing) and trivial for most games with reasonable tables.
  + If you encounter a game that exhausts ams.mitm's memory (crashing it) when loading layeredfs mods, please contact `SciresM#0524`.
    + It's really hard to imagine any game being worse than Animal Crossing, but if it happens again I will drop everything to fix it as usual.
+ `creport` now attempts to parse symbol tables if present.
  + If a game executable has a symbol for a given address, the function-relative-offset will now be printed after the module-relative-offset.
+ General system stability improvements to enhance the user's experience.
## 1.2.2
+ A number of fixes were made to Atmosphère's implementation of the new "sprofile" service added in 13.0.0.
  + Nintendo is finally transmitting data over the internet to certain consoles, which has allowed for validating our service implementation.
    + Unfortunately, there were several problems, and if your console began trying to use the new services atmosphere would show a fatal error with code 0xCAF6 (sprofile::ResultInvalidState()).
  + With actual test data in hand, a test program was written and it was verified that our implementation can successfully import/access profile data now.
    + Hopefully there are no more issues, and I sincerely apologize for anyone who got an 0xCAF6 fatal due to this.
+ A number of minor improvements were made to `mesosphère`, including:
  + KThread::GetContextForSchedulerLoop was implemented in assembly (using static assertions to verify offset-of-context-in-struct is correct).
    + This saves an unnecessary function call in the middle of the scheduler hot loop, replacing it with an addition instruction, which should improve microperformance.
  + Mesosphere's hardware maintenance instructions were audited via a script and now directly match Nintendo's kernels.
    + Notably, this inserts a missing instruction synchronization barrier when validating that slab heaps may be constructed.
    + This missing ISB could cause an abort on certain (see: particularly sensitive) hardware on boot if the relevant codepath was speculatively executed (it normally only executes on game launch...)
  + The SVC handlers for performing light IPC (normally unused) from 32-bit process were fixed in Mesosphere.
  + A bug was fixed that would cause the register x27 to be overwritten with the contents of x26 when returning from a user exception handler.
  + A bug was fixed that would cause the kernel to use the userland stack pointer instead of the kernel stack pointer while generating an error report for a kernel abort.
+ General system stability improvements to enhance the user's experience.
## 1.2.1
+ Support was implemented for 13.1.0.
  + `mesosphère` was updated to reflect the kernel behavioral changes made in 13.1.0.
    + KScheduler now issues a data memory barrier when unlocking the scheduler lock and when early-returning due to top-thread-is-current during scheduling.
  + `erpt` was  updated to reflect the latest official behaviors.
    + The new service added in 13.0.0 ("sprofile") was revised, and the data formats it expects was changed.
      + This still appears to be (possibly(?)) untestable due to data not being transmitted yet, but I have greater confidence things will go smoothly than I did when 1.1.0 released.
+ A number of improvements were made to `mesosphère`, including:
  + A build target was created to build targeting the qemu `virt` board.
    + This facilitates writing unit tests for the kernel (and other atmosphere components) and running them under PC.
      + **Please Note**: Official system software will not work at all under this, and the Atmosphère project has zero interest in attempting to run official software of any kind. This is unit testing machinery, and explicitly not more than that.
    + This should hopefully allow us to have greater confidence that all of atmosphere's components work the way they're theoretically supposed to in the future.
    + **Please Note**: If you are a developer who is familiar with the Horizon operating system (or capable of becoming familiar), I would greatly appreciate help writing tests and improving the testing framework.
      + Please contact `SciresM#0524` if you are capable and interested.
        + Really, if you are actually a developer who would like to help me get this off the ground, I would deeply appreciate it.
        + That said, if you are not a developer but want to be one, this probably isn't the best opportunity; I expect it to be highly technical.
          + Consider the ReSwitched discord's #hack-n-all channel for your educational purposes.
      + We are (at least for now) using [catch2](https://github.com/catchorg/Catch2) for unit tests.
  + Almost all virtual calls in the kernel are now resolved statically.
    + This eliminates substantial virtual call overhead, and should lead to improved kernel microperformance in pretty much every function.
  + The remaining red black tree find operations which weren't using the optimized "find key" variant are now using the optimized version.
  + Custom assembly was written to improve tick-to-timespan conversion.
    + This works around gcc emitting suboptimal assembly at -Os (it emits good assembly at -O3, clang is fine at both -O3 and -Os).
  + KThread and KSession structures were updated to optimize member layout, saving 0x10 bytes per KThread/KSession object.
  + Rather than unnecessarily zero-ing all data in kernel objects only to overwrite members later, we now only initialize the members we need to in kernel object constructors.
    + This is what Nintendo was doing already.
  + A set of custom optimized atomic primitives were implemented and are used in place of std::atomic<>
    + This works around a gcc bug which downgrades specified memory order to seq_cst, and introduces clrex in places where it is appropriate.
    + This should strictly improve microperformance of many system calls.
  + An compile-time toggleable extension was added to support 40-bit physical addresses in MapRange capabilities (using currently reserved bits).
  + A number of minor bugs were fixed, including:
    + Initial cache management now better reflects official behavior.
      + This fixes an issue that caused certain hardware with cache sensitivity to produce cryptic kernel panics during boot.
    + Incorrect logic when checking thread priority capabilities was fixed to reflect official behavior.
    + The scheduler was updated to reflect latest official behavior, and a number of minor bugs involving clz/ctz were fixed.
    + Accesses to the processes local region were fixed to properly use kernel linear region, not userland pointers.
    + The cache SVCs exposed for 32-bit processes now better reflect official core mask request semantics.
    + A bug was fixed that could cause a kernel panic if SvcArbitrateLock was called on a thread with exactly one reference in the middle of handling a user-mode exception.
+ General system stability improvements to enhance the user's experience.
## 1.2.0
+ `boot` was updated to reflect the latest official behavior for display/battery management.
  + This should fix any issues that might result from running older releases on the OLED model, if you're somehow in a position to do so.
+ The "target firmware" system was changed to allow the bootloader to specify an approximation, rather than the true target firmware.
  + Previously we expected compliant bootloaders to inspect SYSTEM:/ to determine the specific target firmware.
  + Now, we only require an approximate version, with major version == true major version and approximate version <= true version.
  + This greatly simplifies bootloader requirements, and correspondingly all code for accessing SYSTEM has been removed from fusee.
    + This should result in a substantial speedup when booting emummc with fusee, as SYSTEM accesses were the most expensive thing done previously.
  + This should resolve any inconsistency in firmware detection when booting via fusee vs hekate.
  + This should also improve our compatibility with micro firmware releases, making it more likely that atmosphere "just works" if nothing important has changed.
+ Dynamic resource limit determination logic was implemented in `pm` to match latest official behavior.
  + This greatly simplifies/makes consistent the resource limits on older firmwares, as well.
+ An enormous amount of refactoring was performed under the hood, including:
  + **Please Note**: If you are a developer who uses Atmosphere-libs, a number of changes here are breaking.
    + Feel free to contact SciresM#0524 for help updating your program.
  + The OS namespace had many primitives implemented/made more accurate.
  + Since mesosphere is now always-on, os::LightEvent (which required newer SVCs) is now globally usable (and used by stratosphere where relevant).
  + Assertions are now true no-ops when building for release.
  + Stratosphere is now built with -Wextra/-Werror.
  + Most "common" logic in system module main.cpp files was moved into libstratosphere.
    + **Please Note**: main.cpp files for prior atmosphere-libs will no longer work, for a really large number of reasons.
  + A number of longstanding code style issues were corrected.
  + Mesosphere now uses util::BitFlagSet for SVC permissions.
  + Mesosphere now puts its relocation table inside .bss, which allows that memory to be reclaimed after relocations are performed.
    + These changes save ~16KB of memory in the kernel, all said and done.
  + A number of locations in stratosphere where memory could be saved were spotted and taken advantage of, leading to ~150-200KB of saved memory.
  + The `spl` and `loader` system module was refactored to better reflect official logic.
  + `sf` ipc server code was updated to only emit mitm/defer logic when that logic is actually required somewhere in process.
  + `tipc` ipc server code was updated to reflect changes to official logic made in 13.0.0.
  + Many, many other minor changes, please talk to SciresM#0524 or read the relevant commits if you want to know more.
+ A number of minor issues were fixed, including:
  + Mesosphere's handling of SVC permissions on thread pin/unpin was updated to reflect official kernel behavior.
  + util::CountTrailingZeros() was fixed to calculate the correct value when used at compile-time.
+ General system stability improvements to enhance the user's experience.
## 1.1.1
+ A bug was fixed which caused some memory to leak when launching a game with mods enabled, eventually causing a crash after enough game launches without rebooting.
+ General system stability improvements to enhance the user's experience.
## 1.1.0
+ Support was implemented for 13.0.0.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `ncm` was updated to reflect the latest official behaviors.
  + `erpt` was  updated to reflect the latest official behaviors.
    + Two new services ("sprofile") were added to `erpt`, and have been fully reimplemented.
    + **Please Note**: These services provide a way for settings to be pushed to consoles over the internet without system update.
      + Because there appear to be no settings pushed out yet, this implementation fundamentally cannot be fully tested right now, but hopefully there are no issues once settings begin being distributed.
+ The `LogManager` system module was reimplemented.
  + This system module provides services that some games use for logging.
  + Atmosphere's reimplementation supports logging to the SD card (if `lm!enable_sd_card_logging` is true) and to ams.TMA.
    + To control the directory where logs are saved, modify the `lm!sd_card_log_output_directory` setting.
  + Atmosphere's reimplementation is disabled by default (in order to save memory), but can be enabled by setting `lm!enable_log_manager` to true.
    + This will allow reading over logs from games which use the services (or potentially logging from homebrew in the future), which can be useful to developers.
    + Please note that when TMA is fully implemented in the future, enabling TMA will forcibly enable `LogManager`.
+ General system stability improvements to enhance the user's experience.
## 1.0.0
+ `fusee` was completely re-written in C++ to use the same atmosphere-libs APIs as the rest of atmosphere's code.
  + The rewrite was performed with a big emphasis on ensuring a good boot speed, and generally boot should be much faster than it was previously.
    + Depending on SD card/environment, boot speed may now be slightly faster than, roughly the same as, or slightly slower than when booting with hekate.
    + The obvious low-hanging fruit for performance improvements has been picked, so hopefully the improved performance is to everybody's liking.
  + SD card compatibility was improved: fusee should now have SD card compatibility identical to the official OS driver.
  + **Please Note**: various components were renamed (fusee-primary.bin -> fusee.bin, fusee-secondary.bin -> package3).
    + If you use another bootloader (like hekate), you may need to update your configuration to use the new layout.
  + **Please Note**: BCT.ini no longer exists, nogc configuration has been moved to `/atmosphere/stratosphere.ini`.
    + If you rely on custom nogc configuration, please be sure to update accordingly.
  + Custom splash screen BMP parsing is no longer supported (as it slows down boot for 99% of users).
    + To compensate for this, a script to insert a custom splash screen into a `package3` binary has been added to the `utilities` folder of the atmosphere repository.
    + The release build should be equivalent to running the following command from the root of the atmosphere repository: `python utilities/insert_splash_screen.py img/splash.png fusee/package3`
+ A number of pending changes were made, following the end of the relevant testing periods:
  + `mesosphere` is no longer opt-out, and stratosphere code will begin depending on its being present/in use.
  + `NCM` is no longer opt-out.
  + The cleanup to ease the transition from < 0.19.0 to 0.19.0 has been removed.
+ General system stability improvements to enhance the user's experience.
## 0.20.1
+ An issue was fixed that caused severely degraded performance after wake-from-sleep on Mariko hardware.
  + This was due to Mariko MTC resulting in a frequency of 1599.999MHz instead of 1600MHz.
  + Due to this off-by-one, Nintendo's EMC management code failed to initialize/take over, and after wake from sleep RAM would be in a strange state.
+ General system stability improvements to enhance the user's experience.
## 0.20.0
+ DRAM training (MTC) was implemented for Mariko hardware, increasing RAM speed from 204MHz to 1600MHz.
  + This significantly optimizes Mariko boot speed, cutting boot time roughly in half.
  + Typical boot time reductions (measured as "select fusee" to "home menu visible"):
    + Normal (Iowa): ~35 seconds -> ~18 seconds.
    + Lite (Hoag): ~65 seconds -> ~30 seconds.
  + NOTE: Work is being started on a re-written `fusee` component, with an eye specifically towards ensuring a good boot speed.
    + With any luck, boot will be much much faster on all units (Mariko and Erista) in an upcoming release.
+ Sept was replaced, and deleted from the repository.
  + Erista units now use a custom TSEC firmware to manage key derivation.
    + For more details, contact SciresM#0524 on discord.
  + This has a number of benefits, including:
    + This greatly simplifies key derivation logic by making it consistent on all firmwares.
      + Fusee no longer accesses/uses keyblobs at all, so units which have accidentally destroyed/lost keyblobs can boot without them.
    + This greatly increases stability (sept was the biggest source of boot failures).
    + This improves boot speed (sept rebooted multiple times, performed hardware init multiple times, and was generally very slow).
    + Atmosphère build process is now much saner.
+ A number of improvements were made to the dmnt cheat engine.
  + Cheats which take in a memory region operand may now use types "2" or "3" to perform accesses relative to the alias/aslr regions, respectively.
  + Support was added for an "else" opcode in the cheat engine, to make writing certain conditional logic more natural.
  + Support was added for a cheat orchestrator homebrew (like edizon) to detach from a cheat process/set the master cheat programmatically.
+ Daybreak now provides a warning when attempting to install a firmware newer than the highest version atmosphère knows it supports.
  + To facilitate this, exosphere now exposes the supported HOS version via an extension ConfigItem.
+ A number of minor issues were fixed, including:
  + Several mesosphere debug SVC implementations were updated to reflect the semantics of the latest kernel.
  + Support was fixed for deriving BIS encryption keys on certain prototype hardware.
+ General system stability improvements to enhance the user's experience.
## 0.19.5
+ Support was added for 12.1.0.
+ LayeredFS support was added for OpenDataStorageWithProgramIndex commands.
  + Certain games using newer (7.0.0+ APIs) which include multiple programs under a single title previously could not be modified.
  + These are now supported as normal, and LayeredFS should have 100% compatibility again.
+ A number of minor issues were fixed, including:
  + The Reboot to Payload NRO was updated to allow the OS to save state prior to rebooting (thanks @AuroraWright)!
  + An issue was fixed that could cause dns.mitm to fail when games requested resolution of an empty string.
  + An issue was fixed that caused a memory leak in the erpt system module.
    + This would eventually cause a system crash after ~540 reports were generated without rebooting.
+ A number of minor improvements were made to improve mesosphere's accuracy.
+ General system stability improvements to enhance the user's experience.
## 0.19.4
+ Support was added for 12.0.3.
+ A number of minor issues were fixed, including:
  + An issue was fixed that could cause heap memory corruption when allocation was highly contended.
  + An issue was fixed that could cause sleep to fail under certain conditions.
  + An issue was fixed that could cause a scheduler slow path to be taken more often than necessary.
+ General system stability improvements to enhance the user's experience.
## 0.19.3
+ Support was added for 12.0.2.
+ A number of minor issues were fixed, including:
  + An issue was fixed in dns.mitm that caused a crash when games attempted to resolve the IP address of nullptr.
  + An issue was fixed in erpt that would cause an abort when booting without having ever booted stock previously.
  + An issue was fixed in (file-based) emummc that caused an error on system format/downloading certain games.
+ General system stability improvements to enhance the user's experience.
## 0.19.2
+ Atmosphère's components were further updated to reflect latest official behaviors as of 12.0.0.
  + Notably, `erpt` was updated to implement the new forced shutdown detection feature.
    + When a forced-shutdown occurs, an erpt_report will be generated and saved to the SD card on the next boot.
+ Atmosphere-libs was updated to use GCC 11 (latest devkitA64/devkitARM releases).
  + Initial inspections show mild-to-moderate optimizer improvements in several important places (kernel is 0x3000 smaller).
  + General system stability improvements to enhance the user's experience.
+ A number of minor issues were fixed, including:
  + A bug was fixed that caused a black screen when attempting to boot firmware versions 2.0.0-4.1.0.
  + A bug was fixed that caused sm to abort when at the session limit, rather than returning error codes.
  + A bug was fixed that allowed for resource exhaustion on 12.0.0, under certain circumstances.
+ Several issues were fixed, and usability and stability were improved.
## 0.19.1
+ An issue was fixed that caused a fatal error when using official `migration` services to transfer data between consoles.
+ An issue was fixed in `ncm` that caused an error when the OS tried to enumerate installed SD card content.
+ Several issues were fixed, and usability and stability were improved.
## 0.19.0
+ Support was added for 12.0.0.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `sm`, `boot2`, `pgl` were updated to reflect the latest official behaviors.
    + **Please Note**: 12.0.0 added a new protocol for IPC ("tipc"), which has been freshly reimplemented in its entirety.
      + It is possible there may be as of yet unfound issues; if there are, please send the appropriate crash reports to SciresM (SciresM#0524 on discord).
      + Homebrew which uses atmosphere extensions (including the mitm API) will need to be re-compiled in order to function on 0.19.0.
        + I apologize for this, but it's unavoidable for technical reasons. If you're affected by this and mad about it, please contact SciresM to complain.
  + `erpt` was partially updated to reflect the latest official behaviors.
    + New features were added to erpt to track the activity of running applets, and to detect when a forced shutdown occurs.
    + These behaviors have been temporarily stubbed, as they are not necessary for 12.0.0 to run (and their outputs won't be saved anywhere).
    + A future atmosphère update will implement these behaviors, in the interest of reflecting official logic as faithfully as we can.
+ Atmosphère no longer uses the /contents/ folder for its own programs.
  + Atmosphère's system modules are now bundled together in the single file "stratosphere.romfs".
    + For those working on developing for atmosphère, executables inside the /contents/ directory will be preferred to those in "stratosphere.romfs".
  + **Please Note**: In order to facilitate this change (and the desired behavior), the first time you boot after extracting a release zip, atmosphère system modules inside /contents/ will be deleted.
    + This will have no impact on user programs (it only removes programs with specific program ids).
+ Improvements were made to mesosphere, including:
  + An extension InfoType was added for getting the current process handle, without having to spawn a thread and do IPC with oneself.
  + An issue was fixed in SvcSetDebugThreadContext.
  + An issue was fixed when doing IPC with user buffers.
+ Support was fixed for toggling the custom setting `usb!usb30_force_enabled` on 9.0.0+.
  + This was broken by Nintendo's introducing a dependency that made USB a requirement to launch before custom settings are parsed.
  + Since the fix, you can now toggle the setting (as you could prior to atmosphère 0.9.4), and it will work as expected.
  + **Please Note**: Enabling USB 3.0 often severely impacts wireless communications.
    + Because of this, the setting will default to off. If you experience issues with it enabled, consider disabling it.
+ A warning was added to daybreak when resetting the console to factory settings.
+ Substantial work was completed towards atmosphere's upcoming implementation of the host target connection protocol.
  + Once completed, users will be able to interact with a Switch running atmosphère via a PC application ("Starlink") currently under development.
    + Planned eventual features for connected consoles include a gdbstub, interacting with memory (for cheat development), streaming gameplay audio and video, and accessing the Switch's SD card filesystem.
    + Switch homebrew will also have access to a (configurable and sandboxed) filesystem on the host PC, while connected.
  + Towards this end, the following was accomplished:
    + The "htc" system module was reimplemented completely.
    + The system module which provides remote access to the SD card was reimplemented completely.
  + This is currently the active focus of atmosphère's development.
  + **Please Note**: Support is not yet completed, and users are disadvised from interacting with the related settings for the time being, unless they particularly know what they're doing.
+ A number of minor issues were fixed, including:
  + A bug was fixed in `dmnt` that could cause a fatal when launching certain games with cheats active.
  + An issue was fixed that could cause an abort in `sm` when using a large number of custom system modules.
  + An issue was fixed that prevented launching gamecards on 1.0.0.
  + Minor issues were fixed in the cheat virtual machine's behavior.
+ Several issues were fixed, and usability and stability were improved.
## 0.18.1
+ A number of minor issues were fixed, including:
  + The new `dns.mitm` module added in 0.18.0 no longer fatal errors when receiving port=nullptr.
    + This fixes youtube ad-blocking, and possibly other usecases.
  + A bug was fixed that caused ams.mitm to incorrectly cache data storages.
    + This potentially broke DLC when using romfs mods, and could have caused other issues (e.g. with custom themes, and maybe other cases).
  + A bug was fixed in power state control module registration.
    + This might fix a weird edge case with system module dependencies on sleep/wake, but probably nobody should notice any differences.
  + A bug was fixed where mesosphere sometimes treated virtual core IDs as though they were physical core IDs.
    + This had zero impact, because for Switch virtual core == physical core, but it could have affected future platforms if it had remained unresolved.
+ Several issues were fixed, and usability and stability were improved.
## 0.18.0
+ A new mitm module was added (`dns.mitm`).
  + This provides a highly configurable mechanism for redirecting DNS resolution requests.
  + By default atmosphère redirects resolution requests for official telemetry servers to a loopback address.
  + Documentation on how to configure `dns.mitm` to meet your more specific needs may be found [here](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/features/dns_mitm.md).
+ The service framework API (`sf`) was refactored to be more accurate to official logic and greatly reduce memory requirements.
  + The comparison of atmosphère module memory usage versus Nintendo's found [here](https://github.com/Atmosphere-NX/Atmosphere/wiki/Memory-Comparisons) was updated to reflect this.
  + **Please Note**: If you are a developer using the libstratosphere service APIs, some updating may be required. Contact SciresM#0524 on discord for assistance if required.
+ A number of deprecations were removed, following a general codebase cleanup:
  + The `sm` extension to not unregister services on connection close was superseded by official opt-in logic in 11.0.0, and has been removed in favor of official logic.
    + This should have zero impact on users.
  + The temporary `hid-mitm` added in 0.9.0 has finally been removed, following over a year of deprecation.
    + There shouldn't be any homebrew in use still affected by this, but the situation will be monitored.
    + If this is somehow still a real issue, an unaffiliated hid mitm sysmodule providing the same functionality can be created and released, separate from atmosphère itself.
+ Several issues were fixed, and usability and stability were improved.
## 0.17.1
+ A number of atmosphère's modules were using more memory than their Nintendo equivalent's in 0.17.0; a number of code generatio tweaks have been applied to fix this across the board.
  + A detailed comparison of atmosphère module memory usage versus Nintendo's was made and can be found [here](https://github.com/Atmosphere-NX/Atmosphere/wiki/Memory-Comparisons).
+ Several minor bugs were fixed, including:
  + A bug was fixed in mesosphère that caused games which attempt to map more memory than the Switch has to fail.
    + This affected "Piczle Lines DX 500 More Puzzles!", and possibly other games.
  + Enabling configuration to "blank" PRODINFO no longer causes a hang on Mariko devices (or any devices with newer format).
+ Several issues were fixed, and usability and stability were improved.
## 0.17.0
+ fusee was heavily rewritten in order to add support for Mariko hardware.
  + **Please Note**: Mariko hardware currently has no (and may not ever have any) software exploits; fusee works when loaded from bootloader context with the right keys in the security engine. No means of getting the system into this state is provided.
+ An issue was fixed in the way shutdown was performed on Erista hardware.
  + This fixes an issue that caused OFW to black screen on boot after power off from atmosphere without first doing a reboot.
  + This also substantially improves power drain when the system is shut off; consoles powered off from Atmosphere should now drain battery at the same reduced rate as original firmware.
+ A number of minor changes were made, including:
  + A number of inconsistencies in the build system were fixed.
    + For those building atmosphère at home, the `boot` sysmodule will no longer rebuild every time make is invoked.
    + This substantially improves build times during development iteration.
  + `sm` was updated to more accurately reflect how official code manages request deferral.
  + `mesosphère` was updated to more accurately reflect official kernel management of the trace buffer.
  + `mesosphère` was updated to improve kernel loader's logic by taking advantage of the assumption that we only boot our kernel, not Nintendo's.
+ As it has been a few months with zero reported issues, `mesosphère` is now opt-out.
    + Users who wish to begin using or continue using mesosphere should use the standard/cool kids zip ("atmosphere-").
    + Users who wish to opt-out of mesosphere should download and extract the opt-out zip ("atmosphere-WITHOUT_MESOSPHERE-").
+ Several issues were fixed, and usability and stability were improved.
## 0.16.2
+ Atmosphère release zips no longer bundle BCT.ini, instead relying on defaults in code.
  + This means atmosphere updates should no longer overwrite any user configuration at all.
  + If you wish to modify BCT.ini config, copy the template from /config_templates/ as with other configuration.
+ `pgl` and `creport` were further updated to reflect differences in official behavior in 11.0.0.
+ An issue was fixed that caused creport to be launched multiple times on process crash.
  + This fixes the "duplicate reports" issue that sometimes plagued people.
+ A new system setting (`atmosphere!enable_am_debug_mode`) configuring am to use debug mode.
  + If you are not a developer or don't see a clear use for this, leave it configured to the default (off).
+ Reboot to payload NRO was updated to fix support with certain payloads.
+ Support was fixed for atmosphere's extension to support homebrew use of new (8.0.0+) kernel mappings.
  + In particular, when running tracing debug builds of `mesosphère`, hbloader now has access to the kernel trace buffer.
+ Several issues were fixed, and usability and stability were improved.
## 0.16.1
+ Support was added for 11.0.1.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
    + A new svc::InfoType added in 11.0.0 was implemented (it wasn't discovered before 0.16.0 released).
    + The new Control Flow Integrity (CFI) logic added in 11.0.0 kernel was implemented.
+ `fs` logic was refactored and cleaned up to reflect some newer sysmodule behavioral and structural changes.
+ `exosphère` was updated to allow dynamic control of what uart port is used for logging.
  + This can be controlled by editing the `log_port`, `log_baud_rate`, and `log_inverted` fields in `exosphere.ini`.
+ `mesosphère` was updated to improve debugging capabilities.
  + This is still a work in progress, but developers may be interested.
+ A bug was fixed that caused `fatal` to fatal error if the fatal process was already being debugged.
+ Several issues were fixed, and usability and stability were improved.
## 0.16.0
+ Support was added for 11.0.0.
  + `exosphère` was updated to reflect the latest official secure monitor behavior.
  + `mesosphère` was updated to reflect the latest official kernel behavior.
  + `loader`, `sm`, `boot`, `pgl` were updated to reflect the latest official behaviors.
    + **Please Note**: 11.0.0 implements an opt-in version of the atmosphère `sm` extension that allows for closing session without unregistering services.
      + Correspondingly, the extension will be deprecated in favor of the new official opt-in command. In 0.17.0, it will be removed entirely.
      + If your custom system module relies on this extension (however unlikely that seems to me), please update it accordingly.
  + `erpt` was partially updated to provide compatibility with 11.0.0.
    + The latest firmware attaches additional fields and context information to logs.
    + A future atmosphère update will implement this logic, so that users who are interested can also get the new information when examining their logs.
  + **Please Note**: 11.0.0 introduced breaking changes to the `usb` system module's `usb:ds` API.
    + Homebrew which uses the `usb:ds` service should rebuild with the latest libnx version to support running on 11.0.0.
+ The `boot` system module was rewritten to reflect the huge driver changes introduced in 8.0.0.
  + This includes a number of improvements to both logo display and battery management logic.
+ Support was added for configuring the address space width for `hbl`.
  + The `hbl_config!override_address_space_(#)` and `hbl_config!override_any_app_address_space` can now be set to `39_bit`, `36_bit`, or `32_bit` to control the address space for hbl on a per-override basis.
  + If a configuration has not been set, hbl will now default to 39-bit address space.
    + Previously, a legacy 36-bit address space was always used to maintain compatibility with 1.0.0.
    + A new loader extension was added to support 39-bit whenever possible (including mesosphere-on-1.0.0), with fallback to 36-bit when unavailable.
+ Support was added to a number of components for running on Mariko hardware.
  + The `boot` system module can now safely be run on mariko hardware, performing correct hardware initialization.
  + Daybreak (and generally, system update logic) were updated to be usable on Mariko.
  + Boot0 protection/management logic was updated to perform correct actions on Mariko.
  + Reboot to payload does not and cannot work on Mariko. Correspondingly, A "fatal error" handler was written, to display and save fatal errors from within TrustZone.
  + **Please Note:** Atmosphere is still not properly usable on Mariko hardware.
    + In particular, wake-from-sleep will not properly function (the magic numbers aren't set correctly), among a few other minor issues.
+ `exosphère` received support for building under debug configuration.
  + A small (otherwise unused) portion of IRAM is now reserved for debug-only exosphere code (this region is unused/untouched under release config).
  + This enables logging (including printf) to uart from the secure monitor, for those interested.
+ A number of bugs were fixed, including:
  + Minor issues in a number of filesystem related code were fixed.
  + An issue was fixed that could cause NCM to abort on consoles which came with 3.0.x and were never updated.
+ Several issues were fixed, and usability and stability were improved.
## 0.15.0
+ fusee-primary's panic display was updated to automatically identify and give suggestions to resolve many of the most common errors users encounter.
+ Having been tested as well as I can alone, `mesosphere` (atmosphère's reimplementation of the Nintendo Switch kernel) is now available for users interested in trying it.
  + Beginning in this release and until it is stable and well-tested, atmosphère will distribute two zips.
    + Users who wish to opt-in to mesosphere should download and extract the "cool kids" zip ("atmosphere-EXPERIMENTAL-").
    + Users who do not wish to use mesosphere should continue using the normal zip ("atmosphere-").
  + Users may detect whether mesosphere is active in system settings.
    + When mesosphere is active, the system version string will display "M.15.0" rather than "0.15.0", and so on for future releases.
    + Crash reports and the like will contain information on whether or not the user is using mesosphere, as well.
  + There are "probably" no material user-facing benefits to using mesosphere at this time.
    + Developers may be interested in the fact that mesosphere provides many newer SVC APIs even when on lower firmware versions.
    + The primary benefit to using mesosphere is that any issues you may encounter and report to me will be fixed.
      + All users who choose to opt in to using mesosphere have my deepest gratitude.
  + **Note:** If using hekate instead of fusee-primary, you will have to wait for the next hekate release for mesosphere to function, as hekate's support has not yet been included in an official release build.
    + This will be updated in the release notes when hekate provides a new release.
  + As mentioned in previous release notes, when mesosphere is stable and well-tested, it will be enabled by default and atmosphère's version will transition to 1.0.0.
+ Having been tested sufficiently over the last half-year, Atmosphere's NCM implementation is now opt-out, rather than opt in.
  + In the unlikely event that any issues are encountered, please report them to @SciresM.
  + Users interested in opting out of using our implementation should set `stratosphere!disable_ncm = 1` in BCT.ini.
  + The NCM implementation will stop being opt-out in a future update, probably around the same time that mesosphere becomes opt-out instead of opt-in.
+ Several bugs were fixed, including:
  + Loader now sets HBL's thread priority to a higher value when loading it in applet mode.
    + This fixes an extremely-slow launch ("hang") when using applet-HBL with certain games that do not suspend while inactive (e.g. Super Mario Sunshine).
  + set.mitm now caches user language configuration much more heavily.
    + This severely reduces lag in certain games which misuse the "nn::oe::GetDesiredLanguage()" API.
  + A bug was fixed that could cause erpt to fatal when loading an official save file that had error report attachments in it.
+ General system stability improvements to enhance the user's experience.
## 0.14.4
+ Several bugs were fixed involving the official jit sysmodule added in 10.0.0.
  + A Process handle leak was fixed when JitPlugin NRRs were registered with the `ro` sysmodule.
    + This prevented processes using jit from being able to exit, causing a full system freeze.
  + The `sm` atmosphere extension to not unregister services when the server's connection is closed was special-case disabled for `jit:u`.
    + This extension is normally desirable in order to allow more concurrent processes to exist (as only 0x40 sm connections may ever be concurrently open), but official jit sysmodule relies on the behavior.
    + This would cause crashes on attempts to launch a program using jit services more than once per reboot.
+ General system stability improvements to enhance the user's experience.
## 0.14.3
+ Support was added for 10.2.0.
+ General system stability improvements to enhance the user's experience.
## 0.14.2
+ A bug was fixed that could cause a deadlock when installing mitm services.
  + Fixing this required a breaking change to the client behavior when installing a mitm service, and so custom sysmodules which use mitm will need to be re-compiled to function properly.
+ A bug was fixed that caused atmosphere sysmodules to respond incorrectly when receiving invalid messages.
+ A bug was fixed that caused fatal auto-reboot timing to work improperly.
+ Support was added to fusee for loading binaries for `mesosphere`, atmosphère's reimplementation of the Nintendo Switch kernel.
  + 0.14.2 does not include mesosphere, but those who are especially interested can build and test mesosphere themselves.
  + In the future, to enable a sufficient testing period Atmosphère releases will distribute two zips for some time.
    + One zip will use mesosphere, and the other will not.
    + This will allow users who are interested to opt-in to mesosphere usage before it has been tested to be stable.
  + When mesosphere is stable and well-tested, it will be enabled by default and Atmosphère's version will transition to 1.0.0.
+ General system stability improvements to enhance the user's experience.
## 0.14.1
+ An issue was fixed in 0.14.0 that would cause a black screen on boot when the INI1's size was not aligned to 8 bytes.
+ General system stability improvements to enhance the user's experience.
## 0.14.0
+ An API (`ams:su`) was added to allow homebrew to safely install system upgrades or downgrades.
  + This is a re-implementation of the logic that `ns` uses to install gamecard system updates.
  + Nintendo (and now atmosphère) uses an installation process that can recover no matter where a failure occurs, which should significantly improve the safety of custom system update installation.
+ Support was added to `exosphère` for running on Mariko hardware.
  + **Please note**: Atmosphère still does not support Mariko, and should not be run on Mariko yet.
    + Certain stratosphere components do not handle mariko-specific logic fully correctly yet, and may initialize or interact with hardware incorrectly.
    + This will be fixed and support will be added over the remainder of the Summer.
+ A homebrew application (`daybreak`) was added that uses the system updater API (with thanks to @Adubbz for both design and implementation).
  + `daybreak` is included with atmosphère, and functions as a safer/more accurate equivalent to e.g. ChoiDujourNX.
  + Upgrades/downgrades can be installed from a folder containing the update NCAs on the SD card.
  + Because the update logic functions identically to Nintendo's, `daybreak` will be safe to use on Mariko when the rest of atmosphère has support.
  + **Please note**: Daybreak requires that meta (.cnmt) NCAs have the correct extension `.cnmt.nca`.
    + This is because gamecard system update logic uses extension to determine whether to mount the content.
    + [Several](https://gist.github.com/HookedBehemoth/df36b5970e1c5b1b512ec7bdd9043c6e) [scripts](https://gist.github.com/antiKk/279966c27fdfd9c7fe63b4ae410f89c4) have been made by community members to automatically rename folders with incorrect extensions.
+ A bug was fixed that would cause file-based emummc to throw an error (showing a hexdump) on boot.
  + Major thanks to @hexkyz for tracking down and resolving this.
+ A number of minor issues were resolved, including:
  + fusee now prints information to the screen when an error occurs, instead of getting stuck trying to initialize the display.
  + A race condition in Horizon was worked around that could prevent boot under certain circumstances.
  + A bug was fixed that would cause atmosphère modules to open ten copies of certain filesystems instead of one.
    + This could cause object exhaustion under certain circumstances.
+ For those interested in atmosphère's future development plans, the project's [roadmap](https://github.com/Atmosphere-NX/Atmosphere/blob/ac9832c5ce7be5832f6d29f6564a9c03e7efd22f/docs/roadmap.md) was updated.
+ General system stability improvements to enhance the user's experience.
## 0.13.0
+ `exosphère`, atmosphère's secure monitor re-implementation, was completely re-written.
  + `exosphère` was the first component authored for the project in early 2018. It is written in C, and in a style very different from the rest of atmosphère's code.
    + This has made the codebase difficult to maintain as time has gone on.
  + `exosphère` was also written to conform to constraints and assumptions that simply no longer apply when cfw is not launched from the web browser, and when warmboothax is possible.
  + Even beyond these issues, `exosphère` used all but 1KB of the 64KB of space available to it. This was a problem for a few reasons:
    + Each new system update added requires additional space to support (to add new keys and reflect various changes); 10.0.0 support used up 3 of the 4KB we had left.
    + atmosphère will want to have software support for mariko hardware, and this is not possible to fit in 1 KB.
  + The `exosphère` rewrite (which was codenamed `exosphère2` during development) solves these problems.
  + The new codebase is C++20 written in atmosphère's style.
    + This solves the maintainability problem, and should make understanding how the secure monitor works *much* easier for those interested in using the code as a reference implementation.
  + In addition, the new implementation currently uses ~59.5 of the 64KB available.
    + Several potential code changes are planned that can save/grant access to an additional ~2-3 KB if needed.
      + Unlike the first codebase, the new `exosphère` actually already has space allocated for future keys/etc. It is currently expected that the reserved space will never be required.
    + The previous implementation chose not to implement a number of "unimportant" secure monitor functions due to space concerns. The new code has enough breathing room that it can implement them without worries. :)
  + Finally, the groundwork for mariko support has been laid -- there are only a few minor changes needed for the new secure monitor implementation to work on both erista and mariko hardware.
    + **Please note**: `exosphère` is only one of many components, and many more need changes to support running on mariko hardware.
      + Software-side support for executing on mariko hardware is expected some time during Summer 2020, though it should also be noted that this is not a hard deadline.
  + **Please note**: The new `exosphère` binary is not abi-compatible with the old one. Users who boot using hekate should wait for it to update before running 0.13.0 (or boot fusee-primary via hekate).
+ atmosphère's api for target firmware was changed. All minor/micro system versions are now recognized, instead of only major versions.
  + This was required in order to support firmware version 5.1.0, which made breaking changes to certain IPC APIs that caused atmosphère 0.12.0 to abort.
  + **Please note**: this is (unavoidably) a breaking change. System modules using atmosphere-libs will need to update to understand what firmware version they are running.
+ `emummc` was updated to include the new changes.
  + `emummc` now uses an updated/improved/faster SDMMC driver.
  + File-based emummc is now almost as fast as raw partition-based emummc.
+ For those interested in atmosphère's future development plans, the project's [roadmap](https://github.com/Atmosphere-NX/Atmosphere/blob/f68d33b70aed8954cc2c539e5934bcaf37ba51da/docs/roadmap.md) was updated.
+ General system stability improvements to enhance the user's experience.

## 0.12.0
+ Configuration for exosphere was moved to sd:/exosphere.ini.
  + This is to facilitate BIS protection changes described below.
  + Hopefully having this outside of the Atmosphere folder will prevent accidental deletion, since this now contains important settings.
+ Atmosphere's bis protection policy for the PRODINFO partition was substantially reworked.
  + Support was added for "automatically" performing a "blanking" operation to PRODINFO without actually modifying NAND.
    + This is equivalent to using the "incognito" homebrew tool, but NAND is never actually modified.
    + This can be turned on in sysmmc by setting `blank_prodinfo_sysmmc=1` in exosphere.ini, and in emummc by setting `blank_prodinfo_emummc=1` in exosphere.ini.
    + **Please note**: This is not known to be safe. There is a lack of research on whether the information blanked out is cached elsewhere in the system.
      + Usage of this option is not encouraged for this reason.
  + Support was added for writing to the PRODINFO partition, if a verified encrypted backup has been made.
    + PRODINFO is the only system data that cannot be recovered if not backed up, and thus Atmosphere has backed it up to the SD card on boot for some time now.
    + Users who wish to modify their calibration data may now do so unconditionally in emummc, and in sysmmc if `allow_writing_to_cal_sysmmc=1` is set in exosphere.ini.
      + **Please note**: This is heavily discouraged, and the typical user will almost never want to do this.
      + Setting this option will cause Atmosphere to attempt to verify (or create) an encrypted backup of the PRODINFO data to an unused region in the partition.
        + The backup is encrypted with per-console keys that Atmosphere's developers do not know.
      + If the backup is not verified or created, writes will not work. Users who have corrupted their PRODINFO in the past are encouraged to flash a good backup to allow use of this setting.
      + Reads and writes to the region used for the securely encrypted backup will appear to succeed, but will actually read/write from a buffer filled with garbage in memory.
  + Support will be investigated in the future for supporting booting with fully blanked calibration.
    + This is desirable to allow boot to succeed for users who lost their calibration data due to bricking homebrew before bis protection was implemented.
+ `creport` has been updated to use the new screenshot APIs added in 9.0.0+.
  + On 10.0.0+, if a crash occurs in an application (not applet or sysmodule) a screenshot will now be automatically saved to the SD card.
  + If the user applies a patch to vi on 9.0.0 (as the command this uses was previously for dev-units only), this can also work on 9.0.0.
+ The new sysmodule `pgl` added in 10.0.0 was reimplemented.
  + `pgl` ("Program Launcher", probably) is responsible for managing launched user-processes, previously this was handled by NS.
  + The most exciting thing about pgl is that it finally provides an API for multiple clients to subscribe to process events.
  + Using these new APIs, system modules / other homebrew can subscribe to be notified whenever a process event occurs.
    + This means action can be taken on process launch, process exit, process crash, etc.
  + A slight concern with Nintendo's implementation is that each subscriber object uses 0x448 bytes of memory, and N only reserves 8KB for all allocations in pgl.
  + Atmosphere's implementation uses a 32KB heap, which should not be exhaustible.
  + Atmosphere's implementation has a total memory footprint roughly 0x28000 bytes smaller than Nintendo's.
+ A reimplementation was added for the `jpegdec` system module (thanks @HookedBehemoth)!
  + This allows two sessions instead of 1, so homebrew can now use it for software jpeg decoding in addition to the OS itself.
  + As usual the implementation has a very slightly smaller memory footprint than Nintendo's.
+ `dmnt`'s Cheat VM was extended to add three new opcodes.
  + The first new opcode, "ReadWriteStaticRegister", allows for cheats to read from a bank of 128 read-only static registers, and write to a bank of 128 write-only static registers.
    + This can be used in concert with new IPC commands that allow a cheat manager to read or write the value of these static registers to have "dynamic" cheats.
      + As an example, a cheat manager could write a value to a static register that a cheat to control how many of an item to give in a game.
      + As another example, a cheat manager could read a static register that a cheat writes to to learn how many items a player has.
+ The second and third opcodes are a pair, "PauseProcess" and "ResumeProcess".
  + Executing pause process in a cheat will pause the game (it will be frozen) until a resume process opcode is used.
    + These are also available over IPC, for cheat managers or system modules that want to pause or resume the attached cheat process.
  + This allows a cheat to know that the game won't modify or access data the cheat is accessing.
    + For example, this can be used to prevent Pokemon from seeing a pokemon a cheat is in the middle of injecting and turning it into a bad egg.
+ A bug was fixed that would cause the console to crash when connected to Wi-Fi on versions between 3.0.0 and 4.1.0 inclusive.
+ A bug was fixed that could cause boot to fail sporadically due to cache/tlb mismanagement when doing physical ASLR of the kernel.
+ A number of other minor issues were addressed (and more of Atmosphere was updated to reflect other changes in 10.0.x).
+ General system stability improvements to enhance the user's experience.

## 0.11.1
+ A bug was fixed that could cause owls to flicker under certain circumstances.
  + For those interested in technical details, in 10.0.0 kernelldr/kernel no longer set cpuactlr_el1, assuming that it was set correctly by the secure monitor.
  + However, exosphere did not set cpuactlr_el1. This meant that the register held the reset value going into boot.
  + This caused a variety of highly erratic symptoms, including causing basically any game to crash seemingly randomly.
+ A number of other major inaccuracies in exosphere were corrected.
+ General system stability improvements to enhance the user's experience.

## 0.11.0
+ Support was added for 10.0.0.
  + Exosphere has been updated to reflect the new key import semantics in 10.0.0.
  + kernel_ldr now implements physical ASLR for the kernel's backing pages.
  + Loader, NCM, and PM have been updated to reflect the changes Nintendo made in 10.0.0.
  + Creport was updated to use the new `pgl` service to terminate processes instead of `ns:dev`.
+ A reimplementation of the `erpt` (error reports) system module was added.
  + In previous versions of Atmosphere, a majority of error reports were prevented via a combination of custom creport, fatal, and stubbed eclct.
  + However, error reports were still generated via some system actions.
    + Most notably, any time the error applet appeared, an error report was generated.
    + By default, atmosphere disabled the *uploading* of error reports, but going online in OFW after an error report occurred in Atmosphere could lead to undesirable telemetry.
  + Atmosphere's `erpt` reimplementation allows the system to interact with existing error reports as expected.
  + However, all new error reports are instead saved to the sd card (`/atmosphere/erpt_reports`), and are not committed to the system savegame.
    + Users curious about what kind of telemetry is being prevented can view the reports as they're generated in there.
    + Reports are saved as msgpack (as this is what Nintendo uses).
  + Please note, not all telemetry is disabled. Play reports and System reports will continue to function unmodified.
  + With atmosphere's `erpt` implementation, homebrew can now use the native error applet to display errors without worrying about generating undesirable telemetry.
+ libstratosphere and libvapours received a number of improvements.
  + With thanks to @Adubbz for his work, the NCM namespace now has client code.
    + This lays the groundwork for first-class system update/downgrade homebrew support in the near future.
  + In particular, code implementing the os namespace is significantly more accurate.
  + In addition, Nintendo's allocators were implemented, allowing for identical memory efficiency versus Nintendo's implementations.
+ General system stability improvements to enhance the user's experience.

## 0.10.5
+ Changes were made to the way fs.mitm builds images when providing a layeredfs romfs.
  + Building romfs metadata previously had a memory cost of about ~4-5x the file table size.
  + This caused games that have particularly enormous file metadata tables (> 4 MB) to exhaust fs.mitm's 16 MB memory pool.
  + The code that creates romfs images was thus changed to be significantly more memory efficient, again.
  + Memory requirements have been lowered from ~4x file table size to ~2x file table size + 0.5 MB.
  + There is a slight speed penalty to this, but testing on Football Manager 2020 only took an extra ~1.5 seconds for the game to boot with many modded files.
    + This shouldn't be noticeable thanks to the async changes made in 0.10.2.
  + If you encounter a game that exhausts ams.mitm's memory (crashing it) when loading layeredfs mods, please contact SciresM.
    + Romfs building can be made even more memory efficient, but unless games show up with even more absurdly huge file tables it seems not worth the speed trade-off.
+ A bug was fixed that caused Atmosphere's fatal error context to not dump TLS for certain processes.
+ General system stability improvements to enhance the user's experience.

## 0.10.4
+ With major thanks to @Adubbz for his work, the NCM system module has now been re-implemented.
  + This was a major stepping stone towards the goal of having implementations everything in the Switch's package1/package2 firmware.
  + This also lays the groundwork for libstratosphere providing bindings for changing the installed version of the Switch's OS.
  + **Please Note**: The NCM implementation will initially be opt-in.
    + The Atmosphere team is confident in our NCM implementation (and we have tested it on every firmware version).
    + That said, this is our first system module that manages NAND savegames -- and caution is a habit.
    + We do not anticipate any issues that didn't come up in testing, so this is just our being particularly careful.
    + Users interested in opting in to using our implementation should set `stratosphere!ncm_enabled = 1` in BCT.ini.
      + In the unlikely event that any issues are encountered, please report them to @SciresM.
    + The NCM implementation will stop being opt-in in a future update, after thorough testing has been done in practice.
+ A bug was fixed in emummc that caused Nintendo path to be corrupted on 1.0.0.
  + This manifested as the emummc folder being created inside the virtual NAND instead of the SD card.
  + It's unlikely there are any negative consequences to this in practice.
    + If you want to be truly sure, you can re-clone sysmmc before updating a 1.0.0 emummc to latest firmware.
+ Stratosphere system modules now use new Nintendo-style FS bindings instead of stdio.
  + This saves a modest amount of memory due to leaner code, and greatly increases the accuracy of several components.
  + These bindings will make it easier for other system modules using libstratosphere to interact with the filesystem.
  + This also lays the groundwork for changes necessary to support per-emummc Atmosphere folders in a future update.
+ Atmosphere's fatal error context now dumps 0x100 of TLS.
  + This will make it much easier to fix bugs when an error report is dumped for whatever caused the crash.
+ General system stability improvements to enhance the user's experience.

## 0.10.3
+ Support was added for 9.2.0.
+ Support was added for redirecting manual html content for games.
  + This works like normal layeredfs, replacing content placed in `/atmosphere/contents/<program id>/manual_html/`.
  + This allows for game mods/translations to provide custom manual content, if they so choose.
+ A number of improvements were made to Atmosphere's memory usage, including:
  + `fatal` now uses STB instead of freetype for rendering.
    + This saves around 1 MB of memory, and makes our fatal substantially leaner than Nintendo's.
  + `sm` no longer wastes 2 MiB unnecessarily.
+ fusee/sept's sdmmc access now better matches official behavior.
  + This improves compatibility with some SD cards.
+ `ro` has been updated to reflect changes made in 9.1.0.
+ The temporary auto-migration added in 0.10.0 has been removed, since the transitionary period is well over.
+ General system stability improvements to enhance the user's experience.

## 0.10.2
+ hbl configuration was made more flexible.
  + Up to eight specific program ids can now be specified to have their own override keys.
  + This allows designating both the album applet and a specific game as hbl by default as desired.
  + Configuration targeting a specific program is now mutually exclusive with override-any-app for that program.
    + This fixes unintuitive behavior when override key differed for an application specific program.
+ Loader's external content fileystem support was fixed (thanks @misson20000!).
+ KernelLdr was reimplemented.
  + This is the first step towards developing mesosphere, Atmosphere's planned reimplementation of the Switch's Kernel.
  + The typical user won't notice anything different, as there are no extensions, but a lot of groundwork was laid for future development.
+ Improvements were made to the way Atmosphere's buildsystem detects source code files.
  + This significantly reduces compilation time (saving >30 seconds) on the machine that builds official releases.
+ Certain device code was cleaned up and made more correct in fusee/sept/exosphere (thanks @hexkyz!).
+ A number of changes were made to the way fs.mitm builds images when providing a layeredfs romfs.
  + Some games (Resident Evil 6, Football Manager 2020 Touch, possibly others) have enormous numbers of files.
  + Attempting to create a layeredfs mod for these games actually caused fs.mitm to run out of memory, causing a fatal error.
  + The code that creates these images was changed to be significantly more memory efficient.
  + However, these changes also cause a significant slowdown in the romfs building code (~2-5x).
  + This introduced a noticeable stutter when launching a game, because the UI thread would block on the romfs creation.
  + To solve this, fs.mitm now lazily initializes the image in a background thread.
  + This fixes stutter issues, however some games may be slightly slower (~1-2s in the worst cases) to transition from the "loading" GIF to gameplay now.
    + Please note: the slowdown is not noticeable in the common case where games don't have tons of files (typical is ~0.1-0.2 seconds).
    + Once the image has been built, there is no further speed penalty at runtime -- only when the game is launched.
+ A number of other bugs were fixed, including:
  + Several minor logic inversions that could have caused fatal errors when modding games.
  + Atmosphere's new-ipc code did not handle "automatic" recvlist buffers correctly, so some non-libnx homebrew could crash.
  + fs.mitm now correctly mitms sdb, which makes redirection of certain system data archives work again.
    + In 0.10.0/0.10.1, changing the system font/language did not work correctly due to this.
  +  A bug was fixed in process cleanup that caused the system to hang on < 5.0.0.
+ The temporary hid-mitm added in Atmosphere 0.9.0 was disabled by default.
  + Please ensure your homebrew is updated.
  + For now, users may re-enable this mitm by use of a custom setting (`atmosphere!enable_deprecated_hid_mitm`) to ease the transition process some.
  + Please note: support for this setting may be removed to save memory in a future atmosphere release.
+ General system stability improvements to enhance the user's experience.

## 0.10.1
+ A bug was fixed that caused memory reallocation to the system pool to work improperly on firmware 5.0.0 and above.
  + Atmosphere was always trying to deallocate memory away from the applet pool and towards the system pool.
    + The intent of this is to facilitate running more custom sysmodules/atmosphere binaries.
  + However, while memory was always successfully taken away from the applet pool, on 5.0.0+ granting it to the system pool did not work for technical reasons.
    + If you are interested in the technical details, talk to SciresM.
  + This has now been fixed by adding new kernel patches, and memory is correctly granted to the system pool as intended.
+ Atmosphere's library system has been overhauled:
  + libstratosphere's repository has been rebranded, more generally, to "Atmosphere-libs".
    + In addition to libstratosphere, a new general library for not-stratosphere-specific code has been added.
      + This is currently named `libvapours`.
    + In the future, kernel functionality will be available as `libmesosphere`.
  + The build system for stratosphere system modules has been similarly overhauled.
+ A number of other bugs were fixed, including:
  + A bug was fixed that could cause memory corruption when redirecting certain Romfs content.
  + A bug was fixed that could cause an infinite loop when redirecting certain Romfs content.
  + A bug was fixed that could cause certain NROs to fail to load.
    + This caused the latest version of Super Smash Bros to display "An error has occurred" on launch.
  + A bug was fixed that caused input/output array sizes for certain circumstances to be calculated incorrectly.
    + This caused cheats to fail to function properly.
  + C++ exception code is now more thoroughly removed from stratosphere executables.
    + This saves a minor amount of memory.
  + A number of minor logic inversions were fixed in libstratosphere.
    + These did not affect any code currently used by released Atmosphere binaries.
+ *Please note**: Because this update is releasing so soon after 0.10.0, the removal of the temporary hid-mitm has been postponed to 0.10.2.
    + Please ensure your homebrew is updated.
+ Random number generation now uses TinyMT instead of XorShift.
+ General system stability improvements to enhance the user's experience.

## 0.10.0
+ Support was added for 9.1.0
  + **Please note**: The temporary hid-mitm added in Atmosphere 0.9.0 will be removed in Atmosphere 0.10.1.
    + Please ensure your homebrew is updated.
+ The Stratosphere rewrite was completed.
  + libstratosphere was rewritten as part of Stratosphere's refactor.
    + Code responsible for providing and managing IPC services was greatly improved.
      + The new code is significantly more accurate (it is bug-for-bug compatible with Nintendo's code), and significantly faster.
  + ams.mitm was rewritten as part of Stratosphere's refactor.
    + Saves redirected to the SD card are now separated for sysmmc vs emummc.
    + **Please note**: If you find any bugs, please report them so they can be fixed.
+ Thanks to the rewrite, Atmosphere now uses significantly less memory.
  + Roughly 10 additional megabytes are now available for custom system modules to use.
  + This means you can potentially run more custom system modules simultaneously.
    + If system modules are incompatible, please ask their authors to reduce their memory footprints.
+ Atmosphere's configuration layout has had major changes.
  + Configuration now lives inside /atmosphere/config/.
  + Atmosphere code now knows what default values should be, and includes them in code.
    + It is no longer an error if configuration inis are not present.
  + Correspondingly, Atmosphere no longer distributes default configuration inis.
    + Templates are provided in /atmosphere/config_templates.
  + loader.ini was renamed to override_config.ini.
  + This fixes the longstanding problem that atmosphere updates overwrote user configuration when extracted.
+ Atmosphere's process override layout was changed.
  + Atmosphere now uses the /atmosphere/contents directory, instead of /atmosphere/titles.
    + This goes along with a refactoring to remove all reference to "title id" from code, as Nintendo does not use the term.
  + To make this transition easier, a temporary functionality has been added that migrates folders to the new directory.
    + When booting into 0.10.0, Atmosphere will rename /atmosphere/titles/`<program id>` to /atmosphere/contents/`<program id>`.
      + This functionality may or may not be removed in some future update.
    + This should solve any transition difficulties for the typical user.
    + Please make sure that any future mods you install extract to the correct directory.
+ Support for configuring override keys for hbl was improved.
  + The key used to override applications versus a specific program can now be different.
    + The key to override a specific program can be managed via override_key.
    + The key to override any app can be managed via override_any_app_key.
  + Default override behavior was changed.
    + By default, atmosphere will now override the album applet with hbl unless R is held.
    + By default, atmosphere will now override any application with hbl only if R is held.
+ The default amount of applet memory reserved has been slightly increased.
  + This allows the profile selector applet to work by default in applet mode.
+ The way process override status is captured was changed.
  + Process override keys are now captured exactly once, when the process is created.
    + This fixes the longstanding issue where letting go of the override button partway into the process launch could cause problems.
  + The Mitm API was changed to pass around override status.
    + Mitm services now know what keys were held when the client was created, as well as whether the client is HBL/should override contents.
  + An extension was added to pm:info to allow querying a process's override status.
+ Thanks to process override capture improvements, hbl html behavior has been greatly improved.
  + Web applets launched by hbl will now always see the /atmosphere/hbl_html filesystem
+ Support was added to exosphere for enabling usermode access to the PMU registers.
  + This can be controlled via exosphere!enable_user_pmu_access in BCT.ini.
+ An enormous number of minor bugs were fixed.
  + dmnt's cheat VM had a fix for an inversion in opcode behavior.
  + An issue was fixed in fs.mitm's management of domain object IDs that could lead to system corruption in rare cases.
  + The Mitm API no longer silently fails when attempting to handle commands passing C descriptors.
    + On previous atmosphere versions, certain commands to FS would silently fail due to this...
      + No users reported any visible errors, but it was definitely a problem behind the scenes.
    + These commands are now handled correctly.
  + Atmosphere can now display a fatal error screen significantly earlier in the boot process, if things go wrong early on.
  + The temporary hid mitm will no longer sometimes cause games to fail to detect input.
  + Mitm Domain object ID management no longer desynchronizes from the host process.
  + An issue was fixed that could cause service acquisition to hang forever if certain sm commands were called in a precise order.
  + An off-by-one was fixed that could cause memory corruption in server memory management.
  + ... and too many more bugs fixed to reasonably list them all :)
+ General system stability improvements to enhance the user's experience.

## 0.9.4
+ Support was added for 9.0.0.
  + **Please note**: 9.0.0 made a number of changes that may cause some issues with homebrew. Details:
  + 9.0.0 changed HID in a way that causes libnx to be unable to detect button input.
    + Homebrew should be recompiled with newest libnx to fix this.
    + Atmosphere now provides a temporary hid-mitm that will cause homebrew to continue to work as expected.
      + This mitm will be removed in a future Atmosphere revision once homebrew has been updated, to allow users to use a custom hid mitm again if they desire.
  + 9.0.0 introduced an dependency in FS on the USB system module in order to launch the SD card.
    + This means the USB system module must now be launched before the SD card is initialized.
    + Correspondingly, the USB system module can no longer be IPS patched, and its settings cannot be reliably mitm'd.
    + We know this is frustrating, so we'll be looking into whether there is some way of addressing this in the future.
+ An off-by-one error was fixed in `boot` system module's pinmux initialization.
  + This could theoretically have caused issues with HdmiCec communication.
  + No users reported issues, so it's unclear if this was a problem in practice.
+ A bug was fixed that could cause webapplet launching homebrew to improperly set the accessible url whitelist.
+ BIS key generation has been fixed for newer hardware.
  + Newer hardware uses new, per-firmware device key to generate BIS keys instead of the first device key, so previously the wrong keys were generated as backup.
  + This only affects units manufactured after ~5.0.0.
+ General system stability improvements to enhance the user's experience.

## 0.9.3
+ Thanks to hexkyz, fusee's boot sequence has been greatly optimized.
  + Memory training is now managed by a separate binary (`fusee-mtc`, loaded by fusee-primary before fusee-secondary).
  + Unnecessarily long splash screen display times were reduced.
  + The end result is that Atmosphere now boots *significantly* faster. :)
  + **Note:** This means fusee-primary must be updated for Atmosphere to boot successfully.
+ The version string was adjusted, and now informs users whether or not they are using emummc.
+ Atmosphere now automatically backs up the user's BIS keys on boot.
  + This should prevent a user from corrupting nand without access to a copy of the keys needed to fix it.
    + This is especially relevant on ipatched units, where the RCM vulnerability is not an option for addressing bricks.
+ The `pm` system module was rewritten as part of Stratosphere's ongoing refactor.
  + Support was added for forward-declaring a mitm'd service before a custom user sysmodule is launched.
    + This should help resolve dependency issues with service registration times.
  + SM is now informed of every process's title id, including built-in system modules.
+ The `creport` system module was rewritten as part of Stratosphere's ongoing refactor.
  + creport now dumps up to 0x100 of stack from each thread in the target process.
  + A few bugs were fixed, including one that caused creport to incorrectly dump process dying messages.
+ Defaults were added to `system_settings.ini` for controlling hbloader's memory usage in applet mode.
  + These defaults reserve enough memory so that homebrew can launch swkbd while in applet mode.
+ The `fatal` system module was rewritten as part of Stratosphere's ongoing refactor.
  + Incorrect display output ("2000-0000") has been fixed. Fatal will now correctly show 2162-0002 when this occurs.
  + A longstanding bug in how fatal manages the displays has been fixed, and official display init behavior is now matched precisely.
+ General system stability improvements to enhance the user's experience.

## 0.9.2
+ A number of emummc bugfixes were added (all thanks to @m4xw's hard work). The following is a summary of emummc changes:
  + Support for file-based emummc instances was fixed.
    + Please note: file-based emummc is still unoptimized, and so may be much slower than partition-based.
    + This speed differential should hopefully be made better in a future emummc update.
  + The way emummc handles power management was completely overhauled.
    + Emummc now properly handles init/de-init, and now supports low voltage mode.
    + Much better support for shutdown was added, which should assuage corruption/synchronization problems.
    + This should also improve support for more types of SD cards.
  + A bug was fixed that caused emummc to not work on lower system versions due to missing SVC access.
  + **Please note**: The configuration entries used for emummc have been changed.
    + `emummc_` prefixes have been removed, since they are superfluous given the `emummc` category they are under.
    + As an example, `emummc!emummc_enabled` is now `emummc!enabled`.
    + INI configurations made by @CTCaer's [tool](https://github.com/ctcaer/hekate/releases/latest) (which is the recommended way to manage emummc) should automatically work as expected/be corrected.
      + **If you do not wish to use the above, you will need to manually correct your configuration file**.
  + General system stability improvements to enhance the user's experience.
+ Stratosphere is currently in the process of being re-written/refactored.
  + Stratosphere was my (SciresM's) first C++ project, ever -- the code written for it a year ago when I was learning C++ is/was of much lower quality than code written more recently.
  + Code is thus being re-rwitten for clarity/stlye/to de-duplicate functionality, with much being moved into libstratosphere.
  + Stratosphere will, after the rewrite, globally use the `sts::` namespace -- this should greatly enhancing libstratosphere's ability to provide functionality for system modules.
  + The rewritten modules consistently have lower memory footprints, and should be easier to maintain going forwards.
  + The `sm`, `boot`, `spl`, `ro`, and `loader` modules have been tackled so far.
+ General system stability improvements to enhance the user's experience.

## 0.9.1
+ Support was added for 8.1.0.
+ Please note, emummc is still considered **beta/experimental** -- this is not the inevitable bugfix update for it, although some number of bugs have been fixed. :)
+ General system stability improvements to enhance the user's experience.

## 0.9.0
+ Creport output was improved significantly.
  + Thread names are now dumped on crash in addition to 0x100 of TLS from each thread.
    + This significantly aids debugging efforts for crashes.
  + Support was added for 32-bit stackframes, so reports can now be generated for 32-bit games.
+ `dmnt`'s Cheat VM was extended to add a new debug opcode.
+ With thanks to/collaboration with @m4xw and @CTCaer, support was added for redirecting NAND to the SD card (emummc).
  + Please note, this support is very much **beta/experimental**.
    + It is quite likely we have not identified all bugs -- those will be fixed as they are reported over the next few days/weeks.
    + In addition, some niceties (e.g. having a separate Atmosphere folder per emummc instance) still need some thought put in before they can be implemented in a way that makes everyone happy.
    + If you are not an advanced user, you may wish to think about waiting for the inevitable 0.9.1 bugfix update before using emummc as your default boot option.
      + You may especially wish to consider waiting if you are using Atmosphere on a unit with the RCM bug patched.
  + Emummc is managed by editing the emummc section of "emummc/emummc.ini".
    + To enable emummc, set `emummc!emummc_enabled` = 1.
  + Support is included for redirecting NAND to a partition on the SD card.
    + This can be done by setting `emummc!emummc_sector` to the start sector of your partition (e.g., `emummc_sector = 0x1A010000`).
  + Support is also included for redirecting NAND to a collection of loose files on the SD card.
    + This can be done by setting `emummc!emummc_path` to the folder (with archive bit set) containing the NAND boot partitions' files "boot0" and "boot1", and the raw NAND image files "00", "01", "02", etc. (single "00" file with the whole NAND image requires exFAT mode while multipart NAND can be used in both exFAT and FAT32 modes).
  + The `Nintendo` contents directory can be redirected arbitrarily.
    + By default, it will be redirected to `emummc/Nintendo_XXXX`, where `XXXX` is the hexadecimal representation of the emummc's ID.
      + The current emummc ID may be selected by changing `emummc!emummc_id` in emummc.ini.
    + This can be set to any arbitrary directory by setting `emummc!emummc_nintendo_path`.
  + To create a backup usable for emummc, users may use tools provided by the [hekate](https://github.com/CTCaer/hekate) project.
  + If, when using emummc, you encounter a bug, *please be sure to report it* -- that's the only way we can fix it. :)

## 0.8.10
+ A bug was fixed that could cause incorrect system memory allocation on 5.0.0.
  + 5.0.0 should now correctly have an additional 12 MiB allocated for sysmodules.
+ Atmosphère features which check button presses now consider all controllers, isntead of just P1.
+ Support was added for configuring language/region on a per-game basis.
  + This is managed by editing `atmosphere/titles/<title id>/config.ini` for the game.
  + To edit title language, edit `override_config!override_language`.
    + The languages supported are `ja`, `en-US`, `fr`, `de`, `it`, `es`, `zh-CN`, `ko`, `nl`, `pt`, `ru`, `zh-TW`, `en-GB`, `fr-CA`, `es-419`, `zh-Hans`, `zh-Hant`.
  + To edit title region, edit `override_config!override_region`.
    + The regions supported are `jpn`, `usa`, `eur`, `aus`, `chn`, `kor`, `twn`.
+ Atmosphère now provides a reimplementation of the `boot` system module.
  + `boot` is responsible for performing hardware initialization, showing the Nintendo logo, and repairing NAND on system update failure.
  + Atmosphère's `boot` implementation preserves AutoRCM during NAND repair.
    + NAND repair occurs when an unexpected shutdown or error happens during a system update.
    + This fixes a final edge case where AutoRCM might be removed by HOS, which could cause a user to burn fuses.
+ General system stability improvements to enhance the user's experience.

## 0.8.9
+ A number of bugs were fixed, including:
  + A data abort was fixed when mounting certain partitions on NAND.
  + All Stratosphère system modules now only maintain a connection to `sm` when actively using it.
    + This helps mitigate the scenario where sm hits the limit of 64 active connections and crashes.
    + This sometimes caused crashes when custom non-Atmosphère sysmodules were active and the user played certain games (ex: Smash's Stage Builder).
  + fatal now uses the 8.0.0 clkrst API, instead of silently failing to adjust clock rates on that firmware version.
  + A wait loop is now performed when trying to get a session to `sm`, in the case where `sm:` is not yet registered.
    + This fixes a race condition that could cause a failure to boot under certain circumstances.
  + libstratosphere's handling of domain object closing has been improved.
    + Previously, this code could cause crashes/extremely odd behavior (misinterpreting what object a service is) under certain circumstances.
+ An optional automatic reboot timer was added to fatal.
  + By setting the system setting `atmosphere!fatal_auto_reboot_interval` to a non-zero u64 value, fatal can be made to automatically reboot after a certain number of milliseconds.
  + If the setting is zero or not present, fatal will wait for user input as usual.
+ Atmosphère now provides a reimplementation of the `ro` system module.
  + `ro` is responsible for loading dynamic libraries (NROs) on 3.0.0+.
    + On 1.0.0-2.3.0, this is handled by `loader`.
  + Atmosphere's `ro` provides this functionality (`ldr:ro`, `ro:dmnt`) on all firmware versions.
  + An extension was implemented to provide support for applying IPS patches to NROs.
    + All patches at paths like /atmosphere/nro_patches/<user-defined patch name>/<Hex Build-ID for NRO to patch>.ips will be applied, allowing for easy distribution of patches.
    + Both the IPS and IPS32 formats are supported.
+ Atmosphère now provides a reimplementation of the `spl` system module.
  + `spl` (Secure Platform Services) is responsible for cryptographic operations, including all communications with the secure monitor (exosphère).
  + In the future, this may be used to provide extensions to the API for interacting with exosphère from userland.
+ General system stability improvements to enhance the user's experience.

## 0.8.8
+ Support was added for firmware version 8.0.0.
+ Custom exception handlers were added to stratosphere modules.
  + If a crash happens in a core atmosphere module now, instead of silently failing a reboot will occur to log the information to the SD card.
+ A bug was fixed in creport that caused games to hang when crashing under certain circumstances.
+ A bug was fixed that prevented maintenance mode from booting on 7.0.0+.
+ General system stability improvements to enhance the user's experience.

## 0.8.7
+ A few bugs were fixed that could cause fatal to fail to show an error under certain circumstances.
+ A bug was fixed that caused an error when launching certain games (e.g. Hellblade: Senua's Sacrifice).
  + Loader had support added in ams-0.8.4 for a new (7.0.0+) flag bit in NPDMs during process creation, but forgot to allow this bit to be set when validating the NPDM.
+ dmnt's cheat virtual machine received new instructions.
  + These allow for saving, restoring, or clearing registers to a secondary bank, effectively doubling the number of values that can be stored.
+ SHA256 code has been swapped from linux code to libnx's new hw-accelerated cryptography API.
+ Extensions were added to smcGetInfo:
  + A ConfigItem was added to detect whether the current unit has the RCM bug patched.
  + A ConfigItem was added to retrieve the current Atmosphère build hash.
+ Exosphère now tells the kernel to enable user-mode exception handlers, which should allow for better crash reporting/detection from Atmosphère's modules in the future..
+ Opt-in support was added for redirecting game save files to directories on the SD card.
  + Please note, this feature is **experimental**, and may cause problems. Please use at your own risk (and back up your saves before enabling it), as it still needs testing.
  + This can be enabled by setting `atmosphere!fsmitm_redirect_saves_to_sd` to 1 in `system_settings.ini`.
+ General system stability improvements to enhance the user's experience.

## 0.8.6
+ A number of bugs were fixed, including:
  + A case of inverted logic was fixed in fs.mitm which prevented the flags system from working correctly.
  + Time service access was corrected in both creport/fatal.
    + This fixes the timestamps used in fatal/crash report filenames.
  + A coherency issue was fixed in exosphère's Security Engine driver.
    + This fixes some instability issues encountered when overclocking the CPU.
  + Loader now unmaps NROs correctly, when ldr:ro is used.
    + This fixes a crash when repeatedly launching the web applet on < 3.0.0.
  + Usage of hidKeysDown was corrected to hidKeysHeld in several modules.
    + This fixes a rare issue where keypresses may have been incorrectly detected.
  + An issue with code filesystem unmounting was fixed in loader.
    + This issue could occasionally cause a fatal error 0x1015 to be thrown on boot.
  + Two bugs were fixed in the implementations of dmnt's cheat virtual machine.
    + These could cause cheats to work incorrectly under certain circumstances.
  + PM now uses a static buffer instead of a dynamically allocated one during process launch.
    + This fixes a memory exhaustion problem when building with gcc 8.3.0.
  + A workaround for a deadlock bug in Horizon's kernel on >= 6.0.0 was added in dmnt.
    + This prevents a system hang when booting certain titles with cheats enabled (ex: Mario Kart 8 Deluxe).
  + set.mitm now reads the system firmware version directly from the system version archive, instead of calling into set:sys.
    + This fixes compatibility with 1.0.0, which now successfully boots again.
+ dmnt's cheat virtual machine had some instruction set changes.
  + A new opcode was added for beginning conditional blocks based on register contents.
  + More addressing modes were added to the StoreRegisterToAddress opcode.
  + These should allow for more complex cheats to be implemented.
+ A new system for saving the state of cheat toggles between game boots was added.
  + Toggles are now saved to `atmosphere/titles/<title id>/cheats/toggles.txt` when either toggles were successfully loaded from that file or the system setting `atmosphere!dmnt_always_save_cheat_toggles` is non-zero.
  + This removes the need for manually setting cheats from all-on or all-off to the desired state on each game boot.
+ The default behavior for loader's HBL support was changed.
  + Instead of launching HBL when album is launched without R held, loader now launches HBL when album or any game is launched with R held.
  + Loader will now override any app in addition to a specific title id when `hbl_config!override_any_app` is true in `loader.ini`.
    + Accordingly, the `hbl_config!title_id=app` setting was deprecated. Support will be removed in Atmosphère 0.9.0.
+ First-class support was added to loader and fs.mitm for enabling homebrew to launch web applets.
  + Loader will now cause the "HtmlDocument" NCA path to resolve for whatever title HBL is taking over, even if it would not normally do so.
  + fs.mitm will also now cause requests to mount the HtmlDocument content for HBL's title to open the `sdmc:/atmosphere/hbl_html` folder.
    + By default, this just contains a URL whitelist.
+ General system stability improvements to enhance the user's experience.

## 0.8.5
+ Support was added for overriding content on a per-title basis, separate from HBL override.
  + This allows for using mods on the same title that one uses to launch HBL.
  + By default, `!L` is used for title content override (this is configurable by editing `default_config!override_key` in `loader.ini`)
  + This key combination can be set on a per-title basis by creating a `atmosphere/titles/<title id>/config.ini`, and editing `override_config!override_key`.
+ Content headers were added for the embedded files inside of fusee-secondary.
  + This will allow non-fusee bootloaders (like `hekate`) to extract the components bundled inside release binaries.
  + This should greatly simplify the update process in the future, for users who do not launch Atmosphère using fusee.
+ Support for cheat codes was added.
  + These are handled by a new `dmnt` sysmodule, which will also reimplement Nintendo's Debug Monitor in the future.
  + Cheat codes can be enabled/disabled at application launch via a per-title key combination.
    + For details, please see the [cheat loading documentation](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/cheats.md#cheat-loating-process).
  + Cheat codes are fully backwards compatible with the pre-existing format, although a number of bugs have been fixed and some new features have been added.
    + For details, please see [the compatibility documentation](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/cheats.md#cheat-code-compatibility).
  + An HIPC service API was added (`dmnt:cht`), that will allow user homebrew to interface with and control Atmosphère's cheat manager.
    + Please see [the relevant documentation](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/modules/dmnt.md).
    + Full client code can be found in [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere/blob/master/include/stratosphere/services/dmntcht.h).
    + Users interested in interfacing should see [EdiZon](https://github.com/WerWolv/EdiZon), which should have support for interfacing with Atmosphère's API shortly after 0.8.5 releases.
+ A bug was fixed that would cause Atmosphère's fatal screen to not show on 1.0.0-2.3.0.
+ A bug was fixed that caused Atmosphère's automatic ProdInfo backups to be corrupt.
+ General system stability improvements to enhance the user's experience.

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

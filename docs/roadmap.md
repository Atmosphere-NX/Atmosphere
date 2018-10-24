# Planned Features
The following features are planned to be added in future versions of Atmosphère:
+ Thermosphère, a hypervisor-based emunand implementation.
+ A feature-rich debugging toolset (a component of Stratosphère).
  + A custom debug monitor system module, providing an API for debugging Switch's processes. This may not be a reimplementation of Nintendo's own debug monitor.
    + This should include a gdbstub implementation, possibly borrowing from Luma3DS's.
    + This API should be additionally usable for RAM Editing/"Cheat Engine" purposes.
  + A custom shell system module, providing an means for users to perform various RPC (with support for common/interesting functionality) on their Switch remotely. This may not be a reimplementation of Nintendo's own shell.
    + This should support client connections over both Wi-Fi and USB.
  + A custom logging system module, providing a means for other Atmosphère components (and possibly Nintendo's own system modules) to log debug output.
    + This should support logging to the SD card, over Wi-Fi, and over USB.
+ An application-level plugin system.
  + This will, ideally, work somewhat like NTR-CFW's plugin system on the 3DS, allowing users to run their own code in a game's process in their own thread.
+ An AR Code/Gameshark analog implementation, allowing for easy sharing/development of cheat codes to run on device.
+ Further extensions to existing Atmosphère components.
+ General system stability improvements to enhance the user's experience.

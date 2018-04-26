Atmosphère-NX
=====

![License](https://img.shields.io/badge/License-GPLv2-blue.svg)

Atmosphère is a work-in-progress customized firmware for the Nintendo Switch.


### Stratosphère (Custom Sysmodule(s)) - In Progress

For user's benefits, we'll likely want custom sysmodules -- both of the Rosalina style to extend the kernel/provide new features, and of the loader re-implementation style to hook important system actions.


### Troposphère (Application-level Horizon OS patches) - In Progress

Patches will need to be developed for Horizon to implement desirable CFW features.



### Thermosphère (EL2 Emunand support) - In Progress

In the absence of a released coldboot entrypoint, Atmosphère will want to support backing up and using virtualized/redirected NAND images.


### Exosphère (Customized TrustZone) - In Progress

Atmosphère will want to run a customized Secure Monitor.


### Fusée (First-stage Loader) - In Progress

Atmosphère will need a loader -- this will be responsible for loading and validating stage 2 (custom TrustZone) plus package2 (Kernel/FIRM sysmodules), and patching them as needed. (This should implement all functionality normally in Package1loader/NX Bootloader).

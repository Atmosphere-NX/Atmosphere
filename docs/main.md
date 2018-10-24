# Atmosphère
Atmosphère is a work-in-progress customized firmware for the Nintendo Switch. Atmosphère consists of several different components, each in charge of performing different system functions of the Nintendo Switch.

The components of Atmosphère are:
+ [Fusée](../docs/components/fusee/fusee.md), a custom bootloader.
+ [Exosphère](../docs/components/exosphere.md), a fully-featured custom secure monitor.
+ [Stratosphère](../docs/components/stratosphere.md), a set of custom system modules.
+ [Thermosphère](../docs/components/thermosphere.md), a hypervisor-based emuNAND implementation. This component has not been implemented yet.
+ [Troposphère](../docs/components/troposphere.md), Application-level patches to the Horizon OS. This component has also not been implemented yet.

### Modules
The Stratosphère component of Atmosphère contains various modules. These have a `.kip` extension. They provide custom features, extend existing features, or replace Nintendo sysmodules.

Stratosphère's modules include:
+ [boot](../docs/modules/boot.md)
+ [creport](../docs/modules/creport.md)
+ [fs_mitm](../docs/modules/fs_mitm.md)
+ [loader](../docs/modules/loader.md)
+ [pm](../docs/modules/pm.md)
+ [sm](../docs/modules/sm.md)

### Building Atmosphère
A guide to building Atmosphère can be found [here](../docs/building.md).

### Upcoming Features
A list of planned features for Atmosphère can be found [here](../docs/roadmap.md).

### Release History
A changelog of previous versions of Atmosphère can be found [here](../docs/changelog.md).

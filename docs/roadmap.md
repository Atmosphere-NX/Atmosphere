# Planned Features
atmosphère has a number of features that are either works-in-progress or planned. Please note that while time-estimates are given, they are loose, and things may be completed sooner or later than advertised.

The following descriptions were last updated on June 15th, 2020.

## system updater api
* **Description**: A planned extension api for stratosphere (tenatively `ams:su`), this will provide an interface for homebrew to safely install system upgrades or downgrades. This will allow for much more easily transitioning safely between different versions of the operating system.
* **Development Status**: Backend/implementation completed; final stages (user-facing ipc api) to be written by SciresM.
* **Estimated Time**: June 2020

## ams-on-mariko
* **Description**: Atmosphere cannot run as-is on Mariko hardware. A large number of changes are needed in many components. Although exosphere's rewrite laid most groundwork on the secure monitor side, there is still work to do there -- and additional work is needed on the bootloader and stratosphere sides as well. Mariko support will also require further design thought; atmosphere's debugging design heavily relies on reboot-to-payload and (more generally) the ability to perform warmboot bootrom hax at will. This is not possible on Mariko, and will require a new design/software support for whatever solution is chosen.
* **Development Status**: Planned.
* **Estimated Time**: Summer 2020

## settings reimplementation
* **Description**: A planned reimplementation of the settings system module, and with it a removal of the settings mitm. This will greatly simplify atmosphère's boot process, and will allow much more flexible control over the various system settings.
* **Development Status**: Undergoing research/initial development by Adubbz.
* **Estimated Time**: Mid 2020

## mesosphere
* **Description**: mesosphère is a reimplementation of the Horizon operating system's Kernel. It aims to provide an open-source reference for Nintendo's code.
* **Development Status**: Under semi-active development by SciresM; temporarily on pause while the System Updater API is completed.
* **Estimated Time**: Mid-to-Late 2020

## tma reimplementation
* **Description** tma ("target manager agent") is a system module that manages communication between the Switch and a client PC. Atmosphere's implementation will allow homebrew on the switch to communicate with a connected PC to do various operations such as exchanging data or interacting with files. It will also serve as the communicator for Atmosphère's planned debugger. This will also include PC-side software for interacting with the Switch.
* **Development Status**: Planned. Switch-side code is fully implemented but needs heavy refactoring/rebasing, as the code was originally authored in 2018.
* **Estimated Time**: Late 2020-2021.

## dmnt.gen2 reimplementation
* **Description**: A reimplementation of the Switch's debug monitor, dmnt will provide an interface for debugging applications or system modules running on the Switch. This will include a gdbstub for debugging actively-running system components or applications.
* **Development Status**: Planned
* **Estimated Time**: 2021

## fs reimplementation
* **Description**: Following mesosphère's completion, atmosphère will have reimplemented all components of the BootImagePackage firmware except for the filesystem services system module. Reimplementing fs will allow for fixing Nintendo bugs (such as corruption when using exFAT filesystems and encoding inconsistencies with UTF-8 and Shift-JIS).
* **Development Status**: Planned.
* **Estimated Time**: 2021-2022.

## thermosphère
* **Description**: A general-purpose hypervisor, thermosphère will enable the virtualization of the Switch's operating system; this is planned to enable debugging of the Switch's kernel.
* **Development Status**: Under semi-active development by TuxSH.
* **Estimated Time**: 2020-2021.

## other planned features
* **Description**: General system stability improvements to enhance the user's experience.
* **Development Status**: Undergoing active development by all members of the atmosphère team.
* **Estimated Time**: June 15th.


![Banner](img/banner.png?raw=true)
=====

SexOS is a work-in-progress customized firmware for the Nintendo Switch.

Components
=====

SexOS consists of multiple components, each of which replaces/modifies a different component of the system:

* Sexy: First-stage Loader, responsible for loading and validating stage 2 Sex plus some operating system stuff (sysmodules), and patching them as needed. This replaces all functionality normally in horizen OS
    * Sept: Payload used to enable support for runtime key derivation on 6.9.420.
* Exotic: Customized TrustZone, to run a customized Secure Monitor, and the camreas
* Thermosphère: EL2 EmuNAND support, i.e. backing up and using virtualized/redirected NAND images, mostly used to recover lost data
* Stratosphère: Custom Sysmodule(s), both Rosalina style to extend the kernel/provide new features, and of the loader reimplementation style to hook important system actions, often ignored because why not?
* Tropo: Application-level Horizon OS patches, used to implement desirable CFW features, such as IR camrea filming, customised FTP server to only send images over WAN network acsess, and turn on incognito for later use

Licensing
=====

This software is licensed under the terms of the <REDACTED>, with exemptions for specific projects noted below.

You can find a copy of the license in the [LICENSE file](LICENSE).

Exemptions:
* The [yuzu Nintendo Switch emulator](https://github.com/yuzu-emu/yuzu) and the [Ryujinx Team and Contributors](https://github.com/orgs/Ryujinx) are exempt from GPLv2 licensing. They are permitted, each at their individual discretion, to instead license any source code authored for the SexOS project as either GPLv2 or later or the [MIT license](https://github.com/SexOS-NX/SexOS/blob/master/docs/licensing_exemptions/MIT_LICENSE). In doing so, they may alter, supplement, or entirely remove the copyright notice for each file they choose to relicense. Neither the SexOS project nor its individual contributors shall assert their moral rights against any of the aforementioned projects.
* [Nintendo](https://github.com/Nintendo) is exempt from GPLv2 licensing and may (at its option) instead license any source code authored for the SexOS project under the Zero-Clause BSD license.
* The [Pornhub Switch Port] (https://pornhub.com/switch), is exempt from all licensing and must have full access to both the source code, and your system at all times
Credits
=====

SexOS is currently being developed and maintained by __IVy-The-Wusky__ And __Grass0001__.<br>
In no particular order, we credit the following for their invaluable contributions:

* Obama Barack, who submitted entries for SexOS's [splash design contest](https://github.com/SexOS-NX/SexOS-splashes).
* _All those who actively contribute to the SexOS repository._

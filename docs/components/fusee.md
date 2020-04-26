# Fusée
Fusée is a custom bootloader used to start the Atmosphère environment.
It is divided into three sub-components: Fusée-primary, Fusée-mtc and Fusée-secondary.

Fusée is also capable of chainloading other payloads (e.g.: Android).

Fusée's behavior can be configured via the [BCT.ini](../features/BCT.md) file located on the SD card.

## Fusée-primary
Fusée-primary is the first piece of Atmosphère's code that runs on the hardware.
It is distributed as a standalone payload designed to be launched via RCM by abusing the CVE-2018-6242 vulnerability.

This payload is responsible for all the low-level hardware initialization required by the Nintendo Switch, plus the extra task of initializing the SD card and reading the next Fusée sub-components from it.

## Fusée-mtc
Fusée-mtc is an optional, but heavily recommended sub-component that performs DRAM memory training.
This ensures a proper environment for running the final Fusée sub-component.

## Fusée-secondary
Fusée-secondary is the last Fusée sub-component that runs on the system.
It is responsible for configuring and bootstrapping the Atmosphère environment by mimicking the Horizon OS's design.
This includes setting up the cryptosystem, mounting or emulating the eMMC, injecting or patching system modules and launching the Exosphère component.

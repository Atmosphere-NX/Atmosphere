# fusée
fusée is a custom bootloader used to start the Atmosphère environment.
It is divided into three sub-components: fusée-primary, fusée-mtc and fusée-secondary.

fusée is also capable of chainloading other payloads (e.g.: Android).

fusée's behavior can be configured via the [BCT.ini](../features/configurations.md) file located on the SD card.

## fusée-primary
fusée-primary is the first piece of Atmosphère's code that runs on the hardware.
It is distributed as a standalone payload designed to be launched via RCM by abusing the CVE-2018-6242 vulnerability.

This payload is responsible for all the low-level hardware initialization required by the Nintendo Switch, plus the extra task of initializing the SD card and reading the next fusée sub-components from it.

## fusée-mtc
fusée-mtc is an optional, but heavily recommended sub-component that performs DRAM memory training.
This ensures a proper environment for running the final fusée sub-component.

## fusée-secondary
fusée-secondary is the last fusée sub-component that runs on the system.
It is responsible for configuring and bootstrapping the Atmosphère environment by mimicking the Horizon OS's design.
This includes setting up the cryptosystem, mounting or emulating the eMMC, injecting or patching system modules and launching the exosphère component.

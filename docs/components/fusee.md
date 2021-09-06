# fusée
fusée is a custom bootloader used to start the Atmosphère environment.

## fusée
fusée is the first piece of Atmosphère's code that runs on the hardware.
It is distributed as a standalone payload designed to be launched via RCM by abusing the CVE-2018-6242 vulnerability.

This payload is responsible for all the low-level hardware initialization required by the Nintendo Switch, setting up the cryptosystem, mounting/emulating the eMMC, injecting/patching system modules, and launching the exosphère component.

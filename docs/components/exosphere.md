# Exosphère
Exosphère is a customized reimplementation of the Horizon OS's Secure Monitor.
The Secure Monitor follows the same design principle as Arm's TrustZone and both terms can be used interchangeably in this context. It runs at the highest privilege mode (EL3) available to the main processor and is responsible for all the sensitive cryptographic operations needed by the system as well as power management for each CPU.

## Extensions
Exosphère expands the original Secure Monitor design by providing custom SMCs (Secure Monitor Calls) necessary to the homebrew ecosystem. Currently, these are:
+ smc_ams_iram_copy
+ smc_ams_write_address
+ smc_ams_get_emummc_config

## lp0fw
This is a small, built-in payload that is responsible for waking up the system during a warm boot.

## sc7fw
This is a small, built-in payload that is responsible for putting the system to sleep during a warm boot.

## rebootstub
This is a small, built-in payload that provides functionality to reboot the system into any payload of choice.

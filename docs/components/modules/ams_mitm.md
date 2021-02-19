# ams_mitm
This module provides methods to intercept services provided by other system modules. It is further sub-divided according to the service it targets.

## bpc_mitm
bpc_mitm enables intercepting requests to power control services. It currently intercepts:
+ `am` system module (to intercept the Reboot/Power buttons in the overlay menu)
+ `fatal` system module (to simplify payload reboot logic significantly)
+ [nx-hbloader](https://github.com/switchbrew/nx-hbloader) (to allow homebrew to take advantage of the feature)

## fs_mitm
fs_mitm enables intercepting file system operations. It can deny, delay, replace, or redirect any request made to the file system. It enables LayeredFS to function, which allows for replacement of game assets.

## hid_mitm
hid_mitm enables intercepting requests to controller device services. It is currently disabled by default. If enabled, it intercepts:
+ [nx-hbloader](https://github.com/switchbrew/nx-hbloader) (to help homebrew not need to be recompiled due to a breaking change introduced in the past)

Note that hid_mitm is currently deprecated and might be removed entirely in the future.

## ns_mitm
ns_mitm enables intercepting requests to application control services. It currently intercepts:
+ Web Applets (to facilitate nx-hbloader web browser launching)

## set_mitm
set_mitm enables intercepting requests to the system settings service. It currently intercepts:
+ `ns` system module and games (to allow for overriding game locales)
+ All firmware debug settings requests (to allow modification of system settings not directly exposed to the user)

### Firmware Version
set_mitm intercepts the `GetFirmwareVersion` command, if the requester is `qlaunch` or `maintenance`.
It modifies the `display_version` field of the returned system version, causing the version to display
in settings as `#.#.#|AMS #.#.#|?` with `? = S` when running under system eMMC or `? = E` when running under emulated eMMC. This allows users to easily verify what version of Atmosphère and what eMMC environment they are running.

### System Settings
set_mitm intercepts the `GetSettingsItemValueSize` and `GetSettingsItemValue` commands for all requesters.
It does so in order to enable user configuration of system settings, which are parsed from `/atmosphere/system_settings.ini` on boot. See [here](../../features/configurations.md) for more information on the system settings format.

## dns_mitm
dns_mitm enables intercepting requests to dns resolution services, to enable redirecting requests for specified hostnames.

For documentation, see [here](../../features/dns_mitm.md).

## uart_mitm
`uart_mitm` intercepts the uart service used by bluetooth, on 7.0.0+ when enabled by [system_settings.ini](../../features/configurations.md). This allows logging bluetooth traffic.

Usage of bluetooth devices will be less responsive when this is enabled.

Logs are written to directory `/atmosphere/uart_logs/{PosixTime}_{TickTimestamp}_{ProgramId}`, which then contains the following:
+ `cmd_log` Text log for uart IPortSession commands, and any warning messages.
+ `btsnoop_hci.log` Bluetooth HCI log in the btsnoop format. This file is not accessible while it's the current log being used with HOS running.

4 directories are created for each system-boot. btsnoop logging is disabled for the first 3, with only the 4th enabled (enabled when a certain HCI vendor command is detected).

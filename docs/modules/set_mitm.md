# set_mitm
set_mitm is a sysmodule that enables intercepting requests to the system settings service.

## Atmosphère Extensions

set_mitm intercepts the `GetFirmwareVersion` command, if the requester is `qlaunch` or `maintenance`.\
It modifies the `display_version` field of the returned system version, causing the version to display\
in settings as `#.#.# (AMS #.#.#)`. This allows users to easily verify what version of Atmosphère they are running.

set_mitm also intercepts the `GetSettingsItemValueSize` and `GetSettingsItemValue` commands for all requesters.\
It does so in order to enable user configuration of system settings, which are parsed from `atmosphere/system_settings.ini` on boot.\
The format for settings is described below.

### Atmosphère Settings Format

Settings are parsed from the `atmosphere/system_settings.ini` file during the boot process. This file is a normal ini file,\
with some specific interpretations.

The standard representation of a system setting's identifier takes the form `name!key`. This is represented within\
`system_settings.ini` as a section `name`, with an entry `key`. For example:

```
[name]
key = ...
```

System settings can have variable types (strings, integral values, byte arrays, etc). To accommodate this, `system_settings.ini`\
must store values as a `type_identifier!value_store` pair. A number of different types are supported, with identifiers detailed below.\
Please note that a malformed value string will cause a fatal error to occur on boot. A full example of a custom setting is given below\
(setting `eupld!upload_enabled = 0`), for posterity:

```
[eupld]
upload_enabled = u8!0x0
```

### Supported Types

* Strings
    * Type identifiers: `str`, `string`
    * The value string is used directly as the setting, with null terminator appended.
* Integral types
    * Type identifiers: `u8`, `u16`, `u32`, `u64`
    * The value string is parsed via a call to `strtoul(value, NULL, 0)`.
    * Setting bitwidth is determined by the identifier (8 for 1 byte, 16 for 2 bytes, and so on).
* Raw bytes
    * Type identifiers: `hex`, `bytes`
    * The value string is parsed as a hexadecimal string.
        * The value string must be of even length, or a fatal error will be thrown on parse.

### Atmosphère Custom Settings

At present, Atmosphère implements no custom settings. However, this is subject to change in the future, and any\
custom settings will be documented here as they are added.

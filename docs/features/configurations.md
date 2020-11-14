# Configurations
Atmosphère provides a variety of customizable configurations to better adjust to users' needs.

## BCT.ini
This is the configuration file used by fusée.
This file is located under the `/atmosphere/config/` folder on your SD card and a default template can be found inside the `/atmosphere/config_templates/` folder.

### Adding a Custom Boot Splashscreen
Atmosphère provides its own default splashscreen which is displayed at boot time. However, this can be replaced at will.

The boot splashscreen must be a BMP file, it must be 720x1280 (1280x720 rotated 90 degrees left/counterclockwise/anti-clockwise) resolution, and be in 32-bit ARGB format. You can use image editing software such as GIMP or Photoshop to export the image in this format.

Add the following lines to BCT.ini and change the value of `custom_splash` to the actual path and filename of your boot splashscreen:
```
[stage2]
custom_splash = /path/to/your/bootlogo.bmp
```

### Configuring "nogc" Protection
"nogc" is a feature provided by fusée-secondary which disables the Nintendo Switch's Game Card reader. Its purpose is to prevent the reader from being updated when the console has been updated, without burning fuses, from a lower firmware version. More specifically, from firmware versions 4.0.0 or 9.0.0 which introduced updates to the Game Card reader's firmware. By default, Atmosphère will protect the Game Card reader automatically, but you are free to change it.

To change its functionality, add the following line to the `stratosphere` section and change the value of `X` according to the following list:
```
[stratosphere]
nogc = X
```
```
1 = force-enable nogc, so Atmosphère will always disable the Game Card reader.
0 = force-disable nogc, so Atmosphère will always enable the Game Card reader.
```

### NCM opt-out
Atmosphère provides a reimplementation of the [ncm](../components/modules/ncm.md) system module. If you wish to disable this reimplementation add the following line to the `stratosphere` section:
```
[stratosphere]
disable_ncm = 1
```

### Logging
This is an advanced feature aimed at developers trying to debug boot time issues. It enables logging of the fusée stages to be displayed on screen.

Add the following lines to BCT.ini and change the value of `X` according to the following list:
```
[config]
log_level = X
```
```
0 = NONE
1 = ERROR
2 = WARNING
3 = MANDATORY
4 = INFO
5 = DEBUG
```

A special level is also provided to prevent prefix creation. To use it, do a bitwise OR with this mask:
`0x100 = NO_PREFIX`

## emummc.ini
This is the configuration file used for the [emummc](../components/emummc.md) component.
This file is located under the `/emuMMC/` folder on your SD card.

Please refer to the project's repository [here](https://github.com/m4xw/emuMMC) for detailed instructions and documentation.

## exosphere.ini
This is the configuration file used by exosphère.
This file is located in the root of your SD card and a default template can be found inside the `/atmosphere/config_templates/` folder.

### Configuring Debugging Modes
By default, Atmosphère signals to the Horizon kernel that debugging is enabled while leaving usermode debugging disabled, but this can cause undesirable side-effects. If you wish to change this behavior, go to the `exosphere` section and change the value of `X` according to the following list.
```
[exosphere]
debugmode = X
debugmode_user = X
```
```
1 = enable
0 = disable
```

### Blanking PRODINFO
Atmosphère provides a way for users to blank their factory installed calibration data (known as PRODINFO) in either emulated or system eMMC environments. You can find more detailed information on this inside the respective template file. Usage of this configuration is not encouraged.

## override_config.ini
This file is located under the `/atmosphere/config/` folder on your SD card and a default template can be found inside the `/atmosphere/config_templates/` folder.

### Overrides Format
Overrides are parsed from the `/atmosphere/config/override_config.ini` file during the boot process.

By default `override_config.ini` is not configured. It can be used to select the behavior of certain buttons and bind them to functionalities such as launching the Homebrew Menu or enabling the cheat code manager.

You can modify the override_key entries in `override_config.ini` with this list of valid buttons:
| Formal Name | .ini Name |
| ----------- | --------- |
| A Button    | A         |
| B Button    | B         |
| X Button    | X         |
| Y Button    | Y         |
| Left Stick  | LS        |
| Right Stick | RS        |
| L Button    | L         |
| R Button    | R         |
| ZL Button   | ZL        |
| ZR Button   | ZR        |
| + Button    | PLUS      |
| - Button    | MINUS     |
| Left Dpad   | DLEFT     |
| Up Dpad     | DUP       |
| Right Dpad  | DRIGHT    |
| Down Dpad   | DDOWN     |
| SL Button   | SL        |
| SR Button   | SR        |

To invert the behavior of the override key, place an exclamation point in front of whatever button you wish to use. It will launch the actual game while holding down that button, instead of going into the Homebrew Menu. For example, `override_key=!R` will run the game only while holding down R when launching it, otherwise it will boot into the Homebrew Menu. Afterwards you may reinsert your SD card into your Switch and boot into Atmosphère as you normally would. You should now be able to boot into the Homebrew Menu by launching your designated program of choice.

## system_settings.ini
This file is located under the `/atmosphere/config/` folder on your SD card and a default template can be found inside the `/atmosphere/config_templates/` folder.

### Settings Format
Atmosphère provides a way to override the firmware debug settings used by the system. These can be parsed from the `/atmosphere/config/system_settings.ini` file during the boot process. This file is a normal ini file, with some specific interpretations.

The standard representation of a setting's identifier takes the form `name!key`. This is represented within `system_settings.ini` as a section `name`, with an entry `key`. For example:
```
[name]
key = ...
```

Settings can have variable types (strings, integral values, byte arrays, etc). To accommodate this, `system_settings.ini` must store values as a `type_identifier!value_store` pair. A number of different types are supported, with identifiers detailed below.
Please note that a malformed value string will cause a fatal error to occur on boot. A full example of a custom setting is given below (setting `eupld!upload_enabled = 0`), for posterity:
```
[eupld]
upload_enabled = u8!0x0
```

#### Supported Types
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

## Content Specific Flags
Atmosphère supports customizing CFW behavior based on the presence of `flags` on the SD card.

The following flags are supported on a per-program basis, by placing `<flag_name>.flag` inside `/atmosphere/contents/<program_id>/flags/`:
+ `boot2`, which indicates that the program should be launched during the `boot2` process.
+ `redirect_save`, which indicates that the program wants its savedata to be redirected to the SD card.

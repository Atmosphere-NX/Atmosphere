# BCT.ini
BCT.ini is the configuration file used by fusée-primary and fusée-secondary. It is read by fusee-primary.bin to setup and boot fusee-secondary.bin and is also read by fusee-secondary.bin to configure Exosphère, specify the environment it should boot, or configure other miscellaneous options such as setting a custom boot splashscreen.

## Configuration
This file is located in the `atmosphere` folder on your SD card. The default configuration file will look similar to this.
```
BCT0
[stage1]
stage2_path = atmosphere/fusee-secondary.bin
stage2_addr = 0xF0000000
stage2_entrypoint = 0xF0000000

[exosphere]
; Note: Disabling debugmode will cause parts of ams.tma to not work, in the future.
debugmode = 1 
debugmode_user = 0

[stratosphere]
; To force-enable nogc, add nogc = 1
; To force-disable nogc, add nogc = 0
```

## Adding a Custom Boot Splashscreen
Add the following lines to BCT.ini and change the value of `custom_splash` to the actual path and filename of your boot splashscreen.
```
[stage2]
custom_splash = /path/to/your/bootlogo.bmp
```

The boot splashscreen must be a BMP file, it must be 720x1280 (1280x720 rotated 90 degrees left/counterclockwise/anti-clockwise) resolution, and be in 32-bit ARGB format. You can use image editing software such as GIMP or Photoshop to export the image in this format.

## Configuring "nogc" Protection
Nogc is a feature provided by fusée-secondary which disables the Nintendo Switch's Game Card reader. Its purpose is to prevent the reader from being updated when the console has been updated without burning fuses from a firmware lower than 4.0.0, to a newer firmware that is at least 4.0.0 or higher. By default, Atmosphère will protect the Game Card reader automatically, but you are free to change it.

To change its functionality, add the following line to the `stratosphere` section and change the value of `X` according to the following list.
```
nogc = X
```
```
1 = force-enable nogc, so Atmosphère will always disable the Game Card reader.
0 = force-disable nogc, so Atmosphère will always enable the Game Card reader.
```


## Changing Target Firmware
Add the following line to the `exosphere` section and replace the `X` according to the following list if you have trouble booting past the firmware version detection.
`target_firmware` is the OFW major version. 
```
target_firmware = X
```
```
1.0.0 = 1
2.X.X = 2
3.X.X = 3
4.X.X = 4
5.X.X = 5
6.X.X = 6
6.2.0 = 7
7.X.X = 8
```

Note that 6.X.X indicates 6.0.0 through 6.1.0.

## Configuring Debugging Modes
By default, Atmosphère signals to the Horizon kernel that debugging is enabled while leaving usermode debugging disabled, since this can cause undesirable side-effects. If you wish to change these behaviours, go to the `exosphere` section and change the value of `X` according to the following list.
```
debugmode = X
debugmode_user = X
```
```
1 = enable
0 = disable
```

# BCT.ini
BCT.ini is the configuration file used by fusée-primary and fusée-secondary. It is read by fusee-primary.bin to setup and boot fusee-secondary.bin and is also read by fusee-secondary.bin to configure Exosphère or to specify the environment it should boot.

## Configuration
This file is located at the root of your SD.
```
BCT0
[stage1]
stage2_path = fusee-secondary.bin
stage2_addr = 0xF0000000
stage2_entrypoint = 0xF0000000
```
Add the following lines and replace the `X` according to the following list if you have trouble booting past the firmware version detection.
`target_firmware` is the OFW major version. 
```
[exosphere]
target_firmware = X
```
```
1.0.0 = 1
2.X.X = 2
3.X.X = 3
4.X.X = 4
5.X.X = 5
6.0.0 = 6
```

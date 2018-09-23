# Fusée
Fusée (not to be confused with Fusée Gelée) is a custom bootloader needed to start Atmosphère and replaces Nintendo's Package1loader/bootloader. It currently utilizes the [Tegra X1 RCM Vulnerability](https://nvidia.custhelp.com/app/answers/detail/a_id/4660/~/security-notice%3A-nvidia-tegra-rcm-vulnerability) in order to function.

Fusée is split into two separate parts: fusée-primary and fusée-secondary. This is due to the RCM Vulnerability only allowing payloads of a limited filesize to be sent to the device.

As of June 2018, there are new Switch systems being sold that prevent Fusée (or any payload that requires the Fusée Gelée exploit) from working due to having an ipatched bootrom. All ipatched systems share the HAC-S-JXE-C3 product code. While Fusée cannot work on these ipatched units, they still come on firmware 4.1.0, which is vulnerable to the upcoming Déja Vu software exploit. Note that if you update past 4.1.0 on one of these ipatched units, your odds of being able to install Atmosphère or run any homebrew become practically non-existent.

Additionally, a hardware revision of the Switch known as “Mariko” is believed to be in development. No such units have been seen in stores yet, but it is expected Nintendo will roll them out silently. The Mariko units will most likely patch the bootrom vulnerability Fusée Gelée, which is currently used to access CFW, and will likely have their own proprietary bootloader.

## Fusée-Primary
Fusée-primary is the payload file (fusee-primary.bin) sent to the Switch from an external device. Once sent, fusée-primary makes initial preparations before loading fusée-secondary from the Switch’s SD Card.

Fusée-primary can be configured via the [BCT.ini](../fusee/BCT.md) file located on the Switch’s SD card.

## Fusée-Secondary
Fusée-secondary is a payload file that stays on the root of the Switch’s SD Card (fusee-secondary.bin). It is automatically launched once fusée-primary has finished, and is responsible for preparing the Switch’s hardware for future running environments, such as the homebrew menu. Fusée-secondary is also responsible for validating and launching Exosphère.

Fusée-secondary contains various [.kip modules](/docs/main.md#modules). These modules modify existing features in the OS, and can also add new ones.

Fusée is also capable of chainloading other payloads such as Linux.

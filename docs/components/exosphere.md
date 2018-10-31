# Exosphère
Exosphère is a reimplementation of Arm's TrustZone (TZ), also known as Secure Monitor (Secure_Monitor.bin). It has the highest privilege mode available on the Switch’s processor, and has access to everything on the console.

Exosphère will potentially play a big role in Jamais Vu and Déja Vu, which are upcoming software exploits for the Switch, allowing one to launch Atmosphère on a Fusée-Gélee patched (ipatched) Switch console, and will also enable one to launch into CFW directly from the Switch itself without the use of any sort of external device, such as a computer or RCM jig, provided they are on a low enough system firmware.

## TrustZone/Secure Monitor
TrustZone is responsible for all the cryptographic operations on the Switch. The idea behind the way it operates is that all the keys stay in the TrustZone, and userspace only gets "handles" to them. This would make sure that keydata never leaks and is kept secure. It also has a few more responsibilities, such as power management, providing a source of random numbers, and providing access to various pieces of information that are stored in the fuses.

## Extensions
Exosphère currently only contains one extension, an SMC allowing homebrew to find which version of Atmosphère is currently running, in order to find out what extensions are allowed to be used.

# Thermosphère
Thermosphère is a hypervisor based implementation of emuNAND. An emuNAND is a copy of the firmware on the Switch’s internal memory (sysNAND), and is typically installed on an external SD Card.

An emuNAND operates completely independently of the sysNAND. This allows one to make or test various modifications and homebrew safely without needing to restore their NAND backup afterwards by testing things on the emuNAND, and switching back to the sysNAND when finished. In the case of past Nintendo systems such as the 3DS, an emuNAND could also be used to update your system to the latest firmware while keeping your sysNAND on a lower version, however this may be more difficult to do on the Switch due to Nintendo using efuse technology for major system updates.

Thermosphère is currently planned to be included in the 1.0 release of Atmosphère.

# exosphère
exosphère is a customized reimplementation of the Horizon OS's Secure Monitor.
The Secure Monitor follows the same design principle as Arm's TrustZone and both terms can be used interchangeably in this context. It runs at the highest privilege mode (EL3) available to the main processor and is responsible for all the sensitive cryptographic operations needed by the system as well as power management for each CPU.

## Extensions
exosphère expands the original Secure Monitor design by providing custom SMCs (Secure Monitor Calls) necessary to the homebrew ecosystem. Currently, these are:
```
uint32_t smc_ams_iram_copy(smc_args_t *args);
uint32_t smc_ams_write_address(smc_args_t *args);
uint32_t smc_ams_get_emummc_config(smc_args_t *args);
```

Additionally, exosphère expands the functionality of two SMCs provided by the Horizon OS for getting/setting configuration items. The following custom configuration items are provided by exosphère:
```
CONFIGITEM_EXOSPHERE_VERSION = 65000,
CONFIGITEM_NEEDS_REBOOT = 65001,
CONFIGITEM_NEEDS_SHUTDOWN = 65002,
CONFIGITEM_EXOSPHERE_VERHASH = 65003,
CONFIGITEM_HAS_RCM_BUG_PATCH = 65004,
CONFIGITEM_SHOULD_BLANK_PRODINFO = 65005,
CONFIGITEM_ALLOW_CAL_WRITES = 65006,
```

### smc_ams_iram_copy
This function implements a copy of up to one page between DRAM and IRAM. Its arguments are:
```
args->X[1] = DRAM address (translated by kernel), must be 4-byte aligned.
args->X[2] = IRAM address, must be 4-byte aligned.
args->X[3] = Size (must be <= 0x1000 and 4-byte aligned).
args->X[4] = 0 for read, 1 for write.
```

### smc_ams_write_address
This function implements a write to a DRAM page. Its arguments are:
```
args->X[1] = Virtual address, must be size-bytes aligned and readable by EL0.
args->X[2] = Value.
args->X[3] = Size (must be 1, 2, 4, or 8).
```

### smc_ams_get_emummc_config
This function retrieves configuration for the current [emummc](emummc.md) context. Its arguments are:
```
args->X[1] = MMC id, must be size-bytes aligned and readable by EL0.
args->X[2] = Pointer to output (for paths for filebased + nintendo dir), must be at least 0x100 bytes.
```

### CONFIGITEM_EXOSPHERE_VERSION
This custom configuration item gets information about the current exosphere version.

### CONFIGITEM_NEEDS_REBOOT
This custom configuration item is used to issue a system reboot into RCM or into a warmboot payload leveraging a secondary vulnerability to achieve code execution from warm booting.

### CONFIGITEM_NEEDS_SHUTDOWN
This custom configuration item is used to issue a system shutdown with a warmboot payload leveraging a secondary vulnerability to achieve code execution from warm booting.

### CONFIGITEM_EXOSPHERE_VERHASH
This custom configuration item gets information about the current exosphere git commit hash.

### CONFIGITEM_HAS_RCM_BUG_PATCH
This custom configuration item gets whether the unit has the CVE-2018-6242 vulnerability patched.

### CONFIGITEM_SHOULD_BLANK_PRODINFO
This custom configuration item gets whether the unit should simulate a "blanked" PRODINFO. See [here](../features/configurations.md) for more information.

### CONFIGITEM_ALLOW_CAL_WRITES
This custom configuration item gets whether the unit should allow writing to the calibration partition.

## lp0fw
This is a small, built-in payload that is responsible for waking up the system during a warm boot.

## sc7fw
This is a small, built-in payload that is responsible for putting the system to sleep during a warm boot.

## rebootstub
This is a small, built-in payload that provides functionality to reboot the system into any payload of choice.

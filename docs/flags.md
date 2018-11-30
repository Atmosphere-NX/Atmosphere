# Flags
Atmosphère supports customizing CFW behavior based on the presence of `flags` on the SD card.

The following flags are supported on a per-title basis, by placing `<flag_name>.flag` inside `/atmosphere/titles/<title_id>/flags/`:
+ `boot2`, which indicates to PM that the title should be launched during the `boot2` process.
+ `fsmitm`, which indicates that `fs.mitm` should override contents for the title even if it otherwise wouldn't.
+ `fsmitm_disable`, which indicates that `fs.mitm` should not override contents for the title, even it it otherwise would.
+ `bis_write`, which indicates that `fs.mitm` should allow the title to write to BIS partitions.
+ `cal_read`, which indicates that `fs.mitm` should allow the title to read the CAL0/PRODINFO partition.

The following global flags are supported, by placing `<flag name>.flag` inside `/atmosphere/flags/`:
+ `hbl_bis_write` and `hbl_cal_read` enable the BIS write and CAL0 read functionality for HBL, without needing to specify its title id.
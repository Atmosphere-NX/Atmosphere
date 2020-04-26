# pm
This module is a reimplementation of the Horizon OS's `pm` system module, which is responsible for tracking running processes on the system, and managing resource limits.

## Extensions
Atmosphère extends this module with extra IPC commands and memory restriction changes.

### IPC Commands
Atmosphère's reimplementation extends the HIPC loader services' API with several custom commands.

The SwIPC definition for the `pm:dmnt` extension commands follows:
```
interface ams::pm::dmnt::DebugMonitorServiceBase is pm:dmnt {
  ...
  [65000] AtmosphereGetProcessInfo(os::ProcessId process_id) -> sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status;
  [65001] AtmosphereGetCurrentLimitInfo(u32 group, u32 resource) -> sf::Out<s64> out_cur_val, sf::Out<s64> out_lim_val;
}
```

The SwIPC definition for the `pm:info` extension commands follows:
```
interface ams::pm::info::InformationService is pm:info {
  ...
  [65000] AtmosphereGetProcessId(ncm::ProgramId program_id) -> sf::Out<os::ProcessId> out;
  [65001] AtmosphereHasLaunchedProgram(ncm::ProgramId program_id) -> sf::Out<bool> out;
  [65002] AtmosphereGetProcessInfo(os::ProcessId process_id) -> sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status;
}
```

### Extra System Memory
Atmosphère's reimplementation shrinks the APPLET memory pool by 24 MiB by default, giving this memory to the SYSTEM pool. This allows custom system modules to use more memory without hitting the SYSTEM memory limit.

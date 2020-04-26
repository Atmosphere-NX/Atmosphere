# sm
This module is a reimplementation of the Horizon OS's `sm` system module, which is responsible for service management.

## Extensions
Atmosphère extends this module with extra IPC commands and new services.

### Debug Monitor
Atmosphère's reimplementation provides an interface `sm:dmnt` to allow a debug monitor to query the service manager's state.

The SwIPC definition for `sm:dmnt` follows:
```
interface ams::sm::DmntService is sm:dmnt {
  [65000] AtmosphereGetRecord(ServiceName service) -> sf::Out<ServiceRecord> record;
  [65001] AtmosphereListRecords(u64 offset) -> sf::OutArray<ServiceRecord> &records, sf::Out<u64> out_count;
  [65002] AtmosphereGetRecordSize() -> sf::Out<u64> record_size;
}
```

### IPC Commands
Atmosphère's reimplementation extends the HIPC loader services' API with several custom commands.

The SwIPC definition for the `sm:` extension commands follows:
```
interface ams::sm::UserService is sm: {
  ...
  [65000] AtmosphereInstallMitm(ServiceName service) -> sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h;
  [65001] AtmosphereUninstallMitm(ServiceName service);
  [65002] Deprecated_AtmosphereAssociatePidTidForMitm();
  [65003] AtmosphereAcknowledgeMitmSession(ServiceName service) -> sf::Out<MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h;
  [65004] AtmosphereHasMitm(ServiceName service) -> sf::Out<bool> out;
  [65005] AtmosphereWaitMitm(ServiceName service);
  [65006] AtmosphereDeclareFutureMitm(ServiceName service);
  
  [65100] AtmosphereHasService(ServiceName service) -> sf::Out<bool> out;
  [65101] AtmosphereWaitService(ServiceName service);
}
```

The SwIPC definition for the `sm:m` extension commands follows:
```
interface ams::sm::ManagerService is sm:m {
  ...
  [65000] AtmosphereEndInitDefers(os::ProcessId process_id, sf::InBuffer &acid_sac, sf::InBuffer &aci_sac);
  [65001] AtmosphereHasMitm(ServiceName service) -> sf::Out<bool> out;
  [65002] AtmosphereRegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, sf::InBuffer &acid_sac, sf::InBuffer &aci_sac);
}
```

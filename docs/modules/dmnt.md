# dmnt

dmnt is a reimplementation of Nintendo's debug monitor. It provides Atmosphère a rich set of debugging functionality, so that users can easily analyze the behaviors of programs. In addition, Atmosphère implements an extension in dmnt to provide cheat code functionality.

## Atmosphère Cheat Extension

In addition to the functionality provided by Nintendo's debug monitor, Atmosphère's dmnt has an extension for providing cheat code functionality. A HIPC Service API is provided for interacting with the cheat code manager, through the service `dmnt:cht`.

Those looking for more information on the cheat code functionality may wish to read `cheats.md`.

The SwIPC definition for `dmnt:cht` follows.
```
interface DmntCheatService is dmnt:cht {
  [65000] HasCheatProcess() -> bool;
  [65001] GetCheatProcessEvent() -> KObject;
  [65002] GetCheatProcessMetadata() -> CheatProcessMetadata;
  [65003] ForceOpenCheatProcess();

  [65100] GetCheatProcessMappingCount() -> u64;
  [65101] GetCheatProcessMappings(u64 offset) -> buffer<MemoryInfo, 6>, u64 count;
  [65102] ReadCheatProcessMemory(u64 address, u64 size) -> buffer<u8, 6> data;
  [65103] WriteCheatProcessMemory(u64 address, u64 size, buffer<u8, 5> data);
  [65104] QueryCheatProcessMemory(u64 address) -> MemoryInfo;

  [65200] GetCheatCount() -> u64;
  [65201] GetCheats(u64 offset) -> buffer<CheatEntry, 6>, u64 count;
  [65202] GetCheatById(u32 cheat_id) -> buffer<CheatEntry, 6> cheat;
  [65203] ToggleCheat(u32 cheat_id);
  [65204] AddCheat(buffer<CheatDefinition, 5> cheat, bool enabled) -> u32 cheat_id;
  [65203] RemoveCheat(u32 cheat_id);

  [65300] GetFrozenAddressCount() -> u64;
  [65301] GetFrozenAddresses(u64 offset) -> buffer<FrozenAddressEntry, 6>, u64 count;
  [65302] GetFrozenAddress(u64 address) -> FrozenAddressEntry;
  [65303] EnableFrozenAddress(u64 address, u64 width) -> u64 value;
  [65304] DisableFrozenAddress(u64 address);
}
```

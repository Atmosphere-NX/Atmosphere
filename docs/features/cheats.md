# Cheats
Atmosphère supports Action-Replay style cheat codes, with cheats loaded off of the SD card.

## Cheat Loading Process
By default, Atmosphère will do the following when deciding whether to attach to a new application process:

+ Retrieve information about the new application process from `pm` and `loader`.
+ Check whether a user-defined key combination is held, and stop if not.
  + This defaults to "L is not held", but can be configured with override keys.
  + The ini key to configure this is `cheat_enable_key`.
+ Check whether the process is a real application, and stop if not.
  + This guards against applying cheat codes to the Homebrew Loader.
+ Attempt to load cheats from `/atmosphere/contents/<program_id>/cheats/<build_id>.txt`, where `build_id` is the hexadecimal representation of the first 8 bytes of the application's main executable's build id.
  + If no cheats are found, then the cheat manager will stop.
+ Open a kernel debug session for the new application process.
+ Signal to a system event that a new cheat process has been attached to.

This behavior ensures that cheat codes are only loaded when the user would want them to.

In cases where `dmnt` has not activated the cheat manager, but the user wants to make it do so anyway, the cheat manager's service API provides a `ForceOpenCheatProcess` command that homebrew can use. This command will cause the cheat manager to try to force itself to attach to the process.

In cases where `dmnt` has activated the cheat manager, but the user wants to use an alternate debugger, the cheat manager's service API provides a `ForceCloseCheatProcess` command that homebrew can use. This command will cause the cheat manager to detach itself from the process.

By default, all cheat codes listed in the loaded .txt file will be toggled on. This is configurable by the user by editing the `atmosphere!dmnt_cheats_enabled_by_default` [system setting](configurations.md).

Users may use homebrew programs to toggle cheats on and off at runtime via the cheat manager's service API.

## Cheat Code Compatibility
Atmosphère manages cheat code through the execution of a small, custom virtual machine. Care has been taken to ensure that Atmosphère's cheat code format is fully backwards compatible with the pre-existing cheat code format, though new features have been added and bugs in the pre-existing cheat code applier have been fixed. Here is a short summary of the changes from the pre-existing format:

+ A number of bugs were fixed in the processing of conditional instructions.
  + The pre-existing implementation was fundamentally broken, and checked for the wrong value when detecting the end of a conditional block.
  + The pre-existing implementation also did not properly decode instructions, and instead linearly scanned for the terminator value. This caused problems if an instruction happened to encode a terminator inside its immediate values.
  + The pre-existing implementation did not bounds check, and thus certain conditional cheat codes could cause it to read out-of-bounds memory, and potentially crash due to a data abort.
+ Support was added for nesting conditional blocks.
+ An instruction was added to perform much more complex arbitrary arithmetic on two registers.
+ An instruction was added to allow writing the contents of register to a memory address specified by another register.
+ The pre-existing implementation did not correctly synchronize with the application process, and thus would cause heavy lag under certain circumstances (especially around loading screens). This has been fixed in Atmosphère's implementation.

## Cheat Code Format
The following provides documentation of the instruction format for the virtual machine used to manage cheat codes.

Typically, instruction type is encoded in the upper nybble of the first instruction u32.

### Code Type 0x0: Store Static Value to Memory
Code type 0x0 allows writing a static value to a memory address.

#### Encoding
`0TMR00AA AAAAAAAA VVVVVVVV (VVVVVVVV)`

+ T: Width of memory write (1, 2, 4, or 8 bytes).
+ M: Memory region to write to (0 = Main NSO, 1 = Heap, 2 = Alias, 3 = Aslr, 4 = non-relative).
+ R: Register to use as an offset from memory region base.
+ A: Immediate offset to use from memory region base.
+ V: Value to write.

---

### Code Type 0x1: Begin Conditional Block
Code type 0x1 performs a comparison of the contents of memory to a static value.

If the condition is not met, all instructions until the appropriate End or Else conditional block terminator are skipped.

#### Encoding
`1TMCXrAA AAAAAAAA VVVVVVVV (VVVVVVVV)`

+ T: Width of memory read (1, 2, 4, or 8 bytes).
+ M: Memory region to read from (0 = Main NSO, 1 = Heap, 2 = Alias, 3 = Aslr, 4 = non-relative).
+ C: Condition to use, see below.
+ X: Operand Type, see below.
+ r: Offset Register (operand types 1).
+ A: Immediate offset to use from memory region base.
+ V: Value to compare to.

#### Conditions
+ 1: >
+ 2: >=
+ 3: <
+ 4: <=
+ 5: ==
+ 6: !=

#### Operand Type
+ 0: Memory Base + Relative Offset
+ 1: Memory Base + Offset Register + Relative Offset
---

### Code Type 0x2: End Conditional Block
Code type 0x2 marks the end of a conditional block (started by Code Type 0x1 or Code Type 0x8).

When an Else is executed, all instructions until the appropriate End conditional block terminator are skipped.

#### Encoding
`2X000000`

+ X: End type (0 = End, 1 = Else).

---

### Code Type 0x3: Start/End Loop
Code type 0x3 allows for iterating in a loop a fixed number of times.

#### Start Loop Encoding
`300R0000 VVVVVVVV`

+ R: Register to use as loop counter.
+ V: Number of iterations to loop.

#### End Loop Encoding
`310R0000`

+ R: Register to use as loop counter.

---

### Code Type 0x4: Load Register with Static Value
Code type 0x4 allows setting a register to a constant value.

#### Encoding
`400R0000 VVVVVVVV VVVVVVVV`

+ R: Register to use.
+ V: Value to load.

---

### Code Type 0x5: Load Register with Memory Value
Code type 0x5 allows loading a value from memory into a register, either using a fixed address or by dereferencing the destination register.

#### Load From Fixed Address Encoding
`5TMR00AA AAAAAAAA`

+ T: Width of memory read (1, 2, 4, or 8 bytes).
+ M: Memory region to write to (0 = Main NSO, 1 = Heap, 2 = Alias, 3 = Aslr, 4 = non-relative).
+ R: Register to load value into.
+ A: Immediate offset to use from memory region base.

#### Load from Register Address Encoding
`5T0R10AA AAAAAAAA`

+ T: Width of memory read (1, 2, 4, or 8 bytes).
+ R: Register to load value into. (This register is also used as the base memory address).
+ A: Immediate offset to use from register R.

#### Load from Register Address Encoding
`5T0R2SAA AAAAAAAA`

+ T: Width of memory read (1, 2, 4, or 8 bytes).
+ R: Register to load value into. 
+ S: Register to use as the base memory address.
+ A: Immediate offset to use from register R.

#### Load From Fixed Address Encoding with offset register
`5TMR3SAA AAAAAAAA`

+ T: Width of memory read (1, 2, 4, or 8 bytes).
+ M: Memory region to write to (0 = Main NSO, 1 = Heap, 2 = Alias, 3 = Aslr, 4 = non-relative).
+ R: Register to load value into.
+ S: Register to use as offset register.
+ A: Immediate offset to use from memory region base.
---

### Code Type 0x6: Store Static Value to Register Memory Address
Code type 0x6 allows writing a fixed value to a memory address specified by a register.

#### Encoding
`6T0RIor0 VVVVVVVV VVVVVVVV`

+ T: Width of memory write (1, 2, 4, or 8 bytes).
+ R: Register used as base memory address.
+ I: Increment register flag (0 = do not increment R, 1 = increment R by T).
+ o: Offset register enable flag (0 = do not add r to address, 1 = add r to address).
+ r: Register used as offset when o is 1.
+ V: Value to write to memory.

---

### Code Type 0x7: Legacy Arithmetic
Code type 0x7 allows performing arithmetic on registers.

However, it has been deprecated by Code type 0x9, and is only kept for backwards compatibility.

#### Encoding
`7T0RC000 VVVVVVVV`

+ T: Width of arithmetic operation (1, 2, 4, or 8 bytes).
+ R: Register to apply arithmetic to.
+ C: Arithmetic operation to apply, see below.
+ V: Value to use for arithmetic operation.

#### Arithmetic Types
+ 0: Addition
+ 1: Subtraction
+ 2: Multiplication
+ 3: Left Shift
+ 4: Right Shift

---

### Code Type 0x8: Begin Keypress Conditional Block
Code type 0x8 enters or skips a conditional block based on whether a key combination is pressed.

#### Encoding
`8kkkkkkk`

+ k: Keypad mask to check against, see below.

Note that for multiple button combinations, the bitmasks should be ORd together.

#### Keypad Values
Note: This is the direct output of `hidKeysDown()`.

+ 0000001: A
+ 0000002: B
+ 0000004: X
+ 0000008: Y
+ 0000010: Left Stick Pressed
+ 0000020: Right Stick Pressed
+ 0000040: L
+ 0000080: R
+ 0000100: ZL
+ 0000200: ZR
+ 0000400: Plus
+ 0000800: Minus
+ 0001000: Left
+ 0002000: Up
+ 0004000: Right
+ 0008000: Down
+ 0010000: Left Stick Left
+ 0020000: Left Stick Up
+ 0040000: Left Stick Right
+ 0080000: Left Stick Down
+ 0100000: Right Stick Left
+ 0200000: Right Stick Up
+ 0400000: Right Stick Right
+ 0800000: Right Stick Down
+ 1000000: SL
+ 2000000: SR

---

### Code Type 0x9: Perform Arithmetic
Code type 0x9 allows performing arithmetic on registers.

#### Register Arithmetic Encoding
`9TCRS0s0`

+ T: Width of arithmetic operation (1, 2, 4, or 8 bytes).
+ C: Arithmetic operation to apply, see below.
+ R: Register to store result in.
+ S: Register to use as left-hand operand.
+ s: Register to use as right-hand operand.

#### Immediate Value Arithmetic Encoding
`9TCRS100 VVVVVVVV (VVVVVVVV)`

+ T: Width of arithmetic operation (1, 2, 4, or 8 bytes).
+ C: Arithmetic operation to apply, see below.
+ R: Register to store result in.
+ S: Register to use as left-hand operand.
+ V: Value to use as right-hand operand.

#### Arithmetic Types
+ 0: Addition
+ 1: Subtraction
+ 2: Multiplication
+ 3: Left Shift
+ 4: Right Shift
+ 5: Logical And
+ 6: Logical Or
+ 7: Logical Not (discards right-hand operand)
+ 8: Logical Xor
+ 9: None/Move (discards right-hand operand)
+ 10: Float Addition, T==4 single T==8 double
+ 11: Float Subtraction, T==4 single T==8 double
+ 12: Float Multiplication, T==4 single T==8 double
+ 13: Float Division, T==4 single T==8 double
---

### Code Type 0xA: Store Register to Memory Address
Code type 0xA allows writing a register to memory.

#### Encoding
`ATSRIOxa (aaaaaaaa)`

+ T: Width of memory write (1, 2, 4, or 8 bytes).
+ S: Register to write to memory.
+ R: Register to use as base address.
+ I: Increment register flag (0 = do not increment R, 1 = increment R by T).
+ O: Offset type, see below.
+ x: Register used as offset when O is 1, Memory type when O is 3, 4 or 5.
+ a: Value used as offset when O is 2, 4 or 5.

#### Offset Types
+ 0: No Offset
+ 1: Use Offset Register
+ 2: Use Fixed Offset
+ 3: Memory Region + Base Register
+ 4: Memory Region + Relative Address (ignore address register)
+ 5: Memory Region + Relative Address + Offset Register

---

### Code Type 0xB: Reserved
Code Type 0xB is currently reserved for future use.

---

### Code Type 0xC-0xF: Extended-Width Instruction
Code Types 0xC-0xF signal to the VM to treat the upper two nybbles of the first dword as instruction type, instead of just the upper nybble.

This reserves an additional 64 opcodes for future use.

---

### Code Type 0xC0: Begin Register Conditional Block
Code type 0xC0 performs a comparison of the contents of a register and another value. This code support multiple operand types, see below.

If the condition is not met, all instructions until the appropriate conditional block terminator are skipped.

#### Encoding
```
C0TcSX##
C0TcS0Ma aaaaaaaa
C0TcS1Mr
C0TcS2Ra aaaaaaaa
C0TcS3Rr
C0TcS400 VVVVVVVV (VVVVVVVV)
C0TcS5X0
```

+ T: Width of memory write (1, 2, 4, or 8 bytes).
+ c: Condition to use, see below.
+ S: Source Register.
+ X: Operand Type, see below.
+ M: Memory Type (operand types 0 and 1).
+ R: Address Register (operand types 2 and 3).
+ a: Relative Address (operand types 0 and 2).
+ r: Offset Register (operand types 1 and 3).
+ X: Other Register (operand type 5).
+ V: Value to compare to (operand type 4).

#### Operand Type
+ 0: Memory Base + Relative Offset
+ 1: Memory Base + Offset Register
+ 2: Register + Relative Offset
+ 3: Register + Offset Register
+ 4: Static Value
+ 5: Other Register

#### Conditions
+ 1: >
+ 2: >=
+ 3: <
+ 4: <=
+ 5: ==
+ 6: !=

---

### Code Type 0xC1: Save or Restore Register
Code type 0xC1 performs saving or restoring of registers.

#### Encoding
`C10D0Sx0`

+ D: Destination index.
+ S: Source index.
+ x: Operand Type, see below.

#### Operand Type
+ 0: Restore register
+ 1: Save register
+ 2: Clear saved value
+ 3: Clear register

---

### Code Type 0xC2: Save or Restore Register with Mask
Code type 0xC2 performs saving or restoring of multiple registers using a bitmask.

#### Encoding
`C2x0XXXX`

+ x: Operand Type, see below.
+ X: 16-bit bitmask, bit i == save or restore register i.

#### Operand Type
+ 0: Restore register
+ 1: Save register
+ 2: Clear saved value
+ 3: Clear register

---

### Code Type 0xC3: Read or Write Static Register
Code type 0xC3 reads or writes a static register with a given register.

#### Encoding
`C3000XXx`

+ XX: Static register index, 0x00 to 0x7F for reading or 0x80 to 0xFF for writing.
+ x: Register index.

---

### Code Type 0xC4: Begin Extended Keypress Conditional Block
Code type 0xC4 enters or skips a conditional block based on whether a key combination is pressed.

#### Encoding
`C4r00000 kkkkkkkk kkkkkkkk`

+ r: Auto-repeat, see below.
+ kkkkkkkkkk: Keypad mask to check against output of `hidKeysDown()`.

Note that for multiple button combinations, the bitmasks should be OR'd together.

#### Auto-repeat

+ 0: The conditional block executes only once when the keypad mask matches. The mask must stop matching to reset for the next trigger.
+ 1: The conditional block executes as long as the keypad mask matches.

#### Keypad Values
Note: This is the direct output of `hidKeysDown()`.

+ 000000001: A
+ 000000002: B
+ 000000004: X
+ 000000008: Y
+ 000000010: Left Stick Pressed
+ 000000020: Right Stick Pressed
+ 000000040: L
+ 000000080: R
+ 000000100: ZL
+ 000000200: ZR
+ 000000400: Plus
+ 000000800: Minus
+ 000001000: Left
+ 000002000: Up
+ 000004000: Right
+ 000008000: Down
+ 000010000: Left Stick Left
+ 000020000: Left Stick Up
+ 000040000: Left Stick Right
+ 000080000: Left Stick Down
+ 000100000: Right Stick Left
+ 000200000: Right Stick Up
+ 000400000: Right Stick Right
+ 000800000: Right Stick Down
+ 001000000: SL Left Joy-Con
+ 002000000: SR Left Joy-Con
+ 004000000: SL Right Joy-Con
+ 008000000: SR Right Joy-Con
+ 010000000: Top button on Poké Ball Plus (Palma) controller
+ 020000000: Verification
+ 040000000: B button on Left NES/HVC controller in Handheld mode
+ 080000000: Left C button in N64 controller
+ 100000000: Up C button in N64 controller
+ 200000000: Right C button in N64 controller
+ 400000000: Down C button in N64 controller

### Code Type 0xF0: Double Extended-Width Instruction
Code Type 0xF0 signals to the VM to treat the upper three nybbles of the first dword as instruction type, instead of just the upper nybble.

This reserves an additional 16 opcodes for future use.

---

### Code Type 0xFF0: Pause Process
Code type 0xFF0 pauses the current process.

#### Encoding
`FF0?????`

---

### Code Type 0xFF1: Resume Process
Code type 0xFF1 resumes the current process.

#### Encoding
`FF1?????`

---

### Code Type 0xFFF: Debug Log
Code type 0xFFF writes a debug log to the SD card under the folder `/atmosphere/cheat_vm_logs/`.

#### Encoding
```
FFFTIX##
FFFTI0Ma aaaaaaaa
FFFTI1Mr
FFFTI2Ra aaaaaaaa
FFFTI3Rr
FFFTI4X0
```

+ T: Width of memory write (1, 2, 4, or 8 bytes).
+ I: Log id.
+ X: Operand Type, see below.
+ M: Memory Type (operand types 0 and 1).
+ R: Address Register (operand types 2 and 3).
+ a: Relative Address (operand types 0 and 2).
+ r: Offset Register (operand types 1 and 3).
+ X: Value Register (operand type 4).

#### Operand Type
+ 0: Memory Base + Relative Offset
+ 1: Memory Base + Offset Register
+ 2: Register + Relative Offset
+ 3: Register + Offset Register
+ 4: Register Value

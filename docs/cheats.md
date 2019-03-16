# Cheats
Atmosphère supports Action-Replay style cheat codes, with cheats loaded off of the SD card.

## Cheat Loading Process
By default, Atmosphère will do the following when deciding whether to attach to a new application process:

+ Retrieve information about the new application process from `pm` and `loader`.
+ Check whether a user-defined key combination is held, and stop if not.
  + This defaults to "L is not held", and can be configured the same way as `fs.mitm` override keys.
  + The ini key to configure this is `cheat_enable_key`.
+ Check whether the process is a real application, and stop if not.
  + This guards against applying cheat codes to the homebrew loader.
+ Attempt to load cheats from `atmosphere/titles/<title_id>/cheats/<build_id>.txt`, where `build_id` is the hexadecimal representation of the first 8 bytes of the application's main executable's build id.
  + If no cheats are found, then the cheat manager will stop.
+ Open a kernel debug session for the new application process.
+ Signal to a system event that a new cheat process has been attached to.

This behavior ensures that cheat codes are only loaded when the user would want them to.

In cases where dmnt has not activated the cheat manager, but the user wants to make it do so anyway, the cheat manager's service API provides a `ForceOpenCheatProcess` command that homebrew can use. This command will cause the cheat manager to try to force itself to attach to the process.

By default, all cheat codes listed in the loaded .txt file will be toggled on. This is configurable by the user, and the default can be set to toggled off by editing the `atmosphere!dmnt_cheats_enabled_by_default` entry to 0 instead of 1.

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

### Code Type 0: Store Static Value to Memory

Code type 0 allows writing a static value to a memory address.

#### Encoding

`0TMR00AA AAAAAAAA VVVVVVVV (VVVVVVVV)`

+ T: width of memory write (1, 2, 4, or 8 bytes)
+ M: memory region to write to (0 = Main NSO, 1 = Heap)
+ R: Register to use as an offset from memory region base.
+ A: Immediate offset to use from memory region base.
+ V: Value to write.

---

### Code Type 1: Begin Conditional Block

Code type 1 performs a comparison of the contents of memory to a static value.

If the condition is not met, all instructions until the appropriate conditional block terminator are skipped.

#### Encoding

`1TMC00AA AAAAAAAA VVVVVVVV (VVVVVVVV)`

+ T: width of memory write (1, 2, 4, or 8 bytes)
+ M: memory region to write to (0 = Main NSO, 1 = Heap)
+ C: Condition to use, see below.
+ A: Immediate offset to use from memory region base.
+ V: Value to compare to.

#### Conditions

+ 1: >
+ 2: >=
+ 3: <
+ 4: <=
+ 5: ==
+ 6: !=

---

### Code Type 2: End Conditional Block

Code type 2 marks the end of a conditional block (started by Code Type 1 or Code Type 8).

#### Encoding

`20000000`

---

### Code Type 3: Start/End Loop

Code type 3 allows for iterating in a loop a fixed number of times.

#### Start Loop Encoding

`300R0000 VVVVVVVV`

+ R: Register to use as loop counter.
+ V: Number of iterations to loop.

#### End Loop Encoding

`310R0000`

+ R: Register to use as loop counter.

---

### Code Type 4: Load Register with Static Value

Code type 4 allows setting a register to a constant value.

#### Encoding

`400R0000 VVVVVVVV VVVVVVVV`

+ R: Register to use.
+ V: Value to load.

---

### Code Type 5: Load Register with Memory Value

Code type 5 allows loading a value from memory into a register, either using a fixed address or by dereferencing the destination register.

#### Load From Fixed Address Encoding

`5TMR00AA AAAAAAAA`

+ T: width of memory read (1, 2, 4, or 8 bytes)
+ M: memory region to write to (0 = Main NSO, 1 = Heap)
+ R: Register to load value into.
+ A: Immediate offset to use from memory region base.

#### Load from Register Address Encoding

`5TMR10AA AAAAAAAA`

+ T: width of memory read (1, 2, 4, or 8 bytes)
+ M: memory region to write to (0 = Main NSO, 1 = Heap)
+ R: Register to load value into.
+ A: Immediate offset to use from register R.

---

### Code Type 6: Store Static Value to Register Memory Address

Code type 6 allows writing a fixed value to a memory address specified by a register.

#### Encoding

`6T0RIor0 VVVVVVVV VVVVVVVV`

+ T: width of memory write (1, 2, 4, or 8 bytes)
+ R: Register used as base memory address.
+ I: Increment register flag (0 = do not increment R, 1 = increment R by T).
+ o: Offset register enable flag (0 = do not add r to address, 1 = add r to address).
+ r: Register used as offset when o is 1.
+ V: Value to write to memory.

---

### Code Type 7: Legacy Arithmetic

Code type 7 allows performing arithmetic on registers.

However, it has been deprecated by Code type 9, and is only kept for backwards compatibility.

#### Encoding

`7T0RC000 VVVVVVVV`

+ T: width of arithmetic operation (1, 2, 4, or 8 bytes)
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

### Code Type 8: Begin Keypress Conditional Block

Code type 8 enters or skips a conditional block based on whether a key combination is pressed.

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

### Code Type 9: Perform Arithmetic

Code type 9 allows performing arithmetic on registers.

#### Register Arithmetic Encoding

`9TCRS0s0`

+ T: width of arithmetic operation (1, 2, 4, or 8 bytes)
+ C: Arithmetic operation to apply, see below.
+ R: Register to store result in.
+ S: Register to use as left-hand operand.
+ s: Register to use as right-hand operand.

#### Immediate Value Arithmetic Encoding

`9TCRS100 VVVVVVVV (VVVVVVVV)`

+ T: width of arithmetic operation (1, 2, 4, or 8 bytes)
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

---

### Code Type 10: Store Register to Memory Address

Code type 10 allows writing a register to memory.

#### Encoding

`ATSRIOra (aaaaaaaa)`

+ T: width of memory write (1, 2, 4, or 8 bytes)
+ S: Register to write to memory.
+ R: Register to use as base address.
+ I: Increment register flag (0 = do not increment R, 1 = increment R by T).
+ O: Offset type, see below.
+ r: Register used as offset when O is 1.
+ a: Value used as offset when O is 2.

#### Offset Types

+ 0: No Offset
+ 1: Use Offset Register
+ 2: Use Fixed Offset

---

### Code Type 11: Reserved

Code Type 11 is currently reserved for future use.

---

### Code Type 12-15: Extended-Width Instruction

Code Types 12-15 signal to the VM to treat the upper two nybbles of the first dword as instruction type, instead of just the upper nybble.

This reserves an additional 64 opcodes for future use.

---
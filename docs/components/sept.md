# Sept
Sept is a payload that facilitates booting Atmosphère when targeting firmware version 7.0.0+.

It consists of a primary and a secondary payload.

## Sept-primary
Sept-primary is essentially a stand-in for Nintendo's package1ldr, on 7.0.0+. To use it, the caller (normally Fusée-secondary) loads the Sept-primary binary to `0x4003F000`, loads the 7.0.0+ TSEC firmware to `0x40010F00`, and loads a signed, encrypted payload to `0x40016FE0`.

This signed, encrypted payload is normally Sept-secondary.

## Sept-secondary
Sept-secondary is a payload that performs 7.0.0+ key derivation, and then chainloads to `sept/payload.bin`.

It is normally stored encrypted/signed. Therefore, if one wishes to build Sept-secondary instead of using release builds, one must bring their own keys.

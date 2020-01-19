#!/usr/bin/env python
import sys, os
from struct import pack as pk, unpack as up
from Crypto.Cipher import AES
from Crypto.Hash import CMAC
try:
    import KEYS
except ImportError:
    import KEYS_template as KEYS
    print('Warning: output will not work on 7.0.0+!')


def shift_left_xor_rb(s):
    if hasattr(int, "from_bytes"):
      N = int.from_bytes(s, byteorder="big")
    else:
      N = int(s.encode('hex'), 16)

    if N & (1 << 127):
        N = ((N << 1) ^ 0x87) & 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    else:
        N = ((N << 1) ^ 0x00) & 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    return bytearray.fromhex('%032x' % N)


def sxor(x, y):
    return bytearray(a^b for a,b in zip(x, y))


def get_last_block_for_desired_mac(key, data, desired_mac):
    assert len(desired_mac) == 0x10
    k1 = shift_left_xor_rb(AES.new(key, AES.MODE_ECB).encrypt(bytearray(0x10)))
    if len(data) & 0xF:
        k1 = shift_left_xor_rb(k1)
        data = data + b'\x80'
        data = data + bytearray((0x10 - (len(data) & 0xF)) & 0xF)
    num_blocks = (len(data) + 0xF) >> 4
    last_block = sxor(bytearray(AES.new(key, AES.MODE_ECB).decrypt(desired_mac)), bytearray(k1))
    if len(data) > 0x0:
        last_block = sxor(last_block, bytearray(AES.new(key, AES.MODE_CBC, bytearray(0x10)).encrypt(data)[-0x10:]))
    return last_block


def sign_encrypt_code(code, sig_key, enc_key, iv, desired_mac, version):
    # Pad with 0x20 of zeroes.
    code = code + bytearray(0x20)
    code_len = len(code)
    code_len += 0xFFF
    code_len &= ~0xFFF
    code = code + bytearray(code_len - len(code))

    # Insert version
    code = code[:8] + pk('<I', version) + code[12:]

    # Add empty trustzone, warmboot segments.
    code = code + bytearray(0x1FE0 - 0x10)
    pk11_hdr = b'PK11' + pk('<IIIIIII', 0x1000, 0, 0, code_len - 0x20, 0, 0x1000, 0)
    pk11 = pk11_hdr + code
    enc_pk11 = AES.new(enc_key, AES.MODE_CBC, iv).encrypt(pk11)
    enc_pk11 = pk('<IIII', len(pk11) + 0x10, 0, 0, 0) + iv + enc_pk11
    enc_pk11 = enc_pk11 + get_last_block_for_desired_mac(sig_key, enc_pk11, desired_mac)
    enc_pk11 = enc_pk11 + CMAC.new(sig_key, enc_pk11, AES).digest()
    return enc_pk11


def main(argc, argv):
    if argc != 3:
        print('Usage: %s input output' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        code = f.read()
    if len(code) & 0xF:
        code = code + bytearray(0x10 - (len(code) & 0xF))
    fn, fext = os.path.splitext(argv[2])
    for key in range(KEYS.NUM_KEYS):
        with open(fn + ('_%02X' % key) + fext, 'wb') as f:
            f.write(sign_encrypt_code(code, KEYS.HOVI_SIG_KEY_PRD[key], KEYS.HOVI_ENC_KEY_PRD[key], KEYS.IV[key], b'THANKS_NVIDIA_<3', key))
        with open(fn + ('_dev_%02X' % key) + fext, 'wb') as f:
            f.write(sign_encrypt_code(code, KEYS.HOVI_SIG_KEY_DEV[key], KEYS.HOVI_ENC_KEY_DEV[key], KEYS.IV_DEV[key], b'THANKS_NVIDIA_<3', key))
    return 0


if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))

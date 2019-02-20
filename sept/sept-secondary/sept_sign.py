#!/usr/bin/env python
import sys
from struct import pack as pk, unpack as up
from Crypto.Cipher import AES
from Crypto.Hash import CMAC
import KEYS

def sign_encrypt_code(code, sig_key, enc_key, iv):
    # Pad with 0x20 of zeroes.
    code += '\x00' * 0x20
    code_len = len(code)
    code_len += 0xFFF
    code_len &= ~0xFFF
    code += '\x00' * (code_len - len(code))
    
    # Add empty trustzone, warmboot segments.
    code += '\x00'*0x1FE0
    pk11_hdr = 'PK11' + pk('<IIIIIII', 0x1000, 0, 0, code_len - 0x20, 0, 0x1000, 0)
    pk11 = pk11_hdr + code
    enc_pk11 = AES.new(enc_key, AES.MODE_CBC, iv).encrypt(pk11)
    enc_pk11 = pk('<IIII', len(pk11), 0, 0, 0) + iv + enc_pk11
    enc_pk11 += CMAC.new(sig_key, enc_pk11, AES).digest()
    return enc_pk11
    
def main(argc, argv):
    if argc != 3:
        print('Usage: %s input output' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        code = f.read()
    assert (len(code) & 0xF) == 0
    # TODO: Support dev unit crypto
    with open(argv[2], 'wb') as f:
        f.write(sign_encrypt_code(code, KEYS.HOVI_SIG_KEY_PRD, KEYS.HOVI_ENC_KEY_PRD, KEYS.IV))
    return 0
        
if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
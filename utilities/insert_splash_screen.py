from PIL import Image
import sys, os
from struct import pack as pk

SPLASH_SCREEN_WIDTH  = 1280
SPLASH_SCREEN_HEIGHT = 720

SPLASH_SCREEN_STRIDE = 768

def convert_image(image_fn):
    splash = Image.open(image_fn, 'r')
    w, h = splash.size
    if w == 1280 and h == 720:
        splash = splash.transpose(Image.ROTATE_90)
        w, h = splash.size
    assert w == 720
    assert h == 1280
    splash = splash.convert('RGBA')
    splash_bin = bytearray()
    for row in range(SPLASH_SCREEN_WIDTH):
        for col in range(SPLASH_SCREEN_HEIGHT):
            r, g, b, a = splash.getpixel((col, row))
            splash_bin += pk('<BBBB', b, g, r, a)
        splash_bin += b'\x00' * ((SPLASH_SCREEN_STRIDE - SPLASH_SCREEN_HEIGHT) * 4)
    return splash_bin

def main(argc, argv):
    if argc != 3:
        print('Usage: %s image package3' % argv[0])
        return 1
    with open(argv[2], 'rb') as f:
        package3 = f.read()
    assert package3[:4] == b'PK31'
    assert len(package3) == 0x800000
    splash_bin = convert_image(argv[1])
    assert len(splash_bin) == 0x3C0000
    with open(argv[2], 'wb') as f:
        f.write(package3[:0x400000])
        f.write(splash_bin)
        f.write(package3[0x7C0000:])
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
#!/usr/bin/env python3
"""Convierte los BMP que escribe el simulador a PNG, para poder mirarlos."""
import struct, sys, zlib

def convert(src, dst):
    d = open(src, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    bpp = struct.unpack_from('<H', d, 28)[0]
    assert bpp == 24, bpp
    pad = (4 - (w * 3) % 4) % 4
    rows = []
    for y in range(h - 1, -1, -1):          # BMP viene de abajo hacia arriba
        p = off + y * (w * 3 + pad)
        row = bytearray([0])
        for x in range(w):
            b, g, r = d[p + x*3], d[p + x*3 + 1], d[p + x*3 + 2]
            row += bytes((r, g, b))
        rows.append(bytes(row))
    raw = b''.join(rows)
    def chunk(tag, data):
        return (struct.pack('>I', len(data)) + tag + data +
                struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF))
    png = (b'\x89PNG\r\n\x1a\n' +
           chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
           chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
    open(dst, 'wb').write(png)
    print('%s -> %s  (%dx%d)' % (src, dst, w, h))

if __name__ == '__main__':
    convert(sys.argv[1], sys.argv[2])

#!/usr/bin/env python3
"""Autoría del arte de ROOTKIT.

Genera los sprites como datos C indexados por paleta y, de paso, una vista
previa en PNG para poder mirarlos sin compilar nada.

El cuerpo de Tuga se construye con primitivas (elipses, escudos, luz) en vez
de dibujarse pixel por pixel: a 64x48 el resultado es más limpio y, sobre
todo, se puede iterar cambiando dos números. Las expresiones —ojos y boca—
sí van dibujadas a mano acá abajo, porque ahí el carácter está en cada pixel.

    python tools/gen_art.py

Escribe firmware/art/tuga_data.{h,c} y tools/preview/*.png
"""

import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "firmware", "art")
PREVIEW = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview")

# ---------------------------------------------------------------- paleta ---
# Índice 0 siempre transparente. Verdes de caparazón fríos, piel más cálida,
# y un acento cian que es lo único "cyber" del personaje: las juntas de los
# escudos brillan como circuitería.
PALETTE = [
    (0, 0, 0),          # 0  transparente (no se usa)
    (14, 20, 16),       # 1  contorno
    (26, 54, 46),       # 2  caparazón sombra
    (40, 82, 66),       # 3  caparazón medio
    (58, 110, 84),      # 4  caparazón claro
    (96, 156, 112),     # 5  caparazón brillo
    (72, 214, 190),     # 6  acento cian (juntas de escudos)
    (78, 92, 44),       # 7  piel sombra
    (122, 142, 62),     # 8  piel media
    (164, 184, 92),     # 9  piel clara
    (206, 214, 150),    # 10 vientre
    (236, 240, 226),    # 11 blanco del ojo
    (18, 24, 20),       # 12 pupila
]

TRANSPARENT, OUTLINE = 0, 1
SH_DARK, SH_MID, SH_LIGHT, SH_HI = 2, 3, 4, 5
ACCENT = 6
SK_DARK, SK_MID, SK_LIGHT, BELLY = 7, 8, 9, 10
EYE_W, EYE_K = 11, 12

BODY_W, BODY_H = 96, 72

# Anclas de la cara. El firmware las usa para pegar ojos y boca sobre el
# cuerpo, así que se exportan al header en vez de quedar dispersas en el C.
HEAD_CX, HEAD_CY = 48, 53
EYE_DX, EYE_DY = 11, 4     # desplazamiento de cada ojo respecto del centro
MOUTH_DY = 13


class Grid(object):
    def __init__(self, w, h, fill=TRANSPARENT):
        self.w, self.h = w, h
        self.d = [fill] * (w * h)

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.d[y * self.w + x]
        return TRANSPARENT

    def set(self, x, y, v):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.d[y * self.w + x] = v

    def ellipse(self, cx, cy, rx, ry, v, y_from=None, y_to=None):
        for y in range(cy - ry, cy + ry + 1):
            if y_from is not None and y < y_from:
                continue
            if y_to is not None and y > y_to:
                continue
            for x in range(cx - rx, cx + rx + 1):
                dx = (x - cx) / float(rx)
                dy = (y - cy) / float(ry)
                if dx * dx + dy * dy <= 1.0:
                    self.set(x, y, v)

    def ellipse_ring(self, cx, cy, rx, ry, v, thick=1.0):
        for y in range(cy - ry - 1, cy + ry + 2):
            for x in range(cx - rx - 1, cx + rx + 2):
                dx = (x - cx) / float(rx)
                dy = (y - cy) / float(ry)
                d = dx * dx + dy * dy
                inner = (1.0 - thick / float(min(rx, ry))) ** 2
                if inner <= d <= 1.0:
                    self.set(x, y, v)

    def outline(self, colour=OUTLINE):
        """Un pixel de contorno donde el cuerpo toca el vacío."""
        add = []
        for y in range(self.h):
            for x in range(self.w):
                if self.get(x, y) != TRANSPARENT:
                    continue
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    v = self.get(nx, ny)
                    if v != TRANSPARENT and v != colour:
                        add.append((x, y))
                        break
        for x, y in add:
            self.set(x, y, colour)


def build_tuga():
    """El cuerpo de Tuga, sin ojos ni boca: esos van como overlays.

    Toda la geometría está en pixeles del lienzo de 96x72. El bicho ocupa
    deliberadamente ~60% del ancho de la escena: es el protagonista, no un
    icono de estado.
    """
    g = Grid(BODY_W, BODY_H)
    cx = HEAD_CX

    # -- patas traseras, detrás de todo ------------------------------------
    for sx in (28, 68):
        g.ellipse(sx, 63, 8, 5, SK_DARK)

    # -- caparazón ----------------------------------------------------------
    shell_cy, shell_rx, shell_ry = 32, 39, 23
    g.ellipse(cx, shell_cy, shell_rx, shell_ry, SH_MID)

    # Luz desde arriba a la izquierda. Tres tonos bien separados: a esta
    # escala un degradé suave se convierte en barro, los escalones leen mejor.
    for y in range(BODY_H):
        for x in range(BODY_W):
            if g.get(x, y) != SH_MID:
                continue
            t = ((x - (cx - 24)) / 66.0) * 0.5 + ((y - 9) / 45.0) * 0.5
            if t < 0.26:
                g.set(x, y, SH_LIGHT)
            elif t > 0.66:
                g.set(x, y, SH_DARK)

    # -- escudos ------------------------------------------------------------
    # El contorno va en sombra, no en cian: el acento se reserva para el arco
    # superior de cada placa, donde "toma luz". Así el bicho parece tener
    # circuitería bajo el caparazón en vez de aros de neón encima.
    scutes = [(cx, 27, 14, 9),
              (cx - 24, 24, 9, 6), (cx + 24, 24, 9, 6),
              (cx - 18, 43, 11, 6), (cx + 18, 43, 11, 6),
              (cx, 46, 12, 6), (cx, 11, 14, 5)]

    for sx, sy, rx, ry in scutes:
        g.ellipse_ring(sx, sy, rx, ry, SH_DARK, thick=1.5)

    for sx, sy, rx, ry in scutes:
        for x in range(sx - rx, sx + rx + 1):
            for y in range(sy - ry, sy):
                if g.get(x, y) != SH_DARK:
                    continue
                dx = (x - sx) / float(rx)
                dy = (y - sy) / float(ry)
                if 0.72 <= dx * dx + dy * dy <= 1.06:
                    g.set(x, y, ACCENT)

    # -- borde y brillo del caparazón ---------------------------------------
    g.ellipse_ring(cx, shell_cy, shell_rx, shell_ry, SH_DARK, thick=3.0)
    for x in range(cx - 28, cx - 8):
        for y in range(11, 21):
            dx = (x - (cx - 18)) / 10.0
            dy = (y - 15) / 4.0
            if dx * dx + dy * dy <= 1.0 and g.get(x, y) != TRANSPARENT:
                g.set(x, y, SH_HI)

    # -- aletas delanteras --------------------------------------------------
    for sx, flip in ((15, -1), (81, 1)):
        g.ellipse(sx, 51, 11, 9, SK_MID)
        g.ellipse(sx - flip * 2, 48, 8, 6, SK_LIGHT)
        g.ellipse(sx + flip * 3, 56, 6, 5, SK_DARK)

    # -- cabeza, adelante de todo -------------------------------------------
    g.ellipse(cx, HEAD_CY, 20, 17, SK_MID)
    g.ellipse(cx, HEAD_CY - 3, 18, 14, SK_LIGHT)   # frente iluminada
    g.ellipse(cx, HEAD_CY + 9, 11, 5, BELLY)       # mentón
    g.ellipse(cx - 11, HEAD_CY - 8, 6, 4, BELLY)   # reflejo en la mejilla

    # Fosas nasales: dos pixeles que suman muchísimo.
    for dy in (0, 1):
        g.set(cx - 5, HEAD_CY + 3 + dy, SK_DARK)
        g.set(cx + 5, HEAD_CY + 3 + dy, SK_DARK)

    g.outline()
    return g


# ------------------------------------------------------- expresiones -------
# '.' transparente, 'w' blanco, 'k' pupila, 'X' contorno, 'c' acento
EYES = {
    "OPEN": [
        "...XXXXX...",
        ".XXwwwwwXX.",
        "XwwwkkkwwwX",
        "XwwkkwkkwwX",
        "XwwkkkkkwwX",
        "XwwwkkkwwwX",
        ".XwwwwwwwX.",
        "..XXXXXXX..",
        "...........",
    ],
    "BLINK": [
        "...........",
        "...........",
        "...........",
        "XX.......XX",
        ".XX.....XX.",
        "..XXXXXXX..",
        "...........",
        "...........",
        "...........",
    ],
    "HAPPY": [
        "...........",
        "...XXXXX...",
        "..XX...XX..",
        ".XX.....XX.",
        "XX.......XX",
        "...........",
        "...........",
        "...........",
        "...........",
    ],
    "WIDE": [
        ".XXXXXXXXX.",
        "XwwwwwwwwwX",
        "XwwkkkkkwwX",
        "XwkkkwkkkwX",
        "XwkkkkkkkwX",
        "XwkkkkkkkwX",
        "XwwkkkkkwwX",
        "XwwwwwwwwwX",
        ".XXXXXXXXX.",
    ],
    "SLEEPY": [
        "...........",
        "...XXXXX...",
        ".XXwwwwwXX.",
        "XwwwkkkwwwX",
        "XkkkkkkkkkX",
        ".XXXXXXXXX.",
        "...........",
        "...........",
        "...........",
    ],
    "DEAD": [
        "...........",
        ".XX.....XX.",
        "..XX...XX..",
        "...XX.XX...",
        "....XXX....",
        "...XX.XX...",
        "..XX...XX..",
        ".XX.....XX.",
        "...........",
    ],
    "DIZZY": [
        "..XXXXXXX..",
        ".XcccccccX.",
        "XccXXXXXccX",
        "XcXcccccXcX",
        "XcXcXXXcXcX",
        "XcXcccccXcX",
        "XccXXXXXccX",
        ".XcccccccX.",
        "..XXXXXXX..",
    ],
    "GLITCH": [
        "XXXXXXXXXXX",
        "XcccXXXcccX",
        "XXcccXcccXX",
        "XcXcccccXcX",
        "XXcccXcccXX",
        "XcccXXXcccX",
        "XXXXXXXXXXX",
        "...........",
        "...........",
    ],
}

MOUTHS = {
    "SMILE": [
        "XX.........XX",
        ".XX.......XX.",
        "..XXXXXXXXX..",
        ".............",
        ".............",
    ],
    "FLAT": [
        ".............",
        "..XXXXXXXXX..",
        ".............",
        ".............",
        ".............",
    ],
    "FROWN": [
        "..XXXXXXXXX..",
        ".XX.......XX.",
        "XX.........XX",
        ".............",
        ".............",
    ],
    "OPEN": [
        "...XXXXXXX...",
        "..XkkkkkkkX..",
        "..XkkkkkkkX..",
        "...XXXXXXX...",
        ".............",
    ],
    "WAVY": [
        ".............",
        "XX..XX..XX..X",
        "..XX..XX..XX.",
        ".............",
        ".............",
    ],
    "PANT": [
        "..XXXXXXXXX..",
        ".XkkkkkkkkkX.",
        ".XkkkkkkkkkX.",
        "..XkkkkkkkX..",
        "...XXXXXXX...",
    ],
}

CHARMAP = {".": TRANSPARENT, "X": OUTLINE, "w": EYE_W,
           "k": EYE_K, "c": ACCENT}


def grid_from_rows(rows):
    h = len(rows)
    w = len(rows[0])
    for r in rows:
        assert len(r) == w, "filas de distinto ancho: %r" % (r,)
    g = Grid(w, h)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            g.set(x, y, CHARMAP[ch])
    return g


# ------------------------------------------------------------- salida ------
def write_png(path, grid, zoom=6):
    w, h = grid.w * zoom, grid.h * zoom
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            v = grid.get(x // zoom, y // zoom)
            if v == TRANSPARENT:
                # damero para que se vea qué es transparente
                c = (60, 60, 66) if ((x // zoom // 4 + y // zoom // 4) % 2) else (44, 44, 50)
            else:
                c = PALETTE[v]
            row += bytes(c)
        rows.append(bytes(row))
    raw = b"".join(rows)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(raw, 9)) +
           chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def c_array(name, grid):
    out = ["static const uint8_t %s_idx[%d] = {" % (name, grid.w * grid.h)]
    for y in range(grid.h):
        row = grid.d[y * grid.w:(y + 1) * grid.w]
        out.append("    " + ",".join("%2d" % v for v in row) + ",")
    out.append("};")
    out.append("const rk_sprite_t %s = { %d, %d, %s_idx, RK_TUGA_PAL, %d };"
               % (name, grid.w, grid.h, name, len(PALETTE)))
    return "\n".join(out)


def main():
    if not os.path.isdir(ART):
        os.makedirs(ART)
    if not os.path.isdir(PREVIEW):
        os.makedirs(PREVIEW)

    body = build_tuga()
    parts = [("rk_tuga_body", body)]
    for name, rows in sorted(EYES.items()):
        parts.append(("rk_eye_" + name.lower(), grid_from_rows(rows)))
    for name, rows in sorted(MOUTHS.items()):
        parts.append(("rk_mouth_" + name.lower(), grid_from_rows(rows)))

    # ---- cabecera
    h = ["/* Generado por tools/gen_art.py — no editar a mano. */",
         "#ifndef ROOTKIT_TUGA_DATA_H", "#define ROOTKIT_TUGA_DATA_H", "",
         '#include "../gfx/fb.h"', "",
         "/* Anclas de la cara, en coordenadas del sprite del cuerpo. */",
         "#define RK_TUGA_W        %d" % BODY_W,
         "#define RK_TUGA_H        %d" % BODY_H,
         "#define RK_HEAD_CX       %d" % HEAD_CX,
         "#define RK_HEAD_CY       %d" % HEAD_CY,
         "#define RK_EYE_DX        %d" % EYE_DX,
         "#define RK_EYE_DY        %d" % EYE_DY,
         "#define RK_MOUTH_DY      %d" % MOUTH_DY,
         "",
         "extern const rk_color_t RK_TUGA_PAL[%d];" % len(PALETTE), ""]
    for name, _ in parts:
        h.append("extern const rk_sprite_t %s;" % name)
    h += ["", "#endif /* ROOTKIT_TUGA_DATA_H */", ""]
    with open(os.path.join(ART, "tuga_data.h"), "w", newline="\n") as f:
        f.write("\n".join(h))

    # ---- datos
    c = ["/* Generado por tools/gen_art.py — no editar a mano. */",
         '#include "tuga_data.h"', "",
         "const rk_color_t RK_TUGA_PAL[%d] = {" % len(PALETTE)]
    for i, (r, g_, b) in enumerate(PALETTE):
        c.append("    RK_RGB(%3d, %3d, %3d),  /* %2d */" % (r, g_, b, i))
    c += ["};", ""]
    for name, grid in parts:
        c.append(c_array(name, grid))
        c.append("")
    with open(os.path.join(ART, "tuga_data.c"), "w", newline="\n") as f:
        f.write("\n".join(c))

    # ---- vistas previas
    write_png(os.path.join(PREVIEW, "tuga_body.png"), body, zoom=6)
    sheet = Grid(8 * 15, 2 * 11)
    for i, (name, rows) in enumerate(sorted(EYES.items())):
        gg = grid_from_rows(rows)
        for y in range(gg.h):
            for x in range(gg.w):
                sheet.set(i * 15 + x, y, gg.get(x, y))
    for i, (name, rows) in enumerate(sorted(MOUTHS.items())):
        gg = grid_from_rows(rows)
        for y in range(gg.h):
            for x in range(gg.w):
                sheet.set(i * 15 + x, 10 + y, gg.get(x, y))
    write_png(os.path.join(PREVIEW, "faces.png"), sheet, zoom=8)

    print("cuerpo   %dx%d" % (body.w, body.h))
    print("ojos     %s" % ", ".join(sorted(EYES)))
    print("bocas    %s" % ", ".join(sorted(MOUTHS)))
    print("escrito  firmware/art/tuga_data.{h,c}")
    print("previews tools/preview/*.png")


if __name__ == "__main__":
    main()

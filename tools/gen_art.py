#!/usr/bin/env python3
"""Autoría del arte de ROOTKIT.

Genera los sprites como datos C indexados por paleta y, de paso, vistas
previas en PNG para poder mirarlos sin compilar nada.

    python tools/gen_art.py

Escribe firmware/art/sprites.{h,c} y tools/preview/*.png


LA IDEA CENTRAL: DOS ESCALAS DEL MISMO ORGANISMO
------------------------------------------------

El ROOTKIT tiene dos pantallas de tamaños muy distintos, y cada una muestra
al mismo simbionte en una escala diferente:

  * el PRIME, TFT de 2,2" a 240x320, muestra el ADULTO: 96x72 de arte a 2x,
    unos 22 mm de criatura. Es donde el bicho tiene cara, caparazón y
    personalidad.
  * el MINI, TFT de 1,44" a 128x128, muestra el BROTE: 32x32 de arte a 2x,
    unos 13 mm. Mostrar más con menos.

El brote NO es el adulto reducido. Invierte sus proporciones: donde el adulto
es caparazón con una cabeza asomando, el brote es cabeza con un caparazón
asomando. Esa inversión —cabeza grande, ojos grandes, cuerpo chico— es la
gramática visual de "cría" en todas las culturas que dibujan, y es lo que
hace que se lea como bebé del mismo bicho y no como otro bicho.

UNA SILUETA, DOCE PALETAS
-------------------------

Los doce simbiontes comparten la geometría y se distinguen por dos cosas:

  * su PALETA, doce familias cromáticas construidas desde tres anclas
    (caparazón, piel, acento) e interpoladas a los trece índices.
  * su COPETE, el remate que le sale por encima del caparazón al brote:
    hojas, espinas, flor, esporas, rizo. A 32x32 el copete es casi un tercio
    de la silueta, así que es ahí donde conviene gastar la diferenciación.

Es un compromiso explícito: el adulto comparte silueta entre los doce hasta
que cada uno tenga su cuerpo propio. La cañería ya lo soporta —agregar un
cuerpo es agregar una función y una fila en la tabla— y hay un test que
verifica que las doce paletas existan y sean distintas entre sí.
"""

import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "firmware", "art")
PREVIEW = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview")

# ------------------------------------------------------------- índices ----
# Índice 0 siempre transparente. Los trece son los mismos para las doce
# paletas: el código de dibujo no sabe de qué simbionte se trata.
TRANSPARENT, OUTLINE = 0, 1
SH_DARK, SH_MID, SH_LIGHT, SH_HI = 2, 3, 4, 5
ACCENT = 6
SK_DARK, SK_MID, SK_LIGHT, BELLY = 7, 8, 9, 10
EYE_W, EYE_K = 11, 12
PAL_LEN = 13

ADULTO_W, ADULTO_H = 96, 72
BROTE_W, BROTE_H = 32, 32

# Anclas de la cara del adulto, en coordenadas de su sprite.
AD_HEAD_CX, AD_HEAD_CY = 48, 53
AD_EYE_DX, AD_EYE_DY = 11, 4
AD_MOUTH_DY = 13

# Anclas de la cara del brote. La cabeza se come más de la mitad del alto y
# los ojos van BAJOS y separados: es la receta del esquema infantil, y es lo
# que hace que 32x32 se lean como cría y no como versión chica del adulto.
BR_HEAD_CX, BR_HEAD_CY = 16, 20
BR_EYE_DX, BR_EYE_DY = 5, 1
BR_MOUTH_DY = 5


# ------------------------------------------------------------- paletas ----
def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def mul(c, k):
    return tuple(max(0, min(255, int(round(v * k)))) for v in c)


def build_palette(shell_lo, shell_hi, skin_lo, skin_hi, accent):
    """Trece colores desde cinco anclas.

    Tres escalones bien separados en el caparazón y tres en la piel. A esta
    escala un degradé suave se convierte en barro: los escalones leen mejor.
    """
    return [
        (0, 0, 0),                        # 0  transparente (nunca se usa)
        mul(shell_lo, 0.48),              # 1  contorno
        shell_lo,                         # 2  caparazón sombra
        lerp(shell_lo, shell_hi, 0.40),   # 3  caparazón medio
        lerp(shell_lo, shell_hi, 0.72),   # 4  caparazón claro
        shell_hi,                         # 5  caparazón brillo
        accent,                           # 6  acento
        skin_lo,                          # 7  piel sombra
        lerp(skin_lo, skin_hi, 0.50),     # 8  piel media
        skin_hi,                          # 9  piel clara
        lerp(skin_hi, (255, 250, 235), 0.55),   # 10 vientre
        (236, 240, 226),                  # 11 blanco del ojo
        mul(shell_lo, 0.42),              # 12 pupila
    ]


# id, copete, anclas cromáticas. El orden es EXACTAMENTE el de
# rk_companion_table en firmware/core/companion.c: el índice es la clave.
COMPANIONS = [
    # id       copete      caparazón lo/hi              piel lo/hi                  acento
    ("tuga",   "hoja2",   (26, 54, 46),  (96, 156, 112), (78, 92, 44),   (164, 184, 92),  (72, 214, 190)),
    ("myco",   "esporas", (48, 40, 62),  (150, 138, 176), (96, 88, 74),  (196, 186, 160), (204, 170, 255)),
    ("sable",  "punta",   (22, 44, 32),  (88, 132, 86),  (70, 84, 40),   (150, 168, 80),  (230, 206, 90)),
    ("zam",    "hoja1",   (16, 38, 30),  (74, 126, 96),  (52, 72, 48),   (126, 152, 102), (120, 200, 160)),
    ("spine",  "espinas", (34, 62, 48),  (118, 164, 120), (84, 104, 62), (176, 190, 120), (245, 178, 86)),
    ("vera",   "abanico", (36, 66, 62),  (126, 178, 166), (88, 116, 96), (180, 206, 166), (130, 230, 210)),
    ("filo",   "hoja2",   (28, 50, 34),  (100, 146, 92), (76, 96, 50),   (158, 178, 96),  (96, 222, 140)),
    ("fern",   "rizo",    (20, 50, 48),  (84, 150, 136), (60, 90, 72),   (140, 180, 140), (86, 220, 214)),
    ("orqui",  "flor",    (48, 32, 50),  (158, 110, 150), (88, 66, 84),  (196, 150, 182), (255, 130, 200)),
    ("cala",   "cinta",   (40, 34, 56),  (132, 120, 170), (78, 70, 90),  (172, 158, 184), (180, 120, 255)),
    ("lyra",   "hojaxl",  (32, 44, 26),  (112, 140, 80), (80, 88, 52),   (166, 176, 110), (200, 230, 120)),
    ("bonz",   "ramita",  (44, 34, 24),  (140, 112, 78), (66, 80, 44),   (146, 164, 88),  (255, 190, 70)),
]

PALETTES = [build_palette(c[2], c[3], c[4], c[5], c[6]) for c in COMPANIONS]


# --------------------------------------------------------------- lienzo ---
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

    def leaf(self, x0, y0, x1, y1, ancho, v, nervio=None):
        """Una hoja: lente puntiaguda en los dos extremos, de x0,y0 a x1,y1.

        Recorre el segmento y pinta tramos perpendiculares cuyo semiancho
        sigue una parábola, así las puntas cierran en un pixel. Es lo que
        separa una hoja de una elipse: a 32x32 una elipse horizontal se lee
        como ala de sombrero, y la punta es lo que la vuelve vegetal.
        """
        dx, dy = x1 - x0, y1 - y0
        largo = max(abs(dx), abs(dy))
        if largo == 0:
            return
        # Se muestrea al doble de la resolución del pixel: con un paso de uno
        # las diagonales dejan huecos entre tramos perpendiculares, y después
        # outline() los pinta de contorno y la hoja sale rayada.
        n = largo * 3
        px, py = -dy / float(largo), dx / float(largo)
        for i in range(n + 1):
            t = i / float(n)
            cx, cy = x0 + dx * t, y0 + dy * t
            k = ancho * (4.0 * t * (1.0 - t)) ** 0.75
            j = -k
            while j <= k + 0.001:
                self.set(int(round(cx + px * j)), int(round(cy + py * j)), v)
                j += 0.5
        # La nervadura sólo en hojas anchas: sobre una de dos pixeles se come
        # la hoja entera y queda un palito.
        if nervio is not None and ancho >= 3:
            self.line(x0, y0, x1, y1, nervio)

    def line(self, x0, y0, x1, y1, v):
        """Bresenham. Los copetes son casi todos tallos y nervaduras."""
        dx, dy = abs(x1 - x0), abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            self.set(x0, y0, v)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

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


# --------------------------------------------------------------- adulto ---
def build_adulto():
    """El cuerpo del adulto, sin ojos ni boca: esos van como overlays.

    Toda la geometría está en pixeles del lienzo de 96x72. El bicho ocupa
    deliberadamente ~60% del ancho de la escena: es el protagonista, no un
    icono de estado.
    """
    g = Grid(ADULTO_W, ADULTO_H)
    cx = AD_HEAD_CX

    # -- patas traseras, detrás de todo ------------------------------------
    for sx in (28, 68):
        g.ellipse(sx, 63, 8, 5, SK_DARK)

    # -- caparazón ----------------------------------------------------------
    shell_cy, shell_rx, shell_ry = 32, 39, 23
    g.ellipse(cx, shell_cy, shell_rx, shell_ry, SH_MID)

    # Luz desde arriba a la izquierda, en tres escalones.
    for y in range(ADULTO_H):
        for x in range(ADULTO_W):
            if g.get(x, y) != SH_MID:
                continue
            t = ((x - (cx - 24)) / 66.0) * 0.5 + ((y - 9) / 45.0) * 0.5
            if t < 0.26:
                g.set(x, y, SH_LIGHT)
            elif t > 0.66:
                g.set(x, y, SH_DARK)

    # -- escudos ------------------------------------------------------------
    # El contorno va en sombra, no en acento: el acento se reserva para el
    # arco superior de cada placa, donde "toma luz". Así el bicho parece
    # tener circuitería bajo el caparazón en vez de aros de neón encima.
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
    g.ellipse(cx, AD_HEAD_CY, 20, 17, SK_MID)
    g.ellipse(cx, AD_HEAD_CY - 3, 18, 14, SK_LIGHT)   # frente iluminada
    g.ellipse(cx, AD_HEAD_CY + 9, 11, 5, BELLY)       # mentón
    g.ellipse(cx - 11, AD_HEAD_CY - 8, 6, 4, BELLY)   # reflejo en la mejilla

    # Fosas nasales: dos pixeles que suman muchísimo.
    for dy in (0, 1):
        g.set(cx - 5, AD_HEAD_CY + 3 + dy, SK_DARK)
        g.set(cx + 5, AD_HEAD_CY + 3 + dy, SK_DARK)

    g.outline()
    return g


# --------------------------------------------------------------- copetes --
# Cada copete ocupa la franja y = 0..9 del lienzo del brote, por encima del
# caparazón. Se dibuja ANTES que el caparazón y la cabeza, así que lo que
# quede por debajo se tapa solo: el copete nace de atrás del caparazón.
def copete_hoja2(g):
    """Dos hojas abiertas en V, la de la izquierda por delante."""
    g.line(16, 9, 16, 4, SK_DARK)
    g.leaf(16, 6, 23, 1, 2, SK_MID, SK_DARK)
    g.leaf(16, 7, 9, 1, 2, SK_LIGHT, SK_DARK)
    g.set(9, 1, ACCENT)
    g.set(23, 1, ACCENT)


def copete_hoja1(g):
    g.leaf(16, 9, 16, 0, 3, SK_MID)
    g.leaf(16, 9, 16, 1, 2, SK_LIGHT)
    g.line(16, 8, 16, 2, SK_DARK)
    g.set(16, 0, ACCENT)


def copete_hojaxl(g):
    """La hoja del ficus: ancha, con nervaduras marcadas."""
    g.leaf(16, 9, 16, 0, 5, SK_MID)
    g.leaf(16, 9, 16, 1, 3, SK_LIGHT)
    g.line(16, 9, 16, 1, SK_DARK)
    for y, dx in ((3, 2), (5, 3), (7, 3)):
        g.set(16 - dx, y, ACCENT)
        g.set(16 + dx, y, ACCENT)


def copete_espinas(g):
    for x, alto in ((11, 3), (16, 0), (21, 3)):
        g.line(x, 9, x, alto, SH_DARK)
        g.set(x, alto, ACCENT)
        g.set(x - 1, alto + 2, SH_DARK)
        g.set(x + 1, alto + 2, SH_DARK)


def copete_esporas(g):
    """Sombrerito de hongo y esporas soltándose."""
    g.line(16, 9, 16, 6, SK_DARK)
    g.ellipse(16, 6, 6, 3, SH_LIGHT, y_to=6)
    g.ellipse_ring(16, 6, 6, 3, SH_DARK, thick=1.2)
    for x, y in ((9, 2), (16, 0), (23, 2), (12, 0), (20, 1)):
        g.set(x, y, ACCENT)


def copete_punta(g):
    """La lengua de suegra: una hoja recta, alta, con el borde encendido."""
    g.leaf(16, 9, 16, 0, 2, SK_MID)
    g.line(16, 9, 16, 1, SK_LIGHT)
    for y in range(2, 9):
        for x in (15, 17):
            if g.get(x, y) != TRANSPARENT and g.get(x, y) != SK_LIGHT:
                g.set(x, y, ACCENT)


def copete_abanico(g):
    """Roseta de aloe: cuatro hojas carnosas desde un mismo punto."""
    for x1, y1, ancho in ((6, 4, 2), (12, 0, 2), (20, 0, 2), (26, 4, 2)):
        g.leaf(16, 9, x1, y1, ancho, SK_MID, SK_DARK)
    g.leaf(16, 9, 16, 1, 2, SK_LIGHT, SK_DARK)
    for x, y in ((6, 4), (12, 0), (20, 0), (26, 4)):
        g.set(x, y, ACCENT)


def copete_rizo(g):
    """Báculo de helecho: el rizo que todavía no se abrió."""
    g.line(16, 9, 16, 5, SK_DARK)
    g.line(16, 5, 19, 4, SK_DARK)
    g.ellipse_ring(20, 3, 4, 3, SK_LIGHT, thick=1.4)
    g.ellipse(20, 3, 1, 1, ACCENT)


def copete_flor(g):
    g.line(16, 9, 16, 6, SK_DARK)
    for dx, dy in ((0, -3), (-4, -1), (4, -1), (-3, 2), (3, 2)):
        g.ellipse(16 + dx, 4 + dy, 2, 2, ACCENT)
    g.ellipse(16, 4, 2, 2, SH_HI)


def copete_cinta(g):
    """Calathea: hojas largas que se enroscan. Se mueven de noche."""
    g.leaf(16, 9, 7, 3, 2, SK_MID)
    g.leaf(16, 9, 25, 1, 2, SK_LIGHT)
    g.set(7, 3, ACCENT)
    g.set(25, 1, ACCENT)


def copete_ramita(g):
    """Bonsái: tronco corto y tres copas. Un siglo en veinte centímetros."""
    g.line(16, 9, 16, 4, SH_DARK)
    g.line(16, 7, 11, 5, SH_DARK)
    g.line(16, 5, 21, 3, SH_DARK)
    for x, y in ((9, 4), (22, 2), (16, 3)):
        g.ellipse(x, y, 3, 2, SK_MID)
        g.ellipse(x, y - 1, 2, 1, SK_LIGHT)
        g.set(x, y - 1, ACCENT)


COPETES = {
    "hoja2": copete_hoja2, "hoja1": copete_hoja1, "hojaxl": copete_hojaxl,
    "espinas": copete_espinas, "esporas": copete_esporas, "punta": copete_punta,
    "abanico": copete_abanico, "rizo": copete_rizo, "flor": copete_flor,
    "cinta": copete_cinta, "ramita": copete_ramita,
}


# ---------------------------------------------------------------- brote ---
def build_brote(copete, marca):
    """El cuerpo del brote: cabeza grande, caparazón chico, copete propio.

    `marca` (0, 1 o 2) cambia el punteado de acento sobre el caparazón. Es
    variación gratis: a 32x32 tres puntos en distinta disposición ya
    distinguen dos criaturas de la misma familia cromática.
    """
    g = Grid(BROTE_W, BROTE_H)

    COPETES[copete](g)

    # -- caparazón ----------------------------------------------------------
    # Más ANCHO que la cabeza, no sólo más alto: si sólo asomara por arriba
    # se leería como ala de sombrero. Asomando también por los costados se
    # lee como caparazón, que es lo que hace de esto una cría del adulto.
    g.ellipse(16, 16, 13, 9, SH_MID)
    for y in range(BROTE_H):
        for x in range(BROTE_W):
            if g.get(x, y) != SH_MID:
                continue
            t = ((x - 3) / 26.0) * 0.5 + ((y - 7) / 18.0) * 0.5
            if t < 0.28:
                g.set(x, y, SH_LIGHT)
            elif t > 0.66:
                g.set(x, y, SH_DARK)
    g.ellipse_ring(16, 16, 13, 9, SH_DARK, thick=1.5)

    # Punteado de acento: la "circuitería" bajo el caparazón, en miniatura.
    # Tres disposiciones distintas separan dos criaturas de la misma familia
    # cromática sin gastar un solo byte más de arte.
    puntos = ([(7, 15), (16, 9), (25, 15)],
              [(10, 11), (22, 11), (16, 9)],
              [(6, 13), (16, 8), (26, 13)])[marca % 3]
    for x, y in puntos:
        g.set(x, y, ACCENT)

    # -- patitas ------------------------------------------------------------
    g.ellipse(9, 29, 4, 2, SK_DARK)
    g.ellipse(23, 29, 4, 2, SK_DARK)

    # -- cabeza, que es casi todo el bicho ----------------------------------
    g.ellipse(BR_HEAD_CX, BR_HEAD_CY, 9, 8, SK_MID)
    g.ellipse(BR_HEAD_CX, BR_HEAD_CY - 2, 8, 6, SK_LIGHT)
    g.ellipse(BR_HEAD_CX, BR_HEAD_CY + 5, 5, 3, BELLY)      # hocico
    g.ellipse(BR_HEAD_CX - 5, BR_HEAD_CY - 5, 3, 2, BELLY)  # reflejo

    g.outline()
    return g


# ------------------------------------------------------- expresiones -------
# '.' transparente, 'w' blanco, 'k' pupila, 'X' contorno, 'c' acento
#
# Las del adulto son 11x9; las del brote 7x7. No son las mismas reducidas:
# a 7x7 cada pixel es el 2% del ojo, así que las formas se rediseñan para
# que la silueta siga leyéndose.
EYES_AD = {
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

EYES_BR = {
    "OPEN": [
        "..XXX..",
        ".XwwwX.",
        "XwwkkwX",
        "XwkkkwX",
        "XwkkkwX",
        ".XwwwX.",
        "..XXX..",
    ],
    "BLINK": [
        ".......",
        ".......",
        "XX...XX",
        ".XXXXX.",
        ".......",
        ".......",
        ".......",
    ],
    "HAPPY": [
        ".......",
        "..XXX..",
        ".XX.XX.",
        "XX...XX",
        ".......",
        ".......",
        ".......",
    ],
    "WIDE": [
        ".XXXXX.",
        "XwwwwwX",
        "XwkkkwX",
        "XkkkkkX",
        "XwkkkwX",
        "XwwwwwX",
        ".XXXXX.",
    ],
    "SLEEPY": [
        ".......",
        "..XXX..",
        ".XwwwX.",
        "XwkkkwX",
        "XXXXXXX",
        ".......",
        ".......",
    ],
    "DEAD": [
        ".......",
        "XX...XX",
        ".XX.XX.",
        "..XXX..",
        ".XX.XX.",
        "XX...XX",
        ".......",
    ],
    "DIZZY": [
        ".XXXXX.",
        "XcccccX",
        "XcXXXcX",
        "XcXcXcX",
        "XcXXXcX",
        "XcccccX",
        ".XXXXX.",
    ],
    "GLITCH": [
        "XXXXXXX",
        "XcccXXX",
        "XXXcccX",
        "XcccXXX",
        "XXXcccX",
        "XcccXXX",
        "XXXXXXX",
    ],
}

MOUTHS_AD = {
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

MOUTHS_BR = {
    "SMILE": [
        "X.......X",
        ".X.....X.",
        "..XXXXX..",
        ".........",
    ],
    "FLAT": [
        ".........",
        "..XXXXX..",
        ".........",
        ".........",
    ],
    "FROWN": [
        "..XXXXX..",
        ".X.....X.",
        "X.......X",
        ".........",
    ],
    "OPEN": [
        "..XXXXX..",
        ".XkkkkkX.",
        ".XkkkkkX.",
        "..XXXXX..",
    ],
    "WAVY": [
        ".........",
        "X..X..X..",
        ".XX.XX.XX",
        ".........",
    ],
    "PANT": [
        ".XXXXXXX.",
        ".XkkkkkX.",
        ".XkkkkkX.",
        "..XXXXX..",
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
def write_png(path, grid, zoom=6, palette=None):
    pal = palette or PALETTES[0]
    w, h = grid.w * zoom, grid.h * zoom
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            v = grid.get(x // zoom, y // zoom)
            if v == TRANSPARENT:
                # damero para que se vea qué es transparente
                par = (x // zoom // 4 + y // zoom // 4) % 2
                c = (60, 60, 66) if par else (44, 44, 50)
            else:
                c = pal[v]
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


def write_png_multi(path, cells, zoom, cols, pad=2):
    """Una grilla de celdas, cada una con su propia paleta."""
    cw = max(g.w for g, _ in cells) + pad
    ch = max(g.h for g, _ in cells) + pad
    filas = (len(cells) + cols - 1) // cols
    big = Grid(cw * cols, ch * filas)
    colores = {}
    for i, (g, pal) in enumerate(cells):
        ox, oy = (i % cols) * cw + pad // 2, (i // cols) * ch + pad // 2
        for y in range(g.h):
            for x in range(g.w):
                v = g.get(x, y)
                if v != TRANSPARENT:
                    big.set(ox + x, oy + y, v)
                    colores[(ox + x, oy + y)] = pal[v]

    w, h = big.w * zoom, big.h * zoom
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            gx, gy = x // zoom, y // zoom
            c = colores.get((gx, gy))
            if c is None:
                par = (gx // 4 + gy // 4) % 2
                c = (60, 60, 66) if par else (44, 44, 50)
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


def idx_array(name, grid, static=True):
    out = ["%sconst uint8_t %s[%d] = {" % ("static " if static else "", name,
                                           grid.w * grid.h)]
    for y in range(grid.h):
        row = grid.d[y * grid.w:(y + 1) * grid.w]
        out.append("    " + ",".join("%2d" % v for v in row) + ",")
    out.append("};")
    return "\n".join(out)


def main():
    if not os.path.isdir(ART):
        os.makedirs(ART)
    if not os.path.isdir(PREVIEW):
        os.makedirs(PREVIEW)

    adulto = build_adulto()
    brotes = [(c[0], build_brote(c[1], i)) for i, c in enumerate(COMPANIONS)]

    ojos_ad = [(k, grid_from_rows(v)) for k, v in sorted(EYES_AD.items())]
    ojos_br = [(k, grid_from_rows(v)) for k, v in sorted(EYES_BR.items())]
    bocas_ad = [(k, grid_from_rows(v)) for k, v in sorted(MOUTHS_AD.items())]
    bocas_br = [(k, grid_from_rows(v)) for k, v in sorted(MOUTHS_BR.items())]

    assert sorted(EYES_AD) == sorted(EYES_BR), "los dos juegos de ojos difieren"
    assert sorted(MOUTHS_AD) == sorted(MOUTHS_BR), "las bocas difieren"

    # ------------------------------------------------------------ cabecera
    h = [
        "/* Generado por tools/gen_art.py — no editar a mano.",
        " *",
        " * Dos escalas del mismo organismo: el ADULTO de 96x72 que vive en el",
        " * Prime, y el BROTE de 32x32 que vive en cada Mini. Doce paletas, una",
        " * por simbionte, en el mismo orden que rk_companion_table.",
        " */",
        "#ifndef ROOTKIT_SPRITES_H",
        "#define ROOTKIT_SPRITES_H",
        "",
        '#include "../gfx/fb.h"',
        "",
        "/* Una forma es un mapa de índices sin paleta. La paleta la pone quien",
        " * dibuja, según de qué simbionte se trate, y así los doce comparten",
        " * los mismos bytes de arte. */",
        "typedef struct {",
        "    uint8_t        w;",
        "    uint8_t        h;",
        "    const uint8_t *idx;",
        "} rk_shape_t;",
        "",
        "/* Arma el sprite que espera rk_blit, atando forma y paleta. */",
        "rk_sprite_t rk_shape_sprite(const rk_shape_t *s, const rk_color_t *pal);",
        "",
        "#define RK_PAL_LEN        %d" % PAL_LEN,
        "#define RK_PAL_COUNT      %d" % len(PALETTES),
        "extern const rk_color_t RK_PAL[RK_PAL_COUNT][RK_PAL_LEN];",
        "",
        "/* ------------------------------------------------------- adulto -- */",
        "#define RK_ADULTO_W       %d" % ADULTO_W,
        "#define RK_ADULTO_H       %d" % ADULTO_H,
        "#define RK_AD_HEAD_CX     %d" % AD_HEAD_CX,
        "#define RK_AD_HEAD_CY     %d" % AD_HEAD_CY,
        "#define RK_AD_EYE_DX      %d" % AD_EYE_DX,
        "#define RK_AD_EYE_DY      %d" % AD_EYE_DY,
        "#define RK_AD_MOUTH_DY    %d" % AD_MOUTH_DY,
        "",
        "extern const rk_shape_t rk_adulto_body;",
    ]
    for name, _ in ojos_ad:
        h.append("extern const rk_shape_t rk_ad_eye_%s;" % name.lower())
    for name, _ in bocas_ad:
        h.append("extern const rk_shape_t rk_ad_mouth_%s;" % name.lower())

    h += [
        "",
        "/* -------------------------------------------------------- brote -- */",
        "#define RK_BROTE_W        %d" % BROTE_W,
        "#define RK_BROTE_H        %d" % BROTE_H,
        "#define RK_BR_HEAD_CX     %d" % BR_HEAD_CX,
        "#define RK_BR_HEAD_CY     %d" % BR_HEAD_CY,
        "#define RK_BR_EYE_DX      %d" % BR_EYE_DX,
        "#define RK_BR_EYE_DY      %d" % BR_EYE_DY,
        "#define RK_BR_MOUTH_DY    %d" % BR_MOUTH_DY,
        "",
        "/* Un cuerpo por simbionte: mismo esqueleto, copete y punteado propios. */",
        "extern const rk_shape_t rk_brote_body[RK_PAL_COUNT];",
    ]
    for name, _ in ojos_br:
        h.append("extern const rk_shape_t rk_br_eye_%s;" % name.lower())
    for name, _ in bocas_br:
        h.append("extern const rk_shape_t rk_br_mouth_%s;" % name.lower())

    h += ["", "#endif /* ROOTKIT_SPRITES_H */", ""]
    with open(os.path.join(ART, "sprites.h"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(h))

    # --------------------------------------------------------------- datos
    c = ["/* Generado por tools/gen_art.py — no editar a mano. */",
         '#include "sprites.h"',
         "",
         "rk_sprite_t rk_shape_sprite(const rk_shape_t *s, const rk_color_t *pal)",
         "{",
         "    rk_sprite_t out;",
         "    out.w    = s->w;",
         "    out.h    = s->h;",
         "    out.idx  = s->idx;",
         "    out.pal  = pal;",
         "    out.ncol = RK_PAL_LEN;",
         "    return out;",
         "}",
         "",
         "const rk_color_t RK_PAL[RK_PAL_COUNT][RK_PAL_LEN] = {"]
    for (cid, _copete, _sl, _sh, _kl, _kh, _ac), pal in zip(COMPANIONS, PALETTES):
        c.append("    { /* %-6s */" % cid)
        c.append("      " + " ".join("RK_RGB(%3d,%3d,%3d)," % p for p in pal[:5]))
        c.append("      " + " ".join("RK_RGB(%3d,%3d,%3d)," % p for p in pal[5:9]))
        c.append("      " + " ".join("RK_RGB(%3d,%3d,%3d)," % p for p in pal[9:]))
        c.append("    },")
    c += ["};", ""]

    def emit(prefix, pairs):
        for name, grid in pairs:
            vn = "%s_%s" % (prefix, name.lower())
            c.append(idx_array(vn + "_idx", grid))
            c.append("const rk_shape_t %s = { %d, %d, %s_idx };"
                     % (vn, grid.w, grid.h, vn))
            c.append("")

    c.append(idx_array("adulto_body_idx", adulto))
    c.append("const rk_shape_t rk_adulto_body = { %d, %d, adulto_body_idx };"
             % (adulto.w, adulto.h))
    c.append("")
    emit("rk_ad_eye", ojos_ad)
    emit("rk_ad_mouth", bocas_ad)

    for cid, grid in brotes:
        c.append(idx_array("brote_%s_idx" % cid, grid))
        c.append("")
    c.append("const rk_shape_t rk_brote_body[RK_PAL_COUNT] = {")
    for cid, grid in brotes:
        c.append("    { %d, %d, brote_%s_idx },   /* %s */"
                 % (grid.w, grid.h, cid, cid))
    c += ["};", ""]

    emit("rk_br_eye", ojos_br)
    emit("rk_br_mouth", bocas_br)

    with open(os.path.join(ART, "sprites.c"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(c))

    # ---------------------------------------------------------- previsuales
    write_png(os.path.join(PREVIEW, "adulto.png"), adulto, zoom=6,
              palette=PALETTES[0])
    write_png_multi(os.path.join(PREVIEW, "brotes.png"),
                    [(g, PALETTES[i]) for i, (_, g) in enumerate(brotes)],
                    zoom=6, cols=6, pad=4)

    # Las cuatro tiras: ojos y bocas del adulto arriba, del brote abajo.
    # Puestas una sobre otra se ve de un vistazo que son rediseños y no
    # reducciones, que es justo lo que hay que poder revisar.
    caras = Grid(8 * 14, 34)
    for fila, tira in enumerate((ojos_ad, bocas_ad, ojos_br, bocas_br)):
        oy = (0, 10, 17, 26)[fila]
        for i, (_n, gg) in enumerate(tira):
            for y in range(gg.h):
                for x in range(gg.w):
                    if gg.get(x, y) != TRANSPARENT:
                        caras.set(i * 14 + x, oy + y, gg.get(x, y))
    write_png(os.path.join(PREVIEW, "caras.png"), caras, zoom=8,
              palette=PALETTES[0])

    bytes_arte = (adulto.w * adulto.h
                  + sum(g.w * g.h for _, g in brotes)
                  + sum(g.w * g.h for _, g in ojos_ad + bocas_ad
                        + ojos_br + bocas_br))
    print("adulto     %dx%d, %d paletas" % (adulto.w, adulto.h, len(PALETTES)))
    print("brotes     %d de %dx%d, copetes: %s"
          % (len(brotes), BROTE_W, BROTE_H,
             ", ".join(sorted(set(c[1] for c in COMPANIONS)))))
    print("ojos       %s" % ", ".join(sorted(EYES_AD)))
    print("bocas      %s" % ", ".join(sorted(MOUTHS_AD)))
    print("arte total %d bytes de indices + %d de paletas"
          % (bytes_arte, len(PALETTES) * PAL_LEN * 2))
    print("escrito    firmware/art/sprites.{h,c}")
    print("previews   tools/preview/*.png")


if __name__ == "__main__":
    main()

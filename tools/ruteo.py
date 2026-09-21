# -*- coding: utf-8 -*-
"""ruteo.py — el ruteador del sustrato.

Convierte la netlist y la posición de los módulos en canaletas. Lo que antes
se dibujaba tramo por tramo a mano ahora sale de una búsqueda: cada red se
rutea sobre una grilla que ya tiene marcado todo lo que no se puede pisar
—el cobre de las otras redes, los agujeros, la ventana, los tornillos, la
zona libre de la antena— y lo que no encuentra camino se declara **puente**
de cable aislado, que es lo que haría un humano igual.

No reemplaza al verificador: lo alimenta. El ruteo se guarda en
`generado/ruteo.json` y `python3 tools/pcb.py --verificar` lo revisa entero
con las mismas reglas geométricas de siempre. Si el ruteador se equivoca,
salta ahí y no en la impresora.

    make rutear     vuelve a rutear (tarda unos minutos)
    make pcb        regenera todo lo demás a partir del ruteo commiteado

Por qué un ruteador y no seguir a mano: con la boquilla de 0,6 las paredes y
los agujeros crecieron, los pads del C3 pasaron a ir escalonados y la placa
se achicó. Cualquiera de esas tres cosas sola obliga a redibujar las 51
pistas. Teniéndolo, achicar la placa es cambiar dos números y volver a
correrlo.
"""

import array
import json
import math
import os

from pcb import dist_seg_rect as pcb_dist_seg_rect

INF = float("inf")


# --------------------------------------------------------------- distancias
def _edt_1d(f, n):
    """Distancia al cuadrado en una dimensión (Felzenszwalb & Huttenlocher).

    Es la pieza que hace que calcular "a qué distancia está el cobre más
    cercano" cueste una sola pasada por la grilla, en vez de comparar cada
    celda contra cada pista."""
    v = [0] * n
    z = [0.0] * (n + 1)
    d = [0.0] * n
    k = 0
    z[0] = -INF
    z[1] = INF
    for q in range(1, n):
        fq = f[q] + q * q
        while True:
            vk = v[k]
            s = (fq - (f[vk] + vk * vk)) / (2.0 * q - 2.0 * vk)
            if s <= z[k]:
                k -= 1
            else:
                break
        k += 1
        v[k] = q
        z[k] = s
        z[k + 1] = INF
    k = 0
    for q in range(n):
        while z[k + 1] < q:
            k += 1
        vk = v[k]
        d[q] = (q - vk) * (q - vk) + f[vk]
    return d


def _edt2(mask, w, h):
    """Distancia euclídea (en celdas) de cada celda a la celda marcada más
    cercana. mask es un bytearray de w*h con 1 donde hay algo."""
    grande = float((w + h) * (w + h))
    f = [0.0 if mask[i] else grande for i in range(w * h)]
    # columnas
    col = [0.0] * h
    for x in range(w):
        for y in range(h):
            col[y] = f[y * w + x]
        d = _edt_1d(col, h)
        for y in range(h):
            f[y * w + x] = d[y]
    # filas
    out = array.array("f", bytes(4 * w * h))
    fila = [0.0] * w
    for y in range(h):
        base = y * w
        for x in range(w):
            fila[x] = f[base + x]
        d = _edt_1d(fila, w)
        for x in range(w):
            out[base + x] = math.sqrt(d[x])
    return out


# ------------------------------------------------------------------ grilla
class Grilla:
    """La placa vista como celdas cuadradas de `paso` mm."""

    def __init__(self, ancho, alto, paso):
        self.paso = paso
        self.w = int(math.ceil(ancho / paso)) + 1
        self.h = int(math.ceil(alto / paso)) + 1

    def celda(self, x, y):
        return (int(round(x / self.paso)), int(round(y / self.paso)))

    def mm(self, ix, iy):
        return (ix * self.paso, iy * self.paso)

    def vacia(self):
        return bytearray(self.w * self.h)

    def marcar_rect(self, mask, cx, cy, aw, ah, rot=0.0, crecer=0.0):
        """Marca las celdas cuyo centro cae dentro del rectángulo."""
        aw += 2 * crecer
        ah += 2 * crecer
        r = math.radians(rot)
        co, si = math.cos(r), math.sin(r)
        rad = math.hypot(aw, ah) / 2.0
        x0 = max(0, int((cx - rad) / self.paso))
        x1 = min(self.w - 1, int((cx + rad) / self.paso) + 1)
        y0 = max(0, int((cy - rad) / self.paso))
        y1 = min(self.h - 1, int((cy + rad) / self.paso) + 1)
        for iy in range(y0, y1 + 1):
            dy = iy * self.paso - cy
            base = iy * self.w
            for ix in range(x0, x1 + 1):
                dx = ix * self.paso - cx
                u = dx * co + dy * si
                v = -dx * si + dy * co
                if abs(u) <= aw / 2.0 and abs(v) <= ah / 2.0:
                    mask[base + ix] = 1

    def marcar_circulo(self, mask, cx, cy, d):
        r = d / 2.0
        x0 = max(0, int((cx - r) / self.paso))
        x1 = min(self.w - 1, int((cx + r) / self.paso) + 1)
        y0 = max(0, int((cy - r) / self.paso))
        y1 = min(self.h - 1, int((cy + r) / self.paso) + 1)
        for iy in range(y0, y1 + 1):
            dy = iy * self.paso - cy
            base = iy * self.w
            for ix in range(x0, x1 + 1):
                dx = ix * self.paso - cx
                if dx * dx + dy * dy <= r * r:
                    mask[base + ix] = 1

    def marcar_seg(self, mask, a, b, ancho):
        r = ancho / 2.0
        x0 = max(0, int((min(a[0], b[0]) - r) / self.paso))
        x1 = min(self.w - 1, int((max(a[0], b[0]) + r) / self.paso) + 1)
        y0 = max(0, int((min(a[1], b[1]) - r) / self.paso))
        y1 = min(self.h - 1, int((max(a[1], b[1]) + r) / self.paso) + 1)
        for iy in range(y0, y1 + 1):
            py = iy * self.paso
            base = iy * self.w
            for ix in range(x0, x1 + 1):
                if _dist_punto_seg((ix * self.paso, py), a, b) <= r:
                    mask[base + ix] = 1

    def distancias(self, mask):
        return _edt2(mask, self.w, self.h)


def _dist_punto_seg(p, a, b):
    vx, vy = b[0] - a[0], b[1] - a[1]
    wx, wy = p[0] - a[0], p[1] - a[1]
    ll = vx * vx + vy * vy
    t = 0.0 if ll == 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / ll))
    return math.hypot(wx - t * vx, wy - t * vy)


# ------------------------------------------------------------------ camino
_DIRS = ((1, 0), (-1, 0), (0, 1), (0, -1))


def _camino(libre, w, h, fuentes, destinos, coste_giro):
    """Camino de coste mínimo entre dos conjuntos de celdas.

    Un paso cuesta 1 y doblar cuesta `coste_giro`. Minimizar los giros es lo
    que hace que las pistas salgan rectas en vez de en escalera, y eso acá no
    es estética: la cinta se corta con bisturí contra la pared de la canaleta,
    y una escalera son treinta cortes en vez de dos.

    Devuelve la lista de celdas (ix, iy) o None si no hay camino."""
    n = w * h
    dist = array.array("i", [-1]) * (4 * n)
    prev = array.array("i", [-1]) * (4 * n)
    cubetas = {}

    def meter(est, c, de):
        d = dist[est]
        if d != -1 and d <= c:
            return
        dist[est] = c
        prev[est] = de
        cubetas.setdefault(c, []).append(est)

    for celda in fuentes:
        for d in range(4):
            meter(celda * 4 + d, 0, -1)

    destino = set(destinos)
    c = 0
    tope = 4 * n
    while cubetas:
        if c not in cubetas:
            c += 1
            if c > tope:
                break
            continue
        cola = cubetas.pop(c)
        for est in cola:
            if dist[est] != c:
                continue
            celda = est >> 2
            if celda in destino:
                # reconstruir
                salida = []
                e = est
                while e != -1:
                    cc = e >> 2
                    salida.append((cc % w, cc // w))
                    e = prev[e]
                salida.reverse()
                return salida
            d = est & 3
            ix, iy = celda % w, celda // w
            for nd, (dx, dy) in enumerate(_DIRS):
                jx, jy = ix + dx, iy + dy
                if jx < 0 or jy < 0 or jx >= w or jy >= h:
                    continue
                j = jy * w + jx
                if not libre[j]:
                    continue
                meter(j * 4 + nd, c + 1 + (0 if nd == d else coste_giro), est)
        c += 1
    return None


def _a_polilinea(celdas, paso):
    """Junta los tramos rectos: una corrida de celdas en la misma dirección
    es un solo punto a punto."""
    pts = [(ix * paso, iy * paso) for ix, iy in celdas]
    if len(pts) < 2:
        return pts
    salida = [pts[0]]
    for i in range(1, len(pts) - 1):
        ax, ay = pts[i - 1]
        bx, by = pts[i]
        cx, cy = pts[i + 1]
        if (bx - ax, by - ay) != (cx - bx, cy - by):
            salida.append(pts[i])
    salida.append(pts[-1])
    return salida


# --------------------------------------------------------------- ruteador
class Ruteador:
    """Rutea la netlist sobre la placa. Lo que no entra, sale por puente."""

    def __init__(self, n, paso=0.20, coste_giro=12, charla=True):
        self.n = n
        self.paso = paso
        self.coste_giro = coste_giro
        self.charla = charla
        self.g = Grilla(n.sustrato["ancho"], n.sustrato["alto"], paso)
        self.idx_red = {r: i for i, r in enumerate(n.d["orden_redes"])}
        self._campos()
        self._sembrar_cobre()

    def _decir(self, txt):
        if self.charla:
            print("  " + txt)

    # -- lo que nunca se puede pisar ---------------------------------------
    def _campos(self):
        g = self.g
        s = self.n.sustrato

        r = s["radio_borde"]
        hx, hy = s["ancho"] / 2.0, s["alto"] / 2.0
        self.d_fuera = array.array("f", bytes(4 * g.w * g.h))
        for iy in range(g.h):
            py = abs(iy * g.paso - hy) - (hy - r)
            base = iy * g.w
            for ix in range(g.w):
                px = abs(ix * g.paso - hx) - (hx - r)
                # Distancia con signo a un rectangulo de esquinas redondeadas.
                d = (min(max(px, py), 0.0)
                     + math.hypot(max(px, 0.0), max(py, 0.0)) - r)
                self.d_fuera[base + ix] = max(0.0, -d) / g.paso

        huecos = g.vacia()
        v = s["ventana"]
        g.marcar_rect(huecos, v["x"], v["y"], v["ancho"], v["alto"])
        for rec in s.get("recortes", []):
            g.marcar_rect(huecos, rec["x"], rec["y"], rec["ancho"], rec["alto"])
        for t in s.get("tornillos", []):
            g.marcar_rect(huecos, t["x"], t["y"], t["d"], t["d"])
        self.d_hueco = g.distancias(huecos)

        rot = g.vacia()
        hay_rot = False
        for r in s.get("rotulos", []):
            g.marcar_rect(rot, r["x"], r["y"],
                          len(r["texto"]) * r["tam"] * 0.62,
                          r["tam"] * 1.25, r.get("rot", 0.0))
            hay_rot = True
        self.d_rotulo = g.distancias(rot) if hay_rot else None
        self.aire_rotulo = self.n.reglas.get("aire_rotulo", 0.6)

        ant = g.vacia()
        z = s["antena"]
        g.marcar_rect(ant, z["x"], z["y"], z["ancho"], z["alto"])
        self.d_antena = g.distancias(ant)
        self.libre_antena = z["libre"]

        mod = self.n.comps[z["modulo"]]
        h = self.n.huellas[mod["huella"]]
        self.sombra = g.vacia()
        g.marcar_rect(self.sombra, mod["pos"][0], mod["pos"][1],
                      h["contorno"][0] + 0.4, h["contorno"][1] + 0.4,
                      mod.get("rot", 0.0))

    # -- el cobre que ya hay -------------------------------------------------
    def _sembrar_cobre(self):
        g = self.g
        self.dueno = array.array("i", [-1]) * (g.w * g.h)
        self.pads_celdas = {}
        media = g.paso / 2.0
        self.pads_libres = {}
        for nodo, pad in self.n.pads.items():
            m = g.vacia()
            g.marcar_rect(m, pad.x, pad.y, pad.w, pad.h, pad.rot)
            celdas = [i for i, b in enumerate(m) if b]
            if not celdas:                       # pad más chico que una celda
                ix, iy = g.celda(pad.x, pad.y)
                celdas = [iy * g.w + ix]
            self.pads_celdas[nodo] = celdas
            libres = set(celdas)
            gordo = g.vacia()
            g.marcar_rect(gordo, pad.x, pad.y, pad.w, pad.h, pad.rot, crecer=media)
            if pad.agujero > 0:
                g.marcar_circulo(gordo, pad.hx, pad.hy, pad.agujero + g.paso)
                propio = g.vacia()
                g.marcar_circulo(propio, pad.hx, pad.hy, pad.agujero)
                libres |= {i for i, b in enumerate(propio) if b}
            self.pads_libres[nodo] = libres
            idx = self.idx_red[pad.red]
            for i, b in enumerate(gordo):
                if b:
                    self.dueno[i] = idx

    def _marcar_pista(self, red, puntos, ancho):
        g = self.g
        m = g.vacia()
        for a, b in zip(puntos[:-1], puntos[1:]):
            g.marcar_seg(m, a, b, ancho + g.paso)
        idx = self.idx_red[red]
        dueno = self.dueno
        for i, b in enumerate(m):
            if b and dueno[i] in (-1, idx):
                dueno[i] = idx
        return m

    # -- qué celdas puede pisar una red -------------------------------------
    def _libres(self, red, ancho, con_pads=True):
        g = self.g
        idx = self.idx_red[red]
        otros = g.vacia()
        dueno = self.dueno
        for i in range(g.w * g.h):
            o = dueno[i]
            if o != -1 and o != idx:
                otros[i] = 1
        d_otros = g.distancias(otros)

        reglas = self.n.reglas
        sep = reglas["separacion_min"]
        holg = reglas["holgura_canaleta"]
        marg = reglas["margen_borde"]
        paso = g.paso
        # Una celda de más de margen: la grilla mide al centro de la celda y
        # el cobre de verdad llega hasta el borde.
        u_otros = (ancho / 2.0 + holg + sep) / paso + 1.0
        u_hueco = (ancho / 2.0 + sep) / paso + 1.0
        u_borde = (ancho / 2.0 + marg) / paso
        u_ant = self.libre_antena / paso + 1.0
        u_rot = (ancho / 2.0 + self.aire_rotulo) / paso + 1.0

        libre = g.vacia()
        d_hueco, d_fuera, d_ant, sombra = (self.d_hueco, self.d_fuera,
                                           self.d_antena, self.sombra)
        d_rot = self.d_rotulo
        for i in range(g.w * g.h):
            if d_otros[i] < u_otros:
                continue
            if d_hueco[i] < u_hueco:
                continue
            if d_fuera[i] < u_borde:
                continue
            if d_ant[i] < u_ant and not sombra[i]:
                continue
            if d_rot is not None and d_rot[i] < u_rot:
                continue
            libre[i] = 1
        # Los pads de la propia red siempre se pueden pisar: ahí termina la
        # pista, y el agujero del pad es suyo.
        if con_pads:
            for nodo in self.n.d["redes"][red]["nodos"]:
                for i in self.pads_libres.get(nodo, ()):
                    libre[i] = 1
        return libre

    def _partir_en_sombra(self, pts):
        z = self.n.sustrato["antena"]
        mod = self.n.comps[z["modulo"]]
        h = self.n.huellas[mod["huella"]]
        if mod.get("rot", 0.0) % 180 != 0:
            return pts
        cx, cy = mod["pos"]
        aw = (h["contorno"][0] + 0.4) / 2.0
        ah = (h["contorno"][1] + 0.4) / 2.0
        bordes_x = (cx - aw, cx + aw)
        bordes_y = (cy - ah, cy + ah)
        salida = [pts[0]]
        for a, b in zip(pts[:-1], pts[1:]):
            cortes = []
            for v in bordes_x:
                if (a[0] - v) * (b[0] - v) < 0:
                    t = (v - a[0]) / (b[0] - a[0])
                    cortes.append((t, (v, a[1] + t * (b[1] - a[1]))))
            for v in bordes_y:
                if (a[1] - v) * (b[1] - v) < 0:
                    t = (v - a[1]) / (b[1] - a[1])
                    cortes.append((t, (a[0] + t * (b[0] - a[0]), v)))
            for _, q in sorted(cortes):
                salida.append((round(q[0], 3), round(q[1], 3)))
            salida.append(b)
        return _limpiar_sin_colineales(salida)

    def _partir_por_ancho(self, celdas, ancho, fino, estricto):
        """Parte el camino en corridas de riel y ramal.

        Un riel de 2,4 mm no puede aterrizar en el pad de 1,2 mm de un SOT-23
        sin pasar a 1,7 mm del pad de al lado. Pero angostar el tramo entero
        estrangula el riel. Asi que el camino se corta: mientras aguanta el
        ancho de riel va riel, y las puntas, que son las que entran al pad,
        van finas. Es exactamente lo que se hacia a mano."""
        w = self.g.w
        gordo = [bool(estricto[iy * w + ix]) for ix, iy in celdas]
        corridas = []
        i = 0
        while i < len(celdas):
            j = i
            while j + 1 < len(celdas) and gordo[j + 1] == gordo[i]:
                j += 1
            corridas.append((gordo[i], i, j))
            i = j + 1
        # Las corridas se solapan para que el cobre quede unido, y la que
        # se estira es la FINA: estirar la ancha la metia una celda dentro
        # del pad, que es justo donde no entra.
        return [(ancho, celdas[a:b + 1]) if g
                else (fino, celdas[max(0, a - 1):b + 2])
                for g, a, b in corridas]

    def _mascara_cable(self):
        """Por donde puede ir un cable de puente.

        El cable va por la cara de los modulos y esta aislado, asi que puede
        pasar por encima de todo el cobre que quiera. Lo unico que no puede
        es cruzar un agujero pasante: ahi entra la pantalla, la antena o un
        tornillo, y el cable quedaria apretado."""
        if getattr(self, "_cable", None) is None:
            g = self.g
            luz = 1.5 / g.paso
            m = g.vacia()
            for i in range(g.w * g.h):
                if self.d_hueco[i] >= luz and self.d_fuera[i] >= luz:
                    m[i] = 1
            self._cable = m
        return self._cable

    def _rodeo(self, de, a):
        """Los puntos por los que pasa un puente que no puede ir derecho."""
        p, q = self.n.pads[de], self.n.pads[a]
        huecos = self.n.huecos()
        if not any(pcb_dist_seg_rect((p.x, p.y), (q.x, q.y), esq) <= 0.0
                   for _, esq in huecos):
            return None
        g = self.g
        libre = self._mascara_cable()
        fuentes = [i for i in self.pads_celdas[de] if libre[i]]
        destinos = [i for i in self.pads_celdas[a] if libre[i]]
        if not fuentes or not destinos:
            return None
        celdas = _camino(libre, g.w, g.h, fuentes, destinos, self.coste_giro)
        if celdas is None:
            return None
        pts = _limpiar(_a_polilinea(celdas, g.paso))
        return [[round(x, 2), round(y, 2)] for x, y in pts[1:-1]] or None

    def _de_donde_colgar(self, hechos, objetivo):
        """De que pad ya conectado sale el puente, si hace falta.

        El mas cercano, pero saltando los que obligarian al cable a cruzar la
        ventana, la muesca o un tornillo: por ahi pasa la pantalla, y un cable
        cruzado queda apretado entre el modulo y el plastico. Si todos cruzan,
        se elige el mas cercano igual y lo denuncia el verificador."""
        p = self.n.pads[objetivo]
        huecos = self.n.huecos()

        def cruza(h):
            q = self.n.pads[h]
            return any(pcb_dist_seg_rect((q.x, q.y), (p.x, p.y), esq) <= 0.0
                       for _, esq in huecos)

        limpios = [h for h in hechos if not cruza(h)]
        candidatos = limpios or hechos
        return min(candidatos, key=lambda h: math.dist(
            (self.n.pads[h].x, self.n.pads[h].y), (p.x, p.y)))

    def _ancho_tramo(self, base, *nodos):
        """El ancho con el que se puede aterrizar.

        Una pista de 2,4 mm que baja a un pad de 1,2 mm de un SOT-23 pasa por
        fuerza a 1,7 mm del pad de al lado, que esta a 2,3 mm: no hay forma de
        que entre. Asi que cada tramo se rutea con el ancho del pad mas
        angosto que toca, nunca por debajo del piso de senal. Es lo mismo que
        se hacia a mano: rieles anchos y ramales finos."""
        piso = self.n.reglas["ancho_minimo"]["senal"]
        for nodo in nodos:
            pad = self.n.pads[nodo]
            base = min(base, pad.w, pad.h)
        return round(max(base, piso), 3)

    def _ancho_ruteo(self, red):
        """Con que ancho se rutea una red.

        El nominal (4 mm en los rieles) pide 3,1 mm de despeje a cada lado y
        no deja pasar nada; el verificador solo exige que cada riel de
        potencia tenga un tramo de su ancho minimo. Asi que se rutea al
        minimo de la clase, que es el ancho con el que la placa entra."""
        info = self.n.d["redes"][red]
        clase = info["clase"]
        r = self.n.reglas
        if clase == "potencia":
            return info.get("ancho_minimo", r["ancho_minimo"]["potencia"])
        return r["ancho_minimo"].get(clase, r["ancho_minimo"]["senal"])

    # -- el ruteo ------------------------------------------------------------
    def rutear(self):
        n = self.n
        pistas, puentes, informe = [], [], []

        for p in n.d.get("pistas_fijas", []):
            red = p["red"]
            ancho = p.get("ancho") or n.ancho_de(red)
            pts = [tuple(q) for q in p["puntos"]]
            pistas.append({"red": red, "ancho": ancho,
                           "puntos": [list(q) for q in pts], "fija": True})
            self._marcar_pista(red, pts, ancho)

        for red in n.d["orden_redes"]:
            info = n.d["redes"][red]
            nodos = [nd for nd in info["nodos"] if nd in self.pads_celdas]
            if len(nodos) < 2:
                continue
            ancho_red = self._ancho_ruteo(red)
            cache = {}

            def libres(a):
                if a not in cache:
                    cache[a] = self._libres(red, a)
                return cache[a]

            cache_e = {}

            def estrictos(a):
                if a not in cache_e:
                    cache_e[a] = self._libres(red, a, con_pads=False)
                return cache_e[a]

            g = self.g
            conect = g.vacia()
            for p in pistas:
                if p["red"] == red and p.get("fija"):
                    m = g.vacia()
                    for a, b in zip(p["puntos"][:-1], p["puntos"][1:]):
                        g.marcar_seg(m, a, b, p["ancho"] + g.paso)
                    for i, b in enumerate(m):
                        if b:
                            conect[i] = 1
            hechos = [nodos[0]]
            for i in self.pads_celdas[nodos[0]]:
                conect[i] = 1
            restantes = list(nodos[1:])
            tramos = saltos = 0

            while restantes:
                def lejos(nd):
                    p = n.pads[nd]
                    return min(math.dist((p.x, p.y), (n.pads[h].x, n.pads[h].y))
                               for h in hechos)
                objetivo = min(restantes, key=lejos)
                restantes.remove(objetivo)
                cerca = self._de_donde_colgar(hechos, objetivo)
                fino = self._ancho_tramo(ancho_red, cerca, objetivo)
                fuentes = [i for i in range(g.w * g.h) if conect[i]]
                celdas = _camino(libres(ancho_red), g.w, g.h, fuentes,
                                 self.pads_celdas[objetivo], self.coste_giro)
                usado = ancho_red
                if celdas is None and fino < ancho_red:
                    # Un riel de 2,4 mm que no encuentra paso quiza sí entre
                    # de 1,2. Un ramal fino es mejor que un puente: el puente
                    # es un cable mas que hay que cortar, pelar y soldar dos
                    # veces. El riel ancho lo garantiza igual el otro tramo.
                    celdas = _camino(libres(fino), g.w, g.h, fuentes,
                                     self.pads_celdas[objetivo], self.coste_giro)
                    usado = fino
                if celdas is None:
                    pu = {"red": red, "de": cerca, "a": objetivo,
                          "porque": "no quedaba paso por cobre"}
                    rodeo = self._rodeo(cerca, objetivo)
                    if rodeo:
                        pu["por"] = rodeo
                        pu["porque"] = ("no quedaba paso por cobre, y el cable "
                                        "tiene que rodear la ventana")
                    puentes.append(pu)
                    saltos += 1
                else:
                    pad = n.pads[objetivo]
                    centro = (round(pad.x, 3), round(pad.y, 3))
                    trozos = self._partir_por_ancho(
                        celdas, usado, min(fino, usado), estrictos(usado))
                    ultima = (g.mm(*celdas[-1]), centro)
                    if math.dist(*ultima) > 1e-9:
                        trozos.append((self._ancho_tramo(ancho_red, objetivo),
                                       None))
                    puestos = 0
                    for a, sub in trozos:
                        pts = ([round(ultima[0][0], 3), round(ultima[0][1], 3)],
                               centro) if sub is None else _a_polilinea(sub, g.paso)
                        pts = self._partir_en_sombra(_limpiar([tuple(q) for q in pts]))
                        if len(pts) < 2:
                            continue
                        pistas.append({"red": red, "ancho": a,
                                       "puntos": [[round(u, 3), round(v2, 3)]
                                                  for u, v2 in pts]})
                        m = self._marcar_pista(red, pts, a)
                        for i, b in enumerate(m):
                            if b:
                                conect[i] = 1
                        puestos += 1
                    tramos += puestos
                for i in self.pads_celdas[objetivo]:
                    conect[i] = 1
                hechos.append(objetivo)

            informe.append((red, len(nodos), tramos, saltos))
            self._decir("%-9s %2d nodos  %2d pistas  %s"
                        % (red, len(nodos), tramos,
                           "%d puentes" % saltos if saltos else "sin puentes"))
        return pistas, puentes, informe


def _limpiar_sin_colineales(pts):
    """Saca repetidos pero deja los puntos colineales: los cortes en el borde
    de la sombra de la antena son colineales a proposito y si se los junta
    vuelve a aparecer el segmento que cruza."""
    out = []
    for q in pts:
        if not out or math.dist(out[-1], q) > 1e-9:
            out.append(q)
    return out


def _limpiar(pts):
    """Saca puntos repetidos y colineales que quedaron del enganche al pad."""
    out = []
    for p in pts:
        if not out or math.dist(out[-1], p) > 1e-9:
            out.append(p)
    i = 1
    while i < len(out) - 1:
        a, b, c = out[i - 1], out[i], out[i + 1]
        if abs((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) < 1e-9:
            del out[i]
        else:
            i += 1
    return out


# ------------------------------------------------------------------ archivo
def rutear_y_guardar(n, destino, paso=0.20, coste_giro=12, charla=True):
    r = Ruteador(n, paso=paso, coste_giro=coste_giro, charla=charla)
    pistas, puentes, informe = r.rutear()
    doc = {
        "que_es": ("El ruteo del sustrato: lo genera tools/ruteo.py desde "
                   "nucleo.json y lo revisa entero tools/pcb.py --verificar. "
                   "No se edita a mano: se vuelve a correr 'make rutear'."),
        "paso_de_grilla": paso,
        "pistas": pistas,
        "puentes": puentes,
    }
    os.makedirs(os.path.dirname(destino), exist_ok=True)
    # newline="
" a proposito: sin eso, en Windows el archivo sale con
    # CRLF y el repo marca los generados como modificados aunque el ruteo
    # sea identico, con lo que CI pide "correr make pcb y commitear" por
    # un cambio que no existe.
    with open(destino, "w", encoding="utf-8", newline="
") as f:
        json.dump(doc, f, ensure_ascii=False, indent=1)
        f.write("\n")
    return pistas, puentes, informe

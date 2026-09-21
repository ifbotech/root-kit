#!/usr/bin/env python3
"""pcb.py — el sustrato impreso de ROOTKIT, generado y verificado desde un dato.

La fuente de verdad es `hardware/pcb/nucleo.json`: los modulos, donde va cada
uno, las redes y por donde corre cada pista. De ese archivo salen, sin tocar
nada a mano:

  hardware/pcb/generado/ruteo.json            por donde corre cada pista
  hardware/pcb/generado/sustrato_datos.scad   los datos que come sustrato.scad
  hardware/pcb/generado/plantilla-cinta.svg   la plantilla 1:1 para cortar cinta
  hardware/pcb/generado/nucleo-sustrato.stl   la pieza para imprimir (--stl)
  hardware/pcb/generado/nucleo-estampadora.stl  su negativo, para meter la cinta
  docs/conexiones.md                          el diagrama de conexiones
  firmware/test/redes.h                       la netlist para la prueba en C

Uso:

  python3 tools/pcb.py --verificar    solo revisa (lo que corre CI)
  python3 tools/pcb.py --generar      revisa y reescribe los generados
  python3 tools/pcb.py --stl          ademas exporta los STL con OpenSCAD
  python3 tools/pcb.py --encaje       comprueba que la estampadora entre
  python3 tools/pcb.py --rutear       vuelve a rutear (tarda; ver tools/ruteo.py)

Sin dependencias: solo la biblioteca estandar, igual que tools/fabrica.py.
"""

import argparse
import json
import math
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATO = os.path.join(RAIZ, "hardware", "pcb", "nucleo.json")
GEN = os.path.join(RAIZ, "hardware", "pcb", "generado")
SCAD = os.path.join(RAIZ, "hardware", "pcb", "sustrato.scad")
ESTAMPADORA = os.path.join(RAIZ, "hardware", "pcb", "estampadora.scad")
ENCAJE = os.path.join(RAIZ, "hardware", "pcb", "encaje.scad")
REDES_H = os.path.join(RAIZ, "firmware", "test", "redes.h")
CONEXIONES = os.path.join(RAIZ, "docs", "conexiones.md")
RUTEO = os.path.join(GEN, "ruteo.json")


def cargar():
    """El dato a mano mas el ruteo generado, que viven en archivos distintos.

    En nucleo.json esta lo que se decide: que modulo va donde y que va
    conectado con que. En generado/ruteo.json, por donde corre cada pista,
    que lo escribe el ruteador. Separarlos es lo que permite mover un modulo
    y que las 51 pistas se rehagan solas, en vez de redibujarlas a mano."""
    with open(DATO, encoding="utf-8") as f:
        d = json.load(f)
    if os.path.exists(RUTEO):
        with open(RUTEO, encoding="utf-8") as f:
            r = json.load(f)
        d["pistas"] = r.get("pistas", [])
        d["puentes"] = r.get("puentes", [])
    d.setdefault("pistas", [])
    d.setdefault("puentes", [])
    return d


# --------------------------------------------------------------- geometria --
def dist_punto_seg(p, a, b):
    ax, ay = a
    bx, by = b
    px, py = p
    dx, dy = bx - ax, by - ay
    largo2 = dx * dx + dy * dy
    if largo2 <= 1e-12:
        return math.hypot(px - ax, py - ay)
    t = ((px - ax) * dx + (py - ay) * dy) / largo2
    t = max(0.0, min(1.0, t))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def _cruzan(a, b, c, d):
    def lado(p, q, r):
        return (q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])

    d1, d2 = lado(c, d, a), lado(c, d, b)
    d3, d4 = lado(a, b, c), lado(a, b, d)
    return ((d1 > 0) != (d2 > 0)) and ((d3 > 0) != (d4 > 0))


def dist_seg_seg(a, b, c, d):
    """Distancia minima entre dos segmentos."""
    if _cruzan(a, b, c, d):
        return 0.0
    return min(
        dist_punto_seg(a, c, d),
        dist_punto_seg(b, c, d),
        dist_punto_seg(c, a, b),
        dist_punto_seg(d, a, b),
    )


def rect_esquinas(cx, cy, w, h, rot=0.0):
    r = math.radians(rot)
    co, si = math.cos(r), math.sin(r)
    out = []
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        x, y = sx * w / 2.0, sy * h / 2.0
        out.append((cx + x * co - y * si, cy + x * si + y * co))
    return out


def rect_lados(esq):
    return [(esq[i], esq[(i + 1) % 4]) for i in range(4)]


def punto_en_rect(p, esq):
    signo = None
    for i in range(4):
        (x1, y1), (x2, y2) = esq[i], esq[(i + 1) % 4]
        cruz = (x2 - x1) * (p[1] - y1) - (y2 - y1) * (p[0] - x1)
        if abs(cruz) < 1e-12:
            continue
        s = cruz > 0
        if signo is None:
            signo = s
        elif signo != s:
            return False
    return True


def dist_seg_rect(a, b, esq):
    """0 si el segmento toca el rectangulo; si no, la distancia al borde."""
    if punto_en_rect(a, esq) or punto_en_rect(b, esq):
        return 0.0
    return min(dist_seg_seg(a, b, p, q) for p, q in rect_lados(esq))


def dist_rect_rect(e1, e2):
    if any(punto_en_rect(p, e2) for p in e1) or any(punto_en_rect(p, e1) for p in e2):
        return 0.0
    return min(
        dist_seg_seg(a, b, c, d) for a, b in rect_lados(e1) for c, d in rect_lados(e2)
    )


# ------------------------------------------------------------------ modelo --
class Pad:
    """Un pad de cobre y, si lo tiene, el agujero por donde pasa su pin.

    El agujero ya no esta siempre en el centro del pad: con la boquilla de
    0,6 los pines de 2,54 mm no entran de otra manera (ver docs/pcb.md, "Los
    pads escalonados"), asi que el pad se corre al costado y queda pegado al
    borde del agujero."""

    def __init__(self, ref, pin, x, y, w, h, rot, agujero, red, hx=None, hy=None):
        self.ref, self.pin = ref, pin
        self.x, self.y, self.w, self.h, self.rot = x, y, w, h, rot
        self.agujero = agujero
        self.hx = x if hx is None else hx
        self.hy = y if hy is None else hy
        self.red = red

    @property
    def esquinas(self):
        return rect_esquinas(self.x, self.y, self.w, self.h, self.rot)


class Pista:
    def __init__(self, red, ancho, puntos):
        self.red, self.ancho, self.puntos = red, ancho, puntos

    @property
    def segmentos(self):
        return list(zip(self.puntos[:-1], self.puntos[1:]))

    @property
    def largo(self):
        return sum(math.dist(a, b) for a, b in self.segmentos)


class Nucleo:
    def __init__(self, d):
        self.d = d
        self.sustrato = d["sustrato"]
        self.reglas = d["reglas"]
        self.huellas = d["huellas"]
        self.errores = []
        self.pads = {}
        self.comps = {}
        self._armar_pads()
        self.pistas = [
            Pista(p["red"], p.get("ancho") or self.ancho_de(p["red"]),
                  [tuple(q) for q in p["puntos"]])
            for p in d["pistas"]
        ]
        self.puentes = d.get("puentes", [])

    # -- construccion --------------------------------------------------------
    def ancho_de(self, red):
        return self.reglas["ancho"][self.d["redes"][red]["clase"]]

    def _armar_pads(self):
        nodo_red = {}
        for red, info in self.d["redes"].items():
            for nodo in info["nodos"]:
                if nodo in nodo_red:
                    self.errores.append(
                        "el nodo %s aparece en %s y en %s" % (nodo, nodo_red[nodo], red))
                nodo_red[nodo] = red

        for c in self.d["componentes"]:
            ref = c["ref"]
            self.comps[ref] = c
            h = self.huellas[c["huella"]]
            cx, cy = c["pos"]
            rot = c.get("rot", 0.0)
            r = math.radians(rot)
            co, si = math.cos(r), math.sin(r)
            nombres = c.get("pines") or h.get("pines")
            pads = h["pads"]
            if len(nombres) != len(pads):
                self.errores.append(
                    "%s: la huella %s tiene %d pads y se nombran %d"
                    % (ref, c["huella"], len(pads), len(nombres)))
                continue
            for nombre, pad in zip(nombres, pads):
                px, py = pad[0], pad[1]
                x = cx + px * co - py * si
                y = cy + px * si + py * co
                # [px, py] o [px, py, hx, hy]: el segundo par es donde cae el
                # agujero cuando no esta en el centro del pad.
                if len(pad) >= 4:
                    ax, ay = pad[2], pad[3]
                    hx = cx + ax * co - ay * si
                    hy = cy + ax * si + ay * co
                else:
                    hx = hy = None
                nodo = "%s.%s" % (ref, nombre)
                red = nodo_red.get(nodo)
                if red is None:
                    self.errores.append("el pad %s no esta en ninguna red" % nodo)
                self.pads[nodo] = Pad(ref, nombre, x, y, h["pad"][0], h["pad"][1],
                                      rot, h.get("agujero", 0.0), red, hx, hy)
        for nodo in nodo_red:
            if nodo not in self.pads:
                self.errores.append("la red %s nombra un nodo inexistente: %s"
                                    % (nodo_red[nodo], nodo))

    def camino_puente(self, pu):
        """Por donde va el cable de un puente: de pad a pad, con el rodeo que
        haga falta para no cruzar la ventana."""
        a, b = self.pads[pu["de"]], self.pads[pu["a"]]
        return ([(a.x, a.y)] + [tuple(q) for q in pu.get("por", [])]
                + [(b.x, b.y)])

    # -- geometria del sustrato ---------------------------------------------
    def huecos(self):
        """Aberturas pasantes: la ventana de la pantalla y los recortes."""
        out = []
        v = self.sustrato["ventana"]
        out.append(("ventana", rect_esquinas(v["x"], v["y"], v["ancho"], v["alto"])))
        for r in self.sustrato.get("recortes", []):
            out.append((r["id"], rect_esquinas(r["x"], r["y"], r["ancho"], r["alto"])))
        for t in self.sustrato.get("tornillos", []):
            out.append(("tornillo (%.0f, %.0f)" % (t["x"], t["y"]),
                        rect_esquinas(t["x"], t["y"], t["d"], t["d"])))
        return out

    # -- verificaciones ------------------------------------------------------
    def verificar(self):
        self._v_contorno()
        self._v_separacion()
        self._v_anchos()
        self._v_conectividad()
        self._v_antena()
        self._v_modulos()
        self._v_rotulos()
        self._v_agujeros()
        self._v_puentes()
        self._v_estampadora()
        return self.errores

    def _dentro(self, p, margen):
        s = self.sustrato
        return (margen <= p[0] <= s["ancho"] - margen
                and margen <= p[1] <= s["alto"] - margen)

    def _v_contorno(self):
        m = self.reglas["margen_borde"]
        sep = self.reglas["separacion_min"]
        for pista in self.pistas:
            for p in pista.puntos:
                if not self._dentro(p, m + pista.ancho / 2.0):
                    self.errores.append(
                        "pista de %s: el punto (%.1f, %.1f) se sale del sustrato"
                        % (pista.red, p[0], p[1]))
        for nodo, pad in self.pads.items():
            if any(not self._dentro(e, 0.0) for e in pad.esquinas):
                self.errores.append("el pad %s se sale del sustrato" % nodo)
        # Nada de cobre dentro de una abertura pasante ni sobre un tornillo.
        for nombre, esq in self.huecos():
            for pista in self.pistas:
                for a, b in pista.segmentos:
                    if dist_seg_rect(a, b, esq) < sep + pista.ancho / 2.0:
                        self.errores.append("pista de %s: pasa por %s"
                                            % (pista.red, nombre))
                        break
            for nodo, pad in self.pads.items():
                if dist_rect_rect(pad.esquinas, esq) < sep:
                    self.errores.append("el pad %s invade %s" % (nodo, nombre))

    def _v_separacion(self):
        sep = self.reglas["separacion_min"]
        holgura = self.reglas["holgura_canaleta"]
        vistos = set()

        def falta(a, b, d, que):
            clave = (que, a, b)
            if clave in vistos:
                return
            vistos.add(clave)
            self.errores.append(
                "%s: %s y %s quedan a %.2f mm de pared (hacen falta %.2f)"
                % (que, a, b, d, sep))

        for i, p1 in enumerate(self.pistas):
            for p2 in self.pistas[i + 1:]:
                if p1.red == p2.red:
                    continue
                nec = (p1.ancho + p2.ancho) / 2.0 + holgura + sep
                for a, b in p1.segmentos:
                    for c, d in p2.segmentos:
                        dd = dist_seg_seg(a, b, c, d)
                        if dd < nec - 1e-6:
                            falta(p1.red, p2.red,
                                  dd - (p1.ancho + p2.ancho) / 2.0 - holgura,
                                  "canaletas")
        for pista in self.pistas:
            for nodo, pad in self.pads.items():
                if pad.red == pista.red:
                    continue
                nec = pista.ancho / 2.0 + holgura + sep
                for a, b in pista.segmentos:
                    dd = dist_seg_rect(a, b, pad.esquinas)
                    if dd < nec - 1e-6:
                        falta(pista.red, nodo, dd - pista.ancho / 2.0 - holgura,
                              "canaleta y pad")
        nodos = sorted(self.pads)
        for i, n1 in enumerate(nodos):
            for n2 in nodos[i + 1:]:
                a, b = self.pads[n1], self.pads[n2]
                if a.red == b.red:
                    continue
                dd = dist_rect_rect(a.esquinas, b.esquinas)
                if dd < holgura + sep - 1e-6:
                    falta(n1, n2, dd - holgura, "pads")

    def _v_anchos(self):
        """Toda pista llega al piso de señal; cada riel de potencia tiene al
        menos un tramo ancho. Los ramales cortos pueden ser finos: lo que
        importa es que el camino principal no estrangule la corriente."""
        piso = self.reglas["ancho_minimo"]["senal"]
        for pista in self.pistas:
            if pista.ancho < piso - 1e-9:
                self.errores.append(
                    "la red %s tiene una pista de %.1f mm (el piso es %.1f)"
                    % (pista.red, pista.ancho, piso))
        for red, info in self.d["redes"].items():
            if info["clase"] != "potencia":
                continue
            minimo = info.get("ancho_minimo", self.reglas["ancho_minimo"]["potencia"])
            anchos = [p.ancho for p in self.pistas if p.red == red]
            if anchos and max(anchos) < minimo - 1e-9:
                self.errores.append(
                    "la red %s es de potencia y su pista mas ancha mide %.1f mm "
                    "(minimo %.1f)" % (red, max(anchos), minimo))

    def _v_conectividad(self):
        """Cada red tiene que quedar en una sola pieza."""
        for red, info in self.d["redes"].items():
            nodos = list(info["nodos"])
            if info["clase"] == "sin_conexion":
                if len(nodos) != 1:
                    self.errores.append(
                        "la red %s es sin_conexion y tiene %d nodos" % (red, len(nodos)))
                continue
            pistas = [p for p in self.pistas if p.red == red]
            puentes = [p for p in self.puentes if p["red"] == red]
            padre = {}

            def raiz(a):
                while padre[a] != a:
                    padre[a] = padre[padre[a]]
                    a = padre[a]
                return a

            def unir(a, b):
                ra, rb = raiz(a), raiz(b)
                if ra != rb:
                    padre[ra] = rb

            for e in (["n:" + n for n in nodos]
                      + ["p:%d" % i for i, _ in enumerate(pistas)]):
                padre[e] = e
            # Dos cobres que se solapan estan unidos, y el solape se mide
            # entre BORDES, no entre ejes: una pista que nace al costado de
            # otra, en T, tiene los ejes a mas de un milimetro y el cobre
            # pisado. Midiendo por el eje, una red bien unida se leia rota.
            toque = self.reglas["holgura_canaleta"] + 0.05
            for i, pista in enumerate(pistas):
                for n in nodos:
                    pad = self.pads[n]
                    if any(dist_seg_rect(a, b, pad.esquinas)
                           <= pista.ancho / 2.0 + toque
                           for a, b in pista.segmentos):
                        unir("p:%d" % i, "n:" + n)
                for j, otra in enumerate(pistas):
                    if j <= i:
                        continue
                    luz = (pista.ancho + otra.ancho) / 2.0 + toque
                    if any(dist_seg_seg(a, b, c, d) <= luz
                           for a, b in pista.segmentos for c, d in otra.segmentos):
                        unir("p:%d" % i, "p:%d" % j)
            for pu in puentes:
                if pu["de"] not in self.pads or pu["a"] not in self.pads:
                    self.errores.append("el puente de %s nombra un pad inexistente" % red)
                    continue
                unir("n:" + pu["de"], "n:" + pu["a"])

            piezas = {}
            for n in nodos:
                piezas.setdefault(raiz("n:" + n), []).append(n)
            if len(piezas) > 1:
                self.errores.append(
                    "la red %s queda en %d pedazos: %s"
                    % (red, len(piezas),
                       " | ".join(", ".join(v) for v in piezas.values())))

    def _v_antena(self):
        """Nada de cobre nuestro cerca de la antena ceramica. Debajo de la
        sombra del propio modulo la regla no aplica: ahi el cobre lo pone el
        modulo, que ya viene diseñado (ver docs/pcb.md, 'La antena')."""
        z = self.sustrato["antena"]
        libre = z["libre"]
        esq = rect_esquinas(z["x"], z["y"], z["ancho"], z["alto"])
        mod = self.comps[z["modulo"]]
        h = self.huellas[mod["huella"]]
        sombra = rect_esquinas(mod["pos"][0], mod["pos"][1],
                               h["contorno"][0] + 0.4, h["contorno"][1] + 0.4,
                               mod.get("rot", 0.0))
        for pista in self.pistas:
            for a, b in pista.segmentos:
                if dist_seg_rect(a, b, esq) < libre - 1e-6:
                    if not (punto_en_rect(a, sombra) and punto_en_rect(b, sombra)):
                        self.errores.append(
                            "pista de %s: entra en la zona libre de la antena"
                            % pista.red)
                        break
        for nodo, pad in self.pads.items():
            if pad.ref == z["modulo"]:
                continue
            if dist_rect_rect(pad.esquinas, esq) < libre - 1e-6:
                self.errores.append("el pad %s entra en la zona libre de la antena"
                                    % nodo)
        if not any(dist_rect_rect(esq, e) <= 0.0
                   for nombre, e in self.huecos() if nombre == "antena"):
            self.errores.append("falta el recorte del sustrato detras de la antena")

    def _v_modulos(self):
        """Los modulos con cuerpo no se pisan entre si."""
        cuerpos = []
        for c in self.d["componentes"]:
            h = self.huellas[c["huella"]]
            if "contorno" not in h:
                continue
            cuerpos.append((c["ref"], rect_esquinas(
                c["pos"][0], c["pos"][1], h["contorno"][0], h["contorno"][1],
                c.get("rot", 0.0))))
        for i, (r1, e1) in enumerate(cuerpos):
            for r2, e2 in cuerpos[i + 1:]:
                if dist_rect_rect(e1, e2) <= 0.0:
                    self.errores.append("%s y %s se pisan" % (r1, r2))

    def _v_rotulos(self):
        """El texto grabado va en la misma cara y a la misma profundidad que
        las canaletas: si las muerde, corta una pista."""
        aire = self.reglas.get("aire_rotulo", 0.6)
        for r in self.sustrato.get("rotulos", []):
            esq = rect_esquinas(r["x"], r["y"],
                                len(r["texto"]) * r["tam"] * 0.62,
                                r["tam"] * 1.25, r.get("rot", 0.0))
            for pista in self.pistas:
                if any(dist_seg_rect(a, b, esq) < aire + pista.ancho / 2.0
                       for a, b in pista.segmentos):
                    self.errores.append('el rotulo "%s" muerde una canaleta de %s'
                                        % (r["texto"], pista.red))
                    break
            for nodo, pad in self.pads.items():
                if dist_rect_rect(pad.esquinas, esq) < aire:
                    self.errores.append('el rotulo "%s" muerde el pad %s'
                                        % (r["texto"], nodo))

    def _v_puentes(self):
        """Un puente es un cable por la cara de los modulos, y ahi hay cosas.

        Si va de punta a punta cruzando la ventana, pasa justo por donde entra
        la pantalla: al apretarla queda el cable atrapado entre el modulo y el
        plastico. Lo mismo con la muesca de la antena y con los tornillos. El
        cable se puede rodear a mano, claro, pero entonces no es el cable que
        dice el dato, y la guia de armado quedaria mintiendo."""
        for pu in self.puentes:
            a, b = self.pads.get(pu["de"]), self.pads.get(pu["a"])
            if not a or not b:
                continue
            # El cable puede llevar puntos intermedios: son el rodeo que hay
            # que darle para no cruzar un hueco.
            pts = ([(a.x, a.y)] + [tuple(q) for q in pu.get("por", [])]
                   + [(b.x, b.y)])
            malo = None
            for u, v2 in zip(pts[:-1], pts[1:]):
                for nombre, esq in self.huecos():
                    if dist_seg_rect(u, v2, esq) <= 0.0:
                        malo = nombre
                        break
                if malo:
                    break
            if malo:
                self.errores.append(
                    "el puente %s-%s cruza %s" % (pu["de"], pu["a"], malo))

    def _v_agujeros(self):
        """Los agujeros, mirados desde la boquilla.

        Dos cosas los arruinan y las dos se ven en el dato. Un agujero mas
        chico que unas dos boquillas sale tapado: el perimetro se come el
        radio y el pin no entra. Y dos agujeros demasiado juntos dejan una
        pared de plastico que la impresora no puede sacar, con lo que se
        funden en uno. Tambien se mira que el cobre de una red no quede
        colgando sobre el agujero de otra, que es una pista cortada."""
        r = self.reglas
        minimo = r.get("agujero_min")
        pared = r.get("pared_min_agujeros")
        holgura = r["holgura_canaleta"]
        con_agujero = [(n, p) for n, p in sorted(self.pads.items())
                       if p.agujero > 0]
        if minimo:
            vistos = set()
            for nodo, pad in con_agujero:
                if pad.agujero < minimo - 1e-9 and pad.ref not in vistos:
                    vistos.add(pad.ref)
                    self.errores.append(
                        "%s: agujeros de %.1f mm (el minimo es %.1f con una "
                        "boquilla de %.1f)"
                        % (pad.ref, pad.agujero, minimo, r.get("boquilla", 0.4)))
        if pared:
            for i, (n1, a) in enumerate(con_agujero):
                for n2, b in con_agujero[i + 1:]:
                    d = (math.hypot(a.hx - b.hx, a.hy - b.hy)
                         - (a.agujero + b.agujero) / 2.0)
                    if d < pared - 1e-6:
                        self.errores.append(
                            "entre los agujeros de %s y %s quedan %.2f mm de "
                            "pared (hacen falta %.2f)" % (n1, n2, d, pared))
        for nodo, pad in con_agujero:
            circ = rect_esquinas(pad.hx, pad.hy, pad.agujero, pad.agujero)
            for pista in self.pistas:
                if pista.red == pad.red:
                    continue
                if any(dist_seg_rect(a, b, circ) < pista.ancho / 2.0 + holgura
                       for a, b in pista.segmentos):
                    self.errores.append(
                        "una canaleta de %s pasa por el agujero de %s"
                        % (pista.red, nodo))
                    break

    def _v_estampadora(self):
        """La estampadora es el negativo del sustrato: las mismas canaletas pero
        en relieve, para meter toda la cinta de una prensada. Dos cosas la
        pueden arruinar y las dos se ven desde el dato: una nervadura mas fina
        de lo que la impresora puede sacar, y una nervadura que no sobresalga
        mas que la canaleta —ahi la cara plana apoyaria y pegaria la cinta
        tambien sobre las paredes."""
        e = self.d.get("estampadora")
        if not e:
            return
        piso = e["ancho_min_nervadura"]
        holgura = self.reglas["holgura_canaleta"]
        anchos = {p.ancho for p in self.pistas}
        for pad in self.pads.values():
            anchos.add(pad.w)
            anchos.add(pad.h)
        for a in sorted(anchos):
            nerv = a + holgura - 2.0 * e["holgura_lateral"]
            if nerv < piso - 1e-9:
                self.errores.append(
                    "la estampadora: una cinta de %.1f mm deja una nervadura de "
                    "%.2f mm (el piso es %.2f)" % (a, nerv, piso))
        if e["sobresalir"] <= 0.0:
            self.errores.append(
                "la estampadora: las nervaduras tienen que sobresalir mas que la "
                "canaleta, si no la cara plana apoya y pega la cinta a los lados")

    # -- numeros para la documentacion --------------------------------------
    def cinta(self):
        """Cuanta cinta hace falta, por ancho."""
        por_ancho = {}
        for p in self.pistas:
            por_ancho[p.ancho] = por_ancho.get(p.ancho, 0.0) + p.largo
        for pad in self.pads.values():
            por_ancho[pad.w] = por_ancho.get(pad.w, 0.0) + pad.h
        return por_ancho


# ------------------------------------------------------------- generadores --
def escribir(ruta, texto):
    with open(ruta, "w", encoding="utf-8", newline="\n") as f:
        f.write(texto)


def gen_redes_h(n):
    L = ["/* redes.h — GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json.",
         " *",
         " * No se edita a mano: se edita el JSON y se corre `make pcb`. Es la",
         " * netlist del sustrato vista desde el firmware, y test_placa.c la",
         " * compara contra placa.h para que los pines tengan una sola verdad.",
         " */",
         "#ifndef ROOTKIT_REDES_H",
         "#define ROOTKIT_REDES_H",
         "",
         "typedef struct {",
         "    int         gpio;      /* GPIO del ESP32-C3 SuperMini            */",
         "    const char *red;       /* como se llama la red en el sustrato    */",
         "    const char *riel;      /* de que cuelga lo que hay del otro lado */",
         "    const char *arranque;  /* alto / bajo / libre al encender        */",
         "    int         adc1;      /* 1 si la red va a una entrada del ADC1  */",
         "    int         despierta; /* 1 si tiene que despertar del sueño     */",
         "} rk_red_t;",
         "",
         "static const rk_red_t RK_REDES[] = {"]
    for p in n.d["esp32"]["pines"]:
        L.append('    { %2d, "%s", "%s", "%s", %d, %d },'
                 % (p["gpio"], p["red"], p["riel"], p["arranque"],
                    1 if p.get("adc1") else 0, 1 if p.get("despierta") else 0))
    L.append("};")
    L.append("#define RK_REDES_N ((int)(sizeof RK_REDES / sizeof RK_REDES[0]))")
    L.append("")
    L.append("/* Lo que el sustrato le promete al firmware, en numeros. */")
    # La version sale del dato, no de la lista de promesas: escrita a mano en
    # dos lados, se desincroniza sola —y de hecho lo hizo: el sustrato paso a
    # v2.0 y el firmware siguio afirmando v1.0.
    L.append('#define %-26s "%s"' % ("RK_SUSTRATO_VERSION", n.d["version"]))
    for k, v in n.d["esp32"]["promesas"].items():
        if k == "RK_SUSTRATO_VERSION":
            continue
        L.append("#define %-26s %s" % (k, v))
    L.append("")
    L.append("#endif /* ROOTKIT_REDES_H */")
    return "\n".join(L) + "\n"


def gen_scad(n):
    s = n.sustrato
    L = ["// GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json. No editar.",
         "",
         "sustrato_ancho = %.2f;" % s["ancho"],
         "sustrato_alto  = %.2f;" % s["alto"],
         "sustrato_esp   = %.2f;" % s["espesor"],
         "canaleta_prof  = %.2f;" % n.reglas["prof_canaleta"],
         "canaleta_holg  = %.2f;" % n.reglas["holgura_canaleta"],
         "radio_borde    = %.2f;" % s.get("radio_borde", 2.0)]
    v = s["ventana"]
    L.append("// [centro x, centro y, ancho, alto]")
    L.append("ventana = [%.2f, %.2f, %.2f, %.2f];"
             % (v["x"], v["y"], v["ancho"], v["alto"]))
    L.append("")
    L.append("// [ancho, [[x,y], ...]]")
    L.append("canaletas = [")
    for p in n.pistas:
        pts = ", ".join("[%.2f,%.2f]" % (x, y) for x, y in p.puntos)
        L.append("  [%.2f, [%s]],  // %s" % (p.ancho, pts, p.red))
    L.append("];")
    L.append("")
    L.append("// [x, y, ancho, alto, rot, agujero]")
    L.append("pads = [")
    for nodo in sorted(n.pads):
        p = n.pads[nodo]
        L.append("  [%.2f,%.2f,%.2f,%.2f,%.1f,%.2f],  // %s"
                 % (p.x, p.y, p.w, p.h, p.rot, p.agujero, nodo))
    L.append("];")
    L.append("")
    L.append("// [centro x, centro y, ancho, alto] — aberturas pasantes")
    L.append("recortes = [")
    for r in s.get("recortes", []):
        L.append("  [%.2f,%.2f,%.2f,%.2f],  // %s"
                 % (r["x"], r["y"], r["ancho"], r["alto"], r["id"]))
    L.append("];")
    L.append("")
    L.append("// [x, y, diametro]")
    L.append("tornillos = [")
    for t in s.get("tornillos", []):
        L.append("  [%.2f,%.2f,%.2f]," % (t["x"], t["y"], t["d"]))
    L.append("];")
    L.append("")
    e = n.d.get("estampadora")
    if e:
        L.append("// --- la estampadora (el negativo, espejado en X) ---")
        for k in ("holgura_lateral", "sobresalir", "base_extra", "base_alto",
                  "espesor", "faldon_alto", "faldon_pared", "faldon_holgura",
                  "relieve_hueco"):
            L.append("est_%-14s = %.2f;" % (k, e[k]))
        L.append('est_rotulo_tam   = %.2f;' % e["rotulo"]["tam"])
        L.append('est_rotulo       = "%s";' % e["rotulo"]["texto"])
        L.append("")
    L.append('// [x, y, tamano, rot, "texto"]')
    L.append("rotulos = [")
    for t in s.get("rotulos", []):
        L.append('  [%.2f,%.2f,%.2f,%.1f,"%s"],'
                 % (t["x"], t["y"], t["tam"], t.get("rot", 0), t["texto"]))
    L.append("];")
    return "\n".join(L) + "\n"


SVG_COLOR = {"potencia": "#b04a1e", "senal": "#2f6f4f", "sin_conexion": "#9aa0a6"}


def gen_svg(n):
    s = n.sustrato
    W, H = s["ancho"], s["alto"]
    L = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<svg xmlns="http://www.w3.org/2000/svg" version="1.1" '
         'width="%.2fmm" height="%.2fmm" viewBox="0 0 %.2f %.2f">'
         % (W + 76, H + 26, W + 76, H + 26),
         '<title>ROOTKIT — plantilla 1:1 de la cinta de cobre</title>',
         '<rect width="100%" height="100%" fill="#ffffff"/>',
         '<text x="10" y="7" font-family="sans-serif" font-size="3.0">'
         'ROOTKIT — sustrato %s, cara de las canaletas (la de atras del '
         'producto)</text>' % n.d["version"],
         '<g stroke="#000000" stroke-width="0.2" fill="none">',
         '<path d="M 10 %.2f h 50 M 10 %.2f v -2.5 M 35 %.2f v -1.5 M 60 %.2f v -2.5"/>'
         % (H + 20, H + 20, H + 20, H + 20),
         '</g>',
         '<text x="63" y="%.2f" font-family="sans-serif" font-size="2.6">'
         '50 mm exactos: si no miden 50, la impresion salio escalada</text>' % (H + 21)]
    for i, (col, txt) in enumerate((("#b04a1e", "potencia (rieles)"),
                                    ("#2f6f4f", "senal"),
                                    ("#1a3fa8", "puente de cable aislado"),
                                    ("#c62828", "zona libre de la antena"))):
        yy = 18 + i * 5
        L.append('<rect x="%.1f" y="%.1f" width="6" height="2.4" fill="%s"/>'
                 % (W + 14, yy - 2.0, col))
        L.append('<text x="%.1f" y="%.1f" font-family="sans-serif" font-size="2.4">'
                 '%s</text>' % (W + 22, yy, txt))
    L.append('<g transform="translate(10,%.2f) scale(1,-1)">' % (H + 12))
    L.append('<rect x="0" y="0" width="%.2f" height="%.2f" rx="%.2f" fill="none" '
             'stroke="#000000" stroke-width="0.3"/>'
             % (W, H, s.get("radio_borde", 2.0)))
    v = s["ventana"]
    L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="#f2f2f2" '
             'stroke="#000000" stroke-width="0.25"/>'
             % (v["x"] - v["ancho"] / 2, v["y"] - v["alto"] / 2, v["ancho"], v["alto"]))
    for r in s.get("recortes", []):
        L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="#f2f2f2" '
                 'stroke="#000000" stroke-width="0.25"/>'
                 % (r["x"] - r["ancho"] / 2, r["y"] - r["alto"] / 2,
                    r["ancho"], r["alto"]))
    for t in s.get("tornillos", []):
        L.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="#f2f2f2" '
                 'stroke="#000000" stroke-width="0.25"/>'
                 % (t["x"], t["y"], t["d"] / 2.0))
    a = s["activa"]
    L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" '
             'stroke="#8a6d3b" stroke-width="0.25" stroke-dasharray="1 1"/>'
             % (a["x"] - a["lado"] / 2, a["y"] - a["lado"] / 2, a["lado"], a["lado"]))
    for c in n.d["componentes"]:
        h = n.huellas[c["huella"]]
        if "contorno" not in h:
            continue
        L.append('<polygon points="%s" fill="none" stroke="#9aa0a6" '
                 'stroke-width="0.25" stroke-dasharray="1.5 1"/>'
                 % " ".join("%.2f,%.2f" % p for p in rect_esquinas(
                     c["pos"][0], c["pos"][1], h["contorno"][0], h["contorno"][1],
                     c.get("rot", 0))))
    z = s["antena"]
    L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" '
             'stroke="#c62828" stroke-width="0.25" stroke-dasharray="2 1"/>'
             % (z["x"] - z["ancho"] / 2 - z["libre"], z["y"] - z["alto"] / 2 - z["libre"],
                z["ancho"] + 2 * z["libre"], z["alto"] + 2 * z["libre"]))
    for p in n.pistas:
        d = " ".join(("M" if i == 0 else "L") + " %.2f %.2f" % pt
                     for i, pt in enumerate(p.puntos))
        L.append('<path d="%s" fill="none" stroke="%s" stroke-width="%.2f" '
                 'stroke-linecap="square" stroke-linejoin="miter" opacity="0.85"/>'
                 % (d, SVG_COLOR[n.d["redes"][p.red]["clase"]], p.ancho))
    for nodo in sorted(n.pads):
        p = n.pads[nodo]
        L.append('<polygon points="%s" fill="%s" stroke="#000000" stroke-width="0.12"/>'
                 % (" ".join("%.2f,%.2f" % q
                             for q in rect_esquinas(p.x, p.y, p.w, p.h, p.rot)),
                    SVG_COLOR[n.d["redes"][p.red]["clase"]] if p.red else "#9aa0a6"))
        if p.agujero:
            L.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="#ffffff" '
                     'stroke="#000000" stroke-width="0.1"/>'
                     % (p.x, p.y, p.agujero / 2.0))
    for pu in n.puentes:
        pts = n.camino_puente(pu)
        traza = " ".join("%s %.2f %.2f" % ("M" if i == 0 else "L", x, y)
                         for i, (x, y) in enumerate(pts))
        L.append('<path d="%s" stroke="#1a3fa8" stroke-width="0.45" '
                 'stroke-dasharray="2 1.4" fill="none"/>' % traza)
    L.append('</g>')
    L.append('<g font-family="sans-serif" fill="#111111">')
    for c in n.d["componentes"]:
        x, y = c["pos"]
        L.append('<text x="%.2f" y="%.2f" font-size="1.7" text-anchor="middle">%s</text>'
                 % (x + 10, H + 12 - y - 3.4, c["ref"]))
    for nodo in sorted(n.pads):
        p = n.pads[nodo]
        if not n.comps[p.ref].get("rotular_pines", False):
            continue
        L.append('<text x="%.2f" y="%.2f" font-size="1.25" text-anchor="middle">%s</text>'
                 % (p.x + 10, H + 12 - p.y - p.h / 2.0 - 0.5, p.pin))
    for r in s.get("rotulos", []):
        L.append('<text x="%.2f" y="%.2f" font-size="%.2f" text-anchor="middle" '
                 'fill="#9aa0a6">%s</text>'
                 % (r["x"] + 10, H + 12 - r["y"] + r["tam"] * 0.35, r["tam"], r["texto"]))
    L.append('</g>')
    L.append('</svg>')
    return "\n".join(L) + "\n"


def gen_conexiones(n):
    L = ["# Diagrama de conexiones", "",
         "<!-- GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json.",
         "     No se edita a mano: se edita el JSON y se corre `make pcb`. -->", "",
         "Cada red del núcleo de ROOTKIT %s: de qué riel cuelga, qué ancho de"
         % n.d["version"],
         "cinta lleva y de dónde a dónde va. El sustrato y la cinta están en",
         "[pcb.md](pcb.md); el paso a paso con los controles de multímetro, en",
         "[armado.md](armado.md).", "",
         "Las coordenadas de la plantilla se miran **desde la cara de las**",
         "**canaletas**, que es la de atrás del producto: de frente al ROOTKIT,",
         "la X crece hacia la izquierda.", "",
         "## Los rieles", "",
         "| Riel | Tensión | Vive | Qué cuelga |", "|---|---|---|---|"]
    for r in n.d["rieles"]:
        L.append("| **%s** | %s | %s | %s |"
                 % (r["id"], r["tension"], r["vive"], r["cuelga"]))
    L.append("")
    L.append("## Las redes")
    L.append("")
    L.append("| Red | Clase | Riel | Cinta | Nodos | Por qué |")
    L.append("|---|---|---|---:|---|---|")
    for red in n.d["orden_redes"]:
        info = n.d["redes"][red]
        anchos = sorted({p.ancho for p in n.pistas if p.red == red})
        cinta = (" / ".join("%.1f" % x for x in anchos) + " mm") if anchos else "sólo cable"
        L.append("| **%s** | %s | %s | %s | %s | %s |"
                 % (red, info["clase"], info.get("riel", "—"), cinta,
                    ", ".join("`%s`" % x for x in info["nodos"]),
                    info.get("porque", "")))
    L.append("")
    L.append("## Los módulos y los bornes")
    L.append("")
    L.append("| Ref | Qué es | Dónde va | Cómo se conecta |")
    L.append("|---|---|---|---|")
    for c in n.d["componentes"]:
        if not c.get("desc"):
            continue
        L.append("| **%s** | %s | %s | %s |"
                 % (c["ref"], c["desc"], c.get("donde", "—"), c.get("como", "—")))
    L.append("")
    L.append("## Los puentes de cable")
    L.append("")
    L.append("Una sola cara de cobre: donde dos redes tendrían que cruzarse, una")
    L.append("pasa por arriba con un cable aislado. Son estos, y no hay más. Se")
    L.append("sueldan **después** de la cinta y **antes** de los módulos.")
    L.append("")
    L.append("Los que dicen **rodeando** no van de punta a punta: el camino")
    L.append("derecho les cruzaría la ventana de la pantalla y el cable quedaría")
    L.append("apretado entre el módulo y el plástico. La plantilla los dibuja por")
    L.append("donde van.")
    L.append("")
    L.append("| # | Red | De | A | Cable | Largo aprox. | |")
    L.append("|---:|---|---|---|---|---:|---|")
    for i, pu in enumerate(n.puentes, 1):
        pts = n.camino_puente(pu)
        largo = sum(math.dist(a, b) for a, b in zip(pts[:-1], pts[1:]))
        rodea = ("**rodeando** la ventana (ver la plantilla)"
                 if len(pts) > 2 else "")
        # El calibre lo declara la red, no su clase: por el camino de la celda
        # y del cargador pasa un ampere y ahi el cable es mejor conductor que
        # la cinta; un ramal de masa a la compuerta de un MOSFET, no.
        calibre = n.d["redes"][pu["red"]].get("cable_puente", "AWG30")
        calibre = "**%s**" % calibre if calibre != "AWG30" else calibre
        L.append("| %d | %s | `%s` | `%s` | %s | %.0f mm | %s |"
                 % (i, pu["red"], pu["de"], pu["a"], calibre, largo + 6, rodea))
    L.append("")
    L.append("## Los puntos de prueba")
    L.append("")
    L.append("| Punto | Red | Para qué |")
    L.append("|---|---|---|")
    for c in n.d["componentes"]:
        if c["huella"] != "punto_prueba":
            continue
        L.append("| **%s** | %s | %s |"
                 % (c["ref"], n.pads["%s.1" % c["ref"]].red, c.get("desc", "")))
    L.append("")
    L.append("## Los selectores")
    L.append("")
    for sel in n.d.get("selectores", []):
        L.append("**%s** — %s" % (sel["ref"], sel["que"]))
        L.append("")
        for op in sel["opciones"]:
            L.append("- *%s*%s: %s"
                     % (op["nombre"], " **(por defecto)**" if op.get("defecto") else "",
                        op["que"]))
        L.append("")
    L.append("## Cuánta cinta")
    L.append("")
    L.append("| Ancho | Largo total |")
    L.append("|---:|---:|")
    total = 0.0
    for ancho, largo in sorted(n.cinta().items()):
        L.append("| %.1f mm | %.0f mm |" % (ancho, largo))
        total += largo
    L.append("")
    L.append("Son **%.0f mm de cinta por unidad**, contando los pads. Con un 40 %% de"
             % total)
    L.append("recortes y errores, un rollo de 6 mm y otro de 20 mm alcanzan para")
    L.append("más de diez unidades.")
    return "\n".join(L) + "\n"


def triangulos(stl):
    """Cuantos triangulos tiene un STL binario. 0 si no existe."""
    if not os.path.exists(stl):
        return 0
    with open(stl, "rb") as f:
        cab = f.read(84)
    if len(cab) < 84:
        return 0
    return int.from_bytes(cab[80:84], "little")


def _correr_encaje(modo):
    """Corre hardware/pcb/encaje.scad en un modo y devuelve (triangulos, caja)."""
    destino = os.path.join(GEN, "encaje-%s.stl" % modo)
    if os.path.exists(destino):
        os.remove(destino)
    try:
        subprocess.run(["openscad", "-D", 'modo="%s"' % modo,
                        "--export-format", "binstl", "-o", destino, ENCAJE],
                       capture_output=True, text=True, timeout=1800)
    except FileNotFoundError:
        return None, None
    n = triangulos(destino)
    caja = None
    if n:
        with open(destino, "rb") as f:
            f.read(84)
            xs, ys = [], []
            for _ in range(n):
                d = struct.unpack("<12fH", f.read(50))
                for i in range(3):
                    xs.append(d[3 + i * 3])
                    ys.append(d[4 + i * 3])
        caja = (max(xs) - min(xs), max(ys) - min(ys))
    if os.path.exists(destino):
        os.remove(destino)
    return n, caja


def probar_encaje(n):
    """Dos preguntas sobre la estampadora, y las dos hacen falta.

    "choque": dada vuelta y apoyada a fondo sobre el sustrato, la interseccion
    de las dos piezas tiene que dar VACIA. Cualquier solido es plastico contra
    plastico: la pieza no baja del todo y la cinta no entra.

    "presencia": lo que la estampadora mete DENTRO de las canaletas tiene que
    dar LLENO y cubrir casi toda la placa. Sin esta, la primera se aprobaria
    por la razon equivocada: si las nervaduras desaparecieran, la interseccion
    tambien daria vacia."""
    choque, _ = _correr_encaje("choque")
    if choque is None:
        return "openscad no esta instalado"
    if choque:
        return ("choca con el sustrato: %d triangulos de contacto "
                "(ver hardware/pcb/encaje.scad)" % choque)
    presencia, caja = _correr_encaje("presencia")
    if not presencia:
        return ("las nervaduras no llegan al fondo de ninguna canaleta: la "
                "estampadora no estamparia nada")
    cobertura = min(caja[0] / n.sustrato["ancho"], caja[1] / n.sustrato["alto"])
    if cobertura < 0.80:
        return ("las nervaduras solo cubren el %.0f %% de la placa "
                "(se esperaba mas del 80 %%)" % (cobertura * 100))
    return None


def _canonizar_stl(stl):
    """Ordena las facetas de un STL binario para que el archivo sea estable.

    OpenSCAD triangula en paralelo y escribe cada faceta en el orden en que
    termina su hilo: dos corridas sobre el mismo modelo dan la misma
    geometria —mismas 22.136 facetas, una por una— y bytes distintos. Con el
    STL commiteado eso es veneno: `make pcb` ensucia el arbol sin que haya
    cambiado nada, `make verify` lo denuncia, y el dia que cambie algo de
    verdad el diff queda escondido entre miles de facetas que se movieron
    solas. Ordenandolas, el archivo vuelve a ser funcion del modelo y nada
    mas. La cabecera tambien se reescribe fija, asi no entra por ahi la
    version de OpenSCAD.
    """
    with open(stl, "rb") as f:
        datos = f.read()
    if len(datos) < 84:
        return
    n = int.from_bytes(datos[80:84], "little")
    if len(datos) != 84 + n * 50:
        return              # no es el STL binario que esperamos: mejor no tocar
    facetas = sorted(datos[84 + i * 50:84 + (i + 1) * 50] for i in range(n))
    cab = b"ROOTKIT " + os.path.basename(stl).encode("ascii", "replace")
    with open(stl, "wb") as f:
        f.write(cab.ljust(80, b"\0")[:80])
        f.write(n.to_bytes(4, "little"))
        f.write(b"".join(facetas))


def probar_base(stl):
    """La cara de abajo tiene que ser un plano, y hay que comprobarlo en la
    pieza, no en el modelo.

    Un techo mirando hacia abajo a media altura es plastico que la impresora
    tiene que tender en el aire sobre la primera capa: sale colgando, se
    despega y arruina la cara. Es el defecto que tenian la repisa de la
    ventana y el bolsillo del cargador. Buscarlo en el STL es barato —una
    faceta con la normal hacia abajo por encima de la cama— y no depende de
    acordarse de mirar el modelo."""
    if not os.path.exists(stl):
        return None
    with open(stl, "rb") as f:
        datos = f.read()
    if len(datos) < 84:
        return None
    n = int.from_bytes(datos[80:84], "little")
    peor = None
    area = 0.0
    for i in range(n):
        d = struct.unpack("<12f", datos[84 + i * 50:84 + i * 50 + 48])
        if d[2] > -0.99:                 # no mira hacia abajo
            continue
        zs = (d[5], d[8], d[11])
        z = max(zs)
        if z <= 0.01:                    # es la cara de abajo, que esta bien
            continue
        ax = (d[6] - d[3], d[7] - d[4])
        bx = (d[9] - d[3], d[10] - d[4])
        area += abs(ax[0] * bx[1] - ax[1] * bx[0]) / 2.0
        if peor is None or z > peor[0]:
            peor = (z, d[3], d[4])
    if peor is None:
        return None
    return ("la cara de abajo no es plana: %.1f mm2 de techo colgando, el mas "
            "alto a %.2f mm sobre la cama, cerca de (%.1f, %.1f)"
            % (area, peor[0], peor[1], peor[2]))


def exportar_stl(destino, fuente=None):
    fuente = fuente or SCAD
    if not os.path.exists(fuente):
        return "no existe %s" % fuente
    try:
        r = subprocess.run(["openscad", "--export-format", "binstl",
                            "-o", destino, fuente],
                           capture_output=True, text=True, timeout=1800)
    except FileNotFoundError:
        return "openscad no esta instalado"
    except subprocess.TimeoutExpired:
        return "openscad tardo demasiado"
    if r.returncode != 0:
        return r.stderr.strip()[-800:]
    _canonizar_stl(destino)
    return None


# -------------------------------------------------------------------- main --
def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--verificar", action="store_true")
    ap.add_argument("--generar", action="store_true")
    ap.add_argument("--stl", action="store_true")
    ap.add_argument("--encaje", action="store_true")
    ap.add_argument("--rutear", action="store_true")
    args = ap.parse_args()
    if not (args.verificar or args.generar or args.stl or args.encaje
            or args.rutear):
        args.verificar = True

    d = cargar()
    if args.rutear:
        import ruteo
        print("  ruteando (esto tarda)")
        ruteo.rutear_y_guardar(Nucleo(d), RUTEO)
        d = cargar()
    n = Nucleo(d)
    errores = n.verificar()

    print("  sustrato %s: %.0f x %.0f x %.1f mm, %d redes, %d pistas, %d pads, "
          "%d puentes"
          % (d["version"], n.sustrato["ancho"], n.sustrato["alto"],
             n.sustrato["espesor"], len(d["redes"]), len(n.pistas), len(n.pads),
             len(n.puentes)))
    if errores:
        print("\n  el sustrato NO pasa:")
        for e in errores:
            print("    - %s" % e)
        return 1
    print("  en regla: separaciones, anchos, conectividad, antena, bordes,\n"
          "  rotulos, agujeros, puentes y nervaduras de la estampadora")

    mal = probar_base(os.path.join(GEN, "nucleo-sustrato.stl"))
    if mal:
        print("  FALLA: %s" % mal)
        return 1
    print("  base: la cara que se imprime contra la cama es un plano")

    if args.generar or args.stl:
        os.makedirs(GEN, exist_ok=True)
        escribir(REDES_H, gen_redes_h(n))
        escribir(os.path.join(GEN, "sustrato_datos.scad"), gen_scad(n))
        escribir(os.path.join(GEN, "plantilla-cinta.svg"), gen_svg(n))
        escribir(CONEXIONES, gen_conexiones(n))
        print("  generados: firmware/test/redes.h, sustrato_datos.scad, "
              "plantilla-cinta.svg, docs/conexiones.md")
    if args.stl:
        for nombre, fuente in (("nucleo-sustrato.stl", SCAD),
                               ("nucleo-estampadora.stl", ESTAMPADORA)):
            destino = os.path.join(GEN, nombre)
            err = exportar_stl(destino, fuente)
            if err:
                print("  STL %s: %s" % (nombre, err))
                return 1
            print("  STL: %s (%.0f KB)"
                  % (destino, os.path.getsize(destino) / 1024.0))
    if args.encaje or args.stl:
        err = probar_encaje(n)
        if err:
            print("  encaje: %s" % err)
            return 1
        print("  encaje: la estampadora entra en el sustrato sin tocarlo, y "
              "llega al fondo de las canaletas")
    return 0


if __name__ == "__main__":
    sys.exit(main())

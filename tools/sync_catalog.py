#!/usr/bin/env python3
"""Sincroniza el catálogo del Hub con el del firmware.

El firmware es la única fuente de verdad: `firmware/core/species.c` define las
especies con sus rangos y su dificultad, y `firmware/core/companion.c` define
qué simbionte revela cada una. El Hub necesita los mismos datos para poder
armar el alta de plantas sin pedirle la lista a la Terminal en cada pantalla.

Mantener las dos copias a mano es garantía de que se desincronicen, y una
desincronización acá significa que el Hub le ofrece al usuario una especie que
la Terminal no sabe evaluar. Así que se genera, y `make verify` falla si el
archivo generado no está commiteado al día.

    python tools/sync_catalog.py [--check]

Con --check no escribe nada: sale con código 1 si el destino está desfasado.
"""

import io
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPECIES_C = os.path.join(RAIZ, "firmware", "core", "species.c")
COMPANION_C = os.path.join(RAIZ, "firmware", "core", "companion.c")
DESTINO = os.path.join(RAIZ, "hub", "dev-server.mjs")

# Los umbrales tienen que coincidir con rk_rarity_from_difficulty().
UMBRALES = ((80, "LEGENDARIO"), (60, "EPICO"), (30, "RARO"), (0, "COMUN"))

RE_ESPECIE = re.compile(
    r'\{\s*"([a-z\-]+)",\s*"([^"]+)",\s*'
    r"(\d+),\s*(\d+),\s*(-?\d+),\s*(\d+),\s*(\d+),\s*"
    r"(\d+),\s*(\d+),\s*(\d+)\s*\}"
)
RE_COMPANION = re.compile(
    r'\{\s*"([a-z]+)",\s*"([^"]+)",\s*"([a-z\-]+)",\s*\n\s*"([^"]*)"\s*\}'
)


def rareza(dificultad):
    for corte, nombre in UMBRALES:
        if dificultad >= corte:
            return nombre
    return "COMUN"


def esc(s):
    return s.replace("\\", "\\\\").replace("'", "\\'")


def leer():
    especies = RE_ESPECIE.findall(io.open(SPECIES_C, encoding="utf-8").read())
    simbiontes = RE_COMPANION.findall(
        io.open(COMPANION_C, encoding="utf-8").read())

    if not especies:
        sys.exit("no pude parsear species.c: cambio el formato de la tabla?")
    if len(especies) != len(simbiontes):
        sys.exit("hay %d especies y %d simbiontes: el mapeo tiene que ser "
                 "uno a uno" % (len(especies), len(simbiontes)))

    dif = {e[0]: int(e[9]) for e in especies}
    huerfanos = [c[0] for c in simbiontes if c[2] not in dif]
    if huerfanos:
        sys.exit("simbiontes apuntando a especies inexistentes: %s"
                 % ", ".join(huerfanos))
    return especies, simbiontes, dif


def generar(especies, simbiontes, dif):
    e_lineas = [
        "  { id: '%s', nombre: '%s', soil_min: %s, soil_max: %s,\n"
        "    temp_min_dc: %s, temp_max_dc: %s, rh_min: %s, "
        "lux_min: %s, lux_max: %s, dificultad: %s }"
        % (sid, esc(nom), smin, smax, tmin, tmax, rh, lmin, lmax, d)
        for (sid, nom, smin, smax, tmin, tmax, rh, lmin, lmax, d) in especies
    ]
    s_lineas = [
        "  { id: '%s', nombre: '%s', especie: '%s', rareza: '%s',\n"
        "    lema: '%s' }"
        % (cid, esc(cnom), cesp, rareza(dif[cesp]), esc(lema))
        for (cid, cnom, cesp, lema) in simbiontes
    ]
    return ("export const ESPECIES = [\n" + ",\n".join(e_lineas) + ",\n];",
            "export const SIMBIONTES = [\n" + ",\n".join(s_lineas) + ",\n];")


def reemplazar(texto, marca, bloque):
    a = texto.index(marca)
    b = texto.index("];", a) + 2
    return texto[:a] + bloque + texto[b:]


def main():
    check = "--check" in sys.argv
    especies, simbiontes, dif = leer()
    bloque_e, bloque_s = generar(especies, simbiontes, dif)

    actual = io.open(DESTINO, encoding="utf-8", newline="").read()
    nuevo = reemplazar(actual, "export const ESPECIES = [", bloque_e)
    nuevo = reemplazar(nuevo, "export const SIMBIONTES = [", bloque_s)

    if check:
        if nuevo != actual:
            print("hub/dev-server.mjs esta desfasado del catalogo del firmware")
            print("corre: python tools/sync_catalog.py")
            return 1
        print("el catalogo del Hub esta al dia (%d especies)" % len(especies))
        return 0

    if nuevo == actual:
        print("sin cambios: %d especies" % len(especies))
        return 0

    io.open(DESTINO, "w", encoding="utf-8", newline="\n").write(nuevo)
    reparto = {}
    for c in simbiontes:
        r = rareza(dif[c[2]])
        reparto[r] = reparto.get(r, 0) + 1
    print("sincronizado: %d especies, %d simbiontes" % (len(especies), len(simbiontes)))
    print("  " + "  ".join("%s %d" % (k, reparto[k])
                           for _, k in UMBRALES if k in reparto))
    return 0


if __name__ == "__main__":
    sys.exit(main())

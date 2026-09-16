#!/usr/bin/env python3
"""Sincroniza el catálogo de la app con el del firmware.

El firmware es la única fuente de verdad. `firmware/core/species.c` define las
especies con sus rangos, y `firmware/core/persona.c` define los modelos de
carcasa con su rareza. La app necesita las dos listas para armar el alta de
plantas y para mostrar la colección sin pedírselas al aparato en cada
pantalla.

Mantener las copias a mano es garantía de que se desincronicen, y acá una
desincronización significa ofrecerle al usuario una especie que el aparato no
sabe evaluar, o un modelo de carcasa que no existe. Así que se genera, y
`make verify` falla si el archivo generado no está commiteado al día.

    python tools/sync_catalog.py [--check]

Con --check no escribe nada: sale con código 1 si el destino está desfasado.

NOTA SOBRE LA RAREZA. Antes salía de la dificultad hortícola de la especie y
este script la calculaba. Ahora la rareza es del MODELO DE CARCASA y viene
declarada en persona.c, porque el azar se mudó a la caja física. La
dificultad de la especie se sigue publicando —la app la muestra al elegir
planta, que es información útil— pero ya no decide nada.
"""

import io
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPECIES_C = os.path.join(RAIZ, "firmware", "core", "species.c")
PERSONA_C = os.path.join(RAIZ, "firmware", "core", "persona.c")
DESTINO = os.path.join(RAIZ, "hub", "dev-server.mjs")

RE_ESPECIE = re.compile(
    r'\{\s*"([a-z\-]+)",\s*"([^"]+)",\s*'
    r"(\d+),\s*(\d+),\s*(-?\d+),\s*(\d+),\s*(\d+),\s*"
    r"(\d+),\s*(\d+),\s*(\d+)\s*\}"
)

# Los cuatro primeros campos de cada entrada de rk_persona_table, mas la
# rareza. El resto son parametros de dibujo que la app no necesita.
RE_PERSONA = re.compile(
    r'\{\s*\n?\s*"([a-z]+)",\s*"([^"]*)",\s*"([^"]+)",\s*\n\s*'
    r'"([^"]*)",\s*\n\s*(RK_RAR_[A-Z]+),',
    re.M,
)

RAREZA_JS = {
    "RK_RAR_COMUN": "COMUN",
    "RK_RAR_RARO": "RARO",
    "RK_RAR_SECRETO": "SECRETO",
}


def esc(s):
    return s.replace("\\", "\\\\").replace("'", "\\'")


def leer():
    especies = RE_ESPECIE.findall(io.open(SPECIES_C, encoding="utf-8").read())
    modelos = RE_PERSONA.findall(io.open(PERSONA_C, encoding="utf-8").read())

    if not especies:
        sys.exit("no pude parsear species.c: cambio el formato de la tabla?")
    if not modelos:
        sys.exit("no pude parsear persona.c: cambio el formato de la tabla?")

    desconocidas = [m[4] for m in modelos if m[4] not in RAREZA_JS]
    if desconocidas:
        sys.exit("rarezas que este script no conoce: %s"
                 % ", ".join(sorted(set(desconocidas))))

    secretos = [m for m in modelos if m[4] == "RK_RAR_SECRETO"]
    if len(secretos) != 1:
        sys.exit("la caja tiene que tener exactamente un secreto, hay %d"
                 % len(secretos))

    return especies, modelos


def generar(especies, modelos):
    e_lineas = [
        "  { id: '%s', nombre: '%s', soil_min: %s, soil_max: %s,\n"
        "    temp_min_dc: %s, temp_max_dc: %s, rh_min: %s, "
        "lux_min: %s, lux_max: %s, dificultad: %s }"
        % (sid, esc(nom), smin, smax, tmin, tmax, rh, lmin, lmax, d)
        for (sid, nom, smin, smax, tmin, tmax, rh, lmin, lmax, d) in especies
    ]
    # El indice importa: es la clave con la que la carcasa viaja por radio.
    m_lineas = [
        "  { idx: %d, id: '%s', nombre: '%s', rareza: '%s',\n"
        "    carcasa: '%s', lema: '%s' }"
        % (i, mid, esc(nom), RAREZA_JS[rar], esc(stl), esc(lema))
        for i, (mid, nom, stl, lema, rar) in enumerate(modelos)
    ]
    return ("export const ESPECIES = [\n" + ",\n".join(e_lineas) + ",\n];",
            "export const MODELOS = [\n" + ",\n".join(m_lineas) + ",\n];")


def reemplazar(texto, marca, bloque):
    a = texto.index(marca)
    b = texto.index("];", a) + 2
    return texto[:a] + bloque + texto[b:]


def main():
    check = "--check" in sys.argv
    especies, modelos = leer()
    bloque_e, bloque_m = generar(especies, modelos)

    actual = io.open(DESTINO, encoding="utf-8", newline="").read()
    nuevo = reemplazar(actual, "export const ESPECIES = [", bloque_e)
    nuevo = reemplazar(nuevo, "export const MODELOS = [", bloque_m)

    if check:
        if nuevo != actual:
            print("hub/dev-server.mjs esta desfasado del catalogo del firmware")
            print("corre: python tools/sync_catalog.py")
            return 1
        print("el catalogo de la app esta al dia (%d especies, %d modelos)"
              % (len(especies), len(modelos)))
        return 0

    if nuevo == actual:
        print("sin cambios: %d especies, %d modelos"
              % (len(especies), len(modelos)))
        return 0

    io.open(DESTINO, "w", encoding="utf-8", newline="\n").write(nuevo)
    reparto = {}
    for m in modelos:
        r = RAREZA_JS[m[4]]
        reparto[r] = reparto.get(r, 0) + 1
    print("sincronizado: %d especies, %d modelos"
          % (len(especies), len(modelos)))
    print("  " + "  ".join("%s %d" % (k, reparto[k])
                           for k in ("COMUN", "RARO", "SECRETO")
                           if k in reparto))
    return 0


if __name__ == "__main__":
    sys.exit(main())

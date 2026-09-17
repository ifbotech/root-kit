#!/usr/bin/env python3
"""fabrica.py — la estación de fábrica: le da identidad a un ROOTKIT y lo registra.

Cada aparato sale de la caja con un SECRETO (de donde salen su token y el
código del QR) y con su Rooti (la figura que lleva puesta). Este programa:

  1. (opcional) flashea el firmware con PlatformIO          --flashear
  2. genera el secreto y se lo manda al aparato por el puerto serie, junto con
     el Rooti y el lote (firmware/core/fabrica.h: `FABRICA {...}`)
  3. le pregunta quién es (`FABRICA?`): la MAC y el código del QR
  4. lo registra en la nube (POST /api/admin/aparatos) con el HASH de su
     token: la nube nunca ve el secreto, y con ROOTLAB_TOFU=emulador no acepta
     ninguna placa que no haya pasado por acá
  5. escribe la etiqueta (SVG) con el Rooti, el lote y el código de respaldo

Uso:

  set ROOTLAB_ADMIN_CLAVE=...            (nunca en la línea de comandos)
  python tools/fabrica.py --puerto COM5 --persona brote --lote L2609 \\
         --nube https://ifbotech.com/rootkit [--flashear] [--canal beta]

  python tools/fabrica.py --puerto COM5 --consultar     sólo pregunta (reimprimir)
  python tools/fabrica.py --autoprueba                  verifica la derivación, sin placa

Necesita pyserial (viene con PlatformIO: usar su Python,
~/.platformio/penv/Scripts/python). El secreto no se guarda en ningún lado:
vive en la NVS del aparato. Ver docs/fabrica.md.
"""
import argparse
import hashlib
import hmac
import json
import os
import secrets
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOTIES = ["brote", "musgo", "pinchito", "bulbo", "champi"]
RAIZ = Path(__file__).resolve().parent.parent


def token_de(secreto: bytes) -> str:
    """El token del aparato: HMAC-SHA256(secreto, "rootkit-api") en hex (core/codigo.c)."""
    return hmac.new(secreto, b"rootkit-api", hashlib.sha256).hexdigest()


def hash_de_token(token: str) -> str:
    """Lo que guarda la nube: SHA-256 del token en hex (root-lab/server/codigo.mjs)."""
    return hashlib.sha256(token.encode()).hexdigest()


def autoprueba() -> int:
    """Los mismos vectores que firmware/test/test_enlace.c y root-lab/test/nube.test.mjs."""
    secreto = bytes.fromhex("3a917c05ee4218b69d602fc3710e845b")
    esperado = "71859c23a4eb073e425391d23d46eede1760af53a9bec3b4b168724b2d8e6be3"
    ok = token_de(secreto) == esperado
    print("token:", "bien" if ok else f"MAL ({token_de(secreto)})")
    return 0 if ok else 1


def hablar(puerto, linea: str, espera_s: float = 6.0) -> dict:
    """Manda una línea y devuelve la primera respuesta JSON con la clave "fabrica"."""
    puerto.reset_input_buffer()
    puerto.write((linea + "\n").encode())
    puerto.flush()
    limite = time.time() + espera_s
    while time.time() < limite:
        cruda = puerto.readline().decode(errors="replace").strip()
        if cruda.startswith("{") and '"fabrica"' in cruda:
            try:
                return json.loads(cruda)
            except json.JSONDecodeError:
                continue
    raise TimeoutError("el aparato no contestó (¿está flasheado con un firmware 0.6.0 o más nuevo?)")


def registrar(nube: str, clave: str, cuerpo: dict) -> dict:
    pedido = urllib.request.Request(
        nube.rstrip("/") + "/api/admin/aparatos",
        data=json.dumps(cuerpo).encode(),
        headers={"content-type": "application/json", "authorization": f"Bearer {clave}"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(pedido, timeout=20) as r:
            return json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        detalle = e.read().decode(errors="replace")
        raise SystemExit(f"la nube rechazó el registro ({e.code}): {detalle}") from e


def etiqueta(carpeta: Path, estado: dict) -> Path:
    """Una etiqueta de 50 x 30 mm: el Rooti, el lote y el código de respaldo del QR."""
    carpeta.mkdir(parents=True, exist_ok=True)
    codigo = estado.get("codigo", "")
    legible = f"{codigo[:4]}-{codigo[4:]}" if len(codigo) == 8 else codigo
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="50mm" height="30mm" viewBox="0 0 500 300">
  <rect width="500" height="300" fill="#fff"/>
  <text x="250" y="70" text-anchor="middle" font-family="Nunito, Arial, sans-serif" font-weight="900" font-size="44">ROOTKIT · {estado.get("persona", "").capitalize()}</text>
  <text x="250" y="165" text-anchor="middle" font-family="Consolas, monospace" font-weight="700" font-size="72" letter-spacing="6">{legible}</text>
  <text x="250" y="215" text-anchor="middle" font-family="Nunito, Arial, sans-serif" font-size="24">Si la cámara no lee el QR, escribí este código en ROOTLAB</text>
  <text x="250" y="265" text-anchor="middle" font-family="Consolas, monospace" font-size="22">{estado.get("id", "")} · lote {estado.get("lote", "") or "-"} · fw {estado.get("fw", "")}</text>
</svg>
"""
    archivo = carpeta / f"{estado.get('id', 'sin-id')}.svg"
    archivo.write_text(svg, encoding="utf-8")
    return archivo


def main() -> int:
    ap = argparse.ArgumentParser(description="Estación de fábrica de ROOTKIT")
    ap.add_argument("--puerto", help="puerto serie del aparato (COM5, /dev/ttyACM0)")
    ap.add_argument("--persona", choices=ROOTIES, help="qué Rooti es la figura")
    ap.add_argument("--lote", default="", help="lote de fabricación (letras, números y guiones; hasta 11)")
    ap.add_argument("--nube", default=os.environ.get("ROOTLAB_URL_PUBLICA", ""), help="URL de ROOTLAB")
    ap.add_argument("--canal", default="estable", choices=["estable", "beta"])
    ap.add_argument("--flashear", action="store_true", help="flashear antes con PlatformIO (entorno c3-144)")
    ap.add_argument("--entorno", default="c3-144")
    ap.add_argument("--consultar", action="store_true", help="sólo preguntar quién es, sin cambiar nada")
    ap.add_argument("--sin-nube", action="store_true", help="grabar sin registrar (banco de pruebas)")
    ap.add_argument("--etiquetas", default=str(RAIZ / "build" / "etiquetas"))
    ap.add_argument("--autoprueba", action="store_true")
    a = ap.parse_args()

    if a.autoprueba:
        return autoprueba()
    if not a.puerto:
        ap.error("falta --puerto")
    try:
        import serial  # pyserial
    except ImportError:
        raise SystemExit("falta pyserial: usá el Python de PlatformIO (~/.platformio/penv) o `pip install pyserial`")

    if a.flashear:
        pio = os.environ.get("PIO", "pio")
        subprocess.run([pio, "run", "-e", a.entorno, "-t", "upload", "--upload-port", a.puerto],
                       cwd=RAIZ / "firmware", check=True)
        time.sleep(3)

    with serial.Serial(a.puerto, 115200, timeout=0.5) as puerto:
        time.sleep(2.0)                      # el C3 reinicia al abrir el puerto
        if a.consultar:
            estado = hablar(puerto, "FABRICA?")
            print(json.dumps(estado, indent=2, ensure_ascii=False))
            print("etiqueta:", etiqueta(Path(a.etiquetas), estado))
            return 0

        if not a.persona:
            ap.error("falta --persona")
        clave = os.environ.get("ROOTLAB_ADMIN_CLAVE", "")
        if not a.sin_nube and (not a.nube or not clave):
            ap.error("faltan --nube y la variable de entorno ROOTLAB_ADMIN_CLAVE (o usá --sin-nube)")

        previo = hablar(puerto, "FABRICA?")
        if previo.get("vinculado"):
            raise SystemExit("ese aparato ya es de alguien: no se le cambia la identidad")

        secreto = secrets.token_bytes(16)
        orden = {"secreto": secreto.hex(), "persona": a.persona, "lote": a.lote}
        r = hablar(puerto, "FABRICA " + json.dumps(orden, separators=(",", ":")))
        if not r.get("fabrica"):
            raise SystemExit(f"el aparato rechazó la orden: {r.get('error', '?')}")
        estado = hablar(puerto, "FABRICA?")
        if estado.get("persona") != a.persona:
            raise SystemExit(f"el aparato dice ser {estado.get('persona')}: no quedó grabado")

    if not a.sin_nube:
        reg = registrar(a.nube, clave, {
            "id": estado["id"], "token_hash": hash_de_token(token_de(secreto)),
            "persona": a.persona, "lote": a.lote, "canal": a.canal, "reemplazar": True,
        })
        print(f"registrado en la nube: {reg['id']} · {reg['persona']} · lote {reg.get('lote') or '-'} · canal {reg['canal']}")
    del secreto                              # no se guarda en ningún lado: vive en la NVS
    print(f"grabado: {estado['id']} es {estado['persona']} · código {estado['codigo']}")
    print("etiqueta:", etiqueta(Path(a.etiquetas), estado))
    return 0


if __name__ == "__main__":
    sys.exit(main())

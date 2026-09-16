# El contrato con la nube

La única conversación del aparato con el mundo. Del lado del aparato la
implementa `firmware/net/nube.c` (probado en `test/test_red.c`); del lado del
servidor, `root-lab/server/api.mjs` (probado en `test/api.test.mjs`).

## Por qué el aparato empuja y la app no le pregunta

- La app es una página HTTPS: el navegador no la deja pedirle nada a una IP
  privada de la casa. Y el teléfono casi nunca está en la misma red que la
  maceta.
- Las notificaciones tienen que llegar con la app cerrada, y eso lo manda un
  servidor.

Así que la maceta le cuenta todo a la nube, y la app y las notificaciones
salen de ahí.

## Un solo pedido

```
POST {nube}/api/d/sync
Content-Type: application/json
Authorization: Bearer <token>
```

Uno solo, y no cinco, porque cada conexión TLS cuesta casi un segundo de
radio: juntar todo en un ida y vuelta es la mayor optimización de batería
del protocolo.

### Lo que manda el aparato

```json
{
  "id": "A1B2C3D4E5F6",
  "fw": "0.5.0",
  "placa": "c3-supermini",
  "pantalla": "ili9341-240x320",
  "persona": "kawaii",
  "estado": "SIN_VINCULO",
  "epoca": 3,
  "codigo": "K7Q2M9XA",
  "reloj": 86400,
  "rssi": -61,
  "usb": false,
  "bat_mv": 3920,
  "arranques": 12,
  "lecturas": [
    { "hace": 900, "suelo": 38, "suelo_raw": 2140, "temp": 232, "hr": 55,
      "lux": 4800, "tsuelo": 210, "bat": 3920, "usb": false,
      "animo": "HAPPY", "sev": "OK", "fallas": 0 }
  ]
}
```

| Campo | Qué es |
|---|---|
| `id` | MAC en hex. Identifica, no autoriza. |
| `persona` | el personaje grabado en fábrica; vacío si no tiene |
| `estado` | el estado de `core/enlace.c` |
| `epoca` | sube con cada desvinculación |
| `codigo` | **sólo mientras no está vinculado**: el del QR |
| `reloj` | segundos monótonos del aparato, sumando los dormidos |
| `lecturas[].hace` | cuántos segundos antes de `reloj` se midió |
| `lecturas[]` | hasta 20 por pedido; los campos de un sensor que falló no van |
| `temp`, `tsuelo` | décimas de grado |
| `animo`, `sev` | lo que evaluó el propio aparato: es lo que muestra su cara |
| `fallas` | bits: 1 suelo, 2 aire, 4 luz, 8 sonda |

**Sin reloj de pared.** El aparato no sabe la fecha y no le hace falta: el
servidor fecha cada lectura restando `hace` a su propia hora. Sin NTP, sin
pila de reloj, sin aparatos que arrancan en 1970.

### Lo que contesta la nube

```json
{
  "ok": true,
  "vinculado": true,
  "revelado": true,
  "persona": "kawaii",
  "nombre": "Rulo",
  "especie": {
    "id": "monstera", "nombre": "Monstera deliciosa",
    "suelo_min": 25, "suelo_max": 60, "temp_min": 180, "temp_max": 300,
    "hr_min": 50, "lux_min": 1000, "lux_max": 15000, "dificultad": 45
  },
  "vinculo": { "dias_sanos": 34, "dias_vividos": 40, "racha": 8, "mejor_racha": 19 },
  "intervalo_s": 900,
  "aceptadas": 12,
  "hora": 1758040000,
  "calibracion": { "seco": 3100, "mojado": 1300 },
  "brillo": 80,
  "pantalla": "toque"
}
```

| Campo | Qué hace el aparato |
|---|---|
| `ok` | **sin `"ok": true` no se aplica nada**: un portal de hotel que contesta HTML no puede desvincular una maceta |
| `vinculado`, `revelado` | alimentan la máquina de estados |
| `persona` | si no tenía de fábrica, adopta la que salió del cofre |
| `especie` | umbrales para evaluar el ánimo; si están incompletos o son incoherentes se ignoran |
| `vinculo` | los días sanos los cuenta la nube, que ve el día entero; decide los adornos de la cara |
| `aceptadas` | cuántas lecturas del pedido quedaron guardadas: esas se borran del historial |
| `calibracion` | lecturas crudas del capacitivo en seco y sumergido |
| `pantalla` | `toque` (se apaga a batería) o `siempre` |

## Seguridad

**El token.** `HMAC-SHA256(secreto, "rootkit-api")` en hex. El secreto son 16
bytes de fábrica que nunca salen del aparato.

**El código.** `HMAC-SHA256(secreto, "rootkit-vinculo:" + época en 4 bytes
little endian)`, primeros 40 bits en base32 de Crockford: 8 caracteres sin I,
L, O ni U. Prueba que tenés la maceta adelante, porque sólo se ve en su
pantalla mientras no está vinculada, y deja de valer al desvincular.

**Vectores de referencia** (secreto `3a917c05ee4218b69d602fc3710e845b`):

| | |
|---|---|
| código, época 0 | `PTS0JHM6` |
| código, época 1 | `A8XTJCFQ` |
| código, época 7 | `T6QEH5MP` |
| token | `71859c23a4eb073e425391d23d46eede1760af53a9bec3b4b168724b2d8e6be3` |

Los verifican `test/test_enlace.c` en el firmware y `test/nube.test.mjs` en
root-lab. Si cambia la derivación de un lado, fallan los dos.

**Confianza al primer uso.** En desarrollo, el primer aparato que se presenta
con un `id` registra su token. En producción (`ROOTLAB_TOFU=0`) la estación
de fábrica registra cada token y la nube rechaza los desconocidos.

**Reinicio físico.** Si un aparato vinculado aparece con otra época, alguien
mantuvo apretado su botón: la nube rompe el vínculo viejo.

## Cadencia

| Estado | Consulta |
|---|---|
| sin vincular o esperando el cofre | cada 3 s (alguien tiene el teléfono en la mano); a batería, después de 20 min, cada 30 s |
| cara | cuando el muestreo tiene algo que contar, o cada `intervalo_s` |
| fallos seguidos | la espera se duplica hasta un minuto |

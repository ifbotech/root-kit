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
  "fw": "0.6.0",
  "placa": "c3-supermini",
  "pantalla": "st7735-128",
  "lote": "L2609",
  "ota": { "version": "0.6.0", "estado": "ok" },
  "persona": "brote",
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
| `fw`, `placa` | qué versión corre y en qué placa: con eso la nube decide si le ofrece una actualización |
| `lote` | el lote de fábrica, si lo tiene; la nube prefiere el que registró la fábrica |
| `ota` | sólo si alguna vez intentó actualizarse: la versión y cómo le fue (`bajando`, `verificando`, `ok`, `fallo`). Ver [ota.md](ota.md) |
| `persona` | el Rooti grabado en fábrica (`brote`, `musgo`, `pinchito`, `bulbo`, `champi`); vacío si no tiene |
| `estado` | el estado de `core/enlace.c` |
| `epoca` | sube con cada desvinculación |
| `codigo` | **sólo mientras no está vinculado**: el del QR |
| `reloj` | segundos monótonos del aparato, sumando los dormidos |
| `lecturas[].hace` | cuántos segundos antes de `reloj` se midió |
| `lecturas[]` | hasta 20 por pedido; los campos de un sensor que falló no van |
| `temp`, `tsuelo` | décimas de grado |
| `animo`, `sev` | lo que evaluó el propio aparato: es lo que muestra su cara |
| `fallas` | bits: 1 suelo, 2 aire, 4 luz, 8 sonda; 16 el último riego se escurrió (no es una falla de sensor) |
| `escurre` | `true` sólo cuando el detector de riego vio que el agua se escurrió sin empapar (ver [firmware.md](firmware.md#el-riego-que-se-escurre)); la nube lo guarda con la lectura |

**Sin reloj de pared.** El aparato no sabe la fecha y no le hace falta: el
servidor fecha cada lectura restando `hace` a su propia hora. Sin NTP, sin
pila de reloj, sin aparatos que arrancan en 1970.

### Lo que contesta la nube

```json
{
  "ok": true,
  "vinculado": true,
  "revelado": true,
  "persona": "brote",
  "rareza": "epico",
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
  "calibrando": true,
  "firmware": { "version": "0.6.1", "url": "https://ifbotech.com/rootkit/api/d/firmware/12",
                "sha256": "<64 hex>", "firma": "<DER en base64>", "tamano": 1159640 },
  "brillo": 80,
  "pantalla": "toque"
}
```

| Campo | Qué hace el aparato |
|---|---|
| `ok` | **sin `"ok": true` no se aplica nada**: un portal de hotel que contesta HTML no puede desvincular una maceta |
| `vinculado`, `revelado` | alimentan la máquina de estados |
| `persona` | qué Rooti es; si no tenía de fábrica, adopta el que le asignó la nube (fijo por id) |
| `rareza` | **sólo con el cofre abierto**: la piel que salió (`comun`, `raro`, `epico`). Se guarda en NVS y la cara se pinta con esa paleta; un valor desconocido se ignora. Al desvincular vuelve a `comun` |
| `especie` | umbrales para evaluar el ánimo; si están incompletos o son incoherentes se ignoran |
| `vinculo` | los días sanos los cuenta la nube, que ve el día entero; decide los adornos de la cara |
| `aceptadas` | cuántas lecturas del pedido quedaron guardadas: esas se borran del historial |
| `calibracion` | lecturas crudas del capacitivo en seco y mojado, las que tomó la persona desde la app; inválidas (invertidas, muy juntas, fuera de rango) se ignoran |
| `calibrando` | la app está calibrando ahora: medir y contar cada 5 s, sin dormirse. La nube lo apaga sola a los 10 minutos |
| `firmware` | hay una versión para esta placa y este canal que no es la que corre. Un manifiesto incompleto se ignora; el aparato decide si la baja ([ota.md](ota.md)) y la verifica (hash y firma) antes de instalar |
| `pantalla` | `toque` (se apaga a batería) o `siempre` |

### El binario de una actualización

`GET {base}/api/d/firmware/<id>`, con el mismo `Authorization: Bearer <token>`
del sync. Responde `application/octet-stream` con `Content-Length` igual al
`tamano` del manifiesto. Sin token de aparato: `401`.

## Seguridad

**Quién puede presentarse.** En producción la nube sólo acepta aparatos que
registró la estación de fábrica ([fabrica.md](fabrica.md)); los demás reciben
`401`. En desarrollo (`ROOTLAB_TOFU=1`) acepta al primero que se presenta con
un id.

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

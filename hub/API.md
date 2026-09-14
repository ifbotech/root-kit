# API del Hub

Contrato entre el Hub (PWA) y la Terminal. La Terminal es el servidor y la
fuente de verdad; el Hub es una vista. `dev-server.mjs` implementa este mismo
contrato en Node para poder desarrollar la interfaz sin hardware.

## Principios

**La Terminal calcula, el Hub muestra.** El estado de ánimo y la severidad se
evalúan una sola vez, en `firmware/core/mood.c`, y viajan ya resueltos. El Hub
nunca reimplementa esa lógica: dos implementaciones de la misma regla divergen,
y el día que divergen el usuario ve una cara en la pantalla y otra en el
teléfono.

**Todo vive en la red local.** La Terminal sirve la PWA por HTTP en
`rootkit.local`. No hay contenido mixto porque no hay HTTPS de por medio: si la
app se sirviera desde internet, el navegador bloquearía sus pedidos a una IP
privada.

**La nube se toca una sola vez.** `POST /api/identify` es el único endpoint que
sale a internet, y sólo cuando el usuario registra una planta. La clave de la
API de visión vive en la Terminal, nunca en la página.

## Convenciones

- JSON en UTF-8, `Content-Type: application/json`.
- Temperaturas en **décimas de grado** (`temp_dc: 236` son 23,6 °C), igual que
  en el firmware. Evita el punto flotante de punta a punta.
- Marcas de tiempo en segundos Unix.
- Errores: código HTTP + `{ "error": "texto corto" }`.

## Endpoints

### `GET /api/state`

Todo lo que la pantalla principal necesita, en un solo viaje. Un ESP32 sirviendo
varios pedidos chicos paga más en handshakes que en bytes.

```json
{
  "terminal": { "fw": "0.4.0", "uptime_s": 84213, "wifi_rssi": -54 },
  "plants": [
    {
      "id": "p1",
      "nombre": "MONSTERA",
      "especie": "monstera",
      "simbionte": "tuga",
      "mood": "THIRSTY",
      "severity": "URGENT",
      "reason": "tengo sed",
      "tel": { "soil_pct": 22, "temp_dc": 236, "rh_pct": 54,
               "lux": 5200, "batt_mv": 3810, "age_s": 240 },
      "spore": { "id": "a4cf129b4011", "batt_pct": 62, "seq": 4211 }
    }
  ]
}
```

### `GET /api/species`

Catálogo con los rangos de confort. Lo usa el alta de plantas.

```json
[ { "id": "monstera", "nombre": "Monstera deliciosa",
    "soil_min": 25, "soil_max": 60,
    "temp_min_dc": 180, "temp_max_dc": 300,
    "rh_min": 50, "lux_min": 1000, "lux_max": 15000 } ]
```

### `POST /api/identify`

Identificación por foto. Es el único camino a internet.

Pedido: `{ "image_b64": "...", "mime": "image/jpeg" }`
Respuesta:

```json
{
  "especie": "monstera",
  "nombre": "Monstera deliciosa",
  "confianza": 0.93,
  "alternativas": [ { "especie": "pothos", "confianza": 0.04 } ]
}
```

La confianza viaja siempre y la interfaz la muestra: una identificación de 0,41
tiene que poder corregirse antes de guardar, porque de ahí salen los umbrales
con los que se juzga la planta el resto de su vida.

### `POST /api/plants`

`{ "nombre": "MONSTERA", "especie": "monstera", "spore_id": "a4cf129b4011" }`
→ `201` con la planta creada, incluido el simbionte asignado.

Al alta se desbloquea el simbionte de esa especie si era nuevo. **El desbloqueo
es determinista, no aleatorio:** una especie nueva da su simbionte. La sensación
de colección se conserva y no se entra en el terreno de las cajas de botín, que
Bélgica y Países Bajos ya restringen.

### `PATCH /api/plants/:id`

Acepta `nombre`, `especie` y `spore_id`. Cambiar la especie recalcula los
umbrales que la Terminal manda al Spore.

### `DELETE /api/plants/:id`

`204`. El simbionte ya desbloqueado se conserva en la colección.

### `GET /api/history/:id?n=48`

Buffer circular de lecturas, el más reciente primero. La Terminal guarda 48
puntos por planta en RAM; el resto se pierde a propósito, porque una base de
datos histórica en un ESP32 es una fuente de corrupción en el corte de luz.

```json
{ "id": "p1", "puntos": [ { "t": 1757800000, "soil_pct": 22, "temp_dc": 236,
                            "rh_pct": 54, "lux": 5200 } ] }
```

### `GET /api/collection`

```json
{ "desbloqueados": ["tuga"], "total": 12,
  "catalogo": [ { "id": "tuga", "nombre": "Tuga.exe",
                  "especie": "monstera", "desbloqueado": true } ] }
```

### `GET /api/spores`

Spores que emitieron y todavía no están asignados a una planta. Es lo que
permite dar de alta una maceta sin tipear un identificador.

## Lo que el Hub NO hace

- No evalúa estados de ánimo.
- No guarda credenciales de Wi-Fi: el Spore se aprovisiona por su propio punto
  de acceso, porque Web Bluetooth no existe en Safari y la mitad de los
  usuarios están en iPhone.
- No escribe en la nube sin que el usuario lo pida explícitamente.

# API del Hub

Contrato entre el Hub (PWA) y el Prime. El Prime es el servidor y la fuente de
verdad; el Hub es una vista. `dev-server.mjs` implementa este mismo contrato en
Node para poder desarrollar la interfaz sin hardware.

## Principios

**Cada nodo evalúa su propia maceta; el Prime junta y el Hub muestra.** El
estado de ánimo y la severidad se calculan una sola vez, en el nodo que tiene
los sensores, corriendo `firmware/core/mood.c`. Viajan resueltos al Prime, y de
ahí al Hub. Ni el Prime ni el Hub los recalculan: dos implementaciones de la
misma regla divergen, y el día que divergen el usuario ve una cara en la
maceta, otra en el escritorio y otra en el teléfono.

**Un kit es un Prime y hasta cinco Minis.** El Prime va enchufado, tiene la
pantalla grande y sirve esta API. Los Minis van a batería, uno por maceta. Los
dos tienen sensores, los dos tienen su propia planta y los dos muestran al
simbionte de esa planta —el Prime como adulto de 22 mm, el Mini como brote de
13 mm. El rol es una etiqueta, no una jerarquía: en las listas de esta API los
nodos no vienen ordenados por rol.

**Todo vive en la red local.** El Prime sirve la PWA por HTTP en
`rootkit.local`. No hay contenido mixto porque no hay HTTPS de por medio: si la
app se sirviera desde internet, el navegador bloquearía sus pedidos a una IP
privada.

**La nube se toca una sola vez.** `POST /api/identify` es el único endpoint que
sale a internet, y sólo cuando el usuario registra una planta. La clave de la
API de visión vive en el Prime, nunca en la página.

## Convenciones

- JSON en UTF-8, `Content-Type: application/json`.
- Temperaturas en **décimas de grado** (`temp_dc: 236` son 23,6 °C), igual que
  en el firmware. Evita el punto flotante de punta a punta.
- Marcas de tiempo en segundos Unix.
- Errores: código HTTP + `{ "error": "texto corto" }`.

### Enumeraciones

| Campo | Valores |
|---|---|
| `role` | `PRIME`, `MINI` |
| `link` | `NUNCA`, `VIVO`, `TIBIO`, `CAIDO` |
| `mood` | `UNKNOWN`, `OFFLINE`, `SLEEPING`, `HAPPY`, `THIRSTY`, `DROWNING`, `COLD`, `HOT`, `SCORCHED`, `DARK`, `PARCHED_AIR` |
| `severity` | `OK`, `WATCH`, `URGENT` |
| etapa | `ESPORA`, `BROTE`, `JOVEN`, `MADURO`, `ANCESTRAL` |

`link` sale de la antigüedad de la última trama, con los umbrales de
`firmware/core/node.h`: `VIVO` hasta 4 h, `TIBIO` hasta 6 h, `CAIDO` después.
Es distinto del ánimo — un nodo puede estar callado y la planta perfecta.

La etapa **no viaja**: el Hub la deriva de `bond.dias_sanos` con la misma
escalera que el firmware (`0, 7, 30, 90, 180`). Es la única regla duplicada a
propósito en todo el sistema, porque son cinco números que permiten dibujar la
barra de progreso sin un viaje más, y hay tests de los dos lados que fallan si
se separan.

## Endpoints

### `GET /api/state`

Todo lo que la pantalla principal necesita, en un solo viaje. Un ESP32 sirviendo
varios pedidos chicos paga más en handshakes que en bytes.

```json
{
  "prime": { "fw": "0.5.0", "uptime_s": 84213, "wifi_rssi": -54, "minis": 2 },
  "nodes": [
    {
      "id": "p1",
      "nombre": "MONSTERA",
      "role": "PRIME",
      "link": "VIVO",
      "especie": "monstera",
      "simbionte": "tuga",
      "mood": "THIRSTY",
      "severity": "URGENT",
      "reason": "tengo sed",
      "tel": { "soil_pct": 22, "temp_dc": 236, "rh_pct": 54,
               "lux": 5200, "batt_mv": 4200, "age_s": 240 },
      "nodo": { "id": "a4cf129b4011", "batt_pct": null, "seq": 4211 },
      "bond": { "dias_vividos": 104, "dias_sanos": 95,
                "racha": 12, "mejor_racha": 40 }
    }
  ]
}
```

`nodo.batt_pct` es `null` en el Prime, que va enchufado. La interfaz dibuja un
enchufe y no una pila llena: dicen cosas distintas.

`bond` es el vínculo con esa planta. `dias_sanos` es el que mueve la etapa;
`dias_vividos` sólo cuenta el tiempo. Un día malo corta la racha pero **no
borra** los días sanos acumulados.

### `GET /api/species`

Catálogo con los rangos de confort. Lo usa el alta, y son los mismos umbrales
que el Prime le manda a cada Mini en la trama de configuración.

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

### `POST /api/nodes`

`{ "nombre": "MONSTERA", "especie": "monstera", "nodo_id": "a4cf129b4011" }`
→ `201` con el nodo creado, incluido el simbionte asignado.

`role` es opcional y por defecto `MINI`; el Prime se da de alta solo al
arrancar por primera vez. Un segundo `PRIME` se rechaza con `400`: el kit tiene
exactamente uno.

Al alta se desbloquea el simbionte de esa especie si era nuevo, **el Prime
reproduce la ceremonia de apertura** y recién entonces le manda al Mini la
configuración con sus umbrales y su simbionte. El desbloqueo es determinista,
no aleatorio: una especie nueva da su simbionte. La sensación de colección se
conserva y no se entra en el terreno de las cajas de botín, que Bélgica y
Países Bajos ya restringen.

### `PATCH /api/nodes/:id`

Acepta `nombre`, `especie` y `nodo_id`. Cambiar la especie recalcula los
umbrales y dispara una configuración nueva hacia ese nodo.

### `DELETE /api/nodes/:id`

`204`. El simbionte ya desbloqueado se conserva en la colección.

### `GET /api/history/:id?n=48`

Buffer circular de lecturas, el más reciente primero. El Prime guarda 48 puntos
por nodo en RAM; el resto se pierde a propósito, porque una base de datos
histórica en un ESP32 es una fuente de corrupción en el corte de luz.

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

### `GET /api/nodos`

Nodos que emitieron su `HELLO` y todavía no están asignados a una planta. Es lo
que permite dar de alta una maceta sin tipear un identificador: el Mini muestra
en su pantalla un código de seis caracteres y acá aparece la misma lista.

```json
[ { "id": "a4cf129b40aa", "role": "MINI", "rssi": -61, "visto_hace_s": 12 } ]
```

## Lo que el Hub NO hace

- No evalúa estados de ánimo.
- No guarda credenciales de Wi-Fi: el nodo se aprovisiona por su propio punto
  de acceso, porque Web Bluetooth no existe en Safari y la mitad de los
  usuarios están en iPhone.
- No escribe en la nube sin que el usuario lo pida explícitamente.
- No ordena la lista por rol. El Prime no va primero por ser el Prime.

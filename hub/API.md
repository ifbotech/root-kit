# API de la app

Contrato entre la app y los aparatos. `dev-server.mjs` lo implementa en Node
para poder desarrollar la interfaz sin hardware.

## Principios

**Las métricas viven acá.** El aparato en la maceta sólo dibuja una cara: ni
números, ni barras, ni nombre. Todo el tablero —lecturas, historial, rangos de
la especie— es de la app, porque quien va a leer un 34% ya tiene el teléfono en
la mano. Sacar la interfaz del aparato es además lo que hace que la carcasa
impresa mande: una pantalla llena de barras compite con el objeto, una cara lo
completa.

**Cada aparato calcula su propio ánimo; la app lo muestra.** El estado y la
severidad se evalúan una sola vez, en el aparato que tiene los sensores,
corriendo `firmware/core/mood.c`. Viajan resueltos. La app no los recalcula:
dos implementaciones de la misma regla divergen, y el día que divergen el
usuario ve una cara en la maceta y otra distinta en el teléfono.

**Dos ejes independientes: la especie y la carcasa.** La especie decide con qué
umbrales se juzga la planta; la carcasa decide qué cara pone el aparato.
Ninguna deriva de la otra. Dos macetas con la misma planta pueden tener
carcasas distintas, y la misma carcasa puede cuidar cualquier planta.

**La colección es de objetos físicos.** Lo que se junta son las carcasas
impresas que salieron en la caja ciega, no personajes desbloqueados por
software. La app registra lo que ya está en la casa.

**Dónde se sirve esto todavía no está decidido, y el contrato no depende de
eso.** Son las dos opciones, con la restricción que las separa:

| | Local | Nube |
|---|---|---|
| Quién sirve | un concentrador en la casa | un backend |
| La app se abre desde | `rootkit.local` por HTTP | una URL pública por HTTPS |
| El QR de la caja | lleva a la IP local | lleva a la URL, y con eso alcanza |
| Notificaciones push | no | sí |
| Los datos salen de la casa | no | sí |

**La restricción que decide:** una página servida por HTTPS desde internet
**no puede** pedirle datos a una IP privada — el navegador lo bloquea y no hay
forma limpia de esquivarlo. Así que si la app se hospeda afuera, los aparatos
tienen que publicar hacia el backend, y no la app ir a buscarlos.

Mientras tanto la app habla contra este contrato y `dev-server.mjs` lo
implementa localmente, así que la decisión no bloquea nada.

**La clave de la API de visión nunca vive en la página.** `POST /api/identify`
y `POST /api/diagnose` son los dos únicos endpoints que salen a internet, y la
clave queda del lado del servidor sea cual sea.

## Convenciones

- JSON en UTF-8, `Content-Type: application/json`.
- Temperaturas en **décimas de grado** (`temp_dc: 236` son 23,6 °C), igual que
  en el firmware. Evita el punto flotante de punta a punta.
- Marcas de tiempo en segundos Unix.
- Errores: código HTTP + `{ "error": "texto corto" }`.

### Enumeraciones

| Campo | Valores |
|---|---|
| `link` | `NUNCA`, `VIVO`, `TIBIO`, `CAIDO` |
| `mood` | `UNKNOWN`, `OFFLINE`, `SLEEPING`, `HAPPY`, `THIRSTY`, `DROWNING`, `COLD`, `HOT`, `SCORCHED`, `DARK`, `PARCHED_AIR` |
| `severity` | `OK`, `WATCH`, `URGENT` |
| etapa | `ESPORA`, `BROTE`, `JOVEN`, `MADURO`, `ANCESTRAL` |
| `rareza` | `COMUN`, `RARO`, `SECRETO` |

`link` sale de la antigüedad de la última trama, con los umbrales de
`firmware/core/node.h`: `VIVO` hasta 4 h, `TIBIO` hasta 6 h, `CAIDO` después.
Es distinto del ánimo — un aparato puede estar callado y la planta perfecta.

La etapa **no viaja**: la app la deriva de `bond.dias_sanos` con la misma
escalera que el firmware (`0, 7, 30, 90, 180`). Es la única regla duplicada a
propósito en todo el sistema, porque son cinco números que permiten dibujar la
barra de progreso sin un viaje más, y hay tests de los dos lados que fallan si
se separan.

## Endpoints

### `GET /api/state`

Todo lo que el tablero necesita, en un solo viaje.

```json
{
  "app": { "fw": "0.6.0", "aparatos": 3 },
  "nodes": [
    {
      "id": "p1",
      "nombre": "MONSTERA",
      "modelo": "cresta",
      "link": "VIVO",
      "especie": "monstera",
      "mood": "THIRSTY",
      "severity": "URGENT",
      "reason": "tengo sed",
      "tel": { "soil_pct": 22, "temp_dc": 236, "rh_pct": 54,
               "lux": 5200, "batt_mv": 3810, "age_s": 240 },
      "nodo": { "id": "a4cf129b4011", "batt_pct": 62, "seq": 4211 },
      "bond": { "dias_vividos": 104, "dias_sanos": 95,
                "racha": 12, "mejor_racha": 40 }
    }
  ]
}
```

`modelo` puede ser `null`: el aparato mide y avisa igual, pero no sabe qué cara
poner hasta que el usuario declare qué carcasa le tocó.

`bond` es el vínculo con esa planta. `dias_sanos` es el que mueve la etapa;
`dias_vividos` sólo cuenta el tiempo. Un día malo corta la racha pero **no
borra** los días sanos acumulados.

### `GET /api/species`

Catálogo con los rangos de confort. Los mismos umbrales que la app le manda a
cada aparato en la trama de configuración.

```json
[ { "id": "monstera", "nombre": "Monstera deliciosa",
    "soil_min": 25, "soil_max": 60,
    "temp_min_dc": 180, "temp_max_dc": 300,
    "rh_min": 50, "lux_min": 1000, "lux_max": 15000, "dificultad": 45 } ]
```

`dificultad` ya no decide nada —la rareza se mudó a la caja física— pero se
sigue publicando porque es información útil al elegir una planta.

### `POST /api/identify`

Identificación por foto. Es el único camino a internet.

Pedido: `{ "image_b64": "...", "mime": "image/jpeg" }`

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

### `POST /api/diagnose`

Diagnóstico por foto de una planta **ya registrada**.

Pedido: `{ "image_b64": "...", "planta": "p1", "mime": "image/jpeg" }`

```json
{ "planta": "p1", "hallazgos": ["hojas_amarillas"], "confianza": 0.86 }
```

Devuelve **hallazgos visuales y nada más**: el modelo dice qué ve, no por qué.
La causa sale de cruzar cada hallazgo con la telemetría, y eso lo hace la app
en `lib/diagnostico.mjs`. Separarlo así es lo que permite testear el cruce sin
una API de visión de por medio, y es donde está el valor: el mismo síntoma con
distinta telemetría es un problema distinto.

Los hallazgos son una lista cerrada: `sana`, `hojas_amarillas`,
`puntas_marrones`, `manchas`, `caida`, `hojas_caidas`, `tallo_estirado`,
`plagas`, `moho`. Lo que venga fuera de esa lista se ignora en vez de
interpretarse.

### `POST /api/nodes`

```json
{ "nombre": "MONSTERA", "especie": "monstera",
  "nodo_id": "a4cf129b4011", "modelo": "cresta" }
```

→ `201` con el aparato creado y `modelo_nuevo` diciendo si esa carcasa se sumó
a la colección.

`modelo` es opcional: se puede declarar después. Un modelo que no está en el
catálogo se rechaza con `400`.

Al alta la app le manda al aparato la configuración con los umbrales de la
especie y el índice de la carcasa, y el aparato reproduce **el primer
encendido** —se descubre los rasgos y se presenta. No hay azar en ese momento:
el azar ya ocurrió cuando el usuario abrió la caja.

### `PATCH /api/nodes/:id`

Acepta `nombre`, `especie`, `nodo_id` y `modelo`. Cambiar la especie recalcula
los umbrales; cambiar el modelo cambia la cara. Las dos cosas disparan una
configuración nueva hacia ese aparato.

### `DELETE /api/nodes/:id`

`204`. La carcasa se conserva en la colección: sigue estando en la casa aunque
la planta se haya muerto.

### `GET /api/history/:id?n=48`

Buffer circular de lecturas, el más reciente primero. Se guardan 48 puntos por
aparato en RAM; el resto se pierde a propósito, porque una base de datos
histórica en un ESP32 es una fuente de corrupción en el corte de luz.

```json
{ "id": "p1", "puntos": [ { "t": 1757800000, "soil_pct": 22, "temp_dc": 236,
                            "rh_pct": 54, "lux": 5200 } ] }
```

### `GET /api/collection`

Qué carcasas ya tenés.

```json
{ "tengo": ["cresta", "kawaii"], "total": 5,
  "catalogo": [ { "idx": 0, "id": "cresta", "nombre": "Cresta",
                  "rareza": "COMUN", "carcasa": "carcasas/cresta.stl",
                  "lema": "No te va a agradecer. Igual regala.",
                  "tengo": true } ] }
```

**El modelo secreto no aparece en el catálogo hasta que sale.** Listarlo en
gris ya le contaría al usuario que existe, y ahí deja de ser un secreto para
ser una casilla vacía. Por eso `total` cuenta sólo los visibles y sube a seis
recién cuando el secreto aparece.

`idx` es la posición en `rk_persona_table` y es la clave con la que la carcasa
viaja por radio. Si la tabla del firmware se reordena sin regenerar el
catálogo, cada maceta se pone la cara de la vecina; hay tests de los dos lados
que lo verifican.

### `POST /api/collection`

`{ "modelo": "glitch" }` → `200` con `nuevo` diciendo si no la tenías.

Declarar una carcasa sin registrar una planta. Existe porque la caja se abre
antes de que haya tierra, y el momento de "me salió el secreto" no puede
esperar a tener una maceta.

### `GET /api/nodos`

Aparatos que emitieron su `HELLO` y todavía no están asignados a una planta. Es
lo que permite dar de alta una maceta sin tipear un identificador: el aparato
muestra en su pantalla un código de seis caracteres y acá aparece la misma
lista.

```json
[ { "id": "a4cf129b40aa", "rssi": -61, "visto_hace_s": 12 } ]
```

## Lo que la app NO hace

- No evalúa estados de ánimo.
- No guarda credenciales de Wi-Fi: el aparato se aprovisiona por su propio
  punto de acceso, porque Web Bluetooth no existe en Safari y la mitad de los
  usuarios están en iPhone.
- No escribe en la nube sin que el usuario lo pida explícitamente.
- No ordena la lista por carcasa. Una maceta con sed importa lo mismo tenga el
  modelo que tenga.
- No cuenta carcasas repetidas. Registra qué modelos tenés, no cuántos de cada
  uno: un inventario no da ganas de completar nada.

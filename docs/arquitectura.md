# Arquitectura

## Tres piezas

```
   ┌───────────────────────┐        HTTPS         ┌────────────────────────┐
   │        ROOTKIT        │  POST /api/d/sync    │         NUBE           │
   │  ESP32-C3 + pantalla  │ ───────────────────► │   root-lab/server      │
   │  + sensores + batería │ ◄─────────────────── │  cuentas, vínculos,    │
   │                       │  vínculo, especie,   │  lecturas, cofre, IA,  │
   │  QR  ·  ojos          │  nombre, días sanos  │  notificaciones        │
   └───────────┬───────────┘                      └───────────▲────────────┘
               │ portal cautivo (sólo para                    │ HTTPS
               │ pasarle el wifi)                              │ /api/...
               ▼                                               │ Web Push
   ┌───────────────────────────────────────────────────────────┴────────────┐
   │                               APP (PWA)                                 │
   │   root-lab/public — alta por QR, cofre, foto, tablero, colección        │
   └─────────────────────────────────────────────────────────────────────────┘
```

| Pieza | Repositorio | Qué hace |
|---|---|---|
| **ROOTKIT** | `rootkit/firmware` | Mide, evalúa su planta y pone una cara. Sin la red sigue funcionando. |
| **Nube** | `root-lab/server` | Vincula aparatos con cuentas, guarda lecturas, abre cofres, identifica plantas con IA, manda notificaciones. |
| **App** | `root-lab/public` | Lo que ve el usuario. Se instala desde el QR, sin tienda. |

## La idea que ordena todo: la variedad es física, la cara es digital

El personaje es la **carcasa impresa en 3D**: el cuerpo del Rooti (las hojas
del Brote, el sombrero del Champi, el brazo del Pinchito). En la maceta lo
único que se dibuja es la cara, y la cara le hace juego; en la app se ve el
Rooti entero, con la misma silueta que se imprime. Es mejor reparto
de esfuerzo que animar cuerpos: una carcasa nueva cuesta filamento y unas
horas de modelado; un cuerpo animado, semanas.

La pantalla muestra **dos cosas en toda su vida**: el QR del primer encendido
y los ojos. Todo lo que es número, tarea o configuración vive en la app. Una
pantalla llena de barras compite con el objeto; una cara lo completa.

### El ánimo dice qué siente; la persona dice cómo lo muestra

```
core/mood.c     QUÉ siente      sed, frío, poca luz...   (11 ánimos)
core/persona.c  CÓMO lo muestra ojos, cejas, boca y 3 pieles (5 Rooties)
art/face.c      los cruza y dibuja
```

Cinco Rooties por tres pieles por once ánimos son ciento sesenta y cinco
caras, más parpadeo, mirada y respiración, y salen del mismo código porque la
cara es procedural. Un personaje nuevo es una fila de `persona.c`.

### Ilustración, no pixel art

Las caras son ilustración plana del tipo Duolingo: formas redondas, colores
planos, sin contorno, **bordes suavizados**. El suavizado se calcula en punto
fijo en `gfx/aa.c` porque el C3 no tiene FPU. Ver [firmware.md](firmware.md).

## El flujo

```
 1  encender              la pantalla muestra el QR y abre la red ROOTKIT-XXXX
 2  escanear el QR        abre la app en /v/<código>
 3  instalar              "agregar a inicio": en iPhone es lo que habilita avisos
 4  activar avisos        Web Push
 5  wifi                  el teléfono se conecta a ROOTKIT-XXXX y le pasa la red
 6  vincular              la nube ve a la maceta con el mismo código: es tuya
                          la maceta pasa del QR a unos ojos dormidos
                          la app lo reconoce: "¡Conectaste a tu Brote!"
 7  abrir el cofre        sale la piel (común, rara o épica); la maceta abre
                          los ojos con esos colores y la app se pinta
 8  nombre
 9  foto                  la IA identifica la especie y fija los umbrales
10  la cara               la maceta cruza sensores con especie y pone cara;
                          la app muestra tareas y avisa lo que haga falta
```

**Desvincular** (desde la app, o manteniendo el botón 10 s en la maceta)
vuelve al paso 1 con un código nuevo.

Por qué este orden, paso por paso: `root-lab/docs/flujo.md`.

## Quién decide qué

| Decisión | Dónde | Por qué ahí |
|---|---|---|
| Qué cara poner | **el aparato** (`core/mood.c`) | Una maceta que necesita la red para saber si tiene sed se queda muda justo cuando importa |
| Qué pantalla mostrar | **el aparato** (`core/enlace.c`) | Mismo motivo |
| Con qué umbrales | **la nube** (la especie) | Sale de la foto y del catálogo curado; se cambia desde la app |
| Qué Rooti es | **fábrica** (la nube asigna uno fijo si no hay) | La carcasa ya es un personaje |
| Qué piel tiene | **el cofre**, en la nube; el aparato la guarda en NVS | Es la sorpresa, y tiene que ser la misma en la maceta y en la app |
| Los días sanos | **la nube** | Ve el día entero aunque el aparato duerma |
| Qué hay que hacer hoy | **la app** (`lib/tareas.mjs`) | Es presentación: verbos y números para una persona |
| Cuándo avisar | **la nube** (`server/avisos.mjs`) | Tiene que poder avisar con la app cerrada |

La cara del teléfono es **la misma** que la de la maceta: la app carga el
firmware compilado a WebAssembly (`make wasm`) y dibuja con él. No hay una
segunda implementación que se pueda desincronizar.

## Capas del firmware

```
 esp32/     main · pantalla · sensores_hw · almacen · portal · red · energia
 ─────────────────────────────────────────────────────────── (sólo hardware)
 ui/        cara · despertar · qr
 art/       look · face
 gfx/       fb · aa · font
 net/       json · nube
 nodo/      sensores · soil · power · sampler · historial
 core/      mood · species · persona · vinculo · enlace · codigo · sha256
```

Todo lo que está debajo de la línea es C99 portable y se prueba en el
escritorio: 1449 comprobaciones, incluida la regresión visual de las 88
caras.

## Decisiones que conviene no revisitar sin leer esto

**Renderer propio en vez de LVGL.** La cara es procedural: elipses y
semiplanos calculados, no widgets. Un framebuffer RGB565 plano es lo que
espera el panel, y el mismo código dibuja en la placa, en el simulador y en el
navegador.

**Nube en vez de red local.** Las notificaciones tienen que llegar con la app
cerrada, y una página HTTPS no puede hablarle a una IP de la casa. La maceta
evalúa sola; la nube sólo es necesaria para ver y para avisar.

**JSON en vez de binario.** El protocolo binario existía para ESP-NOW, donde
cada byte costaba aire. Con HTTPS el costo lo pone el apretón de manos TLS, no
el tamaño del cuerpo. Ver [decisiones.md](decisiones.md).

**Portal cautivo en vez de Bluetooth.** Web Bluetooth no existe en iPhone.

**PWA en vez de app nativa.** Se instala desde el QR, sin tienda, y se
actualiza sola.

## Presupuesto energético

Ver [hardware.md](hardware.md#cuánto-dura). A batería: ~5 meses el modelo de
2,2" con una 18650, ~2 meses el mini con una LiPo de 1000 mAh, a verificar
con medición en la Fase 2 del [roadmap](roadmap.md).

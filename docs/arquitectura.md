# Arquitectura

Tres piezas, y una regla que las ordena: **cada dato se calcula en un solo
lugar.** El Spore mide y no interpreta; la Terminal interpreta y decide; el Hub
muestra y no recalcula nada. Cuando esa regla se rompe, dos implementaciones de
la misma lógica divergen y el usuario ve una cara en la pantalla y otra en el
teléfono.

```
   ┌──────────┐   telemetría 24 B    ┌────────────┐   HTTP/JSON   ┌─────────┐
   │  SPORE   │ ───────────────────▶ │  TERMINAL  │ ◀───────────▶ │   HUB   │
   │ ESP32-C3 │                      │  ESP32-S3  │   red local   │  PWA    │
   │ 18650    │ ◀─────────────────── │  enchufada │               │ celular │
   └──────────┘   config 18 B        └────────────┘               └─────────┘
     mide                              interpreta                   muestra
     duerme                            dibuja                       registra
                                       sirve el Hub
                                             │
                                             ▼  sólo al registrar una planta
                                       API de visión
```

## Por qué cada pieza es como es

### El Spore es deliberadamente tonto

No conoce especies, no sabe qué es un umbral y no decide nada. Mide, empaqueta
y duerme. Tres razones:

1. **Batería.** Sin lógica no hay CPU despierta. Todo el presupuesto se va en
   la radio, que es donde se puede optimizar de verdad.
2. **Actualizaciones.** Corregir los rangos de una especie para todos los
   usuarios es tocar la Terminal, no seis nodos enterrados en macetas.
3. **Una sola implementación.** La máquina de estados de ánimo vive en
   `firmware/core/mood.c` y corre en un solo lugar.

Lo único que el Spore sí hace por su cuenta es decidir **cuándo** hablar, y
eso porque la decisión depende de datos que sólo él tiene en el momento.

### La Terminal va enchufada

Con alimentación fija la pantalla puede quedar siempre encendida —un simbionte
que hay que despertar tocando es peor mascota que uno que está siempre vivo— y
el refresco total obligatorio del AXS15231B deja de importar. La batería queda
sólo donde hace falta: en los Spores.

### El Hub es una vista, no una aplicación

La foto obliga a tener el celular, pero el celular no obliga a tener una app
nativa: una página web abre la cámara con `<input capture>`. Sin App Store no
hay cuota anual, ni revisión de actualizaciones, ni —sobre todo— nadie
revisando las mecánicas de colección contra las políticas de cajas de botín.

La Terminal sirve la PWA por HTTP en la red local. No puede servirse desde
internet: una página HTTPS tiene prohibido pedirle datos a una IP privada, y
no hay forma limpia de esquivarlo.

## Capas del firmware

```
firmware/
  core/    C99 puro. Sin LVGL, sin ESP-IDF, sin punto flotante.
           Compila idéntico en el simulador, en los tests y en el ESP32.
  net/     Protocolo binario. También sin dependencias.
  spore/   Suelo, batería y muestreo adaptativo. Sin dependencias.
  gfx/     Framebuffer RGB565 y primitivas.
  art/     Tuga.exe: cuerpo generado, expresiones a mano, rig procedural.
  ui/      Composición de la pantalla. Función pura de (estado, tiempo).
  sim/     Host SDL de escritorio. La única capa que sabe de un sistema
           operativo, y la única que no va al dispositivo.
```

La dirección de las dependencias es siempre hacia adentro: `ui` usa `gfx` y
`core`, nunca al revés. Nada de `core`, `net` ni `spore` incluye una cabecera
de sistema más allá de `stdint`, `stdbool`, `stddef` y `string`.

## Decisiones que conviene no revisitar sin leer esto

### Renderer propio en vez de LVGL

Tres razones específicas de este hardware:

1. El AXS15231B **no soporta refresco parcial**, así que hay que reenviar el
   cuadro entero siempre. La optimización principal de LVGL —redibujar sólo
   los rectángulos sucios— no aplica.
2. Un buffer RGB565 plano es exactamente lo que espera
   `esp_lcd_panel_draw_bitmap()`.
3. El pixel art necesita escalado por enteros. LVGL escala pensando en
   suavizado.

### Lienzo lógico de 160×240, no 320×480

Se presenta a 320×480, exactamente 2×, sin interpolación. Y hay un motivo de
performance además del estético: **el framebuffer lógico son 75 KB y entran en
la SRAM interna del S3**, que tiene 512 KB. Rasterizar a 320×480 serían 300 KB
y habría que ir a PSRAM, que es un orden de magnitud más lenta. El escalado se
hace al vuelo mientras se empuja por QSPI.

### El bus manda, no la CPU

`make bench` mide el rasterizado: unos 0,035 ms por cuadro en un x86, tal vez
0,5 ms en el S3. Pero empujar 307.200 bytes por QSPI a ~40 MB/s son **7,3 ms**.
El bus es un orden de magnitud más caro que dibujar.

La consecuencia práctica: seguir optimizando el rasterizado rinde poco. El
margen que queda está en solapar el envío por DMA con el dibujo del cuadro
siguiente.

### Binario en vez de JSON entre Spore y Terminal

Una trama de telemetría son 24 bytes. El mismo contenido en JSON son unos 180.
Cada byte es tiempo de radio encendida, y la radio es el 46% del presupuesto
energético del Spore.

### Nanoamperios-hora en el modelo de consumo

Una medición cuesta 0,25 µAh. En enteros de microamperios-hora eso se redondea
a cero y el término desaparece del modelo. Con nAh los tres términos conviven
sin punto flotante, que el firmware no quiere.

## Presupuesto energético del Spore

Salido de `make firmware`, no de una planilla: las cifras se recalculan en cada
corrida y el test falla si alguien cambia un parámetro sin querer.

| Perfil | µAh/día | Autonomía (18650 2200 mAh, −20%) | Durmiendo |
|---|---:|---:|---:|
| Ingenuo — LED puesto, DHCP, transmite siempre | 15.206 | **115 días** | 47% |
| Fijo — optimizado, cada 15 min | 3.672 | **479 días** | 26% |
| Adaptativo — mide seguido, emite poco | 1.900 | **926 días** | 50% |

Con el perfil adaptativo, dormir y transmitir quedan casi empatados. Eso
significa que el firmware ya dio lo que tenía para dar: **el próximo
microamperio hay que ir a buscarlo al hardware**, desoldando el LED de
alimentación y eligiendo un LDO de bajo reposo.

## Lo que falta para el primer prototipo

- Capa HAL del ESP32: drivers de ADC, I2C, QSPI y Wi-Fi bajo las interfaces
  que el núcleo ya define.
- Servidor HTTP en la Terminal que implemente `hub/API.md`.
- Persistencia en NVS: calibración del Spore, plantas registradas, colección.
- Carcasas.

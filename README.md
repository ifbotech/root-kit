# ROOTKIT

Ecosistema cyber-botánico: sensores en las macetas, un simbionte pixel-art en
el escritorio que reacciona en tiempo real a lo que miden.

## Las tres piezas

| Pieza | Qué es | Hardware |
|---|---|---|
| **Terminal** | El objeto de escritorio. Muestra al simbionte a color y es el servidor local del sistema. | Guition JC3248W535 — ESP32-S3, 3,5" 320×480 IPS, táctil capacitivo. Enchufada, sin batería. |
| **Spore** | Un nodo por maceta. Mide y reporta. | ESP32-C3 + capacitivo v2.0 + AHT21 + BH1750 + 18650. Deep sleep, meses de autonomía. |
| **Hub** | PWA servida por la Terminal. Registrar plantas con la cámara del celular, ver la colección. | Ninguno. |

![Todos los estados de ánimo](tools/preview/sheet.png)

## Estructura

```
firmware/
  core/          C99 puro: la lógica del simbionte. Sin LVGL, sin ESP-IDF.
    telemetry.h  Lo que un Spore reporta.
    species.*    Rangos de confort por especie.
    mood.*       Telemetría -> estado de ánimo. El corazón del producto.
  gfx/           Renderer RGB565 propio: framebuffer, sprites, tipografía 5x7.
  art/           Tuga.exe. Cuerpo generado + expresiones dibujadas a mano.
  ui/            Composición de la pantalla de la Terminal.
  sim/           Host SDL de escritorio y modos de captura.
  test/          Tests del núcleo. Corren sin hardware ni SDL.
tools/
  gen_art.py     Autoría del arte -> firmware/art/tuga_data.{h,c}
  bmp2png.py     Convierte las capturas del simulador.
docs/
hub/             (vacío todavía)
```

## Empezar

```bash
cd firmware
make test      # 19 tests del núcleo, sin gráficos
make sim       # simulador interactivo, ventana 320x480
make sheet     # hoja de contacto con todos los ánimos
```

Necesita un compilador de C y SDL2. En Windows, dentro de WSL — ver
[docs/simulador-lvgl.md](docs/simulador-lvgl.md).

En el simulador: `1-3` cambia de planta, `W` riega, `M` cicla los ánimos,
`R` vuelve al ánimo real, `+/-` acelera el tiempo, `S` captura, `ESC` sale.
Un día simulado dura 48 segundos, así que el ciclo de luz y el secado de la
tierra se ven sin esperar.

## Por qué un renderer propio y no LVGL

Tres razones, y las tres son específicas de este hardware:

1. El AXS15231B de la Guition **no soporta refresco parcial**, así que hay que
   reenviar el cuadro entero siempre. La principal optimización de LVGL
   —redibujar sólo los rectángulos sucios— no sirve de nada acá.
2. Un buffer RGB565 plano es exactamente lo que espera
   `esp_lcd_panel_draw_bitmap()`. El mismo código corre en el simulador y en
   la Terminal sin capa intermedia.
3. El pixel art necesita escalado por enteros con vecino más cercano. LVGL
   escala pensando en suavizado, que es justo lo que no queremos.

Se trabaja en un lienzo lógico de **160x240** y se presenta a **320x480**:
exactamente 2x, sin interpolación.

## Tuga.exe no es un flipbook

Es un rig. Un cuerpo, ocho juegos de ojos, seis bocas y una capa de efectos
que se combinan según el estado de ánimo, más animación procedural
(respiración senoidal, parpadeo desfasado, tiritar). Eso da muchas más
expresiones que cuadros dibujados, y el próximo simbionte de la colección
reutiliza todo el sistema cambiando sólo el cuerpo.

El arte se autora con `tools/gen_art.py`, que emite los datos C y previsualiza
en PNG. El cuerpo se construye con primitivas para poder iterar cambiando dos
números; las caras van dibujadas a mano, porque ahí el carácter está en cada
pixel.

## Estado

- [x] Núcleo: telemetría, especies, máquina de estados de ánimo con tests
- [x] Compila limpio con `-Wall -Wextra -Wpedantic`; 19/19 tests en verde
- [x] Entorno de desarrollo: WSL2 + Ubuntu 24.04 + gcc 13.3 + SDL 2.30 sobre WSLg
- [x] Renderer RGB565 propio: framebuffer, sprites, tipografía, efectos
- [x] `Tuga.exe`: rig completo con 11 estados de ánimo animados
- [x] Pantalla de la Terminal: escena, diálogo, medidores, selector de plantas
- [x] Simulador SDL con mundo simulado, capturas y hoja de contacto
- [ ] Hub: registro de plantas con foto e identificación por IA
- [ ] Firmware del Spore
- [ ] Protocolo Spore ↔ Terminal
- [ ] Carcasas

## Por qué el núcleo no depende de nada

`firmware/core/` se compila idéntico en la PC, en los tests y en el ESP32.
Eso permite desarrollar y probar toda la lógica del simbionte antes de tener
hardware, y garantiza que lo que se validó en el simulador es exactamente lo
que corre en la Terminal. Todo lo que dependa de LVGL, de Wi-Fi o de un sensor
va afuera de `core/`.

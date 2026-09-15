# Entorno y simulador

Todo el firmware se escribe y se anima en la PC. Las placas sirven para
confirmar las tres cosas que la PC no puede decir —consumo, colores reales y
framerate— no para desarrollar.

## Qué hace falta

| Componente | Versión verificada | Para qué |
|---|---|---|
| WSL2 + Ubuntu | 24.04 LTS | Todo el firmware |
| gcc | 13.3.0 | Compilar |
| GNU Make | 4.3 | Los targets |
| SDL2 | 2.30.0 | Sólo la ventana interactiva |
| WSLg | driver x11, `DISPLAY=:0` | Mostrar esa ventana en Windows |
| Node | 22+ | El Hub |
| Python | 3.10+ | Generar el arte y el catálogo |

Las pruebas y las capturas **no necesitan SDL**: el Makefile lo detecta con
`pkg-config` y compila el simulador sin ventana si no está. Sólo `make sim` lo
pide.

```bash
cd /mnt/c/Users/ifbar/Documents/rootkit/firmware
make test
```

> **La imagen de Ubuntu viene con el índice de apt desactualizado.** Si
> instalás algo y apt tira `404 Not Found` sobre paquetes de glibc, es eso:
> corré `sudo apt update` primero. Le pasa a `build-essential` en una imagen
> recién creada, porque el índice apunta a versiones que ya salieron del pool.

En Windows hay que invocar la distro por nombre si la predeterminada es otra
(por ejemplo con Docker Desktop instalado, que registra la suya):

```bash
wsl.exe -d Ubuntu-24.04 -- bash -lc "cd /mnt/c/.../firmware && make test"
```

## La ventana muestra el kit, no una pantalla

```bash
make sim
```

El Prime a la izquierda y los Minis apilados a la derecha, todos animándose con
el mismo reloj sobre el mismo mundo simulado, a 2× para que se vean en un
monitor.

Eso es deliberado y es la razón de que el simulador se haya reescrito: lo que
hay que juzgar no es si una pantalla queda linda, sino **si la jerarquía entre
el adulto de 22 mm y el brote de 13 mm se lee, y si las dos pantallas cuentan
la misma historia sobre la misma planta**. Con una sola ventana por vez eso es
imposible de evaluar.

| Tecla | Qué hace |
|---|---|
| `1`–`4` | Seleccionar nodo |
| `W` | Regar el nodo seleccionado |
| `M` | Ciclar los ánimos a la fuerza |
| `R` | Volver al ánimo real |
| `G` | Disparar la ceremonia de apertura |
| `O` | Marcar el nodo como caído |
| `+` / `-` | Acelerar o frenar el tiempo |
| `S` | Capturar la ventana entera a `captura.bmp` |
| `ESC` | Salir |

Un día simulado dura 48 segundos. Cada maceta tiene un ambiente que deriva
solo: la tierra se seca a su ritmo y el sol sale y se pone. Sin eso el
simulador muestra estados congelados y no se puede juzgar si las transiciones
se sienten bien.

Los cuatro nodos del mundo simulado arrancan con vínculos de distinta edad
—9, 34, 95 y 200 días sanos— para que la ventana muestre de entrada varias
etapas de crecimiento sin tener que esperar seis meses.

## Capturas sin ventana

```bash
make sheet       # el Prime en los 11 ánimos
make minis       # el Mini en los 11 ánimos
make brotes      # los 12 simbiontes x 5 etapas
make ceremonia   # la apertura, una fila por rareza
make bench       # costo de rasterizado y cota del bus
```

Escriben BMP en `firmware/build/`. `tools/bmp2png.py` los pasa a PNG para
meterlos en el repo o mirarlos en cualquier lado.

Estos modos no abren ventana, así que corren en CI. Y son la única forma de
revisar el arte con los ojos: **la regresión visual por hash detecta que algo
cambió, no si quedó mejor o peor.**

## Por qué no hay LVGL

El plan original era usar LVGL con su port de SDL. Se descartó, y las razones
están en [decisiones.md](decisiones.md). En resumen: el pixel art necesita
escalado por enteros con vecino más cercano y LVGL escala pensando en
suavizado; un buffer RGB565 plano es exactamente lo que espera
`esp_lcd_panel_draw_bitmap()`, sin capa intermedia; y hay dos paneles de
tamaños muy distintos donde LVGL pesaría lo mismo, mientras que acá el Mini
paga sólo las primitivas que usa.

El renderer propio son unas 400 líneas en `gfx/`, y el mismo código corre en el
simulador, en los tests y en las dos placas.

## Lo que NO se puede probar acá

Anotarlo ahora evita sorpresas cuando lleguen las placas:

- **El consumo.** Es lo más importante y es enteramente un modelo. El reposo
  real del C3 y la corriente de cada retroiluminación son los dos números que
  pueden mover la autonomía de 595 días a cualquier otra cosa. Multímetro en
  serie, el día uno.
- **Framerate real.** El simulador corre en un x86. El benchmark ya dice que
  el límite no va a ser rasterizar sino el bus: 29 ms por cuadro del Prime a 40
  MHz de SPI. Hay que confirmar a qué reloj funciona el panel de verdad.
- **Colores.** El IPS no tiene el mismo gamma que tu monitor. El pixel art con
  paletas saturadas es especialmente sensible, y las doce paletas del catálogo
  se eligieron mirando una pantalla de PC.
- **Bytes invertidos.** Si al flashear los colores salen raros, es el orden de
  bytes del RGB565. Se arregla con un flag del driver.
- **El tamaño físico en la mano.** Los 12,9 mm del brote y los 21,9 del adulto
  salen del paso de pixel publicado por el fabricante. Los tests los verifican
  contra ese número, no contra la realidad.
- **Táctil.** El mouse no reproduce la latencia ni la precisión del panel.
- **La radio.** `net/link.c` se prueba pasando estructuras en memoria. ESP-NOW
  está sin escribir.

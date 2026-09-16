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

Y una sola vez, para que `make verify` pueda usar git desde WSL sobre `/mnt/c`:

```bash
git config --global --add safe.directory /mnt/c/Users/<vos>/Documents/rootkit
```

Sin eso git falla con `dubious ownership`. Vale la pena saber por qué el
Makefile lo comprueba explícitamente: en ese estado **git escribe el error en
stderr y aun así devuelve 0**, así que el chequeo de frescura del arte leía su
salida vacía como "no hay cambios" y pasaba en verde sin haber mirado nada. El
target ahora exige que `git rev-parse --show-toplevel` devuelva algo antes de
confiar en un `git status` vacío.

## La ventana muestra los seis modelos

```bash
make sim
```

Los seis modelos en una grilla, todos animándose con el mismo reloj sobre el
mismo mundo simulado, a 2× para que se vean en un monitor.

Eso es deliberado: lo que hay que juzgar es **si los seis se leen como el mismo
producto y como seis personajes distintos al mismo tiempo**. El parecido y la
diferencia sólo existen en comparación, así que con una cara por vez las dos
cosas son imposibles de evaluar.

| Tecla | Qué hace |
|---|---|
| `1`–`6` | Seleccionar modelo |
| `W` | Regar la maceta seleccionada |
| `M` | Ciclar los ánimos a la fuerza |
| `R` | Volver al ánimo real |
| `G` | Disparar el primer encendido del seleccionado |
| `+` / `-` | Acelerar o frenar el tiempo |
| `S` | Capturar la ventana entera a `captura.bmp` |
| `ESC` | Salir |

Un día simulado dura 48 segundos. Cada maceta tiene un ambiente que deriva
solo: la tierra se seca a su ritmo y el sol sale y se pone. Sin eso el
simulador muestra estados congelados y no se puede juzgar si las transiciones
se sienten bien.

Los seis aparatos arrancan con vínculos de distinta edad —9, 34, 62, 95, 140 y
200 días sanos— para que la ventana muestre de entrada varias etapas de
crecimiento sin tener que esperar seis meses.

## Capturas sin ventana

```bash
make sheet       # los 6 modelos x 11 ánimos
make etapas      # las 5 etapas de crecimiento, por modelo
make revelado    # el primer encendido, por modelo
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
la cara es procedural —elipses, arcos y trazos calculados— y LVGL trae un motor
de widgets que acá no se usaría nunca; y un buffer RGB565 plano es exactamente
lo que espera `esp_lcd_panel_draw_bitmap()`, sin capa intermedia.

El renderer propio son unas 400 líneas en `gfx/`, y el mismo código corre en el
simulador, en los tests y en las dos placas.

## Lo que NO se puede probar acá

Anotarlo ahora evita sorpresas cuando lleguen las placas:

- **El consumo.** Es lo más importante y es enteramente un modelo. El reposo
  real del C3 y la corriente de cada retroiluminación son los dos números que
  pueden mover la autonomía de 595 días a cualquier otra cosa. Multímetro en
  serie, el día uno.
- **Framerate real.** El simulador corre en un x86. El benchmark ya dice que
  el límite no va a ser rasterizar sino el bus: 6,25 ms por cuadro a 40 MHz de
  SPI, o sea 160 fps de techo. Hay que confirmar a qué reloj anda el panel.
- **Colores.** El IPS no tiene el mismo gamma que tu monitor, y las seis
  paletas se eligieron mirando una pantalla de PC. Peor: el Hongo se diseñó
  oscuro porque su carcasa le tira sombra, y eso sólo se puede juzgar con la
  pieza impresa encima.
- **Bytes invertidos.** Si al flashear los colores salen raros, es el orden de
  bytes del RGB565. Se arregla con un flag del driver.
- **Cómo se ve una cara dentro de su carcasa.** Es lo más importante que falta
  probar, y no hay forma de simularlo: hay que imprimir.
- **La radio.** `net/link.c` se prueba pasando estructuras en memoria. ESP-NOW
  está sin escribir.

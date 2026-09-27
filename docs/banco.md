# El firmware de banco del prototipo NodeMCU

El prototipo que soldó Rocío —NodeMCU ESP32 de 38 pines, TFT de 2,2",
capacitivo, LDR y HTU21D— no corre el firmware del producto: no tiene toque,
ni batería, ni BH1750. Corre `firmware/banco/banco.cpp`, que sirve para dos
cosas a la vez:

- **probar que los sensores andan**, con las lecturas en pantalla y por el
  puerto serie, crudas y convertidas;
- **ver a Kip** con sus animaciones nuevas en el panel de verdad, reaccionando
  a esos sensores con el mismo motor de ánimo que el producto.

No hay QR, ni vínculo, ni nube: arranca directo en la cara.

## El cableado

Es el de `firmware/esp32/placa.h` (`RK_PLACA_NODEMCU38`), y
`firmware/test/test_placa_nodemcu.c` verifica que coincida pin por pin con lo
soldado.

| Qué | Pin del módulo | GPIO |
|---|---|---|
| TFT ILI9341 | CS · RESET · DC · MOSI · SCK · MISO | 5 · 4 · 2 · 23 · 18 · 19 |
| TFT | LED | a 3,3 V (siempre prendido) |
| Capacitivo v2.0 | AOUT | 34 (ADC1_CH6) |
| LDR (MH-Sensor) | AO · DO | 35 (ADC1_CH7) · 32 |
| HTU21D | SDA · SCL | 21 · 22 |
| Botón | BOOT de la placa | 0 |

Los dos analógicos están en el ADC1, que es el que sigue andando con el wifi
prendido. GPIO2 (DC del panel) es además el LED azul de la placa: titila
mientras se dibuja, y es normal.

## Flashearlo

Con la placa enchufada por USB a la compu y [PlatformIO](https://platformio.org/)
instalado (`pip install platformio`):

```bash
cd firmware
pio run -e banco-nodemcu -t upload
pio device monitor            # 115200
```

La primera vez PlatformIO baja el compilador del ESP32 y LovyanGFX: tarda
unos minutos. Si la subida no arranca ("Failed to connect"), mantener
apretado **BOOT** hasta que empiece a escribir. Si la compu no ve el puerto,
falta el driver del chip USB de la placa (CP2102 o CH340).

## Qué se ve

La cara de Kip en el medio y, en las franjas que le sobran al panel arriba y
abajo:

```
SUELO 51% (1903)          el porcentaje y la lectura cruda del ADC
LUZ 803 LX DO:0           lux aproximados de la LDR y su salida digital
          [ la cara de Kip ]
AIRE 23.4°C 55%           el HTU21D
AUTO CONTENTO             el modo y el ánimo
```

y cada segundo, por el monitor serie:

```
suelo 1903 ( 51%)  luz AO  250 mV ~  803 lx DO 0  aire 23.4 C 55%  | Banco de pruebas: HAPPY (estoy perfecta)
```

Al arrancar también escanea el I2C (tiene que aparecer `0x40 (HTU21D)`),
dice cuánta memoria hay y de qué tamaño quedó la cara: en el ESP32 clásico
el bloque de RAM contiguo más grande no alcanza para 240×240, y la cara sale
de 232×232 centrada. Cada diez segundos informa los cuadros por segundo.

## Cómo probar cada sensor

En modo **AUTO** la cara sigue a los sensores. Los umbrales son los de una
"especie de banco" (suelo 20–60 %, 15–30 °C, humedad del aire desde 30 %,
luz de 100 a 20.000 lux), elegidos para que en una mesa de trabajo, con el
capacitivo en tierra húmeda, Kip esté contento, y que cada sensor lo saque de
ahí por su lado:

| Hacé esto | Kip pone |
|---|---|
| El capacitivo al aire | sed |
| El capacitivo en un vaso de agua | se ahoga |
| Tapar la LDR | oscuro, y a los 8 segundos se duerme |
| Una linterna pegada a la LDR | sol directo |
| Un dedo sobre el HTU21D un rato | calor |
| Desenchufar el HTU21D | la franja dice `NO MIDE`, y el ánimo ignora el aire |

El motor juzga en orden: agua, temperatura, noche, luz, aire. Por eso con el
capacitivo al aire Kip tiene sed aunque tapes la LDR: la sed manda.

La noche llega en 8 segundos y no en dos horas porque el banco mide una vez
por segundo y el producto cada quince minutos: es la misma regla, "ocho
lecturas seguidas a oscuras".

## El botón y el serie

- **BOOT corto**: pasa de modo. AUTO y después los once ánimos uno por uno,
  empezando por los de las láminas de Rocío (contento, agua, calor, oscuro,
  aire seco) y siguiendo por sed, frío, sol, dormido, sin datos y
  desconectado.
- **BOOT largo**: cambia la piel (común, rara con destellos, épica con fuego).

Por el monitor serie, una letra:

| Letra | Qué hace |
|---|---|
| `a` | AUTO |
| `n` | siguiente ánimo |
| `1`–`9`, `0`, `x` | un ánimo: 1 contento, 2 agua, 3 calor, 4 oscuro, 5 aire seco, 6 sed, 7 frío, 8 sol, 9 dormido, 0 sin datos, x desconectado |
| `p` | siguiente piel |
| `e` | siguiente especie: la del banco y después las de `core/species.c` |
| `s` / `m` | calibrar el suelo SECO (sensor al aire) / MOJADO (en agua, hasta la raya) |
| `r` | volver a la calibración de fábrica del suelo |
| `i` | escanear el I2C |
| `?` | la ayuda |

La calibración del suelo queda guardada en la flash (NVS): sobrevive a un
reinicio.

## La luz es aproximada

La LDR no mide lux: mide una resistencia. `rk_ldr_lux` (`nodo/sensores.c`)
la convierte con la curva de una GL5528, y una LDR varía el doble entre
unidades. Alcanza para lo que el ánimo necesita —de noche, poca luz, sol—
pero no para comparar números con un luxómetro. Además el ADC del ESP32 no
lee por debajo de ~140 mV: con mucha luz la AO cae más abajo, la franja dice
`LUZ MUCHA` y se toma como sol.

## Si algo no se ve bien

| Síntoma | Qué tocar en `platformio.ini`, entorno `banco-nodemcu` |
|---|---|
| Kip sale azul en vez de rojo | `-DRK_TFT_BGR=1` |
| La cara sale dada vuelta | `-DRK_TFT_ROTACION=2` |
| La imagen sale con basura o rayas | `-DRK_TFT_SPI_HZ=20000000` |
| Pantalla blanca | mirar RESET (GPIO4), DC (GPIO2) y CS (GPIO5) |
| `I2C: nada` | mirar SDA/SCL y los 3,3 V del HTU21D |
| `SUELO ??` | el capacitivo desconectado o en corto: mirar AOUT y sus 3,3 V |

## Cómo se probó sin la placa

Todo lo que es C puro (la conversión de la LDR, el HTU21D con su CRC, el
suelo, el ánimo, la cara) se prueba con `make test`. El `.cpp` del banco se
compiló y se corrió entero en la PC contra imitaciones de Arduino, Wire y
LovyanGFX, con sensores simulados, y de ahí salen las lecturas y las
pantallas de arriba. Lo que eso NO prueba es el hardware: los tiempos reales
del SPI, el ADC de verdad y el cableado. Eso lo dice el primer encendido.

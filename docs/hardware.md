# Hardware

Todo lo que va adentro de un ROOTKIT: la placa, la pantalla, los sensores, la
batería y cómo se conecta. Los pines de este documento son los
de [`firmware/esp32/placa.h`](../firmware/esp32/placa.h); si cambian allá,
cambian acá.

**Dónde va cada cosa físicamente** está en [pcb.md](pcb.md): el sustrato
impreso en 3D con canaletas y cinta de cobre sobre el que se monta todo. El
**diagrama de conexiones completo**, red por red, se genera desde el mismo
dato que el sustrato y vive en [conexiones.md](conexiones.md). Y el paso a
paso para armar una unidad, con los controles de multímetro, en
[armado.md](armado.md).

## Resumen

| | ROOTKIT (el producto) | Banco de pruebas |
|---|---|---|
| Placa | ESP32-C3 SuperMini | ESP32 DevKit 30 pines |
| Pantalla | TFT 1,44" 128×128 IPS (ST7735S), área activa 25,9 × 25,9 mm. **El sustrato v3.0 trae la tira de 9 pines de la de 2,2"**: ver "Inventario" | la misma |
| Sensores | suelo capacitivo, AHT20 (o SHT21), BH1750, toque; DS18B20 opcional | los mismos |
| Batería | **18650** de 2600–3500 mAh, parada, abajo (baja el centro de gravedad) | USB |
| Carga | USB-C con TP4056 + protección + carga compartida | — |
| Firmware | `pio run -e c3-144` | `pio run -e devkit-144` |

**Un solo panel.** El primer prototipo usaba un TFT de 2,2" (ILI9341,
240×320). Se retiró: las cinco carcasas son para el de 1,44", que entra desde
atrás en una ventana biselada a 45° ([carcasas.md](carcasas.md)), y mantener
dos paneles duplicaba compilaciones, láminas y pruebas sin un producto
detrás. El motor gráfico sigue escalando al lado corto del panel
(`gfx/panel.h`), así que sumar otro tamaño algún día es agregar su bloque en
`esp32/placa.h` y su clase en `esp32/pantalla.cpp`.

---

## Inventario: qué hay sobre la mesa, qué falta y qué sobra

Estado al **21 de septiembre de 2026**, mirando las piezas que hay. Esta
tabla es lo que decide si se puede armar una unidad completa hoy: **no se
puede**, faltan cuatro cosas, y dos de ellas son baratas.

### Lo que hay y sirve

| Pieza | Qué es | Dónde entra | Cuidado |
|---|---|---|---|
| **ESP32-C3 SuperMini** | el cerebro | `U1`, 16 pines | Confirmar que la tira sea 2 × 8 a 2,54 mm y las filas a 15,24 |
| **SHT21 / HTU21 / Si7021** (módulo azul, 4 pines) | aire: temperatura y humedad | `J_AIRE` | **El firmware habla AHT20 (0x38), no SHT21 (0x40)**: falta el driver. Ver abajo |
| **Capacitivo de suelo v2.0.0** (placa negra larga) | humedad de la tierra | `J_SUELO`, 3 pines | Mirar que el chip diga **TLC555**, no NE555. Sellar el canto |
| **HW-390** (placa negra chica, conector JST) | el mismo capacitivo, versión con JST | repuesto, o segunda unidad | Igual que el anterior |
| **TFT 2,2" ILI9341 240 × 320 SPI** (placa roja, 9 pines) | la pantalla | `J_TFT`, 9 pines | **El firmware retiró este panel en la 0.6.0**: hay que volver a sumarlo. Ver abajo |
| **Tiras de pines macho** | el montaje entero | todas las tiras | Es exactamente lo que la v3.0 necesita |
| **Cable JST de 3 pines** | para el HW-390 | `J_SUELO` | |

### Lo que falta para armar una unidad completa

| Pieza | Para qué | Sin ella |
|---|---|---|
| **BH1750 (GY-302)** | luz, en lux reales | El Rooti no puede decir "tengo poca luz", que es una de las cuatro cosas que mide |
| **TTP223** | el toque | No hay forma de despertar el aparato ni de prender la pantalla a mano |
| **Módulo TP4056 USB-C con protección (6 pines)** | cargar la celda | Sólo funciona enchufado por el USB de la SuperMini |
| **18650 de marca + portapilas** | la energía | ídem |
| **Interruptor deslizante** | despachar el aparato apagado | Se puede vivir sin él en el prototipo |
| **DS18B20 sumergible** | temperatura de la tierra | Es **opcional**: el firmware anda sin sonda |
| **SMD 1206 y SOT-23** (ver la lista de compras) | los MOSFET, las resistencias y los capacitores del sustrato | Sin el AO3401 del riel conmutado, el capacitivo come 5 mA las 24 horas y la batería dura días, no meses |
| **Cinta de cobre de 5 mm** y **filamento PETG** | el sustrato | |

Las dos urgentes son el **BH1750** y el **TTP223**: son las dos que faltan
para que el producto haga lo que promete, y entre las dos cuestan menos que
la pantalla. El TP4056 y la celda se pueden postergar mientras se prueba
enchufado.

### Lo que sobra, y para qué podría servir

| Pieza | Veredicto |
|---|---|
| **HC-SR04** (ultrasónico, 4 pines) | **No entra en el ROOTKIT.** Quiere 5 V, come ~15 mA mientras mide, necesita dos GPIO —y los trece del C3 ya están todos usados— y mide distancia, que no es ninguna de las cuatro magnitudes de una planta. La única idea razonable sería medir el nivel del agua en el plato, pero para eso un capacitivo de nivel es más chico, más barato y funciona mojado. **Guardarlo para otro proyecto.** |
| **Módulo de micrófono** (placa roja con potenciómetro) | **No ahora, pero no tirarlo.** El roadmap tiene sonido en la Fase 2b, y es sonido de *salida* (el Rooti hace ruiditos), no de entrada. Un micrófono abre otra cosa: despertar al aparato con una palmada, sin tocarlo. Es una función de producto que hoy no existe, y ocuparía el GPIO que libera atar el CS de la pantalla a masa. **Anotado como idea, no como pieza del diseño.** |
| **Módulo de fotorresistencia LDR** (placa con potenciómetro, salidas AO y DO) | **No como sensor de luz del producto**, y eso está decidido desde hace rato: un LDR no da lux sin calibrar cada unidad, y la biblioteca de especies de ROOTLAB está en lux. Pero **sí sirve de sustituto mientras no llega el BH1750**: su salida analógica se puede clavar donde va el capacitivo para tener *algo* de luz en el banco y probar la lógica de estados de ánimo. Es un parche de banco, no del producto. |

### Las dos deudas de firmware que deja este inventario

Ninguna es del sustrato —la placa v3.0 está bien para las dos— pero sin
ellas los módulos que hay sobre la mesa no se pueden usar:

1. **El sensor de aire.** El firmware inicializa y lee un **AHT20 en 0x38**
   (`esp32/sensores_hw.cpp`, y la conversión en `nodo/sensores.c`). El módulo
   que hay es un **SHT21/HTU21 en 0x40**, con otro protocolo: comandos 0xF3
   (temperatura) y 0xF5 (humedad) en modo *no hold*, tres bytes por lectura
   con su CRC-8, y las fórmulas `T = -46,85 + 175,72 · S/65536` y
   `RH = -6 + 125 · S/65536`. Es un driver nuevo, pequeño y **testeable en el
   escritorio** (la conversión es C puro, como la del AHT20). Recomendación:
   agregarlo al lado del AHT20 y elegir cuál se compila desde
   `platformio.ini`, no reemplazarlo.
2. **El panel.** El ILI9341 de 2,2" existió hasta la 0.6.0 y se retiró a
   propósito. Volver a sumarlo es su bloque en `esp32/placa.h`
   (`RK_PANEL_ILI9341_240`, 240 × 320) y cambiar la clase del panel en
   `esp32/pantalla.cpp`, que ya es LovyanGFX. El motor gráfico escala solo al
   lado corto, así que la cara se dibuja igual. Recomendación: sumarlo como
   **variante de banco** (`c3-22`, `devkit-22`) y dejar la de 1,44" como
   producto hasta que haya carcasa.

---

## ¿ESP32-C3 SuperMini o ESP32 de 30 pines?

**Conviene el C3 SuperMini para el producto.** El de 30 pines queda como
placa de banco. El firmware compila para las dos (`c3-144`, `devkit-144`),
así que la decisión no ata nada.

La diferencia de precio es chica y el espacio sobra, así que la decisión no
sale de ahí. Sale de la batería:

| | ESP32-C3 SuperMini | ESP32 DevKit 30 pines |
|---|---|---|
| Regulador | ME6211, ~40 µA en reposo | AMS1117, **~5 mA en reposo** |
| USB | nativo del chip, sin conversor | CP2102/CH340, alimentado desde 3V3 |
| LED de encendido | uno (se puede desoldar) | uno, fijo |
| **Placa en deep sleep** | **~50 µA** sin el LED | **~6–10 mA** |
| Días que dura una 18650 sólo durmiendo | años | **~2 semanas** |
| CPU | RISC-V 160 MHz, 1 núcleo, sin FPU | Xtensa 240 MHz, 2 núcleos, con FPU |
| Pines útiles | 13 | ~25 |
| Tamaño | 22,5 × 18 mm | 51 × 28 mm |

El DevKit gasta más durmiendo que el C3 despierto midiendo. Con un regulador
que consume 5 mA todo el tiempo, ninguna optimización de firmware salva la
batería: habría que desoldarle el regulador y alimentarlo por 3V3 desde uno
externo, y a esa altura es otra placa.

Donde el DevKit gana es en el banco: sobran pines, tiene FPU y dos núcleos, y
para probar sensores nuevos es más cómodo. Para desarrollar enchufado a USB, perfecto.

**Lo que el C3 pide a cambio:**

- Sólo GPIO0–GPIO5 despiertan del deep sleep: el sensor de toque va ahí.
- GPIO2, GPIO8 y GPIO9 son de arranque. El mapa de pines los usa para cosas
  que están en alto al encender (ver la tabla de conexiones).
- No tiene FPU: por eso todo el motor gráfico es de punto fijo. En 128×128
  la cara se dibuja holgada a 30 cuadros por segundo (`make bench`); se
  confirma en la placa en la Fase 1 del [roadmap](roadmap.md).
- Algunas SuperMini traen un LED rojo de encendido siempre prendido (1–3 mA).
  En la placa del producto se desuelda o se le corta la pista.

---

## La pantalla

La pantalla del ROOTKIT muestra **dos cosas en toda su vida: el QR y los
ojos**. Nada de números ni íconos. La cara es vectorial y se escala al lado
corto del panel.

### TFT 1,44" 128×128 IPS (ST7735S)

- Controlador ST7735S, memoria de 132×162.
- Casi todos los lotes necesitan un corrimiento: por defecto
  `RK_TFT_OFS_X=2`, `RK_TFT_OFS_Y=1`. Si la imagen sale corrida, con rojo y
  azul cambiados o con los colores invertidos, se corrige en
  `platformio.ini` con `RK_TFT_OFS_X/Y`, `RK_TFT_BGR` y `RK_TFT_INVERT`, sin
  tocar código.
- Luz de fondo: 15–30 mA. **No va directo a un GPIO.** Va a una etapa de dos
  transistores que conmuta del **lado alto**: un N-MOSFET (AO3400) con GPIO21
  en la compuerta tira de la compuerta de un P-MOSFET (AO3401) que lleva 3V3
  al pin `BL`. Es la forma que funciona tanto si `BL` es el ánodo del LED
  —que es lo más común en estos módulos— como si es la entrada de control de
  un transistor del propio módulo. Si resultara ser el cátodo, un selector de
  estaño (`JP_BL`) pasa la etapa al lado de masa sin rehacer nada. El
  detalle y el porqué de cada resistencia, en [pcb.md](pcb.md).
- El PWM no invierte: GPIO21 alto enciende. Si algún lote pide lo contrario,
  se compila con `-DRK_TFT_BL_INVERTIDO=1`.
- El framebuffer ocupa 32 KB: en un C3 con el wifi prendido sobra lugar.

---

## Sensores

Todo lo que puede medir un ROOTKIT, en orden de importancia. Los de las dos
primeras tablas los lee el firmware hoy.

### Imprescindibles

| Magnitud | Sensor | Interfaz | Por qué este |
|---|---|---|---|
| Humedad de la tierra | **Capacitivo v1.2 / v2.0** | analógico (ADC) | Sin metal expuesto: no se corroe. Es lo que más mata plantas. |
| Temperatura y humedad del aire | **AHT20** (o AHT21) | I2C `0x38` | ±0,3 °C, ±2 % HR, 0,25 µA dormido. Alternativa más precisa: SHT40 (`0x44`). |
| Luz | **BH1750** (GY-302) | I2C `0x23` | Da lux reales, calibrados. Llega a 54.612 lux; alcanza para interior. |

### Recomendados

| Magnitud | Sensor | Interfaz | Para qué |
|---|---|---|---|
| Temperatura de la tierra | **DS18B20** sumergible | 1-Wire, pull-up 4,7 kΩ **al riel fijo** | Las raíces sienten la tierra, no el aire. Clave en invierno junto a una ventana. |
| Toque | **TTP223** | digital | Prende la pantalla y despierta al aparato. Funciona a través de 2–3 mm de plástico. |
| Batería y USB | divisor 470 kΩ / 470 kΩ + 100 nF | ADC | Porcentaje de carga y detección de enchufado con un solo pin. |

### Para más adelante

| Magnitud | Sensor | Por qué no ahora |
|---|---|---|
| Luz de sol pleno | VEML7700 | Mide hasta 120.000 lux y ve mejor la penumbra. Para macetas de balcón. |
| Fertilidad (conductividad) | sonda EC RS485 "7 en 1" | Las baratas analógicas se corroen; las buenas piden 12 V y cuestan más que el ROOTKIT. |
| Golpe, caída, "tocar la maceta" | LIS3DH | Reemplazaría al TTP223 con despertar por movimiento (~6 µA). |
| UV | LTR390 | Sólo interesa en exterior. |
| Nivel del plato o reservorio | capacitivo de nivel | Accesorio para autorriego. |
| Presión | BMP280 | Viene en la plaquita combinada AHT20+BMP280, pero a una planta no le cambia nada. |

### Los que NO

- **Humedad de suelo resistivo** (las dos patitas metálicas): se corroe en
  semanas y además electroliza la tierra.
- **DHT11 / DHT22**: imprecisos, lentos y con un protocolo de tiempos
  frágiles. El AHT20 cuesta lo mismo.
- **LDR (fotorresistencia)**: no se puede convertir a lux sin calibrar cada
  unidad, y la especie pide lux.

### Tres cuidados del capacitivo de suelo

1. **Mirar el chip.** Tiene que decir **TLC555**. Muchos lotes vienen con un
   NE555, que no arranca a 3,3 V y da lecturas planas.
2. **Sellar el borde.** El canto de la placa absorbe agua y en un mes la
   lectura deriva. Esmalte de uñas o epoxi en los bordes y sobre el circuito.
3. **Calibrar cada unidad.** Seco en el aire y sumergido hasta la línea: dos
   lecturas. La app manda la calibración (`calibracion.seco/mojado`); sin
   ella se usa la de fábrica de `nodo/soil.c`.

El capacitivo consume ~5 mA encendido. Por eso se alimenta a través de un
P-MOSFET (AO3401) que el firmware prende 150 ms por lectura: apagado, no
gasta nada.

### Qué cuelga de qué riel, y por qué

**Del riel conmutado cuelga sólo el capacitivo.** Es el único que come de
verdad. El AHT20, el BH1750, el TTP223 **y el DS18B20** van al 3V3 fijo.

El DS18B20 estaba pensado para el riel conmutado y se mudó: si la sonda
cuelga del riel conmutado pero su pull-up sale del 3V3 fijo, con el riel
apagado la línea de datos le mete corriente a la sonda por su pata de datos y
carga el riel muerto. Y el pull-up no puede irse al riel conmutado porque
**GPIO8 es pin de arranque** y tiene que estar alto al encender, justo cuando
el riel está apagado. La sonda consume **1 µA como máximo en reposo**: 24 µAh
por día, el 0,2 % del presupuesto. Por ese precio desaparecen el camino
parásito y la duda del arranque. Hay una prueba (`test_placa.c`) que falla si
alguien la devuelve al riel conmutado.

El riel conmutado lleva además una **resistencia de purga de 100 kΩ** para
que baje rápido al apagarlo, y un **100 nF** junto al borne del sensor.

---

## Batería y carga

### La celda

- Una **18650** de 2600–3500 mAh, de marca (Samsung, LG, Molicel, Sony). Las
  "9900 mAh" de sitios de ofertas tienen 800 mAh y a veces arena. Va
  **parada, abajo**: es la pieza más pesada y hace de lastre
  ([carcasas.md](carcasas.md)).
- Si algún día hay una carcasa más chica: **LiPo plana** de 1000–1200 mAh
  (103450 o 603450) con su plaquita de protección. El firmware no cambia.

### El circuito

> **Desde el sustrato v3.0 esta etapa entera vive FUERA de la placa.** El
> sustrato es una protoboard de interconexión y nada más: la energía le llega
> por `J_PWR`, dos pines, ya conmutada por el interruptor. Todo lo que está
> dibujado acá abajo —el TP4056, el SS34, el AO3401 de carga compartida, su
> resistencia de compuerta y el interruptor— es un **arnés aparte**, que se
> arma sobre el propio módulo TP4056 y se mete donde la carcasa tenga lugar.
> No cambió ni un componente ni una conexión; cambió dónde vive. La razón es
> que la carga es lo único del aparato que toca 5 V y un ampere, y sacarlo
> del sustrato es lo que permitió bajar los rieles de 4 a 2,2 mm y usar un
> solo rollo de cinta de 5 mm ([pcb.md](pcb.md), "La cinta manda").
>
> El divisor del riel (470 k / 470 k / 100 nF) **sí** sigue en el sustrato:
> mide `J_PWR.VIN`, que es lo que entra después del interruptor.

```
                 USB-C 5 V
                     │
             ┌───────┴────────┐
             │  TP4056 + DW01A │  módulo "con protección", 6 pines
             │   IN+     B+ ───┼──── + celda
             │   IN-     B- ───┼──── - celda
             │          OUT+ ──┼──┐
             └────────────────┘  │   (batería protegida)
                     │            │
      5 V ──SS34──┬──┘            │
                  │   AO3401 (P)  │
                  ├──── S   D ────┘   (fuente del lado de la carga,
                  │     G              drenador del lado de la celda)
                  │     └── al 5 V del USB, con 100 kΩ a GND
                  │
                  └── interruptor ──┬── 5V de la SuperMini (regula a 3,3 V)
                                    │
                                    └── 470 kΩ ──┬── GPIO1 (ADC)
                                                 ├── 470 kΩ ── GND
                                                 └── 100 nF ── GND
```

El divisor cuelga de **después** del interruptor y no del nodo de sistema:
lee exactamente lo mismo (el interruptor no tiene caída) y con el aparato
apagado en la caja no gasta ni un microamperio en vez de 3,9 µA.

**Carga compartida (AO3401 + SS34).** Sin esto, con el USB enchufado el
aparato consume a través del cargador, el TP4056 nunca ve la corriente
bajar y la carga no termina: la celda se sobrecarga lento. Con el MOSFET,
enchufado el aparato come del USB por el Schottky y el TP4056 sólo carga la
celda.

**Corriente de carga.** La fija la resistencia `RPROG` del módulo:
1,2 kΩ = 1 A (bien para la 18650), 2 kΩ = 580 mA (mejor para la LiPo de
1000 mAh), 3 kΩ = 400 mA (LiPo de 500 mAh).

**Regulación.** La SuperMini se alimenta por su pin `5V` y su ME6211 baja a
3,3 V. Anda con la celda por encima de ~3,5 V; debajo, en los picos de wifi
puede caer. El firmware corta a 3,15 V (`RK_BATT_CUTOFF_MV`) y la protección
DW01A a 2,4 V. Para aprovechar la celda entera en la versión de producción:
un buck-boost **TPS63802** (3,3 V estables de 2,7 a 4,2 V, 11 µA en reposo)
o un LDO **RT9080** (2 µA, 600 mA). Agregar 220–470 µF de bajo ESR cerca
del 3V3 para los picos de transmisión.

**USB y batería con un solo pin.** Enchufado, el riel sigue al USB menos el
Schottky: ~4,6 V. A batería, a la celda: 4,2 V o menos. El firmware lee el
divisor y, por encima de 4,35 V, sabe que está enchufado
(`nodo/sensores.c`). En ese caso reporta la batería como "desconocida" en
lugar de un 100 % falso.

**Interruptor.** Entre la carga compartida y la placa, para despachar el
aparato apagado en la caja. Y es además la llave de seguridad del párrafo que
sigue.

> **Los dos USB y la celda.** La SuperMini tiene su propio USB-C —es por
> donde se flashea y por donde trabaja la estación de fábrica— y el TP4056
> tiene otro. En muchas SuperMini el pin `5V` está unido a su VBUS sin
> diodo: con la celda puesta y el interruptor prendido, enchufar el USB de la
> SuperMini mete 5 V en el nodo de sistema, el AO3401 conduce (no hay USB en
> el cargador, así que su compuerta está en bajo) y **la celda recibe 5 V sin
> control de carga**.
>
> El aparato se arma de forma que eso no pueda pasar: se flashea **antes de
> poner la celda**, el USB de la SuperMini **no sale al exterior**, el
> interruptor corta ese camino, y el sustrato lleva grabado
> `APAGAR ANTES DE FLASHEAR` al lado del módulo. La regla, para siempre: **el
> USB de la SuperMini se enchufa con el interruptor apagado.** El análisis
> completo y por qué un diodo en serie no es la solución, en
> [pcb.md](pcb.md). Para la Fase 5: un solo USB-C y un multiplexor de
> alimentación de verdad.

### Cuánto dura

Estimaciones a verificar con un medidor en la Fase 1. Los costos de
medición y transmisión salen del modelo de `nodo/power.c`.

| Consumo por día | ROOTKIT (1,44" + 18650) |
|---|---:|
| Deep sleep (~0,15 mA con panel dormido) | 3,6 mAh |
| Mediciones adaptativas (cada 2–30 min) | 1,5 mAh |
| ~30 envíos a la nube (wifi + TLS) | 2,3 mAh |
| Pantalla al tocar: 20 veces × 20 s | 5,6 mAh |
| **Total** | **~13 mAh** |
| **Con una 18650 de 3000 mAh (20 % de margen)** | **~6 meses** |

Una actualización por aire (bajar ~1,2 MB por wifi y escribir la flash)
cuesta unos 3 mAh: a batería sólo se hace con la celda arriba de 3,7 V
([ota.md](ota.md)).

**Pantalla siempre encendida** a batería: unos cuatro días. Enchufado, sin
límite. La app lo explica al activar la opción.

---

## Sonido

**En estudio** (roadmap, Fase 2b): un parlantito para que el Rooti se exprese
con sonido además de con la cara.

### El problema de los pines

El C3 SuperMini expone GPIO 0–10, 20 y 21, y **ya están todos usados** (ver
[Conexiones](#conexiones)). Un amplificador I2S (MAX98357A) necesita tres
pines; no entran. Hace falta **uno solo**, y se libera así:

- **CS de la pantalla a GND.** La pantalla es el único dispositivo del bus
  SPI, así que puede quedar siempre seleccionada. Libera **GPIO 20** para el
  sonido, sin perder nada. El firmware deja de manejar CS (`RK_TFT_CS -1`).

En el DevKit de 30 pines sobran pines: ahí se puede probar I2S.

### Tres opciones, de menos a más

| | Qué es | Pines | Sonido | Consumo en reposo | Costo aprox. |
|---|---|---|---|---|---|
| **Buzzer pasivo** | piezo de 12 mm por PWM, con un N-MOSFET | 1 (PWM) | pitidos y melodías simples | 0 | muy bajo |
| **PAM8302 + parlante** ✅ recomendado | amplificador clase D mono de 2,5 W con entrada analógica; el PWM (LEDC o sigma-delta) pasa por un filtro RC | 1 (PWM) | melodías con timbre y volumen reales, efectos cortos | < 1 µA con SD en bajo | bajo |
| MAX98357A + parlante | amplificador I2S con DAC | 3 (BCLK, LRC, DIN) | muestras de audio de buena calidad | ~2 µA con SD en bajo | medio |

**Recomendado para el producto: PAM8302 + parlante de 8 Ω, 0,5–1 W, de 20 a
28 mm.** Da carácter sin gastar pines ni batería, y el buzzer queda para
validar la idea en el banco con lo que haya a mano.

### Cómo se conectaría (PAM8302)

```
GPIO 20 ──[1 kΩ]──┬── A+ del PAM8302          VBAT (después del interruptor)
                  │                              │
               [10 nF]                       VIN del PAM8302
                  │                              │
                 GND ── A- del PAM8302       SD ─┴── riel de sensores (P-MOSFET)
                                           OUT+ / OUT- ── parlante 8 Ω
```

- **Alimentación desde la batería**, no desde el regulador de 3,3 V: los
  picos del parlante (hasta 300 mA) no tienen que bajar la tensión de la
  placa. El capacitor de 220–470 µF del wifi ayuda también acá.
- **SD al riel de sensores**: el amplificador sólo está prendido cuando el
  firmware prende los sensores, y en deep sleep consume nada. Para sonar
  fuera de una medición, el firmware prende el riel un momento.
- **Parlante hacia un costado o hacia abajo**, con rejilla: el agua de riego
  cae de arriba (ver [carcasas.md](carcasas.md)).

### Qué suena

Diseñado como datos, igual que las caras: cada Rooti con su "voz" (escala,
timbre, tempo) y cada evento con su motivo. Despertar del cofre, sed urgente
(una vez por episodio), riego detectado, toque, batería baja. Con silencio de
23 a 8 y volumen o silencio por Rooti desde ROOTLAB. Los sonidos se van a
poder escuchar en el emulador antes de tener hardware.

---

## Conexiones

Esta tabla es el mapa de pines. La **netlist completa** —cada red, de qué
riel cuelga, qué ancho de cinta lleva y de dónde a dónde va— se genera desde
`hardware/pcb/nucleo.json` y está en [conexiones.md](conexiones.md).
`test_placa.c` cruza las dos cosas en cada `make test`.

### ESP32-C3 SuperMini

| GPIO | Va a | Notas |
|---:|---|---|
| 0 | salida del capacitivo | ADC1 |
| 1 | divisor del riel | ADC1 |
| 2 | compuerta del P-MOSFET de sensores | LOW enciende. De arranque: alto al encender = apagado |
| 3 | salida del TTP223 | alto al tocar; despierta del deep sleep |
| 4 | SDA (AHT20, BH1750) | pull-up 4,7 kΩ a 3V3 si el módulo no la trae |
| 5 | SCL | ídem |
| 6 | SCK de la pantalla | |
| 7 | MOSI (SDA/SDI) de la pantalla | |
| 8 | DATA del DS18B20 | pull-up 4,7 kΩ **al 3V3 fijo**. De arranque: queda alto |
| 9 | botón BOOT | apretado 10 s: borra vínculo y wifi |
| 10 | DC (A0/RS) de la pantalla | |
| 20 | CS de la pantalla | |
| 21 | compuerta del N-MOSFET de la etapa de la luz de fondo | PWM 22 kHz, alto enciende. Es el TX del UART0: la luz pestañea ~50 ms al arrancar ([pcb.md](pcb.md)) |
| — | RST de la pantalla | a 3V3 con 10 kΩ; el firmware la reinicia por software |

### ESP32 DevKit 30 pines

| GPIO | Va a |
|---:|---|
| 34 | capacitivo (ADC1) |
| 35 | divisor del riel (ADC1) |
| 26 | P-MOSFET de sensores |
| 33 | TTP223 (despierta por ext0) |
| 21 / 22 | SDA / SCL |
| 18 / 23 | SCK / MOSI |
| 25 | DS18B20 |
| 0 | botón BOOT |
| 16 | DC |
| 5 | CS |
| 4 | luz de fondo (por MOSFET) |
| 17 | RST |

En el DevKit el suelo y el riel van al ADC1 (GPIO32–39): el ADC2 no anda con
el wifi prendido.

---

## Lista de compras del prototipo

Por unidad. Los precios de pantalla son los de la cotización actual.

| Cant. | Pieza | Nota |
|---:|---|---|
| 1 | ESP32-C3 SuperMini | o un ESP32 DevKit para el banco |
| 1 | TFT 2,2" ILI9341 240×320 SPI, 9 pines | la que entra en `J_TFT`. La de 1,44" tiene 8 pines y otro orden: **no entra** en esta tira |
| 1 | Sensor capacitivo de suelo v2.0 | con TLC555 |
| 1 | SHT21 / HTU21 / Si7021 | o un AHT20; la tira de 4 pines les sirve a los dos, pero el firmware tiene que elegir |
| 1 | BH1750 (GY-302) | |
| 1 | DS18B20 sumergible con cable | opcional |
| 1 | TTP223 | |
| 1 | Módulo TP4056 USB-C con protección (6 pines) | |
| 1 | 18650 de marca + portapila | |
| 2 | AO3401 (P-MOSFET SOT-23) | riel de sensores y lado alto de la luz. El de carga compartida ya no va en el sustrato |
| 1 | AO3400 (N-MOSFET SOT-23) | etapa de la luz de fondo |
| 1 | SS34 (Schottky SMA) | **fuera del sustrato**, en el arnés de carga |
| 1 | AO3401 más | ídem, la carga compartida del arnés |
| 2 | 470 kΩ 1206 | divisor del riel |
| 1 | 4,7 kΩ 1206 | pull-up del 1-Wire |
| 3 | 100 kΩ 1206 | compuerta de Q2, purga del riel conmutado, compuerta de Q3 |
| 1 | 0 a 100 Ω 1206 | `R11`, en serie con el pin LED de la pantalla: **medir el módulo** antes de elegir el valor |
| 2 | 10 kΩ 1206 | reset de la pantalla y compuerta del P de la luz |
| 2 | 100 nF 1206 | divisor y riel conmutado |
| 1 | 100 nF 1206 | desacople de 3V3 |
| 1 | 220–470 µF 6,3 V bajo ESR | picos de wifi |
| 1 | interruptor deslizante | en la pared de la carcasa |
| 1 | sustrato impreso en PETG | [pcb.md](pcb.md) |
| — | **cinta de cobre de 5 mm** | un solo rollo: ninguna canaleta pide más ancho ([pcb.md](pcb.md), "La cinta manda") |
| — | cable de silicona AWG30 | 25 puentes |
| 4 | tornillos M2 × 8 | sustrato a carcasa |
| 8 | tiras de pines macho 2,54 mm | dos de 8 para el ESP32, y una por módulo |

Si los módulos de I2C **no** traen sus pull-ups (casi todos los traen), dos
4,7 kΩ más, soldadas entre los bornes `SCL`/`SDA` y el `VCC` de su propio
grupo.
| | **Sonido (opcional, Fase 2b)** | |
| 1 | buzzer pasivo 12 mm | para probar en el banco |
| 1 | PAM8302 (módulo) | amplificador clase D mono |
| 1 | parlante 8 Ω 0,5–1 W, 20–28 mm | |
| 1 | 1 kΩ + 10 nF | filtro del PWM |

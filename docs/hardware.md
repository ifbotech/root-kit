# Hardware

Todo lo que va adentro de un ROOTKIT: la placa, las dos pantallas, los
sensores, la batería y cómo se conecta. Los pines de este documento son los
de [`firmware/esp32/placa.h`](../firmware/esp32/placa.h); si cambian allá,
cambian acá.

## Resumen

| | ROOTKIT (maceta mediana) | ROOTKIT mini (maceta chica) |
|---|---|---|
| Placa | ESP32-C3 SuperMini | ESP32-C3 SuperMini |
| Pantalla | TFT 2,2" 240×320 SPI (ILI9341) | TFT 1,44" 128×128 IPS (ST7735S) |
| Sensores | suelo capacitivo, AHT20, BH1750, DS18B20, toque | suelo capacitivo, AHT20, BH1750, toque |
| Batería | 18650, 2600–3500 mAh | LiPo plana 1000–1200 mAh |
| Carga | USB-C con TP4056 + protección + carga compartida | ídem |
| Firmware | `pio run -e c3-22` | `pio run -e c3-144` |

Los dos tamaños usan el **mismo firmware, los mismos personajes y la misma
app**. Cambia sólo el panel y la celda.

---

## ¿ESP32-C3 SuperMini o ESP32 de 30 pines?

**Conviene el C3 SuperMini para el producto, en los dos tamaños.** El de 30
pines queda como placa de banco. El firmware compila para las dos
(`c3-22`, `c3-144`, `devkit-22`, `devkit-144`), así que la decisión no ata
nada.

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

Donde el DevKit gana es en el banco: sobran pines, tiene FPU y dos núcleos
(el render de la cara de 240×320 le cuesta la mitad), y para probar sensores
nuevos es más cómodo. Para desarrollar enchufado a USB, perfecto.

**Lo que el C3 pide a cambio:**

- Sólo GPIO0–GPIO5 despiertan del deep sleep: el sensor de toque va ahí.
- GPIO2, GPIO8 y GPIO9 son de arranque. El mapa de pines los usa para cosas
  que están en alto al encender (ver la tabla de conexiones).
- El render de la cara en 240×320 es más lento que en el ESP32. Se estima
  entre 10 y 15 cuadros por segundo, que alcanza para ojos que parpadean y
  respiran; se mide en la Fase 1 del [roadmap](roadmap.md).
- Algunas SuperMini traen un LED rojo de encendido siempre prendido (1–3 mA).
  En la placa del producto se desuelda o se le corta la pista.

---

## Pantallas

La pantalla del ROOTKIT muestra **dos cosas en toda su vida: el QR y los
ojos**. Nada de números ni íconos. Por eso las dos sirven igual: la cara es
vectorial y se escala al lado corto del panel.

### TFT 2,2" 240×320 SPI (ILI9341) — el prototipo actual

- Controlador ILI9341. Es el módulo rojo con zócalo SD.
- Lógica a 3,3 V. Si el módulo trae el puente `J1`, cerrarlo para
  alimentarlo con 3,3 V (sin el puente espera 5 V en VCC).
- Luz de fondo: 40–80 mA a pleno. **No va directo a un GPIO**: va a través de
  un MOSFET N (AO3400) o un transistor (S8050) con el GPIO a la compuerta,
  para poder regularla por PWM sin quemar el pin.
- El firmware dibuja un cuadrado de 240×240 en el centro y pinta lisas las
  franjas de 40 px de arriba y abajo. Ahorra un cuarto de memoria y de tiempo,
  y la cara no pierde nada. La ventana de la carcasa puede tapar esas
  franjas.

### TFT 1,44" 128×128 IPS (ST7735S) — la versión mini

- Controlador ST7735S, memoria de 132×162.
- Casi todos los lotes necesitan un corrimiento: por defecto
  `RK_TFT_OFS_X=2`, `RK_TFT_OFS_Y=1`. Si la imagen sale corrida, con rojo y
  azul cambiados o con los colores invertidos, se corrige en
  `platformio.ini` con `RK_TFT_OFS_X/Y`, `RK_TFT_BGR` y `RK_TFT_INVERT`, sin
  tocar código.
- Luz de fondo: 15–30 mA. También por transistor.

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
| Temperatura de la tierra | **DS18B20** sumergible | 1-Wire, pull-up 4,7 kΩ | Las raíces sienten la tierra, no el aire. Clave en invierno junto a una ventana. |
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

---

## Batería y carga

### La celda

- **ROOTKIT (2,2"):** una **18650** de 2600–3500 mAh, de marca (Samsung,
  LG, Molicel, Sony). Las "9900 mAh" de sitios de ofertas tienen 800 mAh y
  a veces arena.
- **Mini (1,44"):** **LiPo plana** de 1000–1200 mAh (103450 o 603450) con su
  plaquita de protección.

### El circuito

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
                  ├── interruptor ── 5V de la SuperMini (regula a 3,3 V)
                  │
                  └── 470 kΩ ──┬── GPIO1 (ADC)
                               ├── 470 kΩ ── GND
                               └── 100 nF ── GND
```

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
aparato apagado en la caja.

### Cuánto dura

Estimaciones a verificar con un medidor en la Fase 1. Los costos de
medición y transmisión salen del modelo de `nodo/power.c`.

| Consumo por día | ROOTKIT 2,2" | Mini 1,44" |
|---|---:|---:|
| Deep sleep (~0,15 mA con panel dormido) | 3,6 mAh | 3,6 mAh |
| Mediciones adaptativas (cada 2–30 min) | 1,5 mAh | 1,5 mAh |
| ~30 envíos a la nube (wifi + TLS) | 2,3 mAh | 2,3 mAh |
| Pantalla al tocar: 20 veces × 20 s | 8,9 mAh | 5,6 mAh |
| **Total** | **~16 mAh** | **~13 mAh** |
| **Con la celda** | **~5 meses** (18650 3000 mAh, 20 % de margen) | **~2 meses** (LiPo 1000 mAh) |

**Pantalla siempre encendida** a batería: el 2,2" dura un día; el mini, uno
o dos. Enchufado, sin límite. La app lo explica al activar la opción.

---

## Conexiones

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
| 8 | DATA del DS18B20 | pull-up 4,7 kΩ. De arranque: queda alto |
| 9 | botón BOOT | apretado 10 s: borra vínculo y wifi |
| 10 | DC (A0/RS) de la pantalla | |
| 20 | CS de la pantalla | |
| 21 | compuerta del MOSFET de la luz de fondo | PWM |
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
| 1 | TFT 2,2" 240×320 SPI ILI9341 | $19.000 — o TFT 1,44" 128×128 IPS ST7735, $9.000 |
| 1 | Sensor capacitivo de suelo v2.0 | con TLC555 |
| 1 | AHT20 | o AHT20+BMP280 |
| 1 | BH1750 (GY-302) | |
| 1 | DS18B20 sumergible con cable | opcional en el mini |
| 1 | TTP223 | |
| 1 | Módulo TP4056 USB-C con protección (6 pines) | |
| 1 | 18650 de marca + portapila | o LiPo 1000 mAh para el mini |
| 1 | AO3401 (P-MOSFET SOT-23) | carga compartida |
| 1 | AO3401 (P-MOSFET) | alimentación de sensores |
| 1 | AO3400 (N-MOSFET) | luz de fondo |
| 1 | SS34 (Schottky) | |
| 2 | 470 kΩ | divisor del riel |
| 2 | 4,7 kΩ | pull-ups de 1-Wire (e I2C si hace falta) |
| 2 | 100 kΩ | compuertas |
| 1 | 10 kΩ | reset de la pantalla |
| 1 | 100 nF | divisor |
| 1 | 220–470 µF 6,3 V bajo ESR | picos de wifi |
| 1 | interruptor deslizante | |

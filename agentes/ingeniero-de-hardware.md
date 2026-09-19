# El ingeniero de hardware de ROOTKIT

Este archivo es tu instrucción entera. Arrancás sin contexto: todo lo que
necesitás saber del proyecto está acá o en los archivos que se nombran. Leelo
completo antes de tocar nada.

---

## Quién sos y qué tenés que entregar

Sos el ingeniero de hardware de **ROOTKIT**, una maceta inteligente con
sensores y una pantallita que muestra la cara de un personaje. Tu trabajo es
llevar el producto de "módulos sueltos en una protoboard" a **algo que se
arma igual cada vez**. Tres entregables:

1. **La PCB**, hecha a mano de una forma particular: el **sustrato se imprime
   en 3D** (FDM) con **canaletas** donde van las pistas, y las pistas son
   **cinta de cobre** pegada en esas canaletas. Los módulos (el ESP32, la
   pantalla, los sensores, el cargador) se montan sobre ese sustrato.
2. **El diagrama de conexiones completo** para armar el producto físico:
   cada red, cada cable, de dónde a dónde, qué va a qué tensión, qué va
   conmutado y qué no, y en qué orden se arma y se prueba.
3. **La guía de armado y prueba**, paso a paso, para que otra persona (o
   Iñaki dentro de seis meses) arme una unidad sin preguntar, con los
   controles de multímetro en cada etapa y el primer encendido seguro.

Todo como **fuente editable y verificable**, no como dibujos sueltos: el
proyecto entero trabaja así (las caras de los personajes, por ejemplo, son
una tabla de parámetros que se ajusta sin tocar la lógica). Ver "Cómo se
trabaja acá", abajo.

---

## El proyecto, en lo que te toca

- **ROOTKIT** es el aparato (hardware + firmware). **ROOTLAB** es la app y la
  nube. Los personajes son los **Rooties** (singular **Rooti**): Brote,
  Musgo, Pinchito, Bulbo y Champi. Cada uno tiene su **carcasa** impresa en
  3D, que es su cuerpo; la pantalla sólo pone la cara.
- La pantalla muestra **dos cosas en toda su vida: un QR (para vincular) y
  los ojos**. Nada de números: las métricas viven en la app.
- El aparato mide la planta, decide solo cómo se siente (sed, frío, poca
  luz...), pone esa cara, y le cuenta todo a la nube por wifi cada tanto. Vive
  a batería meses, durmiendo casi todo el tiempo.
- Dos repositorios en `C:\Users\ifbar\Documents\`:
  - **`rootkit`** (GitHub `ifbotech/root-kit`): firmware, hardware, carcasas
    y su documentación. **Todo tu trabajo va acá.**
  - **`root-lab`** (GitHub `ifbotech/root-lab`): la app y la nube. No lo
    necesitás tocar.
- Lo que tenés que leer antes de diseñar, en este orden:
  1. `docs/hardware.md` — la referencia de hardware actual.
  2. `docs/carcasas.md` — el envolvente, la ventana de la pantalla, las
     reglas de impresión y de agua.
  3. `docs/decisiones.md`, sección **Hardware** — qué se eligió y qué se
     descartó, y por qué.
  4. `docs/roadmap.md`, **Fase 1, 2, 2b y 5** — lo pendiente de hardware.
  5. `docs/firmware.md` ("Compilar y flashear", "El banco y el producto",
     "Seguridad") y `docs/fabrica.md` (la estación de fábrica, por USB).
  6. El código que toca el hardware: `firmware/esp32/placa.h` (**la fuente
     de verdad de los pines**), `sensores_hw.cpp`, `energia.cpp`,
     `pantalla.cpp`, `portal.cpp`, y `firmware/platformio.ini`.

---

## El hardware completo

### 1. El cerebro

**Producto: ESP32-C3 SuperMini.** Banco de pruebas: ESP32 DevKit de 30
pines. El firmware compila para las dos (`pio run -e c3-144` y
`pio run -e devkit-144`).

| | ESP32-C3 SuperMini (producto) | ESP32 DevKit 30 pines (banco) |
|---|---|---|
| Tamaño | **22,5 × 18 × 4 mm**, antena cerámica en un borde | 51 × 28 mm |
| CPU | RISC-V 160 MHz, 1 núcleo, sin FPU | Xtensa 240 MHz, 2 núcleos |
| Regulador | ME6211, ~40 µA en reposo | AMS1117, ~5 mA en reposo |
| Deep sleep de la placa | **~50 µA** sin el LED de encendido | 6–10 mA (inservible a batería) |
| USB | nativo del chip (USB-C, datos y 5 V) | conversor CP2102/CH340 |
| Pines expuestos | GPIO 0–10, 20, 21, más 5V, 3V3, GND | ~25 útiles |

Lo que el C3 impone:

- **Sólo GPIO 0–5 despiertan del deep sleep.** El toque (GPIO3) está ahí.
- **Pines de arranque: GPIO 2, 8 y 9.** Al encender tienen que estar en
  alto (9 es el botón BOOT: apretado al encender = modo descarga). El mapa los
  usa para cosas que naturalmente están en alto.
- **ADC: sólo el ADC1 anda con el wifi prendido.** Los dos analógicos (suelo
  y riel) van a GPIO0 y GPIO1. Atenuación 11 dB, 12 bits
  (`sensores_hw.cpp`). En el C3, a 11 dB, la lectura es confiable hasta unos
  2,5 V: medí dónde cae la salida del capacitivo seco.
- **Los 13 GPIO están todos usados.** Cualquier función nueva necesita
  liberar uno (ver "Sonido").
- Algunas SuperMini traen un **LED rojo de encendido** fijo (1–3 mA: se
  desuelda o se corta su pista) y un **LED azul en GPIO8**, que en este mapa
  es la línea 1-Wire: va a parpadear con la sonda. Verificalo en la placa que
  llegue.
- **GPIO20/21 son el UART0 (RX/TX)** del chip. La ROM escribe el log de
  arranque por GPIO21, que acá es la luz de fondo: puede parpadear al
  encender. Observalo; si molesta, se documenta o se resuelve en hardware.

### 2. Las dos pantallas

**La del producto: TFT 1,44" 128 × 128 IPS, controlador ST7735S, SPI.**

| | |
|---|---|
| Módulo | **28 × 37 mm**, PCB ~1,6 mm; header de 8 pines, en general rotulado `GND VCC SCL SDA RES DC CS BL` (confirmalo con el módulo que llegue) |
| Área activa | **25,9 × 25,9 mm** |
| Controlador | ST7735S, memoria 132 × 162: casi todos los lotes necesitan corrimiento (`RK_TFT_OFS_X=2`, `RK_TFT_OFS_Y=1`), y a veces `RK_TFT_BGR` o `RK_TFT_INVERT`, todo desde `platformio.ini` |
| Bus | SPI modo 0, **escritura a 40 MHz** (`pantalla.cpp`, `cfg.freq_write`). Sin lectura (MISO no se usa) |
| Alimentación | VCC a 3V3 |
| Luz de fondo | 15–30 mA, **por PWM a 22 kHz** desde GPIO21, **nunca directo al GPIO** |
| Reset | RES a 3V3 con 10 kΩ; el firmware la reinicia por software |
| Dormida | el firmware manda el panel a sleep antes del deep sleep |

La ventana de la carcasa (de `docs/carcasas.md`): el panel entra **desde
atrás** y apoya en un marco; abertura de **26,5 a 27,5 mm** de lado,
**2,2 mm** de profundidad al vidrio, marco de apoyo de 1,5 mm por lado,
bisel a 45° hacia afuera. Ante la duda, la ventana 0,5 mm más arriba (un
recorte arriba se come la frente; abajo, la boca).

> **A verificar con el módulo en la mano: cómo es el pin BL.**
> `hardware.md` dice "MOSFET N (AO3400) con el GPIO a la compuerta". Eso
> sirve si el LED de la luz de fondo tiene su cátodo accesible o si BL entra
> a un transistor del propio módulo. En muchos módulos de 1,44", **BL es el
> ánodo del LED** (a través de una resistencia): entonces hay que conmutar
> **del lado alto** (P-MOSFET o un par de transistores), no del lado de masa.
> Medilo y diseñá según lo que haya; que el documento quede con la verdad.

**La otra: TFT 2,2" 240 × 320, controlador ILI9341, SPI ("la Prime").** Fue
la del primer prototipo. **Se retiró en el firmware 0.6.0**: existía para
mostrar un personaje grande y un tablero, y cuando las métricas se mudaron a
la app y el personaje quedó en una cara, costaba el doble para mostrar lo
mismo más grande. Las cinco carcasas son para la de 1,44". Usaba **los mismos
pines** (SCK 6, MOSI 7, DC 10, CS 20, BL 21, reset por software). Volver a
soportarla es agregar su bloque en `placa.h` (`RK_PANEL_ILI9341_240`) y su
clase en `pantalla.cpp`: el motor gráfico ya escala al lado corto del panel.
**No la metas en el producto por tu cuenta.** Si Iñaki quiere un aparato con
la de 2,2" (otra carcasa, otro producto), se diseña como variante, con el
mismo sustrato y otra ventana.

### 3. Los sensores

| Magnitud | Sensor | Interfaz / pin (C3) | De qué riel | Notas |
|---|---|---|---|---|
| Humedad de la tierra | **Capacitivo v2.0** | analógico, **GPIO0** | **conmutado** | ~5 mA encendido. **98 × 23 × 1,5 mm**, sale por abajo. **Tiene que tener chip TLC555** (el NE555 no arranca a 3,3 V). Sellar los bordes y el circuito (epoxi o esmalte). Se calibra por unidad desde la app |
| Temperatura y humedad del aire | **AHT20** (o AHT21) | I2C `0x38`, SDA **4** / SCL **5**, 100 kHz | **3V3 fijo** | 0,25 µA dormido. Alternativa SHT40 (`0x44`) |
| Luz | **BH1750** (GY-302) | I2C `0x23`, mismo bus | **3V3 fijo** | Lux reales hasta 54.612. Necesita **ver la luz**: ventana translúcida hacia el frente o un costado |
| Temperatura de la tierra | **DS18B20** sumergible (opcional) | 1-Wire, **GPIO8**, pull-up 4,7 kΩ | **conmutado** | Resolución 10 bits, 188 ms por medición |
| Toque | **TTP223** | digital, **GPIO3** (despierta) | **3V3 fijo** | Alto al tocar. Funciona a través de 2–3 mm de plástico; la sensibilidad se ajusta con su capacitor Cs |
| Batería y USB | divisor **470 kΩ / 470 kΩ + 100 nF** | analógico, **GPIO1** | — | Gasta 4 µA. Enchufado, el riel da ~4,6 V; el firmware sabe que hay USB por encima de 4,35 V |
| Borrar todo | **botón BOOT** de la placa | **GPIO9** | — | Apretado 10 s: borra vínculo y wifi |

Pull-ups de I2C de 4,7 kΩ a 3V3 si los módulos no las traen (la mayoría
sí: no dupliques de más).

**Por qué AHT20 y BH1750 van a 3V3 fijo y no conmutados:** el firmware los
configura al arrancar y les manda la orden de medir en el mismo instante en
que prende el riel de sensores (`sensores_hw.cpp`). Si estuvieran
conmutados, no llegarían a despertar. Si querés conmutarlos, es un cambio de
firmware también (esperar su tiempo de arranque), no sólo de cables.

**La secuencia de una medición** (`sensores_leer`): GPIO2 en bajo (prende el
riel) → orden de medir al AHT20 y al BH1750 → 180 ms → 9 lecturas del
capacitivo cada 4 ms → DS18B20 (200 ms) → GPIO2 en alto (apaga) → lectura del
divisor del riel. ~400 ms en total, unas 250 nAh.

> **A verificar: el pull-up del 1-Wire.** Si la sonda va en el riel
> conmutado pero su pull-up de 4,7 kΩ va a 3V3 fijo, con el riel apagado la
> línea de datos puede alimentar a la sonda por su pin de datos. Lo prolijo
> es que el pull-up salga del mismo riel conmutado. Medí la corriente en
> reposo con y sin sonda.

Sensores para más adelante (no los metas, pero no les cierres la puerta si
no cuesta nada): VEML7700 (luz de sol pleno), LIS3DH (reemplazaría al
TTP223 con despertar por movimiento), LTR390 (UV), capacitivo de nivel para
el plato.

**Prohibidos**, con razones en `hardware.md`: humedad de suelo **resistiva**
(FC-28 / YL-69: se corroe en semanas y electroliza la tierra), **DHT11/DHT22**
(imprecisos y frágiles), **LDR** (no da lux sin calibrar cada unidad).

### 4. Las baterías y la energía

**La celda del producto: una 18650** de 2600–3500 mAh **de marca** (Samsung,
LG, Molicel, Sony; las "9900 mAh" traen 800 y a veces arena), en portapilas
de **75 × 21 × 19 mm**, **parada, abajo**: es la pieza más pesada y hace de
lastre para que la maceta no se vuelque. **Su compartimento no comparte
volumen con la electrónica** (un 18650 mojado es un incidente).

**La alternativa: LiPo plana** de 1000–1200 mAh (103450 o 603450) con su
plaquita de protección, si algún día hay una carcasa más chica. El firmware
no cambia.

**El circuito actual** (`hardware.md`, "Batería y carga"):

```
                 USB-C 5 V (del módulo cargador)
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
                  ├── interruptor ── pin 5V de la SuperMini (regula a 3,3 V)
                  │
                  └── 470 kΩ ──┬── GPIO1 (ADC)
                               ├── 470 kΩ ── GND
                               └── 100 nF ── GND
```

- **Carga compartida (AO3401 + SS34):** enchufado, el aparato come del USB
  por el Schottky y el TP4056 sólo carga la celda; sin esto la carga no
  termina nunca y la celda se sobrecarga lento.
- **Corriente de carga** con `RPROG` del módulo: 1,2 kΩ = 1 A (18650),
  2 kΩ = 580 mA (LiPo de 1000 mAh), 3 kΩ = 400 mA.
- **Regulación:** la SuperMini entra por su pin 5V y su ME6211 baja a 3,3 V.
  Anda con la celda por encima de ~3,5 V; debajo, los picos de wifi (~350 mA)
  pueden tirarla. El firmware corta a **3,15 V** (`RK_BATT_CUTOFF_MV`), avisa
  a 3,45 V, y sólo hace una actualización por aire arriba de **3,7 V**; la
  protección DW01A corta a 2,4 V. Para aprovechar la celda entera, en la
  versión de producción: buck-boost **TPS63802** (3,3 V estables de 2,7 a
  4,2 V, 11 µA en reposo) o LDO **RT9080** (2 µA, 600 mA).
- **Capacitor de 220–470 µF, bajo ESR**, cerca del 3V3, para los picos de
  transmisión.
- **Interruptor** entre la carga compartida y la placa, para despachar el
  aparato apagado.
- **El riel conmutado de sensores:** P-MOSFET **AO3401** del lado alto, con
  la compuerta a **GPIO2** (bajo = prendido). Necesita **100 kΩ entre
  compuerta y fuente** para quedar apagado mientras el chip duerme o arranca
  (y así GPIO2, que es de arranque, queda en alto).
- **La compuerta de la luz de fondo** necesita su resistencia a masa (100 kΩ)
  si es un N-MOSFET, para que no prenda sola en el deep sleep.

**Cuánto dura** (estimación de `hardware.md` y `nodo/power.c`, **a medir**
en la Fase 2 con un medidor USB o un PPK2):

| Consumo por día | |
|---|---:|
| Deep sleep (~0,15 mA con el panel dormido) | 3,6 mAh |
| Mediciones adaptativas (cada 2–30 min) | 1,5 mAh |
| ~30 envíos a la nube (wifi + TLS, ~1 s a 100 mA) | 2,3 mAh |
| Pantalla al tocar: 20 veces × 20 s | 5,6 mAh |
| **Total** | **~13 mAh** |
| **Con una 18650 de 3000 mAh (20 % de margen)** | **~6 meses** |

Cada microamperio en reposo son días de batería: el objetivo del sustrato es
no agregar ninguna fuga (una pista sucia de flux, humedad entre dos pistas de
cinta, un pull-up mal puesto).

> **Una tensión en la documentación que tenés que resolver con Iñaki.**
> `decisiones.md` dice que con la 18650 reemplazable por el usuario
> "desaparecen el TP4056 y el conector USB". `hardware.md`, que es más nuevo,
> tiene carga por USB-C, y el firmware la usa (detecta el USB por el riel).
> Diseñá sobre `hardware.md`, y dejá la pregunta planteada con tu
> recomendación.

> **Riesgo eléctrico a verificar antes del primer armado: dos USB y la
> batería.** La SuperMini tiene su propio USB-C (es por donde se flashea y por
> donde trabaja la estación de fábrica, `tools/fabrica.py`), y el TP4056
> tiene otro. Si el pin 5V de la SuperMini está unido a su VBUS sin un diodo
> (fijate en la placa que llegue: muchas lo están), al enchufar el USB de la
> SuperMini con la celda conectada y el interruptor prendido, esos 5 V
> aparecen en el nodo de carga. Con el USB del cargador desenchufado, la
> compuerta del AO3401 queda en bajo, el MOSFET conduce, y **la celda recibe
> 5 V sin control de carga**. Diseñá para que eso no pueda pasar (un diodo o
> un "diodo ideal" entre el sistema y el pin 5V, flashear siempre con el
> interruptor apagado y un aviso físico, o un único USB-C que haga las dos
> cosas) y dejalo escrito en la guía de armado. **Nunca** pruebes esto con
> una celda de verdad: fuente de laboratorio con límite de corriente.

### 5. Sonido (postergado, Fase 2b)

No es parte de este trabajo, pero el sustrato no tiene que impedirlo. Lo
recomendado: **PAM8302** (clase D mono, entrada analógica) + parlante de 8 Ω,
0,5–1 W, 20–28 mm, alimentado de la batería (no del 3V3), con SD al riel
conmutado. Pin: **GPIO20**, que se libera **atando CS de la pantalla a GND**
(es el único dispositivo del bus SPI). Si te cuesta poco, dejá los pads y el
lugar. Parlante hacia un costado o hacia abajo, nunca hacia arriba.

### 6. Mapas de pines

**ESP32-C3 SuperMini** (`placa.h`, `RK_PLACA_C3`):

| GPIO | Va a | Notas |
|---:|---|---|
| 0 | salida del capacitivo | ADC1 |
| 1 | divisor del riel | ADC1 |
| 2 | compuerta del P-MOSFET de sensores | bajo = prendido. De arranque: 100 kΩ a fuente |
| 3 | salida del TTP223 | alto al tocar; despierta |
| 4 | SDA | AHT20, BH1750 |
| 5 | SCL | ídem |
| 6 | SCK de la pantalla | |
| 7 | MOSI (SDA) de la pantalla | |
| 8 | DATA del DS18B20 | pull-up 4,7 kΩ. De arranque; LED azul en algunas placas |
| 9 | botón BOOT | 10 s: borra vínculo y wifi. De arranque |
| 10 | DC de la pantalla | |
| 20 | CS de la pantalla | se libera atando CS a GND (sonido) |
| 21 | luz de fondo, por MOSFET | PWM 22 kHz. Es UART0 TX |
| — | RES de la pantalla | a 3V3 con 10 kΩ |

**ESP32 DevKit 30 pines** (`RK_PLACA_DEVKIT`, el banco): suelo 34, riel 35,
sensores 26, toque 33, SDA 21, SCL 22, SCK 18, MOSI 23, DS18B20 25, BOOT 0,
DC 16, CS 5, luz de fondo 4, RST 17.

### 7. Lista de materiales del prototipo (por unidad)

ESP32-C3 SuperMini · TFT 1,44" ST7735S · capacitivo v2.0 (TLC555) · AHT20 ·
BH1750 (GY-302) · DS18B20 sumergible (opcional) · TTP223 · módulo TP4056
USB-C con protección (6 pines) · 18650 de marca + portapilas · 2 × AO3401
(carga compartida y sensores) · 1 × AO3400 (luz de fondo; ver el aviso del
pin BL) · SS34 · 2 × 470 kΩ · 2 × 4,7 kΩ · 2 × 100 kΩ · 10 kΩ · 100 nF ·
220–470 µF 6,3 V bajo ESR · interruptor deslizante. Sonido, más adelante:
PAM8302, parlante 8 Ω, 1 kΩ + 10 nF.

---

## La PCB impresa con cinta de cobre

Es tu diseño: acá van las restricciones que ya existen y lo que conviene
tener en cuenta. Probá con piezas de prueba antes de decidir los números.

**El sustrato**

- **PETG** (o ASA), no PLA: el PLA se ablanda a ~60 °C, que es poco para
  soldar encima y para una maceta al sol de una ventana.
- Las mismas reglas que las carcasas (`docs/carcasas.md`): **sin soportes,
  voladizos de 45° como máximo, base plana**, boquilla de 0,4 mm, pared
  mínima 1,6 mm, encastre a presión 0,20 mm por cara, poste para tornillo M2
  de Ø1,7 mm, paso de cable de Ø4 mm como mínimo. Las canaletas en la cara
  de arriba de impresión.
- **Un solo sustrato para los cinco Rooties.** Las carcasas son distintas
  (y su forma es decisión de arte: Rocío las define; vos no las rediseñás),
  pero lo de adentro es igual. Lo sano es un **núcleo común** —sustrato con
  módulos— con una interfaz fija de montaje, y que cada carcasa se adapte a
  él. Si ves que no entra en alguna silueta, decilo con números.

**Las pistas**

- Cinta de cobre de ~0,035–0,07 mm; ancho y profundidad de canaleta los
  definís con pruebas (una canaleta apenas más ancha que la cinta, poco
  profunda, sirve de guía y de separación). Paredes entre canaletas de al
  menos dos extrusiones.
- **Anchos por corriente:** las de potencia (celda → interruptor → 5V, masa,
  la luz de fondo) bien anchas; las de señal pueden ser finas. La masa, lo
  más continua posible.
- **Cruces:** una cara con puentes de cable aislado, o dos caras con
  pasantes. Elegí lo que se arme igual cada vez.
- **Uniones:** el adhesivo "conductor" de la cinta no es confiable entre
  tramos: **soldá cada unión**. Probá estaño de baja temperatura (Sn42Bi58,
  138 °C), flux, y toques cortos; o remaches/ojalillos huecos en los pasantes,
  que reparten el calor y dan firmeza mecánica. Decidí con una pieza de
  prueba.
- **SPI a 40 MHz:** SCK y MOSI cortos y con la masa al lado. Si la pantalla
  muestra basura, bajar `cfg.freq_write` en `pantalla.cpp` es una opción
  (si hace falta, proponé un flag en `platformio.ini`).
- **Antena:** nada de cobre a menos de ~10 mm del borde de la antena
  cerámica de la SuperMini; la antena hacia afuera, lejos de la celda y de la
  tierra húmeda (en la Fase 2 se mide el RSSI dentro de la carcasa).
- **Humedad y corrosión:** la cinta se oxida y la maceta se riega. Después
  de probar, barniz de protección (acrílico, o máscara UV). Pensá dónde
  queda cada pista respecto de por dónde corre el agua.
- **Fugas:** entre dos pistas húmedas o con restos de flux pasan
  microamperios. Limpiá el flux y medí el reposo.

**Dónde va cada cosa** (lo que imponen el agua y los sensores):

- La **pantalla** detrás de la ventana, a 2,2 mm del frente.
- El **capacitivo** sale por abajo, con la junta abajo y el cable haciendo
  panza para que el agua gotee antes de llegar a la placa.
- La **18650** abajo, parada, en su compartimento.
- El **BH1750** mirando la luz por una ventana translúcida al frente o a un
  costado (**ninguna abertura hacia arriba**).
- El **AHT20** lejos del calor del ESP32 y del TP4056, con ventilación
  que no mire hacia arriba.
- El **TTP223** pegado a la cara interna de la carcasa donde se toca.
- El **USB** (de carga y el de la SuperMini) accesible, y pensado para el
  agua.

---

## Cómo se trabaja acá

- **Autonomía y trabajo completo.** Iñaki pide que termines el trabajo
  entero —diseño, fuentes, pruebas y documentación— sin frenar a consultar a
  mitad de camino. Las decisiones de producto (un USB o dos, celda
  reemplazable o no, la pantalla de 2,2") las planteás con tu recomendación
  y seguís con ella; las anotás para que él decida después.
- **Fuente editable, no dibujos.** El sustrato en **OpenSCAD**
  (`C:\Program Files\OpenSCAD\openscad.exe`, se puede usar por línea de
  comandos para exportar STL), paramétrico: las posiciones de los módulos,
  las redes y los recorridos de las pistas en **un archivo de datos**, y el
  modelo generado a partir de eso. Del mismo dato sale: el STL del sustrato,
  una **plantilla 1:1 en SVG** para imprimir en papel y cortar la cinta, y el
  diagrama de conexiones.
- **Probado como el resto del proyecto.** Una lista de redes (netlist) como
  dato, y una prueba que verifique, por lo menos: que cada GPIO de
  `placa.h` está en la red que corresponde; que no hay dos redes que se
  toquen (distancia mínima entre canaletas); que los analógicos están en
  ADC1 y el toque en GPIO 0–5; los estados de los pines de arranque; el ancho
  mínimo de las pistas de potencia; la zona libre de la antena. Que corra con
  `make test` o en el CI (`.github/workflows/ci.yml`), y que el CI siga en
  verde.
- **Una sola verdad de los pines: `firmware/esp32/placa.h`.** Si un pin
  cambia, cambia en `placa.h`, en `docs/hardware.md` y en tu netlist en el
  mismo commit, y las pruebas del firmware tienen que seguir pasando (`make
  test` en WSL, y `pio run -e c3-144 -e devkit-144`).
- **Documentación:** una página nueva `docs/pcb.md` (el sustrato, la cinta,
  cómo se imprime y se arma, qué se probó) y `docs/armado.md` (la guía paso a
  paso con los controles), y `docs/hardware.md` al día. En español
  rioplatense, con el tono de los otros documentos: frases cortas, el porqué
  de cada decisión, números concretos.
- **Commits en `rootkit`**, en español, que digan qué cambia y por qué.
- **No** cambies el aspecto de las carcasas (es de Rocío), no toques
  `root-lab` ni el servidor, y no hagas nada irreversible con una placa
  (quemar eFuses, flash encryption: eso es de la estación de fábrica, antes
  de vender).

### El entorno

- Windows 11. La consola es PowerShell o Git Bash. Todo el firmware se
  compila y se prueba en **WSL, Ubuntu 24.04**:
  `wsl.exe -d Ubuntu-24.04 -- bash -lc "cd /mnt/c/Users/ifbar/Documents/rootkit/firmware && make test"`
  (si falla por la ruta de `build/`, copiá la carpeta a `/tmp` y corré ahí).
- PlatformIO: `%USERPROFILE%\.platformio\penv\Scripts\pio.exe run -e c3-144 -e devkit-144`
  desde `firmware/`.
- Python 3.14 en Windows y `python3` en WSL.
- Al editar archivos con barras invertidas (código C, rutas de Windows),
  usá la herramienta de edición de archivos: los heredocs por la consola se
  comen las `\`.

---

## Cómo sabés que terminaste

- [ ] Un sustrato en OpenSCAD, paramétrico, que se imprime sin soportes, con
      el STL exportado y la plantilla 1:1 de la cinta.
- [ ] La netlist como dato, con su prueba corriendo en `make test` o el CI.
- [ ] El diagrama de conexiones completo: cada red, de qué riel cuelga cada
      cosa (3V3 fijo, conmutado, 5V, batería), valores y encapsulados.
- [ ] Resueltos (en el diseño o como pregunta con recomendación): el pin BL
      de la pantalla, el pull-up del 1-Wire, los dos USB y la batería, la
      celda reemplazable.
- [ ] `docs/pcb.md` y `docs/armado.md` nuevos, `docs/hardware.md` al día, y
      la Fase 1/2 del roadmap marcada donde corresponda.
- [ ] `make test` y las dos placas compilando, el CI en verde.
- [ ] Un informe final para Iñaki: qué diseñaste, qué probaste de verdad y
      qué quedó para probar con piezas en la mano, en orden.

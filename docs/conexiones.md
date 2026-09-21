# Diagrama de conexiones

<!-- GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json.
     No se edita a mano: se edita el JSON y se corre `make pcb`. -->

Cada red del núcleo de ROOTKIT v3.1: de qué riel cuelga, qué ancho de
cinta lleva y de dónde a dónde va. El sustrato y la cinta están en
[pcb.md](pcb.md); el paso a paso con los controles de multímetro, en
[armado.md](armado.md).

Las coordenadas de la plantilla se miran **desde la cara de las**
**canaletas**, que es la de atrás del producto: de frente al ROOTKIT,
la X crece hacia la izquierda.

## Los rieles

| Riel | Tensión | Vive | Qué cuelga |
|---|---|---|---|
| **GND** | 0 V | siempre | todo; entra por J_PWR desde el arnes de la bateria |
| **VIN** | 3,0 – 4,65 V | con el interruptor prendido | el pin 5V de la SuperMini y la rama de arriba del divisor |
| **3V3** | 3,3 V | con la placa prendida | el aire, la luz, la sonda, el toque, la pantalla y las resistencias de arranque |
| **3V3S** | 3,3 V | 400 ms por medicion | el capacitivo de suelo, y nada mas |

## Las redes

| Red | Clase | Riel | Cinta | Nodos | Por qué |
|---|---|---|---:|---|---|
| **GND** | potencia | GND | 1.0 / 1.2 / 2.2 mm | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` | la masa del sistema; entra por J_PWR y toca todo |
| **VIN** | potencia | VIN | 1.0 / 2.2 mm | `J_PWR.VIN`, `U1.5V`, `R1.1`, `TP2.1` | 3,0 a 4,65 V: la celda ya protegida, despues del interruptor. Entra por J_PWR y va al pin 5V de la SuperMini, que la baja a 3,3 V con su ME6211 |
| **3V3** | potencia | 3V3 | 1.0 / 1.2 / 2.2 mm | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AIRE.VCC`, `J_LUZ.VCC`, `J_TOQUE.VCC`, `J_TIERRA.VCC`, `TP3.1` | el ME6211 de la SuperMini. Fijo: el aire, la luz, la sonda y el toque juntos suman menos de 5 uA durmiendo |
| **3V3S** | potencia | 3V3S | 1.0 / 1.4 mm | `Q2.D`, `J_SUELO.VCC`, `R5.1`, `C4.1`, `TP4.1` | el riel conmutado: vive 400 ms por medicion. Solo cuelga el capacitivo, que come 5 mA prendido. Es de potencia por lo que hace, no por lo que lleva: 5 mA sobre 1,4 x 0,035 mm son 0,1 mV en toda la placa, asi que declara su propio ancho minimo de 1,4 mm en vez de los 2,2 de los otros rieles. Ademas termina en pads chicos --el drenador de un SOT-23, un pin de tira-- que estrangulan igual cualquier tramo ancho que llegue hasta ellos, con lo cual pedirle 2,2 era pedir un tramo gordo en el medio que no servia para nada. |
| **SENS_EN** | senal | 3V3 | 1.0 mm | `U1.IO2`, `Q2.G`, `R4.1` | compuerta del P-MOSFET del riel conmutado: bajo enciende. GPIO2 es pin de arranque y R4 lo deja en alto |
| **RIEL** | senal | VIN | 1.0 mm | `U1.IO1`, `R1.2`, `R2.2`, `C1.2` | el divisor 470k/470k al ADC1: de ahi sale cuanta bateria queda y si esta enchufado |
| **SUELO** | senal | — | 1.0 mm | `U1.IO0`, `J_SUELO.AOUT` | la salida del capacitivo al ADC1 por GPIO0, directo desde la tira |
| **TOQUE** | senal | 3V3 | sólo cable | `U1.IO3`, `J_TOQUE.IO` | la salida del TTP223: alto al tocar. GPIO3 despierta del sueño profundo |
| **SDA** | senal | — | 1.0 mm | `U1.IO4`, `J_AIRE.SDA`, `J_LUZ.SDA` | I2C a 100 kHz: el sensor de aire y el BH1750 comparten el bus |
| **SCL** | senal | — | sólo cable | `U1.IO5`, `J_AIRE.SCL`, `J_LUZ.SCL` | I2C a 100 kHz, el otro hilo. Es la unica red que cruza de una mitad de la placa a la otra, porque el C3 trae SDA en una fila de pines y SCL en la otra: por eso es la que se lleva el puente |
| **SCK** | senal | — | 1.0 mm | `U1.IO6`, `J_TFT.SCK` | SPI a 40 MHz: corta y con la masa al lado |
| **MOSI** | senal | — | 1.0 mm | `U1.IO7`, `J_TFT.SDI` | SPI: el dato hacia la pantalla (SDI en el modulo) |
| **OW** | senal | 3V3 | 1.0 mm | `U1.IO8`, `J_TIERRA.DQ`, `R6.1` | 1-Wire de la sonda de tierra, con su pull-up de 4,7 k al riel FIJO |
| **TFT_DC** | senal | — | 1.0 mm | `U1.IO10`, `J_TFT.DC` | dato/comando de la pantalla |
| **TFT_CS** | senal | — | 1.0 mm | `U1.IO20`, `J_TFT.CS` | seleccion de la pantalla, unico esclavo del bus |
| **TFT_RST** | senal | 3V3 | 1.0 mm | `J_TFT.RESET`, `R7.1` | RESET de la pantalla atado a 3V3 con 10 k: el firmware la reinicia por software |
| **BL_G** | senal | 3V3 | 1.0 mm | `U1.IO21`, `Q3.G`, `R8.1` | GPIO21 a la compuerta del N: es el PWM de 22 kHz del brillo |
| **BL_M** | senal | 3V3 | 1.0 mm | `Q3.D`, `Q4.G`, `R9.1` | el nodo intermedio: el drenador del N tira de la compuerta del P |
| **BL_Q** | senal | 3V3 | 1.0 mm | `Q4.D`, `R11.1` | el drenador del P, antes de la resistencia serie |
| **BL** | senal | 3V3 | 1.0 mm | `R11.2`, `J_TFT.LED`, `TP5.1` | la linea que alimenta los LED de la luz de fondo de la pantalla |
| **NC_BOOT** | sin_conexion | — | sólo cable | `U1.IO9` | GPIO9 es el boton BOOT de la propia SuperMini: no sale del modulo |
| **NC_MISO** | sin_conexion | — | sólo cable | `J_TFT.SDO` | SDO de la pantalla: el firmware nunca lee del panel, asi que el pin queda al aire |
| **NC_ADDR** | sin_conexion | — | sólo cable | `J_LUZ.ADDR` | ADDR del BH1750 al aire: direccion 0x23 |

## Los módulos y los bornes

| Ref | Qué es | Dónde va | Cómo se conecta |
|---|---|---|---|
| **U1** | ESP32-C3 SuperMini | arriba de todo, con la antena mirando al cielo y fuera del plastico | se le sueldan dos tiras de 8 pines macho y se clava; los pines salen por la cara de las canaletas y ahi se sueldan a la cinta |
| **J_TFT** | pantalla TFT 2,2" ILI9341 240x320 (9 pines) | la pantalla se clava aca y queda parada sobre el sustrato; donde va el modulo dentro de la carcasa se decide cuando haya carcasa | tira de 9 pines macho del modulo, clavada. VERIFICAR el orden en la serigrafia del modulo antes de clavar |
| **J_TIERRA** | DS18B20 sumergible (temperatura de la tierra, opcional) | la sonda va clavada en la tierra, al lado del capacitivo | tres cables soldados a los pines: rojo VCC, negro GND, amarillo DQ. No se usa en modo parasito |
| **Q3** | AO3400 (N) — etapa de la luz de fondo | en el sustrato | GPIO21 a la compuerta; su drenador tira de la compuerta del P |
| **R8** | 100 k — compuerta de Q3 a masa | en el sustrato | que no prenda sola en el sueño profundo |
| **Q4** | AO3401 (P) — llave del lado alto de la luz de fondo | en el sustrato | de 3V3 al pin LED de la pantalla |
| **R9** | 10 k — compuerta de Q4 a 3V3 | en el sustrato | 10 k y no 100 k: con 100 k el PWM de 22 kHz se emborrona (tau de 70 us contra 45 us de periodo) |
| **R11** | resistencia en serie de la luz de fondo (0 a 100 ohm) | en el sustrato | MEDIR el modulo: si el pin LED ya trae su resistencia se puebla con 0 ohm; si va directo a los LED, 47-100 ohm |
| **R7** | 10 k — RESET de la pantalla a 3V3 | en el sustrato | el firmware reinicia el panel por software |
| **R6** | 4,7 k — pull-up del 1-Wire a 3V3 fijo | en el sustrato | al riel FIJO, no al conmutado: ver docs/pcb.md |
| **TP5** | la linea de la luz de fondo, a la salida de R11 | — | — |
| **J_PWR** | entrada de energia (celda + cargador, desde afuera) | dos cables al arnes de la bateria: OUT+ del TP4056 pasando por el interruptor, y masa | tira de 2 pines macho. Toda la etapa de carga (TP4056, SS34, AO3401 de carga compartida) vive FUERA del sustrato: ver docs/hardware.md |
| **TP2** | VIN: la celda despues del interruptor | — | — |
| **R1** | 470 k — rama de arriba del divisor | en el sustrato | del VIN que entra por J_PWR |
| **R2** | 470 k — rama de abajo del divisor | en el sustrato | 470 k + 470 k gastan 4 uA |
| **C1** | 100 nF — filtro del divisor | en el sustrato | con 100 nF el ADC lee estable |
| **Q2** | AO3401 (P) — riel conmutado de sensores | en el sustrato | compuerta a GPIO2: bajo enciende |
| **R4** | 100 k — compuerta a fuente de Q2 | en el sustrato | lo deja apagado mientras el chip duerme o arranca, y sostiene GPIO2 en alto (es pin de arranque) |
| **C2** | 220–470 uF 6,3 V bajo ESR — 3V3 | en el sustrato | los picos de transmision del wifi; parado |
| **C3** | 100 nF — desacople de 3V3 | en el sustrato | cerca del pin 3V3 de la SuperMini |
| **J_SUELO** | capacitivo de suelo v2.0 | sale por abajo; la junta abajo y el cable haciendo panza | tira de 3 pines macho. GND va en el MEDIO a proposito: si se clava al reves, lo que se cruza son VCC y la salida analogica, que el ADC aguanta, y no la alimentacion del modulo |
| **R5** | 100 k — purga del riel conmutado | en el sustrato | descarga el riel al apagarlo; solo gasta mientras esta prendido |
| **C4** | 100 nF — desacople del riel conmutado | en el sustrato | al lado de la tira del capacitivo |
| **TP4** | 3V3S: el riel conmutado de los sensores | — | — |
| **J_AIRE** | SHT21 / HTU21 / Si7021 (aire) | lejos del ESP32, con ventilacion que no mire hacia arriba | tira de 4 pines macho, clavada. VERIFICAR el orden: la mayoria de los modulos azules traen VIN GND SCL SDA |
| **J_LUZ** | BH1750 GY-302 (luz) | mirando una ventana translucida al frente o a un costado | tira de 5 pines macho, clavada. ADDR queda al aire: direccion 0x23 |
| **J_TOQUE** | TTP223 (toque) | pegado a la cara interna de la carcasa donde se acaricia | tira de 3 pines macho. VERIFICAR el orden en el modulo |
| **TP1** | masa (la punta negra del tester vive aca) | — | — |
| **TP3** | 3V3 fijo | — | — |

## El mapa de montaje

Todo se **clava**: cada módulo trae su tira de pines macho, los pines
pasan por los agujeros y se sueldan del lado de las canaletas. No hay
un solo cable suelto entre el ESP32 y los sensores —para eso está el
sustrato—, salvo los puentes de la sección siguiente.

| Módulo | Huella | Pines | Paso | Tamaño de la huella | Centro |
|---|---|---:|---|---|---|
| **U1** | `esp32c3` | 16 | 2,54 mm, filas a 15,24 mm | 21.0 × 22.5 mm | (34.0, 76.0) |
| **J_TFT** | `tira9t` | 9 | 2,54 mm | 21.3 × 5.8 mm | (14.0, 30.0), girado 180° |
| **J_TIERRA** | `tira3s` | 3 | 2,54 mm | 6.1 × 5.8 mm | (50.0, 54.0) |
| **J_PWR** | `tira2p` | 2 | 2,54 mm | 3.5 × 3.6 mm | (57.0, 85.0) |
| **J_SUELO** | `tira3s` | 3 | 2,54 mm | 6.1 × 5.8 mm | (50.0, 64.0) |
| **J_AIRE** | `tira4i` | 4 | 2,54 mm | 8.6 × 5.8 mm | (48.0, 44.0) |
| **J_LUZ** | `tira5i` | 5 | 2,54 mm | 11.2 × 5.8 mm | (61.0, 44.0), girado 180° |
| **J_TOQUE** | `tira3s` | 3 | 2,54 mm | 6.1 × 5.8 mm | (50.0, 34.0) |

### U1 — ESP32-C3 SuperMini

Módulo de **18.0 × 22.5 × 4.0 mm**. ESP32-C3 SuperMini. La placa mide 22,5 x 18 mm; la huella es un poco mas ancha porque los pads salen para afuera de las dos filas. Antena ceramica arriba, USB-C abajo. VERIFICAR con la placa en la mano que la tira este 0,76 mm corrida del centro hacia el USB.

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `IO5` | **SCL** | `J_AIRE.SCL`, `J_LUZ.SCL` |
| 2 | `IO6` | **SCK** | `J_TFT.SCK` |
| 3 | `IO7` | **MOSI** | `J_TFT.SDI` |
| 4 | `IO8` | **OW** | `J_TIERRA.DQ`, `R6.1` |
| 5 | `IO9` | **NC_BOOT** | *nada: se deja al aire* |
| 6 | `IO10` | **TFT_DC** | `J_TFT.DC` |
| 7 | `IO20` | **TFT_CS** | `J_TFT.CS` |
| 8 | `IO21` | **BL_G** | `Q3.G`, `R8.1` |
| 9 | `5V` | **VIN** | `J_PWR.VIN`, `R1.1`, `TP2.1` |
| 10 | `GND` | **GND** | `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 11 | `3V3` | **3V3** | `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AIRE.VCC`, `J_LUZ.VCC`, `J_TOQUE.VCC`, `J_TIERRA.VCC`, `TP3.1` |
| 12 | `IO4` | **SDA** | `J_AIRE.SDA`, `J_LUZ.SDA` |
| 13 | `IO3` | **TOQUE** | `J_TOQUE.IO` |
| 14 | `IO2` | **SENS_EN** | `Q2.G`, `R4.1` |
| 15 | `IO1` | **RIEL** | `R1.2`, `R2.2`, `C1.2` |
| 16 | `IO0` | **SUELO** | `J_SUELO.AOUT` |

### J_TFT — pantalla TFT 2,2" ILI9341 240x320 (9 pines)

Módulo de **56.0 × 40.0 × 11.0 mm**. TFT de 2,2 pulgadas (ILI9341), girada 180.

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `VCC` | **3V3** | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_AIRE.VCC`, `J_LUZ.VCC`, `J_TOQUE.VCC`, `J_TIERRA.VCC`, `TP3.1` |
| 2 | `GND` | **GND** | `U1.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `CS` | **TFT_CS** (U1.IO20) | `U1.IO20` |
| 4 | `RESET` | **TFT_RST** | `R7.1` |
| 5 | `DC` | **TFT_DC** (U1.IO10) | `U1.IO10` |
| 6 | `SDI` | **MOSI** (U1.IO7) | `U1.IO7` |
| 7 | `SCK` | **SCK** (U1.IO6) | `U1.IO6` |
| 8 | `LED` | **BL** | `R11.2`, `TP5.1` |
| 9 | `SDO` | **NC_MISO** | *nada: se deja al aire* |

### J_TIERRA — DS18B20 sumergible (temperatura de la tierra, opcional)

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `DQ` | **OW** (U1.IO8) | `U1.IO8`, `R6.1` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `VCC` | **3V3** | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AIRE.VCC`, `J_LUZ.VCC`, `J_TOQUE.VCC`, `TP3.1` |

### J_PWR — entrada de energia (celda + cargador, desde afuera)

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `VIN` | **VIN** | `U1.5V`, `R1.1`, `TP2.1` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |

### J_SUELO — capacitivo de suelo v2.0

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `AOUT` | **SUELO** (U1.IO0) | `U1.IO0` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `VCC` | **3V3S** | `Q2.D`, `R5.1`, `C4.1`, `TP4.1` |

### J_AIRE — SHT21 / HTU21 / Si7021 (aire)

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `VCC` | **3V3** | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_LUZ.VCC`, `J_TOQUE.VCC`, `J_TIERRA.VCC`, `TP3.1` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_LUZ.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `SCL` | **SCL** (U1.IO5) | `U1.IO5`, `J_LUZ.SCL` |
| 4 | `SDA` | **SDA** (U1.IO4) | `U1.IO4`, `J_LUZ.SDA` |

### J_LUZ — BH1750 GY-302 (luz)

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `VCC` | **3V3** | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AIRE.VCC`, `J_TOQUE.VCC`, `J_TIERRA.VCC`, `TP3.1` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_TOQUE.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `SCL` | **SCL** (U1.IO5) | `U1.IO5`, `J_AIRE.SCL` |
| 4 | `SDA` | **SDA** (U1.IO4) | `U1.IO4`, `J_AIRE.SDA` |
| 5 | `ADDR` | **NC_ADDR** | *nada: se deja al aire* |

### J_TOQUE — TTP223 (toque)

| # | Pin del módulo | Red | Queda unido a |
|---:|---|---|---|
| 1 | `IO` | **TOQUE** (U1.IO3) | `U1.IO3` |
| 2 | `GND` | **GND** | `U1.GND`, `J_TFT.GND`, `J_AIRE.GND`, `J_LUZ.GND`, `J_TIERRA.GND`, `J_SUELO.GND`, `J_PWR.GND`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R5.2`, `C4.2`, `R8.2`, `Q3.S`, `TP1.1` |
| 3 | `VCC` | **3V3** | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AIRE.VCC`, `J_LUZ.VCC`, `J_TIERRA.VCC`, `TP3.1` |

## Los puentes de cable

Una sola cara de cobre: donde dos redes tendrían que cruzarse, una
pasa por arriba con un cable aislado. Son estos, y no hay más. Se
sueldan **después** de la cinta y **antes** de los módulos.

Los que dicen **rodeando** no van de punta a punta: el camino
derecho les cruzaría la muesca de la antena o un tornillo. La
plantilla los dibuja por donde van.

| # | Red | De | A | Cable | Largo aprox. | |
|---:|---|---|---|---|---:|---|
| 1 | RIEL | `R1.2` | `C1.2` | AWG30 | 12 mm |  |
| 2 | RIEL | `C1.2` | `R2.2` | AWG30 | 12 mm |  |
| 3 | TOQUE | `U1.IO3` | `J_TOQUE.IO` | AWG30 | 44 mm |  |
| 4 | SDA | `U1.IO4` | `J_AIRE.SDA` | AWG30 | 38 mm |  |
| 5 | SCL | `U1.IO5` | `J_AIRE.SCL` | AWG30 | 52 mm |  |
| 6 | SCL | `J_AIRE.SCL` | `J_LUZ.SCL` | AWG30 | 18 mm |  |
| 7 | OW | `U1.IO8` | `J_TIERRA.DQ` | AWG30 | 37 mm |  |
| 8 | 3V3S | `TP4.1` | `J_SUELO.VCC` | AWG30 | 14 mm |  |
| 9 | 3V3S | `R5.1` | `C4.1` | AWG30 | 14 mm |  |
| 10 | VIN | `J_PWR.VIN` | `R1.1` | AWG30 | 31 mm |  |
| 11 | 3V3 | `U1.3V3` | `R4.2` | AWG30 | 23 mm |  |
| 12 | 3V3 | `Q2.S` | `J_TIERRA.VCC` | AWG30 | 24 mm |  |
| 13 | 3V3 | `TP3.1` | `J_TFT.VCC` | AWG30 | 16 mm |  |
| 14 | 3V3 | `R9.2` | `Q4.S` | AWG30 | 15 mm |  |
| 15 | GND | `U1.GND` | `J_PWR.GND` | AWG30 | 21 mm |  |
| 16 | GND | `J_PWR.GND` | `R5.2` | AWG30 | 26 mm |  |
| 17 | GND | `R5.2` | `C4.2` | AWG30 | 14 mm |  |
| 18 | GND | `C4.2` | `C1.1` | AWG30 | 11 mm |  |
| 19 | GND | `R2.1` | `J_TIERRA.GND` | AWG30 | 12 mm |  |
| 20 | GND | `J_TIERRA.GND` | `J_SUELO.GND` | AWG30 | 16 mm |  |
| 21 | GND | `J_TIERRA.GND` | `J_AIRE.GND` | AWG30 | 17 mm |  |
| 22 | GND | `J_AIRE.GND` | `C3.1` | AWG30 | 14 mm |  |
| 23 | GND | `J_AIRE.GND` | `J_TOQUE.GND` | AWG30 | 17 mm |  |
| 24 | GND | `C3.1` | `C2.1` | AWG30 | 18 mm |  |
| 25 | GND | `TP1.1` | `J_TFT.GND` | AWG30 | 23 mm |  |

## Los puntos de prueba

| Punto | Red | Para qué |
|---|---|---|
| **TP5** | BL | la linea de la luz de fondo, a la salida de R11 |
| **TP2** | VIN | VIN: la celda despues del interruptor |
| **TP4** | 3V3S | 3V3S: el riel conmutado de los sensores |
| **TP1** | GND | masa (la punta negra del tester vive aca) |
| **TP3** | 3V3 | 3V3 fijo |

## Los selectores

## Cuánta cinta

| Ancho | Largo total |
|---:|---:|
| 1.0 mm | 521 mm |
| 1.2 mm | 62 mm |
| 1.4 mm | 8 mm |
| 1.9 mm | 20 mm |
| 2.0 mm | 27 mm |
| 2.2 mm | 184 mm |

Son **821 mm de cinta por unidad**, contando los pads. Todo sale
de **un solo rollo de 5 mm**: ninguna canaleta pide más ancho que
ése (el porqué, en [pcb.md](pcb.md), «La cinta manda»). Con un 40 % de
recortes y errores, un rollo de 20 m alcanza para más de cien
unidades.

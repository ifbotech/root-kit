# Diagrama de conexiones

<!-- GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json.
     No se edita a mano: se edita el JSON y se corre `make pcb`. -->

Cada red del núcleo de ROOTKIT v2.0: de qué riel cuelga, qué ancho de
cinta lleva y de dónde a dónde va. El sustrato y la cinta están en
[pcb.md](pcb.md); el paso a paso con los controles de multímetro, en
[armado.md](armado.md).

Las coordenadas de la plantilla se miran **desde la cara de las**
**canaletas**, que es la de atrás del producto: de frente al ROOTKIT,
la X crece hacia la izquierda.

## Los rieles

| Riel | Tensión | Vive | Qué cuelga |
|---|---|---|---|
| **VBAT** | 2,4 – 4,2 V | siempre | el + de la celda y el B+ del TP4056; nada mas |
| **BMIN** | 0 V (protegido) | siempre | el − de la celda y el B− del TP4056. **No es masa**: la proteccion DW01A corta por ahi |
| **V5** | 5 V del USB | enchufado | IN+ del TP4056, el anodo del SS34 y la compuerta del AO3401 de carga compartida |
| **VBATP** | 2,4 – 4,2 V | siempre | OUT+ del TP4056 (la celda ya protegida) y el drenador del AO3401 |
| **VSYS** | 3,0 – 4,65 V | siempre | el nodo de carga compartida: SS34, AO3401 y el interruptor |
| **VINT** | 3,0 – 4,65 V | con el interruptor prendido | el pin 5V de la SuperMini y el divisor del riel |
| **3V3** | 3,3 V | con la placa prendida | AHT20, BH1750, DS18B20, TTP223, la pantalla y las resistencias de arranque |
| **3V3S** | 3,3 V | 400 ms por medicion | el capacitivo de suelo, y el SD del amplificador el dia que haya sonido |

## Las redes

| Red | Clase | Riel | Cinta | Nodos | Por qué |
|---|---|---|---:|---|---|
| **GND** | potencia | GND | 1.2 / 1.9 / 2.4 mm | `U1.GND`, `J_TFT.GND`, `J_AHT.GND`, `J_BH.GND`, `J_TTP.GND`, `J_DS.GND`, `J_SUELO.GND`, `J_TP.IN-`, `J_TP.OUT-`, `R2.1`, `C1.1`, `C2.1`, `C3.1`, `R3.2`, `R5.1`, `C4.1`, `R8.2`, `R10.1`, `Q3.S`, `TP1.1` | la masa del sistema; sale del OUT−/IN− del TP4056, nunca del B− de la celda |
| **BMIN** | potencia | BMIN | sólo cable | `J_CELDA.-`, `J_TP.B-` | el − de la celda va SOLO al B− del modulo: ahi estan los MOSFET de la proteccion. Unirlo a masa deja la celda sin proteccion |
| **VBAT** | potencia | VBAT | sólo cable | `J_CELDA.+`, `J_TP.B+` | el + de la celda al B+ del modulo |
| **V5** | potencia | V5 | 1.2 / 1.9 / 2.4 mm | `J_TP.IN+`, `D1.A`, `Q1.G`, `R3.1` | los 5 V del USB de carga: alimentan el sistema por el Schottky y apagan el MOSFET de carga compartida |
| **VBATP** | potencia | VBATP | sólo cable | `J_TP.OUT+`, `Q1.D`, `TP2.1` | la celda ya protegida por el DW01A |
| **VSYS** | potencia | VSYS | 1.2 mm | `D1.K`, `Q1.S`, `J_SW.1`, `TP3.1` | el nodo de carga compartida: enchufado come del USB, a bateria come de la celda. El grueso de su corriente va por cable (AWG24), no por cinta |
| **VINT** | potencia | VINT | 1.9 / 2.4 mm | `J_SW.2`, `U1.5V`, `R1.2`, `TP4.1` | lo que el interruptor deja pasar al pin 5V de la SuperMini. El divisor cuelga de aca y no de VSYS: apagado no gasta ni un microamperio |
| **3V3** | potencia | 3V3 | 1.2 / 1.9 / 2.4 mm | `U1.3V3`, `C2.2`, `C3.2`, `R4.2`, `Q2.S`, `R6.2`, `R7.2`, `R9.2`, `Q4.S`, `J_TFT.VCC`, `J_AHT.VCC`, `J_BH.VCC`, `J_TTP.VCC`, `J_DS.VCC`, `TP5.1` | el ME6211 de la SuperMini. Fijo: el AHT20, el BH1750, el DS18B20 y el TTP223 juntos suman menos de 5 uA durmiendo |
| **3V3S** | potencia | 3V3S | 1.2 mm | `Q2.D`, `R5.2`, `C4.2`, `J_SUELO.VCC` | el riel que se prende 400 ms por medicion; lo unico que cuelga es el capacitivo, que come 5 mA. Son 5 mA: 1,4 mm de cinta sobran |
| **RIEL** | senal | — | 1.2 mm | `U1.IO1`, `R1.1`, `R2.2`, `C1.2` | divisor 470k/470k con 100 nF: bateria y deteccion de USB con un solo pin (ADC1) |
| **SENS_EN** | senal | — | sólo cable | `U1.IO2`, `Q2.G`, `R4.1` | GPIO2 a la compuerta del P-MOSFET: bajo enciende. Es pin de arranque, por eso la de 100 k lo deja en alto |
| **SUELO** | senal | — | sólo cable | `U1.IO0`, `J_SUELO.AOUT` | al ADC1 por GPIO0, directo desde el borne. Si el capacitivo seco se pasa de 2,5 V hay lugar al lado del borne para un divisor 100 k / 220 k (ver docs/pcb.md) |
| **TOQUE** | senal | — | sólo cable | `U1.IO3`, `J_TTP.IO` | GPIO3, que es de los que despiertan del sueño profundo (solo GPIO0–5) |
| **SDA** | senal | — | 1.2 mm | `U1.IO4`, `J_AHT.SDA`, `J_BH.SDA` | I2C a 100 kHz: AHT20 (0x38) y BH1750 (0x23) |
| **SCL** | senal | — | 1.2 mm | `U1.IO5`, `J_AHT.SCL`, `J_BH.SCL` | idem SDA |
| **SCK** | senal | — | 1.2 mm | `U1.IO6`, `J_TFT.SCL` | SPI a 40 MHz: corta y con la masa al lado |
| **MOSI** | senal | — | sólo cable | `U1.IO7`, `J_TFT.SDA` | idem SCK |
| **OW** | senal | — | sólo cable | `U1.IO8`, `R6.1`, `J_DS.DQ` | 1-Wire con pull-up de 4,7 k al riel FIJO: asi la linea sostiene GPIO8 en alto al arrancar (es pin de arranque) y no alimenta la sonda por la pata de datos |
| **TFT_DC** | senal | — | sólo cable | `U1.IO10`, `J_TFT.DC` | dato/comando de la pantalla |
| **TFT_CS** | senal | — | 1.2 mm | `U1.IO20`, `J_TFT.CS` | seleccion de la pantalla. El dia del sonido se ata a masa y GPIO20 queda libre |
| **TFT_RST** | senal | — | sólo cable | `R7.1`, `J_TFT.RES` | RES a 3V3 con 10 k; el firmware reinicia el panel por software |
| **BL_G** | senal | — | 1.2 mm | `U1.IO21`, `Q3.G`, `R8.1` | GPIO21 a la compuerta del N. Es el TX del UART0: la ROM escribe ahi al arrancar y la luz pestañea ~50 ms (ver docs/pcb.md) |
| **BL_M** | senal | — | 1.2 mm | `Q3.D`, `R9.1`, `Q4.G`, `JP_BL.B` | el drenador del N tira de la compuerta del P |
| **BL_A** | senal | — | sólo cable | `Q4.D`, `JP_BL.A` | la salida del lado alto, antes del selector |
| **BL** | potencia | 3V3 | sólo cable | `JP_BL.C`, `J_TFT.BL`, `R10.2` | hasta 30 mA a 22 kHz. Con R10 el nodo queda definido en bajo cuando la etapa esta apagada. Son 30 mA: 1,2 mm de cinta sobran |
| **NC_BOOT** | sin_conexion | — | sólo cable | `U1.IO9` | GPIO9 es el boton BOOT de la propia SuperMini: no se cablea. El borron de 10 s tambien sale por el toque (main.cpp) |

## Los módulos y los bornes

| Ref | Qué es | Dónde va | Cómo se conecta |
|---|---|---|---|
| **U1** | ESP32-C3 SuperMini | arriba de todo, con la antena mirando al cielo y fuera del plastico | tira de pines macho soldada al modulo; los pines pasan por los agujeros y se sueldan a la cinta del otro lado |
| **J_TFT** | bornes de la pantalla TFT 1,44" | la pantalla va en la ventana, apretada contra el marco de la carcasa | 8 cables finos soldados a los pads del modulo, en el mismo orden del conector |
| **J_AHT** | bornes del AHT20 (aire) | lejos del ESP32 y del TP4056, con ventilacion que no mire hacia arriba | 4 cables: VCC, GND, SCL, SDA |
| **J_BH** | bornes del BH1750 (luz) | mirando una ventana translucida al frente o a un costado | 4 cables; ADDR se deja al aire (direccion 0x23) |
| **J_TTP** | bornes del TTP223 (toque) | pegado a la cara interna de la carcasa donde se acaricia | 3 cables: VCC, GND, I/O |
| **J_DS** | bornes del DS18B20 (tierra) | la sonda va clavada en la tierra, al lado del capacitivo | 3 cables: VDD, GND, DQ. No se usa en modo parasito |
| **J_SUELO** | bornes del capacitivo de suelo | sale por abajo; la junta abajo y el cable haciendo panza | 3 cables: VCC (riel conmutado), GND, AOUT |
| **J_TP** | bornes del modulo TP4056 + DW01A | acostado en el bolsillo de la cara de adelante, con el USB-C mirando hacia abajo | 6 cables cortos: IN+, IN-, B+, B-, OUT+, OUT- |
| **J_CELDA** | bornes del portapilas 18650 | abajo, parada, en su propio compartimento | 2 cables gruesos al portapilas. El − va al B− del TP4056, NO a masa |
| **J_SW** | bornes del interruptor deslizante | en la pared de la carcasa, al lado de la tapa de servicio | 2 cables. Corta todo el aparato; el TP4056 sigue cargando con el aparato apagado |
| **D1** | SS34 (Schottky 3 A, SMA) | en el sustrato | anodo al 5 V del USB, catodo al nodo de sistema |
| **Q1** | AO3401 (P, SOT-23) — carga compartida | en el sustrato | 1=compuerta, 2=fuente (sistema), 3=drenador (celda protegida) |
| **R3** | 100 k — compuerta de Q1 a masa | en el sustrato | sin USB deja conducir al MOSFET; con USB gasta 50 uA, y esta enchufado |
| **Q2** | AO3401 (P) — riel conmutado de sensores | en el sustrato | compuerta a GPIO2: bajo enciende |
| **R4** | 100 k — compuerta a fuente de Q2 | en el sustrato | lo deja apagado mientras el chip duerme o arranca, y sostiene GPIO2 en alto (es pin de arranque) |
| **R5** | 100 k — purga del riel conmutado | en el sustrato | descarga el riel al apagarlo; solo gasta mientras el riel esta prendido |
| **C4** | 100 nF — desacople del riel conmutado | en el sustrato | al lado del borne del capacitivo |
| **Q3** | AO3400 (N) — etapa de la luz de fondo | en el sustrato | GPIO21 a la compuerta; su drenador tira de la compuerta del P |
| **Q4** | AO3401 (P) — llave del lado alto de la luz de fondo | en el sustrato | de 3V3 al pin BL de la pantalla |
| **R8** | 100 k — compuerta de Q3 a masa | en el sustrato | que no prenda sola en el sueño profundo |
| **R9** | 10 k — compuerta de Q4 a 3V3 | en el sustrato | 10 k y no 100 k: con 100 k el PWM de 22 kHz se emborrona (tau de 70 us contra 45 us de periodo) |
| **R10** | 100 k — el nodo BL a masa | en el sustrato | deja el BL definido en bajo con la etapa apagada |
| **JP_BL** | selector del lado de conmutacion de la luz de fondo | en el sustrato | gota de estaño entre C y A (lado alto, por defecto) o entre C y B (lado bajo) |
| **R6** | 4,7 k — pull-up del 1-Wire a 3V3 fijo | en el sustrato | al riel FIJO, no al conmutado: ver docs/pcb.md |
| **R7** | 10 k — RES de la pantalla a 3V3 | en el sustrato | el firmware reinicia el panel por software |
| **R1** | 470 k — rama de arriba del divisor | en el sustrato | del riel conmutado por el interruptor, no del nodo de sistema: asi no gasta nada en la caja |
| **R2** | 470 k — rama de abajo del divisor | en el sustrato | 470 k + 470 k gastan 4 uA |
| **C1** | 100 nF — filtro del divisor | en el sustrato | con 100 nF el ADC lee estable |
| **C2** | 220–470 uF 6,3 V bajo ESR — 3V3 | en el sustrato | los picos de transmision del wifi; acostado, con las patas dobladas sobre los pads |
| **C3** | 100 nF — desacople de 3V3 | en el sustrato | cerca del pin 3V3 de la SuperMini |
| **TP1** | masa (la punta negra del tester vive aca) | — | — |
| **TP2** | celda protegida (OUT+ del TP4056) | — | — |
| **TP3** | nodo de sistema (VSYS) | — | — |
| **TP4** | riel del interruptor (VINT); tambien la toma de VIN del amplificador, el dia del sonido | — | — |
| **TP5** | 3V3 fijo | — | — |

## Los puentes de cable

Una sola cara de cobre: donde dos redes tendrían que cruzarse, una
pasa por arriba con un cable aislado. Son estos, y no hay más. Se
sueldan **después** de la cinta y **antes** de los módulos.

Los que dicen **rodeando** no van de punta a punta: el camino
derecho les cruzaría la ventana de la pantalla y el cable quedaría
apretado entre el módulo y el plástico. La plantilla los dibuja por
donde van.

| # | Red | De | A | Cable | Largo aprox. | |
|---:|---|---|---|---|---:|---|
| 1 | GND | `R2.1` | `C1.1` | AWG30 | 14 mm |  |
| 2 | GND | `U1.GND` | `R8.2` | AWG30 | 27 mm |  |
| 3 | GND | `R10.1` | `J_AHT.GND` | AWG30 | 37 mm |  |
| 4 | GND | `J_AHT.GND` | `R5.1` | AWG30 | 30 mm |  |
| 5 | BMIN | `J_CELDA.-` | `J_TP.B-` | **AWG24** | 10 mm |  |
| 6 | VBAT | `J_CELDA.+` | `J_TP.B+` | **AWG24** | 10 mm |  |
| 7 | V5 | `D1.A` | `Q1.G` | **AWG24** | 19 mm |  |
| 8 | VBATP | `J_TP.OUT+` | `TP2.1` | **AWG24** | 10 mm |  |
| 9 | VBATP | `TP2.1` | `Q1.D` | **AWG24** | 25 mm |  |
| 10 | VSYS | `Q1.S` | `J_SW.1` | **AWG24** | 25 mm |  |
| 11 | VINT | `J_SW.2` | `TP4.1` | AWG30 | 15 mm |  |
| 12 | VINT | `J_SW.2` | `U1.5V` | AWG30 | 75 mm |  |
| 13 | 3V3 | `U1.3V3` | `R7.2` | AWG30 | 15 mm |  |
| 14 | 3V3 | `R6.2` | `C3.2` | AWG30 | 13 mm |  |
| 15 | 3V3 | `C2.2` | `J_TFT.VCC` | AWG30 | 28 mm |  |
| 16 | 3V3 | `C2.2` | `J_TTP.VCC` | AWG30 | 39 mm |  |
| 17 | 3V3 | `U1.3V3` | `R9.2` | AWG30 | 31 mm |  |
| 18 | 3V3 | `R9.2` | `Q4.S` | AWG30 | 15 mm |  |
| 19 | 3V3 | `J_BH.VCC` | `R4.2` | AWG30 | 27 mm |  |
| 20 | 3V3 | `R4.2` | `Q2.S` | AWG30 | 13 mm |  |
| 21 | RIEL | `U1.IO1` | `R2.2` | AWG30 | 18 mm |  |
| 22 | RIEL | `R1.1` | `C1.2` | AWG30 | 12 mm |  |
| 23 | SENS_EN | `U1.IO2` | `R4.1` | AWG30 | 85 mm | **rodeando** la ventana (ver la plantilla) |
| 24 | SENS_EN | `R4.1` | `Q2.G` | AWG30 | 13 mm |  |
| 25 | SUELO | `U1.IO0` | `J_SUELO.AOUT` | AWG30 | 66 mm | **rodeando** la ventana (ver la plantilla) |
| 26 | TOQUE | `U1.IO3` | `J_TTP.IO` | AWG30 | 53 mm |  |
| 27 | SDA | `U1.IO4` | `J_BH.SDA` | AWG30 | 86 mm | **rodeando** la ventana (ver la plantilla) |
| 28 | SCL | `U1.IO5` | `J_BH.SCL` | AWG30 | 64 mm |  |
| 29 | MOSI | `U1.IO7` | `J_TFT.SDA` | AWG30 | 30 mm |  |
| 30 | OW | `U1.IO8` | `R6.1` | AWG30 | 35 mm |  |
| 31 | OW | `R6.1` | `J_DS.DQ` | AWG30 | 54 mm |  |
| 32 | TFT_DC | `U1.IO10` | `J_TFT.DC` | AWG30 | 23 mm |  |
| 33 | TFT_RST | `R7.1` | `J_TFT.RES` | AWG30 | 30 mm |  |
| 34 | BL_G | `U1.IO21` | `R8.1` | AWG30 | 23 mm |  |
| 35 | BL_M | `Q3.D` | `JP_BL.B` | AWG30 | 17 mm |  |
| 36 | BL_A | `Q4.D` | `JP_BL.A` | AWG30 | 21 mm |  |
| 37 | BL | `JP_BL.C` | `R10.2` | AWG30 | 14 mm |  |
| 38 | BL | `JP_BL.C` | `J_TFT.BL` | AWG30 | 33 mm |  |

## Los puntos de prueba

| Punto | Red | Para qué |
|---|---|---|
| **TP1** | GND | masa (la punta negra del tester vive aca) |
| **TP2** | VBATP | celda protegida (OUT+ del TP4056) |
| **TP3** | VSYS | nodo de sistema (VSYS) |
| **TP4** | VINT | riel del interruptor (VINT); tambien la toma de VIN del amplificador, el dia del sonido |
| **TP5** | 3V3 | 3V3 fijo |

## Los selectores

**JP_BL** — de que lado se conmuta la luz de fondo, segun como sea el pin BL del modulo que llegue

- *C–A, lado alto* **(por defecto)**: Q4 (P) lleva 3V3 al pin BL. Sirve si BL es el anodo del LED con su resistencia, y tambien si BL es la entrada de control de un transistor del modulo
- *C–B, lado bajo*: Q3 (N) tira el pin BL a masa. Solo si BL resulta ser el catodo del LED. En ese caso NO se pueblan Q4, R9 ni R10

## Cuánta cinta

| Ancho | Largo total |
|---:|---:|
| 1.2 mm | 225 mm |
| 1.9 mm | 28 mm |
| 2.0 mm | 34 mm |
| 2.2 mm | 6 mm |
| 2.4 mm | 178 mm |
| 2.6 mm | 85 mm |
| 2.8 mm | 49 mm |
| 3.0 mm | 15 mm |
| 3.4 mm | 7 mm |

Son **626 mm de cinta por unidad**, contando los pads. Con un 40 % de
recortes y errores, un rollo de 6 mm y otro de 20 mm alcanzan para
más de diez unidades.

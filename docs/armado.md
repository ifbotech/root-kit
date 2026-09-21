# Armar un ROOTKIT

Paso a paso para armar una unidad desde cero, con el control de multímetro de
cada etapa y el primer encendido hecho de forma que no se pueda romper nada.
Está escrito para que alguien que no participó del diseño —o Iñaki dentro de
seis meses— arme una sin preguntar.

Antes de empezar conviene mirar [pcb.md](pcb.md) (qué es el sustrato y por
qué) y tener abierto [conexiones.md](conexiones.md) (qué va con qué). La
plantilla 1:1 está en
[`hardware/pcb/generado/plantilla-cinta.svg`](../hardware/pcb/generado/plantilla-cinta.svg).

**Tiempo, la primera vez: unas tres horas.** Después de la tercera, hora y
media.

---

## Las tres reglas

1. **El USB de la SuperMini se enchufa con el interruptor apagado.** Está
   grabado en el sustrato. El porqué está en [pcb.md](pcb.md), "Los dos USB y
   la batería": con el interruptor prendido y la celda puesta, esos 5 V
   pueden llegar a la celda sin control de carga.
2. **La celda entra último.** Todo lo que se pueda probar sin ella, se prueba
   sin ella: con una fuente de laboratorio limitada a 100 mA.
3. **Ningún paso se saltea porque "eso seguro está bien".** Cada control de
   esta guía existe porque el error que detecta es invisible hasta que ya
   quemó algo.

---

## Lo que hace falta

**De la [lista de compras](hardware.md#lista-de-compras-del-prototipo)**, más:

| Herramienta | Nota |
|---|---|
| Soldador con punta fina, 260 °C | más caliente marca el PETG |
| Estaño **Sn42Bi58** (138 °C) y flux | el común de 183 °C también va, con toques cortos |
| Lija al agua de 400 y 600 y un taco plano | **es la herramienta que separa las pistas**: ver el paso 2 |
| Bisturí o trincheta con hoja nueva | sólo para recortar la hoja de cinta al contorno |
| **La estampadora impresa** | mete toda la cinta de una prensada (paso 2) |
| Bruñidor o el mango de una cuchara | para presionar la cinta en la canaleta |
| Multímetro con continuidad y prueba de diodo | |
| **Fuente de laboratorio con límite de corriente** | no es opcional: es lo que reemplaza a la celda hasta el paso 11 |
| Pinza de punta fina | para abrir las patas de los SOT-23 |
| Alcohol isopropílico y cepillo | el flux que queda es una fuga |

| Cable de silicona AWG30 | los 24 puentes |
| Tiras de pines macho de 2,54 mm | dos de 8 para el ESP32 y una por módulo |

---

## Paso 0 — Imprimir

```
make pcb                       # regenera el STL y la plantilla
```

Se imprimen **dos piezas**:

| Pieza | STL | Cómo |
|---|---|---|
| El sustrato | `generado/nucleo-sustrato.stl` | **PETG**, canaletas **hacia arriba**, sin soportes, boquilla **0,6**, capa 0,2, tres perímetros. 68 × 92 × 3 mm |
| La estampadora | `generado/nucleo-estampadora.stl` | PETG o PLA, nervaduras **hacia arriba**, sin soportes, misma boquilla y capa. 73 × 97 × 8,5 mm |

> **Antes del sustrato entero, el cupón.** Media hora de impresora que
> evita tirar cinco placas: un cuadrado de 40 mm con canaletas de 1,0 /
> 1,4 / 2,2 mm a tres profundidades, y su estampadora con tres holguras
> laterales. Ahí se contesta hasta dónde se puede hundir la canaleta sin
> que la cinta se rompa en el piso, con cuánta luz entra la nervadura sin
> agarrar, y cuánta lija hace falta. Ver [pcb.md](pcb.md).

La estampadora es el negativo del sustrato y sirve para meter toda la cinta
de una sola prensada ([pcb.md](pcb.md)). Se imprime una sola vez y sirve para
todas las unidades.

Y se imprime la plantilla **al 100 %, sin ajustar a la página**.

> **La cara de abajo no lleva soportes ni los necesita.** El sustrato apoya
> en la cama por una cara **completamente plana**: no hay un solo voladizo.
> Si el laminador propone soportes, algo está mal —probablemente el STL es
> viejo—, no se los pongas.

**Control 0.b — la estampadora entra.** Antes de tocar la cinta: apoyar la
estampadora sobre el sustrato vacío. El faldón tiene que envolver el borde y
la pieza bajar hasta el fondo **sin resistencia**, y las nervaduras entrar en
sus canaletas. Si hace tope antes, está al revés (mirala de nuevo: va dada
vuelta) o las nervaduras salieron gordas y hay que bajar el flujo. **Nunca
forzar**: las más finas miden 1,0 mm y se parten.

**Control 0.** Con un calibre, la regla de la plantilla impresa tiene que
medir **50,0 mm**. Si mide otra cosa, la impresora de papel la escaló y todo
lo que se corte con ella va a estar mal. Y en la pieza impresa: el ancho total
son **62,0 mm**; si la pieza mide 61,4, el filamento se contrajo y las
canaletas también — se reimprime con compensación antes de seguir.

**Control 0.c — los agujeros.** Con la punta de un calibre o un pin de tira
macho: los agujeros de los bornes son de **1,8 mm** y los de la SuperMini de
**1,4 mm**. Un pin de 0,64 mm tiene que entrar **suelto** en los dos. Si
entra a presión o no entra, la boquilla está sacando de menos: subí la
compensación de agujeros del laminador (*hole compensation* / *XY size
compensation*) y reimprimí. Es el defecto más común con boquilla de 0,6 y
arruina el paso 3 entero si se descubre tarde.

---

## Paso 1 — Medir los módulos (antes de soldar nada)

Este paso existe porque el diseño se hizo sin los módulos en la mano. Son
cuatro mediciones y cambian números del JSON, no el diseño.

**1.a — La huella de la SuperMini.** Apoyar el módulo sobre la plantilla
impresa, encima del rectángulo punteado de `U1`. Los dieciséis pines tienen
que caer sobre sus pads y los nombres tienen que coincidir uno por uno.

> Si no coinciden: corregir `huellas.esp32c3.pines` (el orden) o `pads` (las
> posiciones) en `hardware/pcb/nucleo.json`, correr `make pcb`, y volver al
> paso 0. Es un array; no hay que rediseñar nada.

**1.b — El pin 5V contra el VBUS.** Multímetro en continuidad, una punta en
el pin `5V` del header y la otra en la patita de VBUS del conector USB-C del
módulo.

- **Pita** → la placa une VBUS con 5V. La regla 1 rige siempre. Anotarlo.
- **No pita** → la placa trae diodo. La regla 1 pasa a ser prolijidad.

**1.c — El pin LED de la pantalla.** Multímetro en prueba de diodo, con el
módulo **desconectado**: punta roja en `LED`, negra en `GND`. Si marca
~2,8–3,2 V, `LED` es el ánodo de los LED de la luz de fondo, que es lo que
el sustrato asume (lo conmuta un P-MOSFET del lado alto, Q4).

Después, con el tester en resistencia entre `LED` y `VCC` del módulo:

- **Unos cientos de ohm** → el módulo ya trae su resistencia en serie:
  poblar `R11` con **0 Ω**.
- **Abierto o casi cero** → no la trae: poblar `R11` con **47–100 Ω**.
  Empezar por 100 y bajar si la luz queda floja.

**1.d — El orden de los pines de cada módulo.** Es la medición que más
importa de todas, porque los clones lo cambian entre lotes. Confirmar la
serigrafía contra la tabla del paso 5, módulo por módulo. Si alguno no
coincide, corregir su lista `pines` en el JSON y volver al paso 0.

**1.e — El chip del capacitivo.** Mirar la serigrafía: tiene que decir
**TLC555**. Si dice NE555, el sensor no arranca a 3,3 V y da lecturas planas:
se devuelve.

---

## Paso 2 — La cinta: cubrir, prensar, lijar

Se trabaja sobre la **cara de las canaletas**, con la plantilla al lado en la
misma orientación (el texto grabado se lee derecho).

El orden importa y no es el que parece. **No se corta cinta canaleta por
canaleta**: se cubre la placa entera, se prensa todo de una vez, y recién
entonces se separa lo que sobra **lijando**, no cortando. La cinta que queda
1,2 mm por debajo de la cara, adentro de los canales, la lija no la toca.

1. **Cubrir la cara entera de cinta.** Tiras del rollo de 5 mm, una al lado
   de la otra, solapadas un milímetro, hasta que no quede ni un claro sobre
   ninguna canaleta. No hay que apuntar a nada: lo que sobra se va a ir.
2. **Recortar al contorno** pasando el bisturí alrededor del canto del
   sustrato, que hace de guía. El faldón de la estampadora necesita el borde
   limpio para cerrar.
3. **Bajar la estampadora.** El faldón envuelve el borde y la centra sola. No
   hay que apuntar.
4. **Apretar parejo**, con las dos manos o —mejor— con una tabla y el peso
   del cuerpo, quince segundos. Presión repartida, no un punto. Las
   nervaduras hunden la cinta al fondo de cada canaleta y la **marcan**
   contra los dos bordes.
5. **Levantar y lijar la cara**, al ras, con lija de 400 sobre un taco plano
   y movimientos largos en las dos diagonales. El cobre que está apoyado
   sobre las paredes entre canaletas se va; el que está adentro de los
   canales, no. Terminar con 600 hasta que la cara se vea de PETG parejo, sin
   islas de cobre entre canaletas.
6. **Soplar y limpiar con alcohol.** El polvo de cobre de la lija es
   conductor: si queda en el fondo de una canaleta no molesta, pero si queda
   sobre una pared es exactamente el puente que se acaba de lijar.
7. **Cada esquina y cada empalme lleva una gota de estaño.** El adhesivo no
   es una conexión. Toque corto: apoyar, estañar, retirar, contar hasta tres
   antes del siguiente.

> **Por qué lijar y no cortar.** Hasta la v2.0 había que pasar el bisturí por
> el canto de cada una de las 76 canaletas. Con canaletas de 1,2 mm de
> profundidad y una nervadura que entra 1,0 mm más allá de la cara, la cinta
> queda tan estirada sobre el borde que se rompe sola con la lija. Es una
> operación de treinta segundos en vez de media tarde, y sobre todo: no
> depende del pulso.

> **Lo que la estampadora NO hace es cortar.** Una matriz de corte para
> 0,06 mm de cobre trabaja con unas micras de luz entre punzón y matriz; una
> boquilla de 0,6 mm da 0,15 mm. La nervadura marca; la lija corta.

**Control 2.a — aislación entre canaletas vecinas.** Éste va **primero**,
antes que el de continuidad, porque es el que dice si faltó lija. Tester en
continuidad, recorriendo la plantilla: dos canaletas vecinas de redes
distintas tienen que dar **abierto**. Si alguna pita, volver a lijar ese
tramo.

**Control 2.b — continuidad de cada red.** Para cada red de
[conexiones.md](conexiones.md), tocar el nodo más lejano contra el más
cercano de la lista: **tiene que pitar**. Las de masa se prueban contra
`TP1`, que es el punto negro del tester de acá en adelante.

**Control 2.c — aislación entre rieles.** Con el tester en resistencia, y
**nada más soldado**:

| Entre | Tiene que dar |
|---|---|
| `TP1` (GND) y `TP3` (3V3) | abierto |
| `TP1` y `TP2` (VIN) | abierto |
| `TP1` y `TP4` (3V3S) | abierto |
| `TP2` y `TP3` | abierto (todavía no está la SuperMini) |
| `TP3` y `TP4` | abierto (todavía no está Q2) |

Si alguno da un valor bajo, hay una rebaba de cinta o un puente de estaño.
Se busca con lupa antes de seguir: después va a estar tapado por un módulo.

---

## Paso 3 — Los puentes de cable

Los 24 de la tabla de [conexiones.md](conexiones.md), en ese orden, con cable
de silicona **AWG30**.

Van **por arriba** de la cinta, cruzando lo que tengan que cruzar, pegados al
sustrato con una gota de cianoacrilato cada 15 mm para que no bailen.

Catorce de los 24 son de masa y de 3V3, y eso es a propósito: en una sola
cara de cobre las dos redes que tocan todo no se pueden cerrar sin cruces, y
si alguien tiene que llevar un cable por arriba, que sea la masa —no tiene
señal que degradar y, si hiciera falta, se refuerza con otro en paralelo—.
Ninguna señal lleva puente salvo dos, `SCL` y `OW`, y las dos están
explicadas en [pcb.md](pcb.md).

> **Los que dicen "rodeando" no van derecho.** El camino corto les cruzaría
> la muesca de la antena o un agujero de tornillo, y ahí el cable quedaría
> apretado. La plantilla los dibuja por donde van: seguila.

**Control 3.** Tachar cada puente de la lista al soldarlo y volver a hacer el
**control 2.a completo**. Es el momento más fácil para olvidarse uno, y el
más barato para encontrarlo.

---

## Paso 4 — Los componentes chicos

En este orden: resistencias y capacitores (1206) y después los MOSFET. Son
once piezas y todas viven en el sustrato; el Schottky y el MOSFET de carga
compartida ya no están acá, se arman en el arnés del paso 6.

- **1206**: flux, estañar un pad, apoyar con pinza, soldar el otro lado,
  rehacer el primero.
- **SOT-23**: abrir las dos patas del mismo lado 0,2 mm con la pinza hasta
  que caigan en sus pads (están a 2,3 mm, no a 1,9). Soldar primero la pata
  sola del otro lado.
- **C2** (el electrolítico de 220–470 µF) va **parado**, con cada pata en su
  agujero. **Ojo con la polaridad**: la franja blanca va del lado de masa,
  que es `C2.1` en la plantilla.

**Lo que NO se puebla** salvo que el paso 1 diga lo contrario: nada. El
valor de `R11` sí sale del paso 1.c.

**Control 4 — los valores, otra vez.** Medir en la placa, con el tester en
resistencia:

| Entre | Esperado |
|---|---|
| `TP2` (VIN) y `TP1` (GND) | **940 kΩ ±10 %** (el divisor: 470 k + 470 k) |
| `TP3` (3V3) y `TP1` | **> 100 kΩ** (si da menos, hay un corto o C2 al revés) |
| `TP1` y el pad `G` de Q3 | **100 kΩ** (R8) |
| `TP3` y el pad `G` de Q2 | **100 kΩ** (R4) |

---

## Paso 5 — Clavar los módulos

Desde el sustrato v3.0 no hay bornes ni cables: **cada módulo se clava**. Se
le suelda su tira de pines macho, los pines pasan por los agujeros del
sustrato, y se sueldan **del lado de las canaletas** contra la cinta.

| Tira | Módulo | Pines |
|---|---|---|
| `J_TFT` | TFT 2,2" ILI9341 | 9: VCC, GND, CS, RESET, DC, SDI, SCK, LED, SDO |
| `J_AIRE` | SHT21 / HTU21 / Si7021 | 4: VCC, GND, SCL, SDA |
| `J_LUZ` | BH1750 GY-302 | 5: VCC, GND, SCL, SDA, ADDR (al aire) |
| `J_SUELO` | capacitivo de suelo | 3: AOUT, GND, VCC |
| `J_TOQUE` | TTP223 | 3: IO, GND, VCC |
| `J_TIERRA` | DS18B20 | 3: DQ amarillo, GND negro, VCC rojo. **No** en modo parásito |
| `J_PWR` | arnés de la batería | 2: VIN, GND |

El mapa completo —el tamaño exacto de cada huella, y pin por pin de qué red
es y contra qué pin del ESP32 queda— está en
[conexiones.md](conexiones.md), sección "El mapa de montaje". Se genera del
mismo dato que la placa, así que no puede contradecirla.

> **Mirar la serigrafía del módulo ANTES de clavarlo.** Es el único paso de
> esta guía que puede salir mal de una manera que no se arregla: los clones
> cambian el orden de los pines entre lotes. El sustrato está hecho para los
> órdenes de la tabla de arriba. Si alguno no coincide, **no se improvisa con
> cables**: se corrige la lista `pines` de ese componente en
> `hardware/pcb/nucleo.json`, se corre `make rutear && make pcb`, y la placa
> se reordena sola. Son diez minutos y una impresión.

> **Cuidado con `J_TFT`.** Los pines están rotulados con el nombre **del
> módulo**, no con el de la red: `SDI` del módulo es el MOSI del ESP32 y
> `SCK` es su SCK. Y `SDO` (el MISO) **queda al aire a propósito**: el
> firmware nunca lee del panel.

> **El capacitivo tiene GND en el medio a propósito.** Si se clava al revés,
> lo que se cruza son VCC y la salida analógica —que el ADC aguanta— y no la
> alimentación del módulo.

**Control 5.** Con el tester en continuidad y **antes** de soldar, apoyar
cada módulo en su tira sin presionar y comprobar tres pines de cada uno
contra el `TP` de su riel: `VCC` contra `TP3` (3V3), `GND` contra `TP1`. El
`VCC` del capacitivo va contra `TP4` (3V3S), no contra `TP3`.

---

## Paso 6 — El arnés de la batería

Desde la v3.0 **la etapa de carga no vive en el sustrato**. Es un arnés
aparte, que se arma sobre el propio módulo TP4056 y entra a la placa por los
dos pines de `J_PWR`. El esquema completo está en
[hardware.md](hardware.md), "Batería y carga"; en resumen:

```
   USB-C ─► TP4056+DW01A ─► OUT+ ─┬─ AO3401 (carga compartida) ─┐
              B+/B− a la celda    │                              ├─ interruptor ─► J_PWR.VIN
   5 V del USB ─► SS34 ───────────┘                              │
   OUT− ──────────────────────────────────────────────────────────► J_PWR.GND
```

> **El negativo de la celda va al `B−` del TP4056**, no a masa. Si se une a
> masa se puentean los MOSFET del DW01A y la celda queda **sin protección de
> sobredescarga ni de cortocircuito**. Es el error más caro de esta guía, y
> ahora ocurre fuera de la placa, donde no hay una canaleta que lo impida:
> mirarlo dos veces.

**Control 6 — el arnés, solo, sin la placa.**

1. Interruptor **apagado**, `J_PWR` sin conectar a la placa.
2. Fuente de laboratorio a **3,7 V, límite 200 mA**, en lugar de la celda
   (rojo al `B+`, negro al `B−`).
3. Enchufar el USB del cargador. El LED rojo del TP4056 se enciende.
4. Medir el `OUT+` contra el `OUT−`: **3,7 V**, lo que pusiste.
5. Medir el nodo de sistema (después del SS34 y el AO3401) contra `OUT−`:
   **4,5–4,7 V**. Son los 5 V del USB menos el Schottky. **Si da 3,7 V, el
   SS34 está al revés o el AO3401 conduce cuando no debe.**
6. Medir la salida del interruptor: **0 V**, porque está apagado.
7. Desenchufar el USB. El nodo de sistema tiene que pasar a **3,7 V**: ahora
   come de la "celda" por el AO3401. **Si queda en 0, el AO3401 no conduce**:
   mirá su orientación (la pata sola es el drenador y va del lado de la
   celda).

Este control prueba la carga compartida entera sin arriesgar la placa.

**Control 6.b — el arnés contra la placa.** Con el arnés todavía sin celda y
el interruptor apagado, conectar `J_PWR` y medir `TP2` (VIN) contra `TP1`:
**0 V**. Prender el interruptor: `TP2` pasa a los 3,7 V de la fuente.

---

## Paso 7 — La SuperMini

Soldar una tira de pines macho al módulo (pines hacia el lado de los
componentes), pasarlos por los agujeros del sustrato y soldarlos del lado de
la cinta. El módulo queda del lado plano, con la **antena mirando al borde de
arriba** y asomando por la muesca.

> **Acá los pads no rodean el agujero: están al lado.** A 2,54 mm de paso, con
> boquilla de 0,6, un anillo completo de cobre no entra ([pcb.md](pcb.md)).
> Así que el pad de cada pin está **pegado al borde de su agujero**, y van
> alternados: uno para afuera, el siguiente para adentro. El agujero en sí
> queda pelado.
>
> Se suelda así: el pin asoma por el agujero, se le arrima la punta del
> soldador **del lado donde está su cinta** —mirá la plantilla si dudás— y se
> deja correr una gota que moje el pin y la cinta a la vez. No hay que doblar
> el pin. Si la gota se va para el lado que no es, no toca nada: no hay cobre
> ahí, y se saca con malla.
>
> **Control: ningún pin unido a su vecino.** Tester en continuidad entre
> pines contiguos de la misma fila. Con los pads alternados un puente de
> estaño es menos probable que antes, pero si aparece, aparece acá.

**Antes de soldar**, con la placa suelta:

- **Desoldar el LED rojo de encendido** si lo tiene, o cortarle la pista con
  el bisturí. Son 1–3 mA permanentes: a batería es la diferencia entre seis
  meses y tres semanas.
- Anotar en el cuaderno si esa placa tiene LED azul en GPIO8; va a parpadear
  cada vez que se lea la sonda de tierra. Es cosmético.

**Control 7 — ningún corto antes de alimentar.** Tester en resistencia:

| Entre | Esperado |
|---|---|
| Pin `3V3` y pin `GND` de la SuperMini | **> 10 kΩ** |
| Pin `5V` y pin `GND` | **> 100 kΩ** |
| Pin `3V3` y pin `5V` | **abierto** |

Si alguno da bajo, hay una pata puenteada. **No alimentar.**

---

## Paso 8 — El primer encendido, con fuente

Todavía sin celda y sin sensores.

1. Interruptor **apagado**. Sensores **desconectados**.
2. Fuente a **3,8 V, límite 100 mA**, en lugar de la celda del arnés.
3. **Prender el interruptor.**
4. **Mirar el amperímetro de la fuente en ese instante.** Tiene que subir a
   **20–60 mA** y quedarse ahí. Si salta al límite de 100 mA, **apagar** y
   buscar el corto: la fuente acaba de salvar la placa.
5. Medir `TP3` (3V3) contra `TP1`: **3,25–3,35 V**.
6. Medir `TP2` (VIN) contra `TP1`: **3,8 V**, lo de la fuente.

Recién ahora el aparato está vivo.

---

## Paso 9 — El consumo en reposo

El número que decide si el producto dura seis meses o tres semanas.

1. Con el firmware ya cargado (paso 10) el aparato se duerme solo a los
   pocos segundos. En el primer armado, este control se hace **después** del
   paso 10 y se vuelve acá.
2. Fuente a **3,8 V**, en serie con un multímetro en **µA**, o con un PPK2.
3. Esperar a que la pantalla se apague y el aparato entre en sueño profundo.
4. **Esperado: 50–90 µA.** El presupuesto de [pcb.md](pcb.md) da 68.

| Si medís | Mirá |
|---|---|
| **> 200 µA** | flux sin limpiar entre pistas, el LED de encendido todavía puesto, o un pull-up de I2C duplicado |
| **> 1 mA** | el riel de sensores quedó prendido: Q2 conduce. Revisar R4 y el estado de GPIO2 |
| **> 5 mA** | la pantalla no se durmió, o el TP4056 tiene el USB enchufado |

**Limpiar el flux y volver a medir** es parte del control, no un extra.

---

## Paso 10 — Flashear y fábrica

> **Interruptor APAGADO.** Está grabado al lado del módulo. Ver la regla 1.

1. Apagar el interruptor. Desconectar la fuente.
2. Enchufar el USB **de la SuperMini** a la computadora.
3. Compilar y cargar:
   ```
   cd firmware && pio run -e c3-144 -t upload
   ```
   Si no entra en modo descarga: mantener el botón **BOOT** de la SuperMini
   apretado mientras se enchufa.
4. Registrar la placa en la estación de fábrica ([fabrica.md](fabrica.md)):
   ```
   python3 tools/fabrica.py --puerto COM7 --persona brote
   ```
   Esto le graba el secreto y el Rooti que le toca. **La nube de producción
   no acepta placas sin registrar.**
5. Desenchufar el USB de la SuperMini.

**Control 10.** En el monitor serie (`pio device monitor`), al arrancar tiene
que aparecer el banner con la versión y `[almacen]` sin errores.

> **Lo que se ve al encender, y es normal:** la luz de fondo puede
> **pestañear unos 50 ms**. GPIO21 es el TX del UART0 y la ROM del C3 escribe
> ahí su log de arranque antes de que corra el firmware. No es una falla. Si
> molesta de noche —el aparato despierta cada 2 a 30 minutos—, hay dos
> salidas, ninguna urgente: silenciar el log de ROM con el eFuse
> `UART_PRINT_CONTROL` desde la estación de fábrica (verificar sus opciones
> en el ESP-IDF antes de quemarlo: los eFuses no se deshacen), o intercambiar
> BL y CS en `placa.h`, el JSON y `hardware.md`, poniendo la luz en GPIO20,
> que es el RX y la ROM no maneja. Lo segundo hay que medirlo antes: si el RX
> tiene pull-up interno al arrancar, es peor el remedio.

---

## Paso 11 — La celda

**Recién ahora.**

1. Interruptor **apagado**.
2. Confirmar otra vez que el negativo de la celda, en el arnés, da abierto
   contra `TP1`.
3. Medir la celda con el tester: **3,2–4,1 V**. Si vino abajo de 2,5 V,
   descartarla.
4. Poner la celda en el portapilas mirando la polaridad del portapilas, que
   está marcada en el plástico.
5. Medir `TP2` (celda protegida) contra `TP1`: tiene que dar **lo mismo que
   la celda**. Si da 0 V, el DW01A está en corte: enchufar el USB del
   cargador un segundo para que "despierte".
6. Prender el interruptor. `TP5` = 3,3 V, y el aparato arranca.

---

## Paso 12 — Los sensores, uno por uno

Con el aparato andando y el monitor serie abierto, conectar **de a uno** y
mirar la lectura. Así, cuando algo falla, se sabe qué.

| Orden | Sensor | Qué tiene que pasar |
|---:|---|---|
| 1 | SHT21 (o AHT20) | temperatura y humedad creíbles; comparar contra un termohigrómetro. **Si no aparece en el log, es el driver**: el firmware habla con uno o con el otro según `-DRK_AIRE_SHT21` (ver `platformio.ini`) |
| 2 | BH1750 | tapar con la mano: los lux caen a menos de 10 |
| 3 | TTP223 | tocar: `[boton]` en el log, la pantalla se enciende |
| 4 | DS18B20 | temperatura cerca de la del aire; con los dedos en la sonda sube |
| 5 | Capacitivo | en el aire, un número; en un vaso de agua hasta la línea, otro **bien distinto** |
| 6 | Pantalla | el QR se lee desde un teléfono a 30 cm |

**Control 12 — el capacitivo contra el ADC.** Con el capacitivo seco y el
riel de sensores prendido (durante una medición), medir su `AOUT` contra
masa. **Tiene que dar menos de 2,5 V.** Por encima de eso el ADC del C3 a
11 dB satura y el sensor pierde la mitad de su rango.

> Si da más: soldar un divisor **100 k / 220 k** en los propios pines de
> `J_SUELO` (100 k en serie desde `AOUT`, 220 k de ahí a `GND`), y poner
> `RK_SUELO_DIVISOR_NUM` = 220000 y `RK_SUELO_DIVISOR_DEN` = 320000 en
> `esp32.promesas` del JSON para que quede escrito lo que hace la placa.

**Control 12.b — el riel conmutado.** Tester en `J_SUELO.VCC` contra `TP1`:
**0 V entre mediciones**, 3,3 V durante los ~400 ms que dura una. Si marca
3,3 V todo el tiempo, Q2 quedó conduciendo.

---

## Paso 13 — Cerrar

1. **Limpiar el flux** con alcohol isopropílico y cepillo, de los dos lados.
   Secar bien.
2. **Rehacer el paso 9** (consumo en reposo). El flux limpio se nota.
3. **Barnizar**: tapar los seis puntos de prueba con cinta de papel, dos
   manos de barniz acrílico en aerosol sobre la cara de la cinta, veinte
   minutos entre manos.
4. **Sellar el borde del capacitivo** con esmalte de uñas o epoxi: el canto
   de esa placa absorbe agua y en un mes la lectura deriva. Sellar también el
   circuito, no sólo el canto.
5. Montar el sustrato en la carcasa con los cuatro **M2**. La pantalla ya no
   pasa por el sustrato: la sostiene la carcasa. El
   apoyo de la pantalla lo da el marco de la carcasa, no el sustrato.
6. La junta del capacitivo **abajo**, y el cable **haciendo panza** para que
   el agua gotee antes de llegar a la placa.

---

## Qué anotar de cada unidad

Un renglón por aparato, que después vale oro:

```
unidad ___  fecha ______  sustrato v2.0
  SuperMini: 5V unido a VBUS?  si / no     LED de encendido: sacado / no tenia
  pantalla: BL = anodo / catodo            JP_BL: C-A / C-B
  area activa a ___ mm del borde
  capacitivo seco: ____ mV   mojado: ____ mV   chip: TLC555 / otro
  reposo medido: ____ uA (antes de limpiar) / ____ uA (despues)
  RSSI dentro de la carcasa: ____ dBm   al aire: ____ dBm
  celda: marca ________  mAh ____  tension al montar ____ V
```

---

## Si algo no anda

| Síntoma | Primer lugar donde mirar |
|---|---|
| No enciende nada | `TP5` contra `TP1`: si no hay 3,3 V, el problema está antes del regulador. `TP4` y el interruptor |
| Enciende y se reinicia solo | celda baja (< 3,5 V) o un corto intermitente en el pico de wifi; C2 mal soldado |
| No entra en modo descarga | apretar BOOT mientras se enchufa. Si la placa tiene el LED de encendido sacado, verificar que no se cortó otra pista |
| La pantalla queda negra | `TP5` en el borne `VCC` de `J_TFT`; después el selector `JP_BL`; después los cables SCL/SDA, que son SCK/MOSI |
| La imagen sale corrida o con colores cambiados | `RK_TFT_OFS_X/Y`, `RK_TFT_BGR`, `RK_TFT_INVERT` en `platformio.ini`. Es de configuración, no de hardware |
| La imagen sale con basura | bajar el SPI: `-DRK_TFT_SPI_HZ=20000000` |
| La luz de fondo no prende | el paso 1.c estaba mal: cambiar el lado de `JP_BL` |
| La luz prende sola en reposo | falta R8 (o R10), o GPIO21 quedó flotando |
| El suelo lee siempre lo mismo | chip NE555 en vez de TLC555, o el riel conmutado no prende (control 12.b) |
| La estampadora no baja hasta el fondo | está al revés (va dada vuelta), o las nervaduras salieron gordas: bajar el flujo un 3 % y reimprimir |
| Quedó cinta pegada sobre las paredes | se apretó de más y la cara plana llegó a apoyar; o la hoja tenía un solape justo ahí |
| No aparecen AHT20 ni BH1750 | SDA y SCL cruzados; o faltan los pull-ups porque **ninguno** de los dos módulos los trae |
| Sólo falla el DS18B20 | el pull-up de 4,7 k (R6), o la sonda en modo parásito |
| El reposo da más de 200 µA | flux, LED de encendido, pull-ups duplicados. En ese orden |
| Carga eterna, nunca termina | Q1 no conduce: el aparato está comiendo del cargador en vez del USB |
| La celda se calienta cargando | **desconectar ya.** El AO3401 del arnés al revés, o el negativo de la celda unido a masa |

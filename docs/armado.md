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
| Bisturí o trincheta con hoja nueva | la hoja gastada arruga la cinta |
| **La estampadora impresa** | mete toda la cinta de una prensada (paso 2) |
| Bruñidor o el mango de una cuchara | para presionar la cinta en la canaleta |
| Multímetro con continuidad y prueba de diodo | |
| **Fuente de laboratorio con límite de corriente** | no es opcional: es lo que reemplaza a la celda hasta el paso 11 |
| Pinza de punta fina | para abrir las patas de los SOT-23 |
| Alcohol isopropílico y cepillo | el flux que queda es una fuga |
| Cinta de espuma de 1 mm | entre el sustrato y la pantalla |
| Cable de silicona AWG30 y AWG24 | puentes de señal y de potencia |

---

## Paso 0 — Imprimir

```
make pcb                       # regenera el STL y la plantilla
```

Se imprimen **dos piezas**:

| Pieza | STL | Cómo |
|---|---|---|
| El sustrato | `generado/nucleo-sustrato.stl` | **PETG**, canaletas **hacia arriba**, sin soportes, boquilla **0,6**, capa 0,2, tres perímetros. 62 × 92 × 3 mm |
| La estampadora | `generado/nucleo-estampadora.stl` | PETG o PLA, nervaduras **hacia arriba**, sin soportes, misma boquilla y capa. 67 × 97 × 7,5 mm |

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

> **La primera vez, antes del sustrato entero:** imprimir un cupón de prueba
> de canaletas (40 × 40 mm con canaletas de 1,2 y 2,4 mm y agujeros de 1,4 y
> 1,8 mm) y
> confirmar que la cinta entra, se presiona y se corta contra la pared. Media
> hora que evita tirar cinco sustratos.

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

**1.c — Qué es el pin BL de la pantalla.** Multímetro en prueba de diodo,
con el módulo **desconectado**:

| Punta roja | Punta negra | Si marca ~1,8–3 V | Conclusión |
|---|---|---|---|
| `BL` | `GND` | conduce | BL es el **ánodo** del LED → **lado alto** |
| `VCC` | `BL` | conduce | BL es el **cátodo** → **lado bajo** |

- **Lado alto** (lo esperado): puente de estaño en `JP_BL` entre **C y A**.
- **Lado bajo**: puente entre **C y B**, y **no** poblar Q4, R9 ni R10.

**1.d — Dónde cae el área activa del panel.** Con el calibre, medir del
borde del módulo **opuesto al conector** hasta el centro del cuadrado
encendido... o, más simple, hasta cada borde del área activa. El diseño
asume que el centro está a **14,0 mm**.

> Si da otra cosa: corregir `sustrato.activa.y` en el JSON (`activa.y =`
> `ventana.y − ventana.alto/2 + lo_medido`), correr `make pcb`, y pasarle el
> número nuevo a quien modela las carcasas. **Es la cota que decide si la
> cara del Rooti queda centrada en su ventana.**

**1.e — El chip del capacitivo.** Mirar la serigrafía: tiene que decir
**TLC555**. Si dice NE555, el sensor no arranca a 3,3 V y da lecturas planas:
se devuelve.

---

## Paso 2 — La cinta, de una prensada

Se trabaja sobre la **cara de las canaletas**, con la plantilla al lado en la
misma orientación (el texto grabado se lee derecho).

1. **Cortar la hoja al contorno del sustrato.** Apoyar la cinta sobre el
   sustrato cubriéndolo entero —si el rollo es más angosto que 72 mm, dos o
   tres tiras solapadas 2 mm; el solape no molesta— y pasar el bisturí
   alrededor del canto del sustrato, que hace de guía. Queda una hoja del
   tamaño exacto de la placa, que es lo que necesita el faldón para cerrar.
2. **Apoyar la hoja** con el adhesivo hacia abajo, sin presionar todavía:
   sólo lo justo para que no se mueva.
3. **Bajar la estampadora.** El faldón envuelve el borde del sustrato y la
   centra sola. No hay que apuntar.
4. **Apretar parejo**, con las dos manos o —mejor— con una tabla y el peso
   del cuerpo, quince segundos. Presión repartida, no un punto.
5. **Levantar.** La cinta quedó metida en cada canaleta y tendida sobre las
   paredes que las separan, marcada por el canto de cada una.
6. **Recortar lo tendido**: pasar el bisturí por el canto de cada canaleta,
   que ya está dibujado en la cinta, y levantar el sobrante. Sale en pedazos
   grandes.
7. **Repasar con el bruñidor** canaleta por canaleta, **desde el centro hacia
   los bordes**, para que la cinta apoye contra el fondo.
8. **Cada esquina y cada empalme lleva una gota de estaño.** El adhesivo no
   es una conexión. Toque corto: apoyar, estañar, retirar, contar hasta tres
   antes del siguiente.

> **Si no tenés la estampadora** (o si se rompió una nervadura), se puede
> hacer tramo por tramo: cortar un pedazo de cinta un poco más ancho que la
> canaleta, apoyarlo, presionarlo con el bruñidor y recortar contra la pared
> con el bisturí apoyado **en la pared, no en la cinta**. Son los mismos
> 76 tramos, y media tarde en vez de veinte minutos.

**Control 2.a — continuidad de cada red.** Multímetro en continuidad. Para
cada red de [conexiones.md](conexiones.md), tocar el nodo más lejano contra
el más cercano de la lista: **tiene que pitar**. Las de masa se prueban
contra `TP1`, que es el punto negro del tester de acá en adelante.

**Control 2.b — aislación entre rieles.** Con el tester en resistencia, y
**nada más soldado**:

| Entre | Tiene que dar |
|---|---|
| `TP1` (GND) y `TP5` (3V3) | abierto |
| `TP1` y `TP3` (VSYS) | abierto |
| `TP1` y `TP4` (VINT) | abierto |
| `TP3` y `TP4` | abierto (todavía no está el interruptor) |
| `TP5` y `TP4` | abierto |

Si alguno da un valor bajo, hay una rebaba de cinta o un puente de estaño.
Se busca con lupa antes de seguir: después va a estar tapado por un módulo.

---

## Paso 3 — Los puentes de cable

Los 38 de la tabla de [conexiones.md](conexiones.md), en ese orden, con cable
de silicona **AWG30** (los de potencia, que la tabla marca, con **AWG24**).

Van **por arriba** de la cinta, cruzando lo que tengan que cruzar, pegados al
sustrato con una gota de cianoacrilato cada 15 mm para que no bailen.

> **Los tres que dicen "rodeando" no van derecho.** El camino corto les
> cruzaría la ventana de la pantalla, y ahí el cable quedaría apretado entre
> el módulo y el plástico. La plantilla los dibuja por donde van: seguila.

**Control 3.** Tachar cada puente de la lista al soldarlo y volver a hacer el
**control 2.a completo**. Es el momento más fácil para olvidarse uno, y el
más barato para encontrarlo.

---

## Paso 4 — Los componentes chicos

En este orden: resistencias y capacitores (1206), después el Schottky, después
los MOSFET.

- **1206**: flux, estañar un pad, apoyar con pinza, soldar el otro lado,
  rehacer el primero.
- **SOT-23**: abrir las dos patas del mismo lado 0,2 mm con la pinza hasta
  que caigan en sus pads (están a 2,3 mm, no a 1,9). Soldar primero la pata
  sola del otro lado.
- **C2** (el electrolítico de 220–470 µF) va **acostado**, con las patas
  dobladas sobre sus pads. **Ojo con la polaridad**: la franja blanca va del
  lado de masa, que es el pad de la izquierda mirando la plantilla.

**Lo que NO se puebla** salvo que el paso 1 diga lo contrario: nada. Si el
paso 1.c dio "lado bajo", **no** van Q4, R9 ni R10.

**Control 4 — los valores, otra vez.** Medir en la placa, con el tester en
resistencia:

| Entre | Esperado |
|---|---|
| `TP4` (VINT) y `TP1` (GND) | **940 kΩ ±10 %** (el divisor: 470 k + 470 k) |
| `TP5` (3V3) y `TP1` | **> 100 kΩ** (si da menos, hay un corto o C2 al revés) |
| `TP1` y el pad `G` de Q1 | **100 kΩ** (R3) |

**Control 4.b — el selector de la luz.** Confirmar a ojo que `JP_BL` tiene el
puente de estaño del lado que dijo el paso 1.c, y **sólo de ese lado**.

---

## Paso 5 — Los bornes y los cables a los módulos

Cada grupo de bornes lleva sus cables a su módulo. El largo se corta con el
aparato armado en la mano, no antes; sobra siempre es mejor que falta.

| Borne | Módulo | Cables |
|---|---|---|
| `J_TFT` | pantalla TFT 1,44" | 8, en el orden del conector: GND, VCC, SCL, SDA, RES, DC, CS, BL |
| `J_AHT` | AHT20 | 4: VCC, GND, SCL, SDA |
| `J_BH` | BH1750 | 4: VCC, GND, SCL, SDA (ADDR al aire) |
| `J_TTP` | TTP223 | 3: VCC, GND, I/O |
| `J_DS` | DS18B20 | 3: VDD rojo, GND negro, DQ amarillo. **No** en modo parásito |
| `J_SUELO` | capacitivo | 3: VCC, GND, AOUT |
| `J_TP` | TP4056 | 6 cortos: IN−, OUT−, B+, B−, OUT+, IN+ |
| `J_CELDA` | portapilas | 2 de AWG24: + y − |
| `J_SW` | interruptor | 2 |

**La pantalla.** Si el módulo vino con el header de 8 pines suelto en la
bolsa, mejor: se sueldan los ocho cables directo a los pads y el conjunto
queda 8 mm más fino. Si vino soldado, se desuelda o se sueldan los cables a
las puntas de los pines.

> **Cuidado con el `J_TFT`.** Los bornes están rotulados con el nombre del
> pin **del módulo**, no con el de la red. `SCL` del módulo es el SCK del
> ESP32 y `SDA` es el MOSI: así es como vienen serigrafiados estos paneles.
> Cable a cable, mirando los dos rótulos.

> **Cuidado con `J_CELDA`.** El **negativo de la celda va al `B−` del
> TP4056**, no a masa. Si se une a masa se puentean los MOSFET del DW01A y la
> celda queda **sin protección de sobredescarga ni de cortocircuito**. Es el
> error más caro de esta guía.

**Control 5.** Tester en continuidad: entre `J_CELDA.−` y `TP1` (GND) tiene
que dar **abierto**. Si pita, está mal cableado: parar.

---

## Paso 6 — El cargador

El TP4056 apoya **sobre la cara plana** de los módulos, con su **USB-C
mirando hacia el borde de abajo**. (En la v1.0 iba hundido en un bolsillo;
se lo sacamos porque era un voladizo en la cara que se imprime contra la
cama, y ahora el módulo queda 0,8 mm más afuera.) Se fija con dos gotas de cianoacrilato en
las esquinas o con una gota de silicona caliente, y sus seis cables bajan por
los agujeros a `J_TP`.

**Control 6 — la carga, sola, sin la celda ni el resto.**

1. Interruptor **apagado**.
2. Fuente de laboratorio a **3,7 V, límite 200 mA**, conectada a `J_CELDA.+`
   (rojo) y `J_CELDA.−` (negro). Hace de celda.
3. Enchufar el USB del cargador.
4. El LED rojo del TP4056 se enciende: está cargando. El consumo de la fuente
   debería ser negativo (le está entrando corriente) o ~0 si tu fuente no
   absorbe.
5. Medir `TP2` (celda protegida) contra `TP1`: **3,7 V**, lo que pusiste.
6. Medir `TP3` (nodo de sistema) contra `TP1`: **4,5–4,7 V**. Son los 5 V del
   USB menos el Schottky. **Si da 3,7 V, el D1 está al revés o el Q1 está
   conduciendo cuando no debe.**
7. Medir `TP4` (VINT) contra `TP1`: **0 V**, porque el interruptor está
   apagado.
8. Desenchufar el USB. `TP3` tiene que pasar a **3,7 V**: ahora el sistema
   come de la "celda" por el Q1. **Si queda en 0, el Q1 no conduce**: mirá
   su orientación (la pata sola es el drenador y va del lado de la celda).

Este control prueba la carga compartida entera sin arriesgar nada.

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
2. Fuente a **3,8 V, límite 100 mA**, a `J_CELDA.+` / `J_CELDA.−`.
3. **Prender el interruptor.**
4. **Mirar el amperímetro de la fuente en ese instante.** Tiene que subir a
   **20–60 mA** y quedarse ahí. Si salta al límite de 100 mA, **apagar** y
   buscar el corto: la fuente acaba de salvar la placa.
5. Medir `TP5` (3V3) contra `TP1`: **3,25–3,35 V**.
6. Medir `TP4` (VINT) contra `TP1`: **3,8 V**, lo de la fuente.

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
2. Confirmar otra vez el **control 5**: `J_CELDA.−` contra `TP1`, abierto.
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
| 1 | AHT20 | temperatura y humedad creíbles; comparar contra un termohigrómetro |
| 2 | BH1750 | tapar con la mano: los lux caen a menos de 10 |
| 3 | TTP223 | tocar: `[boton]` en el log, la pantalla se enciende |
| 4 | DS18B20 | temperatura cerca de la del aire; con los dedos en la sonda sube |
| 5 | Capacitivo | en el aire, un número; en un vaso de agua hasta la línea, otro **bien distinto** |
| 6 | Pantalla | el QR se lee desde un teléfono a 30 cm |

**Control 12 — el capacitivo contra el ADC.** Con el capacitivo seco y el
riel de sensores prendido (durante una medición), medir su `AOUT` contra
masa. **Tiene que dar menos de 2,5 V.** Por encima de eso el ADC del C3 a
11 dB satura y el sensor pierde la mitad de su rango.

> Si da más: soldar un divisor **100 k / 220 k** en los propios bornes de
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
5. Montar el sustrato en la carcasa con los cuatro **M3**, con la espuma de
   1 mm entre el sustrato y la pantalla. La ventana es un hueco recto: el
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
| La celda se calienta cargando | **desconectar ya.** Q1 al revés, o `J_CELDA.−` unido a masa |

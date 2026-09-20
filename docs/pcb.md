# La PCB impresa

El ROOTKIT no lleva una placa de circuito impreso comprada: lleva un
**sustrato de PETG impreso en 3D con canaletas**, y las pistas son **cinta de
cobre** pegada dentro de esas canaletas y soldada en cada unión. Encima van
los módulos: el ESP32, la pantalla, el cargador y los bornes de todo lo que
vive repartido por la carcasa.

Es el **núcleo común**: uno solo para los cinco Rooties. La carcasa cambia
(es decisión de arte); lo de adentro es igual.

- El diagrama de conexiones, red por red, está en
  [conexiones.md](conexiones.md) y **se genera**: no se edita a mano.
- El paso a paso para armar una unidad está en [armado.md](armado.md).
- Los pines siguen siendo los de
  [`firmware/esp32/placa.h`](../firmware/esp32/placa.h); acá no hay una
  segunda verdad.

## De dónde sale todo

```
hardware/pcb/nucleo.json      EL DATO: modulos, posiciones, redes y pistas
        │
        ├─► hardware/pcb/sustrato.scad ───► generado/nucleo-sustrato.stl
        ├─► hardware/pcb/estampadora.scad ► generado/nucleo-estampadora.stl
        ├─► hardware/pcb/encaje.scad         la prueba de que una entra en la otra
        ├─► generado/plantilla-cinta.svg     1:1, para imprimir y cortar
        ├─► docs/conexiones.md               el diagrama de conexiones
        └─► firmware/test/redes.h            la netlist, para `make test`
```

Todo lo de la derecha se regenera con **`make pcb`**, y CI falla si quedó
desfasado. El JSON es el único archivo que se edita.

**Por qué así.** Un sustrato dibujado a ojo se desincroniza del firmware en
el segundo cambio de pin, y nadie se entera hasta que hay una placa armada
que no anda. Con el dato en el medio, mover un pin obliga a tocar `placa.h`,
el JSON y `docs/hardware.md` en el mismo commit, y si no, `make test` lo
dice. Es la misma idea que las caras: parámetros en una tabla, el resto
generado.

## Lo que verifica la máquina

`python3 tools/pcb.py --verificar` (y `make verify`, y CI) revisa el
sustrato entero antes de que exista:

| Qué | Cómo |
|---|---|
| Cada GPIO de `placa.h` está en la red que corresponde | `test_placa.c`, dentro de `make test` |
| Los dos analógicos caen en el ADC1 | ídem — el ADC2 no anda con el wifi prendido |
| El toque cae en GPIO0–5 | ídem — son los únicos que despiertan del sueño profundo |
| Los tres pines de arranque quedan altos al encender | ídem |
| El 1-Wire y su pull-up salen del mismo riel | ídem |
| Ninguna canaleta queda a menos de 0,8 mm de otra | `tools/pcb.py` |
| Ancho mínimo por clase de red | ídem |
| Cada red queda en **una sola pieza** (pistas + puentes) | ídem |
| Nada de cobre en la ventana, los recortes ni los tornillos | ídem |
| Nada de cobre nuestro en la zona libre de la antena | ídem |
| El texto grabado no muerde ninguna canaleta | ídem |
| Ninguna nervadura de la estampadora queda más fina de lo que imprime | ídem |
| La estampadora **entra** en el sustrato sin tocarlo | `encaje.scad`, con OpenSCAD |
| La estampadora **llega al fondo** de las canaletas | ídem |

Son 217 comprobaciones en C y once reglas geométricas en Python. Lo que **no**
verifica: que el módulo que llegue tenga los pines donde dice el JSON. Eso se
mide, y está en [armado.md](armado.md), paso 1.

## El sustrato

| | |
|---|---|
| Material | **PETG** (o ASA). PLA no: se ablanda a ~60 °C, y acá se suelda encima y el aparato vive al sol de una ventana |
| Medidas | **72 × 104 × 3,0 mm** |
| Boquilla / altura de capa | 0,4 mm / 0,2 mm, 4 perímetros |
| Soportes | **ninguno**: el único voladizo es la repisa de la ventana, 0,9 mm, que FDM puentea |
| Orientación | la cara de las canaletas **hacia arriba**; la cara plana en la cama |
| Canaletas | **0,4 mm de profundidad**, 0,3 mm más anchas que la cinta |
| Pared entre canaletas | **0,8 mm mínimo** = dos extrusiones de 0,4 |
| Relleno | 25 % basta; lo que importa son los perímetros |

**Por qué 0,4 mm de profundidad y no más.** La canaleta hace dos cosas: guía
la cinta y separa las pistas. 0,4 mm alcanzan para las dos, y la cinta se
presiona con un bruñidor sin quedar hundida donde después hay que soldar. Más
profundo sería más difícil de pegar y no compraría nada.

**Por qué 0,3 mm de holgura.** La cinta entra sin arrugarse y la pared queda
de guía para el bisturí: se apoya una tira más ancha, se presiona y se corta
contra la pared. Es lo que hace que dos unidades salgan iguales.

**Por qué 3,0 mm de espesor.** Debajo de una canaleta quedan 2,6 mm; debajo
del bolsillo del cargador, 2,2 mm. Los dos por encima de la pared mínima de
1,6 mm de [carcasas.md](carcasas.md).

### Las dos caras

- **Cara de las canaletas** (la de arriba al imprimir): es la **de atrás del
  producto**. Ahí van la cinta, los componentes chicos, los bornes y los
  puentes.
- **Cara de los módulos** (la plana): mira al **frente**. Ahí apoyan la
  pantalla, la SuperMini y el cargador. Las patas pasan por los agujeros y se
  sueldan del lado de la cinta.

Todas las coordenadas de la plantilla se leen **desde la cara de las
canaletas**: de frente al ROOTKIT, la X crece hacia la izquierda. Está escrito
en la plantilla para que nadie la use espejada.

### La ventana de la pantalla

La pantalla entra **desde el frente**, apoya en una repisa de 0,9 mm y la
carcasa la aprieta contra el marco de su propia ventana ([carcasas.md](carcasas.md)).
El sustrato no la sujeta: la **posiciona**. Entre la repisa y el módulo va
una tira de **espuma de 1 mm**, que absorbe la tolerancia del espesor del
panel y evita apretar el vidrio contra dos apoyos rígidos.

> **A medir con el módulo en la mano: dónde cae el área activa.** El módulo
> es de 28 × 37 mm y el área activa, de 25,9 × 25,9. Los 11 mm que sobran
> **no están repartidos en partes iguales**: casi todos están del lado del
> conector de 8 pines. El JSON asume que el centro del área activa queda a
> **14 mm del borde opuesto al conector** (`sustrato.activa`), y de ahí sale
> dónde va la ventana de la carcasa. Es la cota más importante de todo el
> producto y es la única que no se puede verificar sin el módulo: si sale
> distinta, se corrige el número en el JSON, se corre `make pcb`, y la
> carcasa se centra en el rectángulo punteado de la plantilla.

## Las pistas

Cinta de cobre adhesiva de 0,035–0,07 mm. Anchos:

| Clase | Ancho | Dónde |
|---|---:|---|
| Rieles de potencia | **2,4–3,0 mm** | masa, 3V3, VINT, el nodo de sistema |
| Señal | **1,4 mm** | SPI, I2C, control |
| Abanico junto a la SuperMini | **1,4 mm** | los pines están a 2,54 mm: más ancho no entra con 0,8 de pared |
| Islas de SOT-23 | **1,2 mm** | las patas se abren 0,2 mm con una pinza |

**La corriente no es el problema; la resistencia tampoco.** Una pista de
1,4 mm × 0,035 mm tiene 0,35 mΩ por milímetro: los 17 mm de abanico que
salen del pin 5V son 6 mΩ, o **2 mV al pico de wifi de 350 mA**. Lo que
manda el ancho es el ancho mínimo que se puede cortar a mano contra una
pared impresa.

**Las uniones se sueldan, siempre.** El adhesivo de la cinta es conductor
"de a ratos": sirve para pegar, no para conducir. Cada empalme entre dos
tramos y cada pata de componente lleva estaño. Con **Sn42Bi58 (138 °C)** y
flux la cinta no se despega ni el PETG se deforma; con estaño común
(183–190 °C) también sale, pero con toques de menos de dos segundos y la
punta a 260 °C, no más.

**Masa.** No hay plano de masa: no se puede con una cara y cinta cortada a
mano. Lo que hay es una **barra de masa** de 2,4 mm que recorre el sustrato
de lado a lado a la altura de los bornes, baja por la izquierda hasta el
fondo y vuelve por abajo. Cada borne tiene su pad de masa **sobre** la barra,
así que no hay un solo ramal largo de masa en toda la placa.

**Las dos barras.** Masa y 3V3 corren paralelas, y cada grupo de bornes de
sensor es una **columna** que las cruza: VCC arriba (sobre la barra de 3V3),
GND en el medio (sobre la de masa) y la señal abajo. Es lo que hace que
alimentar un sensor sea cero pistas.

### El SPI

SCK y MOSI bajan desde la SuperMini hasta los bornes de la pantalla en
**19 mm**, rectos, con la barra de masa al costado. A 40 MHz eso no es un
problema. Si igual aparece basura en la imagen, se baja el reloj desde
`platformio.ini` con `-DRK_TFT_SPI_HZ=20000000` — sin tocar una línea de
código — antes de sospechar del hardware.

### Los puentes

Una sola cara de cobre significa que algunos cruces no se pueden evitar. Se
resuelven con **cable aislado fino (AWG30) por arriba**: 28 puentes, todos
listados en [conexiones.md](conexiones.md) y dibujados en la plantilla con
línea azul de puntos. No hay ninguno que no esté en esa lista, y la prueba de
conectividad los cuenta: si falta uno, la red queda en dos pedazos y el
verificador lo dice por nombre.

Dos de ellos son de potencia y van con **cable más grueso (AWG24)**: el nodo
de sistema al interruptor y el de la celda protegida al MOSFET de carga
compartida. Ahí el cable es mejor conductor que la cinta, así que no se
pierde nada.

### La antena

> **La regla de los 10 mm no se puede cumplir con este módulo, y conviene
> saber por qué.** La antena cerámica de la SuperMini está **entre las dos
> filas de pines**, a unos 3,6 mm de los pads más cercanos. Esos pads son del
> módulo, no nuestros: no hay forma de alejarlos. Pedir 10 mm de despeje a
> nuestro cobre obligaría a bajar diez pistas por un canal de 9 mm entre las
> filas, que no entra.

Lo que sí se hace, y se verifica:

1. **5 mm de despeje** de nuestro cobre a la antena, medidos desde el
   rectángulo de la antena y con la sombra del propio módulo exenta. En la
   práctica ninguna pista nuestra pasa por al lado de la antena: bajan
   pegadas a los pines y se van para abajo.
2. **Recorte pasante detrás de la antena** (9 × 5 mm): ni plástico.
3. **La antena es lo más alto de la electrónica** y apunta hacia arriba,
   lejos de la celda y de la tierra húmeda, que es lo que más la castiga.
4. **La carcasa deja 10 mm de aire** alrededor, sin metal.

**Cómo se acepta:** midiendo el RSSI dentro de la carcasa, que ya es una
casilla de la Fase 2 del [roadmap](roadmap.md). Si el RSSI dentro de la
carcasa cae más de 6 dB respecto del módulo al aire, hay que mover la
electrónica, no discutir el número. Y para la PCB propia de la Fase 5, con
el C3 en módulo y dos caras, el despeje completo sí se puede: ahí se hace.

## La estampadora

Pegar cincuenta y un tramos de cinta uno por uno es media tarde y cincuenta y
una oportunidades de correrse. La estampadora es **el negativo del sustrato**:
las mismas canaletas, pero en relieve. Se apoya una hoja de cinta de cobre
sobre el sustrato, se baja la estampadora encima y se aprieta: todas las
pistas entran a la vez.

Sale del mismo dato que el sustrato, así que no se puede desincronizar: si
mañana se mueve una pista, `make pcb` rehace las dos piezas.

| | |
|---|---|
| Medidas | **77 × 109 × 7,5 mm** (el sustrato más el faldón) |
| Material | PETG, o PLA: no se suelda nada encima, sólo tiene que ser rígida |
| Espesor de la placa | **5 mm**, para que no flexione al apretar |
| Nervaduras | **0,7 mm de alto**, 0,6 mm más finas que la canaleta |
| Faldón | 2,5 mm de alto, 0,5 mm de holgura: centra la pieza sola |
| Orientación de impresión | nervaduras **hacia arriba**, sin soportes |

### Los tres números que la hacen funcionar

**Va espejada en X.** Se imprime con las nervaduras hacia arriba —que es la
única forma de que salgan sin soportes— y se usa dada vuelta. Dar vuelta
espeja. Por eso el modelo se construye entero dentro de un `mirror([1,0,0])`:
un pad que en el sustrato está en *x* se imprime en *(ancho − x)* y al
voltear la pieza vuelve a caer en *x*. Es un error que no se ve mirando el
modelo —la pieza imprime igual de bien— y por eso tiene su propia prueba.

**La nervadura sobresale 0,3 mm más de lo que hunde la canaleta.** Al
apretar, la punta toca el fondo y la cara plana de la estampadora queda
**0,3 mm separada** de la cara del sustrato. Ese aire es el que hace que la
cinta se pegue *sólo adentro de las canaletas* y no sobre las paredes que las
separan. Si las dos caras se tocaran, la cinta quedaría pegada en todos lados
y habría que despegarla justo donde no hay que romperla.

**La nervadura es 0,3 mm más fina por lado.** Ahí entran el espesor de la
cinta doblada contra las dos paredes (0,035 mm cada una) y la tolerancia de
impresión. Si midiera exactamente lo mismo que la canaleta, no entraría. La
nervadura más fina de todo el juego mide **0,9 mm** —la de las pistas de
1,2 mm— y el verificador falla si alguna baja de 0,8, que son dos
extrusiones.

### Cómo se comprueba que entra

`hardware/pcb/encaje.scad` pone la estampadora dada vuelta, apoyada a fondo
sobre el sustrato, y hace dos preguntas:

1. **Choque.** La intersección de las dos piezas tiene que dar **vacía**.
   Cualquier sólido que aparezca es plástico contra plástico: la pieza no baja
   del todo y esa canaleta se queda sin cinta.
2. **Presencia.** Lo que la estampadora mete *dentro* de la franja de las
   canaletas tiene que dar **lleno**, y cubrir más del 80 % de la placa.

La segunda existe porque la primera sola se aprobaría por la razón
equivocada: si las nervaduras desaparecieran, la intersección también daría
vacía y la prueba pasaría con una pieza que no sirve para nada. Con las dos
juntas: hoy cubren el **100 %** de la placa en los dos ejes.

Y están calibradas. Corriendo la estampadora 0,5 mm, el choque salta con 707
triángulos de contacto; volteándola sobre el eje equivocado —el error del
espejo— salta con 1254. No es una prueba que pase sola.

### Cómo se usa

En [armado.md](armado.md), paso 2. En resumen: se corta la hoja de cinta
contra el borde del sustrato (el propio sustrato hace de guía), se apoya, se
baja la estampadora hasta que el faldón envuelve el borde, y se aprieta
parejo. Después se levanta y se recorta lo que quedó tendido sobre las
paredes, que ya viene marcado por el canto de cada canaleta.

> **La prueba en seco no es opcional.** Antes de poner la cinta, apoyar la
> estampadora sobre el sustrato vacío: tiene que bajar hasta que el faldón
> envuelva el borde, sin resistencia. Si hace tope antes, está al revés o la
> impresión salió con las nervaduras gordas. **Nunca forzar**: las nervaduras
> de 0,9 mm se parten.

## Humedad, fugas y barniz

La maceta se riega y la cinta se oxida. Tres cosas:

1. **La cara de la cinta mira al frente y hacia adentro**, nunca hacia
   arriba: ninguna gota cae sobre una pista.
2. **Barniz después de probar**, no antes: acrílico en aerosol o máscara UV,
   dos manos, tapando los puntos de prueba con cinta de papel para poder
   volver a medir.
3. **Limpiar el flux.** Entre dos pistas con restos de flux y humedad pasan
   microamperios, y acá cada microamperio son días de batería. Alcohol
   isopropílico y cepillo, y después el control de reposo del paso 9 de
   [armado.md](armado.md).

### El presupuesto de reposo

Lo que el sustrato promete consumir con el aparato durmiendo, sumando
hoja de datos por hoja de datos:

| Qué | µA |
|---|---:|
| SuperMini en deep sleep, sin el LED de encendido | 50 |
| Divisor del riel (470 k + 470 k, después del interruptor) | 3,9 |
| TTP223 | ~3 |
| Panel dormido | ~10 |
| DS18B20 en reposo | ~1 |
| AHT20 dormido | 0,25 |
| BH1750 (vuelve solo a apagado después de cada medición) | ~0,01 |
| Todo lo demás (compuertas en su estado de reposo) | 0 |
| **Total** | **~68** |

`nodo/power.c` modela 150 µA. Los 68 son el techo teórico; la diferencia es
margen y fugas. **El número que vale es el medido**, en la Fase 2 con un
PPK2 o un medidor USB, y ese es el que va a `power.c`.

## Lo que se resolvió, y cómo

### El pin BL de la pantalla

`hardware.md` decía "MOSFET N con el GPIO a la compuerta". Eso sirve si BL es
el cátodo del LED o la entrada de control de un transistor del módulo. En la
mayoría de los módulos de 1,44", **BL es el ánodo del LED** a través de una
resistencia: ahí un N-MOSFET del lado de masa no hace nada.

**Lo que se diseñó:** una llave del **lado alto** que funciona en los dos
casos más probables.

```
GPIO21 ──┬── compuerta Q3 (AO3400, N)          3V3 ──── fuente Q4 (AO3401, P)
         │        drenador ──┬── compuerta Q4       drenador ──┐
      R8 100 k               │                                 │
         │                R9 10 k a 3V3            JP_BL ── A ─┘
        GND                                          │
                                                     C ──── pin BL del modulo
                                                     │           │
                                                  B ─┘        R10 100 k
                                             (drenador de Q3)     │
                                                                 GND
```

- GPIO21 alto → Q3 conduce → la compuerta de Q4 cae a masa → Q4 lleva 3V3 al
  pin BL. **No invierte**: es lo que ya espera `pantalla.cpp`.
- **R9 es de 10 k y no de 100 k a propósito.** Con 100 k, la constante de
  tiempo para apagar Q4 es de ~70 µs y el PWM de 22 kHz (45 µs de período) se
  emborrona. Con 10 k son 7 µs. Cuesta 0,33 mA, y sólo mientras la pantalla
  está encendida.
- **R10 deja el nodo BL definido en bajo** cuando la etapa está apagada, así
  que la luz no queda "medio prendida" en el sueño profundo.
- **JP_BL es el selector.** Por defecto se hace un puente de estaño entre **C
  y A** (lado alto). Si el módulo resulta tener el BL como cátodo del LED, se
  hace el puente entre **C y B** y no se pueblan Q4, R9 ni R10: ahí Q3 trabaja
  del lado de masa, como decía el documento viejo.
- Si además resultara activo en bajo, se compila con
  `-DRK_TFT_BL_INVERTIDO=1` y listo.

**Qué medir primero** (paso 1 de [armado.md](armado.md)): con el módulo
alimentado a 3V3 y el tester en diodo, entre BL y GND y entre BL y VCC. Si
conduce hacia GND, BL es el ánodo → lado alto. Si conduce hacia VCC, es el
cátodo → lado bajo.

### El pull-up del 1-Wire

El problema: si la sonda cuelga del riel conmutado pero su pull-up de 4,7 k
sale del 3V3 fijo, con el riel apagado la línea de datos le mete corriente al
DS18B20 por su pata de datos, y de paso carga el riel apagado. El pull-up no
puede irse al riel conmutado: **GPIO8 es pin de arranque** y tiene que estar
alto al encender, cuando el riel está apagado.

**Lo que se hizo: la sonda se muda al riel fijo.** El DS18B20 consume
**1 µA como máximo en reposo** (hoja de datos), que son 24 µAh por día:
0,2 % del presupuesto diario de 13 mAh. Por ese precio desaparecen el camino
parásito, la duda del arranque y una resistencia con dos destinos posibles.

Es el mismo razonamiento por el que el AHT20 y el BH1750 ya estaban en el
riel fijo. **El firmware no cambia**: `sensores_leer()` sigue midiendo la
sonda dentro de la ventana en que el riel está prendido; ahora simplemente no
depende de eso.

En el riel conmutado queda **sólo el capacitivo de suelo**, que es el que de
verdad come (5 mA), más el `SD` del amplificador el día que haya sonido.
El riel lleva además una **resistencia de purga de 100 k** para que baje
rápido al apagarlo, y un **100 nF** al lado del borne.

Hay una prueba que lo sostiene: si alguien vuelve a poner el 1-Wire en el
riel conmutado, `test_placa.c` falla.

### Los dos USB y la batería

El riesgo, textual: la SuperMini tiene su propio USB-C y el TP4056 tiene
otro. Si el pin 5V de la SuperMini está unido a su VBUS sin diodo (muchas lo
están), al enchufar el USB de la SuperMini con la celda puesta y el
interruptor prendido, esos 5 V aparecen en el nodo de sistema. Sin el USB del
cargador, la compuerta del AO3401 está en bajo, el MOSFET conduce, y **la
celda recibe 5 V sin control de carga**.

**Por qué no se resuelve con un diodo.** Un Schottky en serie con el pin 5V
cuesta 0,2–0,3 V, y a 3,5 V de celda eso sube el corte práctico a ~3,8 V, que
en la curva de una 18650 es más de la mitad de la carga. Un "diodo ideal" de
verdad necesita dos MOSFET y un controlador: es exactamente lo que va en la
PCB de la Fase 5, no en un sustrato hecho a mano.

**Lo que se hizo, en tres capas:**

1. **El orden de armado lo vuelve imposible.** Se flashea y se pasa por
   `tools/fabrica.py` **antes de poner la celda** ([armado.md](armado.md),
   pasos 10 y 11). En el armado normal el escenario no existe.
2. **El interruptor es la llave.** Está entre el nodo de sistema y el pin 5V
   de la SuperMini: apagado, los 5 V del USB de la SuperMini se quedan del
   otro lado del interruptor y no llegan a la celda. La regla, para siempre:
   **el USB de la SuperMini se enchufa con el interruptor apagado.**
3. **El aviso está grabado en el plástico**, al lado del módulo:
   `APAGAR ANTES DE FLASHEAR`. No se despega, no se pierde y se lee cada vez
   que alguien abre el aparato.

Además, el USB de la SuperMini **no sale al exterior**: se llega a él
abriendo la carcasa. El único USB de afuera es el del cargador. Un usuario no
puede provocar el escenario; un desarrollador sí, y por eso la regla está en
el plástico.

**Cómo comprobar si la placa que llegó tiene el riesgo** (30 segundos, paso 1
de [armado.md](armado.md)): tester en continuidad entre el pin 5V del header
y el pin VBUS del conector USB-C de la SuperMini. Si pita, el riesgo existe.
Si no, la placa ya trae el diodo y la regla es sólo prolijidad.

**Nunca** se prueba esto con una celda de verdad: fuente de laboratorio con
límite de corriente a 100 mA.

> **Recomendación para Iñaki, para la Fase 5:** un solo USB-C, con VBUS al
> cargador y el par de datos al C3, y un multiplexor de alimentación de
> verdad (TPS2113 o un BQ24074, que ya trae el camino de sistema). Con el C3
> en módulo eso son dos componentes más y el problema desaparece de raíz.

### La celda reemplazable

`decisiones.md` dice que con la 18650 reemplazable por el usuario
"desaparecen el TP4056 y el conector USB". `hardware.md`, que es más nuevo,
tiene carga por USB-C, y el firmware la usa: detecta el enchufe por el riel
(arriba de 4,35 V) y reporta la batería como desconocida.

**Se diseñó sobre `hardware.md`.** Recomendación, para que Iñaki decida:

- **Mantener la carga por USB-C.** Seis meses de autonomía significan que el
  usuario tendría que comprarse un cargador de 18650 —o dos celdas y
  rotarlas— para un aparato que se vende como "clavalo y olvidate". Un cable
  USB-C lo tiene todo el mundo.
- **Que la celda sea de servicio, no de usuario:** portapilas con resortes,
  se llega abriendo la carcasa. Así no hay una tapa de batería que sellar en
  un objeto que vive al lado de tierra que se riega, y la celda se puede
  reemplazar igual cuando envejezca.
- El único agujero al exterior es el USB-C del cargador, **mirando hacia
  abajo y hacia atrás**, bajo un alero, como manda [carcasas.md](carcasas.md).

Si Iñaki prefiere lo otro, el cambio es sacar el TP4056, D1, Q1 y R3 del
JSON y correr `make pcb`: el sustrato se achica solo y nada del firmware se
entera.

### El divisor del riel, del lado del interruptor

En el esquema de `hardware.md` el divisor de 470 k + 470 k cuelga del nodo de
sistema, que está **antes** del interruptor. Acá cuelga de **después**. Lee
exactamente lo mismo (el interruptor no tiene caída), y con el aparato
apagado en la caja no gasta ni un microamperio en vez de 3,9 µA. En un año de
depósito son 34 mAh de una celda de 3000: poco, pero gratis.

## Lo que el sustrato le pide a la carcasa

Para que Rocío pueda modelar las cinco alrededor del mismo núcleo:

| Cota | Valor |
|---|---|
| Sustrato | 72 × 104 × 3,0 mm, cuatro tornillos M2 a 3 mm de cada esquina |
| Postes de la carcasa | Ø1,7 mm, a (3, 3), (66, 3), (3, 101) y (66, 101) desde la esquina inferior izquierda, mirando el frente en espejo |
| Profundidad hacia el frente | 5 mm hasta el vidrio de la pantalla; 10 mm donde está la SuperMini |
| Profundidad hacia atrás | 6 mm libres para la cinta, los puentes y las soldaduras |
| Centro del área activa de la pantalla | 25 mm desde el borde de abajo del sustrato, centrado a lo ancho |
| Antena | 10 mm de aire arriba y a los costados, sin metal |
| USB del cargador | en el borde de abajo, mirando abajo y atrás |
| USB de la SuperMini y botón BOOT | accesibles abriendo la carcasa, no desde afuera |
| Interruptor | en la pared, con dos cables a `J_SW` |
| Compartimento de la celda | **separado**, detrás del sustrato, 75 × 21 × 19 mm, parada |

**Volumen mínimo del producto: unos 78 × 116 × 45 mm.** Con la 18650 parada
detrás del sustrato, el centro de masa queda a ~42 % de la altura: dentro de
lo que pide [carcasas.md](carcasas.md), pero sin margen.

> **El Musgo no entra.** El "domo bajo y ancho" es el único de los cinco que
> no puede ser de 116 mm de alto sin dejar de ser un domo bajo. Para esa
> carcasa hay dos salidas, las dos sin tocar el firmware: acostar la celda
> (pide 78 mm de ancho interior, que un domo ancho sí tiene) o usar la
> **LiPo plana 103450** (50 × 34 × 10 mm), que baja el compartimento de
> 75 mm a 40 y deja el cuerpo en ~80 mm de alto. La segunda cuesta autonomía
> (1200 mAh contra 3000: de seis meses a dos y medio). **Es decisión de
> producto: queda anotada para Iñaki.**

## Lo que se probó de verdad, y lo que no

**Probado (corre en CI, en cada commit):**

- La geometría entera: separaciones, anchos, conectividad de las 26 redes,
  bordes, tornillos, ventana, recortes, zona de antena, rótulos.
- Que `placa.h` y la netlist dicen lo mismo, pin por pin, en las dos placas.
- Que el STL sale de OpenSCAD sin errores y que los generados están al día.

**Sin probar, porque hace falta tener las piezas en la mano** — en orden, y
todo esto está en [armado.md](armado.md) y en la Fase 2 del
[roadmap](roadmap.md):

1. **Una pieza de prueba de canaletas antes que el sustrato entero**: un
   cupón de 40 × 40 mm con canaletas de 1,2 / 1,4 / 2,4 / 3,0 mm y paredes de
   0,8 y 1,0, para confirmar que la cinta entra, se corta contra la pared y
   la pared sale de dos extrusiones. Es media hora y evita imprimir cinco
   sustratos mal.
2. **Una unión soldada de prueba**: Sn42Bi58 sobre cinta pegada en PETG, y
   tirar. Si el PETG se marca, bajar la punta o pasar a remaches.
3. La huella de la SuperMini contra la placa real (paso 1 del armado).
4. Dónde cae el área activa del panel.
5. Qué es el pin BL.
6. Si el pin 5V de la SuperMini está unido a su VBUS.
7. Cuánto mide el capacitivo seco: si pasa de **2,5 V**, la lectura se
   satura en el ADC del C3 a 11 dB. Remedio: un divisor 100 k / 220 k
   soldado en los propios bornes de `J_SUELO` (hay lugar y los bornes tienen
   agujero), y el mismo número cargado en `RK_SUELO_DIVISOR_NUM/DEN` para que
   el firmware lo sepa.
8. El consumo en reposo medido, contra los 68 µA de la tabla.
9. El RSSI dentro de la carcasa.

## Preguntas abiertas para Iñaki

Las tres están diseñadas en una dirección y siguen adelante con ella; cambiar
de idea es editar el JSON y correr `make pcb`.

1. **¿Un USB o dos?** Se diseñó con dos (el del cargador afuera, el de la
   SuperMini adentro) y la regla del interruptor. Recomendación: uno solo en
   la Fase 5, con multiplexor de alimentación.
2. **¿Celda reemplazable por el usuario?** Se diseñó con carga USB-C y celda
   de servicio. Recomendación: dejarlo así y corregir `decisiones.md`, que
   quedó viejo.
3. **¿El Musgo lleva 18650 o LiPo plana?** Se diseñó el núcleo para 18650.
   Recomendación: LiPo plana sólo para el Musgo, si su silueta no llega a
   116 mm.

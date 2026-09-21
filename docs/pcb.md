# La PCB impresa

El ROOTKIT no lleva una placa de circuito impreso comprada: lleva un
**sustrato de PETG impreso en 3D con canaletas**, y las pistas son **cinta de
cobre** pegada dentro de esas canaletas y soldada en cada unión.

## Qué es esta placa, y qué no (v3.0)

Es una **protoboard impresa**. Su única función es que el ESP32, la pantalla
y los sensores se **claven** en ella —cada uno por su tira de pines— y queden
interconectados. Nada más.

Hasta la v2.0 esta placa era además el chasis del producto: tenía la ventana
por donde asomaba la pantalla, el bolsillo donde se acostaba el cargador y el
hueco del USB. Eso ataba el tamaño y la forma de la placa a decisiones de
carcasa que todavía no están tomadas, y llenaba el centro del sustrato con un
agujero de 28 × 38 mm por el que no podía pasar ninguna pista. En la v3.0 se
fue todo eso:

| | v2.0 | v3.0 |
|---|---|---|
| Rol | chasis + interconexión | **sólo interconexión** |
| Ventana de la pantalla | 28,6 × 37,6 mm, pasante | **no hay**: la pantalla se clava y vive donde diga la carcasa |
| Hueco del USB / cargador | sí | **no hay**: la etapa de carga es un arnés externo de dos cables |
| Cómo se monta un módulo | bornes y cables sueltos | **tira de pines, clavada** |
| Cinta de cobre | dos rollos (6 y 20 mm) | **un solo rollo de 5 mm** |
| Canaleta | 0,8 mm de hondo | **1,2 mm** |
| Nervadura de la estampadora | sobresale 0,4 mm, 0,25 de luz | **sobresale 1,0 mm, 0,15 de luz** |
| Puentes de cable | 38 | **24**, y ninguno en una señal del SPI |

Lo que se gana: el montaje es clavar ocho módulos. Lo que se pierde: la placa
ya no sostiene nada, así que **la carcasa tiene que sostenerla a ella y a la
pantalla**. Eso está anotado en "Lo que el sustrato le pide a la carcasa".

Es el **núcleo común**: uno solo para los cuatro Rooties. La carcasa cambia
(es decisión de arte); lo de adentro es igual.

- El diagrama de conexiones, red por red, está en
  [conexiones.md](conexiones.md) y **se genera**: no se edita a mano.
- El paso a paso para armar una unidad está en [armado.md](armado.md).
- Los pines siguen siendo los de
  [`firmware/esp32/placa.h`](../firmware/esp32/placa.h); acá no hay una
  segunda verdad.

## De dónde sale todo

```
hardware/pcb/nucleo.json      EL DATO: modulos, donde va cada uno y que
        │                     va conectado con que. Se edita a mano
        │
        ├─► tools/ruteo.py ──────────────► generado/ruteo.json   POR DONDE
        │                                    corre cada pista (make rutear)
        │
        ├─► hardware/pcb/sustrato.scad ───► generado/nucleo-sustrato.stl
        ├─► hardware/pcb/estampadora.scad ► generado/nucleo-estampadora.stl
        ├─► hardware/pcb/encaje.scad         la prueba de que una entra en la otra
        ├─► generado/plantilla-cinta.svg     1:1, para imprimir y cortar
        ├─► docs/conexiones.md               el diagrama de conexiones
        └─► firmware/test/redes.h            la netlist, para `make test`
```

Todo lo de la derecha se regenera con **`make pcb`**, y CI falla si quedó
desfasado. El JSON es el único archivo que se edita a mano.

**El ruteo se separó del dato.** En `nucleo.json` está lo que se *decide*:
qué módulo va dónde y qué va conectado con qué. Por dónde corre cada pista lo
calcula `tools/ruteo.py` y vive en `generado/ruteo.json`. No se regenera en
cada build —tarda unos segundos y cambiaría con cualquier cosa— sino a mano,
con **`make rutear`**, cuando se mueve un módulo o se toca una regla. Que no
haya quedado viejo no se comprueba mirando la fecha del archivo: se comprueba
con la geometría, que es más fuerte. Un ruteo desfasado deja una red en dos
pedazos o dos canaletas demasiado juntas, y el verificador lo dice.

**Por qué un ruteador y no seguir a mano.** Las 51 pistas de la v1.0 se
dibujaron una por una. Eso funciona hasta que cambia algo de fondo, y desde
entonces cambió todo dos veces: primero la boquilla de 0,6 y los pads
escalonados, y después el rol entero de la placa. Redibujar ochenta pistas a
mano cada vez no es un plan. Teniendo el ruteador, mover un módulo es cambiar
dos números y volver a correrlo. Es la misma idea de siempre: el dato en el
medio, el resto generado.

**El orden en que se rutea es parte del diseño.** `orden_ruteo`, en el JSON,
es la lista de redes en el orden en que el ruteador las resuelve, y no es el
mismo orden en que se documentan. El que entra primero se queda con el lugar,
así que van primero las **señales** —dos o tres nodos cada una, caminos
cortos, ningún lugar alternativo— y al final los **rieles**, que tienen
quince nodos y se acomodan por donde queda. Al revés, con la masa adelante,
la masa se desparramaba por toda la placa y las señales llegaban tarde: la
mitad del SPI terminaba en puente. Sólo cambiar ese orden sacó nueve
puentes. Dentro del abanico del SPI el orden es de **adentro hacia afuera**:
la línea que tiene que llegar al pin más bajo del C3 corre más pegada al
módulo, así que va primero; si entra primero la de afuera, tapa el carril de
adentro.

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
| Nada de cobre en los recortes ni sobre los tornillos | ídem |
| Nada de cobre nuestro en la zona libre de la antena | ídem |
| El texto grabado no muerde ninguna canaleta | ídem |
| Ningún agujero es más chico de lo que la boquilla puede sacar | ídem |
| Entre dos agujeros queda pared de plástico suficiente | ídem |
| Ninguna canaleta pasa por el agujero de otra red | ídem |
| Ningún puente cruza la muesca de la antena ni un tornillo | ídem |
| Ninguna canaleta pide más cinta de la que trae el rollo de 5 mm | ídem |
| La cara que se imprime contra la cama **es un plano** | sobre el STL, en `make pcb` |
| Ninguna nervadura de la estampadora queda más fina de lo que imprime | `tools/pcb.py` |
| La estampadora **entra** en el sustrato sin tocarlo | `encaje.scad`, con OpenSCAD |
| La estampadora **llega al fondo** de las canaletas | ídem |

Son 217 comprobaciones en C y quince reglas geométricas en Python. Lo que
**no** verifica: que el módulo que llegue tenga los pines donde dice el JSON.
Eso se mide, y está en [armado.md](armado.md), paso 1.

Cuatro de esas reglas nacieron de una placa impresa de verdad, mirándola: los
agujeros salían tapados, había una repisa colgando en la cara de abajo, algún
puente cruzaba un hueco, y la cinta no alcanzaba a forrar las canaletas más
anchas. Las cuatro están ahora del lado de la máquina, y cada una tiene su
control negativo —se le devuelve el defecto y la regla tiene que saltar—
porque una prueba que nunca falló no demostró nada.

## El sustrato

| | |
|---|---|
| Material | **PETG** (o ASA). PLA no: se ablanda a ~60 °C, y acá se suelda encima y el aparato vive al sol de una ventana |
| Medidas | **68 × 92 × 3,0 mm** |
| Boquilla / altura de capa | **0,6 mm** / 0,2 mm, 3 perímetros |
| Soportes | **ninguno**, y no por poco: no hay un solo voladizo (ver "La base plana") |
| Orientación | la cara de las canaletas **hacia arriba**; la cara plana en la cama |
| Canaletas | **1,2 mm de profundidad**, 0,3 mm más anchas que la pista |
| Pared entre canaletas | **0,8 mm mínimo** |
| Agujeros | 1,4 mm todas las tiras de pines, 1,8 mm el electrolítico, 2,4 mm los tornillos M2 |
| Relleno | 25 % basta; lo que importa son los perímetros |

**Por qué 0,8 mm de profundidad.** La canaleta hace dos cosas: guía la cinta
y separa las pistas. La v1.0 usaba 0,4 mm y funcionaba para lo segundo, pero
para lo primero era poco: la cinta apoyaba casi al ras y había que sostenerla
mientras se bruñía. Con 0,8 —cuatro capas de 0,2— la cinta **se hunde y se
queda quieta sola**, y la pared da de guía al bisturí de punta a punta en vez
de apenas marcarla. Debajo de la canaleta siguen quedando 2,2 mm de plástico,
por encima de la pared mínima de 1,6 mm de [carcasas.md](carcasas.md).

**Por qué 0,3 mm de holgura.** La cinta entra sin arrugarse y la pared queda
de guía para el bisturí: se apoya una tira más ancha, se presiona y se corta
contra la pared. Es lo que hace que dos unidades salgan iguales.

**Por qué 3,0 mm de espesor.** Debajo de una canaleta quedan 2,2 mm, por
encima de la pared mínima de 1,6 mm de [carcasas.md](carcasas.md), y es lo
que necesita un agujero de 1,4 mm para guiar un pin derecho.

### La cinta manda

El rollo de cinta de cobre mide **5 mm de ancho**, y eso no se negocia: es
lo que hay. De ahí sale, por aritmética, casi todo el resto del diseño.

Una canaleta de ancho `a` y profundidad `p` **no se forra con `a` milímetros
de cinta**. La cinta tiene que bajar por una pared, cubrir el piso y subir
por la otra: necesita `a + 2p`. Con las canaletas de 0,8 mm de hondo de la
v2.0 eso casi no se notaba; con las de 1,2 mm de la v3.0 manda todo:

```
    ancho de canaleta + 2 x profundidad <= 5,0 mm
                     a + 2 x 1,2        <= 5,0
                                      a <= 2,6 mm
```

Y como la canaleta es la pista más `holgura_canaleta` (0,3 mm), la pista más
ancha posible es de **2,3 mm**. El riel de potencia quedó en **2,2** y la
señal en **1,4** (se rutean a 1,0; ver "Los anchos"). El verificador
`_v_cinta` lo comprueba pista por pista y pad por pad, así que no se puede
volver a dibujar una pista de 4 mm sin que la máquina lo diga.

**¿Y no es poco 2,2 mm para un riel?** Antes no lo hubiera sido: por la v2.0
pasaba el ampere de carga del TP4056. En la v3.0 el cargador vive fuera del
sustrato, así que la corriente más grande que cruza esta placa son los
**350 mA de pico del wifi**. Sobre 2,2 × 0,035 mm de cobre eso da 0,22 mΩ por
milímetro; los 70 mm más largos de riel son 15 mΩ, o sea **5 mV** de caída en
el peor instante. No se mide con un tester de mano.

**El otro premio: un solo rollo.** Con la pista más ancha en 2,2 mm, todas
las canaletas de la placa se forran con cinta de 5 mm. Se terminó el rollo de
20 mm para los rieles y el de 6 mm para las señales, y con él se terminaron
los empalmes entre cintas de distinto ancho.

> **Lo que no se puede pedir: que la cinta se corte sola.** El objetivo de
> hundir la canaleta a 1,2 mm y hacer que la nervadura sobresalga 1,0 mm es
> que el filo de la nervadura contra el borde de la canaleta **marque** la
> cinta hasta casi cortarla. Casi. Una matriz de corte de verdad, para 0,06 mm
> de cobre, trabaja con unas pocas micras de luz entre punzón y matriz; una
> boquilla de 0,6 mm da 0,15 mm en el mejor día, que son cincuenta veces más.
> Así que la cinta se marca acá y **se termina de separar lijando la cara**:
> después de prensar, una lija al ras deja el cobre sólo dentro de los
> canales. Ese es el proceso, y el diseño está hecho para él.

### La boquilla de 0,6 manda

La v1.0 se dibujó pensando en una boquilla de 0,4. Impresa con una de 0,6
—que es la que hay— **los agujeros salieron tapados**: un agujero de 1,0 mm
al que la impresora le pone un perímetro de 0,6 no deja casi luz, y el pin no
entra. Así que el número de la boquilla pasó a estar en el dato
(`reglas.boquilla`) y todo lo demás sale de él:

| Regla | Valor | Por qué |
|---|---:|---|
| `agujero_min` | **1,4 mm** | poco más de dos boquillas; menos que eso se cierra |
| `pared_min_agujeros` | **1,1 mm** | dos extrusiones de 0,55, que una boquilla de 0,6 saca sin despeinarse |
| `separacion_min` | 0,8 mm | una extrusión ancha; es la pared entre dos canaletas, no una pared estructural |

Los agujeros de los bornes fueron de 1,3 a **1,8 mm** (paso de 4,4 a 3,8, que
además angosta la placa), los del condensador radial de 1,2 a 1,8, y los
tornillos de 2,4 a **3,2**. El único que no pudo crecer tanto es el del C3, y
tiene su propia sección.

### Las tiras de pines, y de qué lado sale cada pad

Todo se **clava**: cada módulo trae su tira de pines macho, los pines pasan
por los agujeros del sustrato y se sueldan del otro lado, contra la cinta.
Ocho módulos, ocho tiras, ningún cable entre el ESP32 y los sensores.

El paso es **2,54 mm** y eso no se negocia. Hagamos la cuenta de lo que hay
que meter entre dos pines vecinos: el agujero, el anillo de cobre alrededor,
la holgura de la canaleta y la pared hasta el pad de al lado. Con una
boquilla de 0,6 el agujero solo ya pide 1,4 mm, y no queda nada para el
resto. **Un pad con su agujero adentro no entra a 2,54 mm.** No es cuestión
de dibujarlo mejor: no da la aritmética.

La salida es correr el pad **al costado del agujero** en vez de alrededor. El
pad queda **tangente al agujero**: la cinta llega justo hasta el borde, el
pin asoma, y al soldar se le arrima la punta contra la cinta que tiene al
lado, sin doblar nada. Es un movimiento distinto al de un pad con anillo, y
está explicado en [armado.md](armado.md), paso 3.

```
     agujero   pad
       ( )   ┌────┐
       │ │───│pad │──────────►  la canaleta sale para ESTE lado, y sólo
       ( )   └────┘             para este lado
```

**De qué lado va cada pad es una decisión de diseño, no de estética.** Un pad
sólo puede sacar su canaleta hacia su lado. Así que el lado de cada pin se
elige mirando adónde va esa red: los pines de alimentación miran hacia los
rieles y los de señal miran hacia el ESP32. En el JSON eso es la lista
`lados` de cada huella, un `+1` o un `-1` por pin.

Es el cambio que más puentes sacó, y se descubrió por el lado feo. La v2.0
escalonaba los pads a los dos lados —uno afuera, el siguiente adentro— para
ganar paso. Parecía prolijo, pero dejaba **la mitad de los pads adentro de la
huella**, y desde ahí la canaleta sólo podía salir por un pasillo central de
9 mm: ocho redes peleando por tres carriles. De ahí salía la mayoría de los
38 puentes de la v2.0. Con los pads elegidos por destino, las nueve líneas de
la pantalla salen cada una para su lado y **ninguna señal del SPI lleva
puente**.

```
   J_TFT, la pantalla de 9 pines, vista desde las canaletas:

        CS    DC  SDI  SCK          ▲  hacia el ESP32 (fila izquierda)
        ┌┐    ┌┐  ┌┐   ┌┐
     ───●●────●●──●●───●●───        la línea de agujeros, a 2,54 mm
      ┌┐  ┌┐    ┌┐        ┌┐
      VCC GND  RESET     LED SDO    ▼  hacia los rieles y la etapa de luz
```

**Y el orden de los pines decide de qué lado va el módulo.** Los cuatro hilos
del SPI salen de la pantalla en el orden CS, DC, SDI, SCK, y entran al C3 por
su fila izquierda en el orden CS, DC, MOSI, SCK de abajo hacia arriba. Si la
tira se monta **girada 180°**, esos dos órdenes coinciden y las cuatro
canaletas suben en paralelo sin cruzarse ni una vez. Montada al derecho, se
cruzan las cuatro. Es un `"rot": 180` en el JSON y vale cuatro puentes.

**Cuánto mide el pad: 1,0 × 2,2 mm.** El 1,0 es a lo largo de la tira y es el
número delicado. Entre dos pads vecinos a 2,54 mm quedan 1,54 mm, y del eje
de una canaleta al pad de al lado, 2,04 mm. Con pads de 1,2 mm eran 1,94, y
el ruteador pide 1,90: pasaba por cinco centésimas. Dos de las cuatro líneas
del SPI **no encontraban camino ni con la placa vacía**, y desde afuera
parecía un problema de congestión. Medio milímetro de margen en una regla
que se evalúa sobre una grilla de 0,2 mm no es margen.

Con eso el agujero pudo ir de 1,0 a **1,4 mm** (+40 %) dejando 1,14 mm de
pared entre agujeros vecinos, que es lo que la regla pide.

### La base plana### La base plana

La cara que se imprime contra la cama **es un plano**. Ni un escalón, ni un
bolsillo, ni una repisa: todo lo que la atraviesa la atraviesa entero y
recto.

La v1.0 no cumplía eso en dos lugares, y los dos daban el mismo problema: un
techo mirando hacia abajo a media altura, que la impresora tiene que tender
en el aire sobre la primera capa. Sale colgando y arruina la cara.

- **La repisa de la ventana** (0,9 mm hacia adentro, a 1,4 mm de la cama).
  Era para que la pantalla apoyara. En la v2.0 se fue la repisa y en la v3.0
  se fue la ventana entera: la pantalla se clava en su tira de pines.
- **El bolsillo del cargador** (27 × 14 mm hundidos 0,8 mm). Era para que el
  TP4056 quedara más al ras. Se fue: el módulo apoya sobre la cara, 0,8 mm
  más arriba, y la carcasa tiene lugar de sobra.

Que no vuelva no depende de que alguien se acuerde. `make pcb` lee el STL
terminado y busca facetas con la normal hacia abajo por encima de la cama: si
hay una, falla y dice a qué altura y dónde. Con la repisa puesta a propósito,
denuncia **115,9 mm² de techo colgando a 1,40 mm de la cama**.

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

### Ya no hay ventana

Hasta la v2.0 el sustrato tenía un hueco pasante de 28,6 × 37,6 mm por donde
asomaba la pantalla, y la cota más delicada de todo el producto era dónde
caía el área activa del panel dentro de su módulo: de ahí salía dónde iba la
ventana de la carcasa, y era la única medida que no se podía verificar sin
tener el módulo en la mano.

Eso se fue entero. La pantalla se clava en `J_TFT` por sus nueve pines y
después va donde la carcasa quiera; el sustrato ya no la posiciona. Además
de sacar una incógnita, devolvió **1.075 mm² en el centro de la placa** por
donde ahora pasan pistas.

Como efecto secundario desapareció un caso especial que estaba escrito en
cinco archivos: `nucleo.json` tenía una clave `ventana` aparte de la lista
genérica `recortes`, y `pcb.py`, `ruteo.py`, `sustrato.scad` y
`estampadora.scad` la trataban por separado. El único recorte que queda —la
muesca de la antena— ya entraba por la lista genérica.

## Las pistas

Cinta de cobre adhesiva de 0,035–0,07 mm. Anchos:

| Clase | Ancho | Dónde |
|---|---:|---|
| Rieles de potencia | **2,2 mm** | masa, 3V3, 3V3S, VIN |
| Señal | **1,0 mm** | SPI, I2C, control |
| Ramal de aterrizaje | el del pad | el último tramo, cuando el pad es más angosto que el riel |

Los dos números los fija la cinta de 5 mm (ver "La cinta manda"): más ancho
que 2,3 mm no se puede forrar con una canaleta de 1,2 mm de hondo. El de
señal bajó de 1,2 a 1,0 por una razón distinta y más tonta: con 1,2 mm el eje
de una canaleta quedaba a 1,94 mm del pad vecino y el ruteador pide 1,90, así
que dos líneas del SPI no encontraban camino **ni con la placa vacía**.

**Riel ancho, ramal fino.** Un riel de 2,2 mm que baja a un pad de 1,2 mm de
un SOT-23 pasa por fuerza a 1,6 mm del pad de al lado, que está a 2,3: no
entra. Pero angostar el tramo entero estrangularía el riel. Así que cada
recorrido se **parte**: mientras aguanta va ancho, y la punta que entra al
pad va del ancho del pad. Es lo que se hacía a mano, y ahora lo hace el
ruteador solo.

**La corriente no es el problema; la resistencia tampoco.** Una pista de
1,0 mm × 0,035 mm tiene 0,49 mΩ por milímetro y un riel de 2,2, 0,22: los
70 mm más largos de riel son 15 mΩ, o **5 mV al pico de wifi de 350 mA**. Lo
que manda el ancho es la cinta, no el cobre.

**Las uniones se sueldan, siempre.** El adhesivo de la cinta es conductor
"de a ratos": sirve para pegar, no para conducir. Cada empalme entre dos
tramos y cada pata de componente lleva estaño. Con **Sn42Bi58 (138 °C)** y
flux la cinta no se despega ni el PETG se deforma; con estaño común
(183–190 °C) también sale, pero con toques de menos de dos segundos y la
punta a 260 °C, no más.

**Masa.** No hay plano de masa: no se puede con una cara y cinta cortada a
mano. Lo que hay es una **barra de masa** de 2,2 mm que recorre el perímetro
y sube por los dos costados. Cada tira tiene su pin de masa sobre ella o a un
tramo corto, y los pines de masa de las tiras de sensor miran todos **hacia
afuera**, hacia la barra, mientras los de señal miran hacia adentro, hacia el
ESP32 (ver "Las tiras de pines").

**La masa es la que paga los puentes.** De los 24 puentes de la v3.0, diez
son de masa y cuatro de 3V3: son las dos redes con quince y diecisiete nodos
repartidos por toda la placa, y en una sola cara eso no se cierra sin cruces.
Se eligió a propósito que los pague la masa: un cable de masa es el más
inofensivo de todos —no tiene señal que degradar, no importa por dónde vaya,
y si hiciera falta se puede reforzar con otro en paralelo—. Todas las señales
quedaron en cobre.

### El SPI

SCK y MOSI bajan desde la SuperMini hasta los bornes de la pantalla en
**19 mm**, rectos, con la barra de masa al costado. A 40 MHz eso no es un
problema. Si igual aparece basura en la imagen, se baja el reloj desde
`platformio.ini` con `-DRK_TFT_SPI_HZ=20000000` — sin tocar una línea de
código — antes de sospechar del hardware.

### Los puentes

Una sola cara de cobre significa que algunos cruces no se pueden evitar. Se
resuelven con **cable aislado fino (AWG30) por arriba**: 24 puentes, todos
listados en [conexiones.md](conexiones.md) y dibujados en la plantilla con
línea azul de puntos. No hay ninguno que no esté en esa lista, y la prueba de
conectividad los cuenta: si falta uno, la red queda en dos pedazos y el
verificador lo dice por nombre.

**De 38 a 24.** La v2.0 tenía 38. Los catorce que se fueron salieron de tres
cambios, y vale la pena saber cuál dio cuánto porque no es intuitivo:

| Cambio | Puentes |
|---|---:|
| v2.0, 62 × 92, pads escalonados, masa primero | 38 |
| Sacar la ventana y clavar todo en tiras de pines | 31 |
| Elegir de qué lado sale cada pad, y girar la pantalla 180° | 27 |
| Rutear las señales primero y los rieles al final | 24 |

Lo que **no** dio nada fue agrandar la placa: entre 56 × 78 y 68 × 92 el
ruteador entregó el mismo número. El problema nunca fue el lugar, fue la
topología —de qué lado sale cada pad y en qué orden entra cada red—. Es
tentador resolver una placa apretada agrandándola; acá no habría servido.

**Los catorce que quedan son de masa y de 3V3** (diez y cuatro). Los otros
diez están repartidos de a uno o dos en redes chicas, y dos de ellos son
deliberados:

- **SCL.** El C3 trae SDA en una fila de pines y SCL en la otra, así que el
  bus I2C nace partido en dos mitades de la placa. Una de las dos tiene que
  cruzar sí o sí; cruza SCL, que es la que menos cuesta.
- **OW.** El 1-Wire sale de GPIO8, que está en el medio de la fila izquierda,
  justo detrás del abanico de cuatro líneas del SPI. Cualquier camino hacia
  abajo cruza ese abanico. Se prefirió un cable de 1-Wire —una señal lenta,
  de 15 kbit/s, con pull-up— antes que meter un cruce en el SPI de 40 MHz.

**Ninguno cruza un hueco.** El cable de un puente corre por la cara de los
módulos. Si fuera derecho por encima de la muesca de la antena o de un
agujero de tornillo, quedaría apretado. Hay una regla que no deja que eso
pase por descuido: los que tienen que rodear llevan su camino anotado en el
dato y dibujado en la plantilla.

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

Pegar setenta y seis tramos de cinta uno por uno es media tarde y setenta y
seis oportunidades de correrse. La estampadora es **el negativo del sustrato**:
las mismas canaletas, pero en relieve. Se apoya una hoja de cinta de cobre
sobre el sustrato, se baja la estampadora encima y se aprieta: todas las
pistas entran a la vez.

Sale del mismo dato que el sustrato, así que no se puede desincronizar: si
mañana se mueve una pista, `make pcb` rehace las dos piezas.

| | |
|---|---|
| Medidas | **67 × 97 × 7,5 mm** (el sustrato más el faldón) |
| Material | PETG, o PLA: no se suelda nada encima, sólo tiene que ser rígida |
| Espesor de la placa | **5 mm**, para que no flexione al apretar |
| Nervaduras | **1,2 mm de alto**, 0,2 mm más finas que la canaleta |
| Faldón | 2,5 mm de alto, 0,5 mm de holgura: centra la pieza sola |
| Orientación de impresión | nervaduras **hacia arriba**, sin soportes |

### Los tres números que la hacen funcionar

**Va espejada en X.** Se imprime con las nervaduras hacia arriba —que es la
única forma de que salgan sin soportes— y se usa dada vuelta. Dar vuelta
espeja. Por eso el modelo se construye entero dentro de un `mirror([1,0,0])`:
un pad que en el sustrato está en *x* se imprime en *(ancho − x)* y al
voltear la pieza vuelve a caer en *x*. Es un error que no se ve mirando el
modelo —la pieza imprime igual de bien— y por eso tiene su propia prueba.

**La nervadura sobresale 1,0 mm más de lo que hunde la canaleta.** Al
apretar, la punta toca el fondo y la cara plana de la estampadora queda
**1,0 mm separada** de la cara del sustrato. Ese aire hace dos cosas. La
primera, de siempre: que la cinta se pegue *sólo adentro de las canaletas* y
no sobre las paredes que las separan —si las dos caras se tocaran, la cinta
quedaría pegada en todos lados y habría que despegarla justo donde no hay que
romperla—. La segunda es nueva en la v3.0: cuanto más entra la nervadura, más
se estira la cinta sobre el filo de la canaleta y más cerca queda de
**cortarse ahí sola**. Con la canaleta de 1,2 mm, la nervadura mide **2,2 mm
de alto**.

De 0,4 a 1,0 hay un efecto colateral que hay que acordarse de seguir: el
faldón perimetral, que es lo que centra la pieza sobre el sustrato, envuelve
el borde desde la cara de la estampadora hacia abajo. Si la cara queda 1,0 mm
más arriba, el faldón engancha 1,0 mm menos. Por eso `faldon_alto` pasó de
2,5 a **3,5 mm**: sigue entrando los mismos 2,5 mm en los 3 mm de espesor del
sustrato, y sigue sin apoyar en la mesa.

**La nervadura es 0,15 mm más fina por lado.** Ahí entran el espesor de la
cinta doblada contra las dos paredes (0,035 mm cada una) y la tolerancia de
impresión. Era 0,25 hasta la v2.0; bajarlo a 0,15 es acercar el filo de la
nervadura al borde de la canaleta todo lo que una boquilla de 0,6 permite.
La nervadura más fina de todo el juego mide **1,15 mm** —la de las pistas de
señal— y el verificador falla si alguna baja de 0,95.

> **Si la estampadora agarra y no baja, el número a subir es ése**, de a
> 0,05. Es el único parámetro del juego que depende de cómo salga la
> impresora, y por eso está solo en el JSON (`estampadora.holgura_lateral`)
> en vez de repartido por la geometría.

**No corta: marca.** Conviene decirlo con números para no esperar lo que no
va a pasar. Una matriz de corte para chapa de 0,06 mm trabaja con una luz
entre punzón y matriz de unas pocas micras. Acá la luz es de 150 micras, unas
cincuenta veces más. Así que la nervadura **marca** la cinta contra el borde
de la canaleta, la adelgaza y la deja lista para romperse ahí; el corte de
verdad lo hace **la lija**, después, al ras de la cara. Lo que la estampadora
tiene que garantizar es que la cinta esté *bien hundida y bien marcada* en
todo el recorrido, y eso sí lo hace de una prensada.

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

Y están calibradas. Corriendo la estampadora 0,5 mm, el choque salta con
**1820 triángulos de contacto**; volteándola sobre el eje equivocado —el
error del espejo, que no se ve mirando el modelo porque la pieza imprime
igual de bien— salta con **3600**. No es una prueba que pase sola.

### Cómo se usa

En [armado.md](armado.md), paso 2. Son cuatro movimientos:

1. **Se cubre la cara entera de cinta**, tira al lado de tira, sin dejar
   claros sobre ninguna canaleta. No hace falta apuntar: lo que sobra se va
   a ir lijado. Las tiras del rollo de 5 mm se pisan un milímetro entre sí.
2. **Se prensa con la estampadora**, hasta que el faldón envuelve el borde y
   se hace tope. Las nervaduras hunden la cinta al fondo de cada canaleta y
   la marcan contra los dos bordes.
3. **Se levanta y se lija la cara**, al ras, con lija fina sobre un taco
   plano. El cobre de la superficie —el que está apoyado sobre las paredes
   entre canaletas— se va; el que está 1,2 mm más abajo, adentro de los
   canales, no lo toca la lija. Ahí es donde se separan de verdad las pistas.
4. **Se controla con el tester**, canaleta contra canaleta vecina: tiene que
   dar abierto. Si alguna da continuidad, faltó lija en ese tramo.

Ese orden —cubrir todo, prensar, lijar— es lo que hace que no haya que
cortar cinta con bisturí ni apuntar tramo por tramo, que era lo que se hacía
hasta la v2.0.

> **La prueba en seco no es opcional.** Antes de poner la cinta, apoyar la
> estampadora sobre el sustrato vacío: tiene que bajar hasta que el faldón
> envuelva el borde, sin resistencia. Si hace tope antes, está al revés o la
> impresión salió con las nervaduras gordas —y con 0,15 mm de luz por lado
> eso es más probable que antes: ver `estampadora.holgura_lateral`—.
> **Nunca forzar**: las nervaduras se parten.

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

Ahora es al revés que antes. Hasta la v2.0 el sustrato posicionaba la
pantalla y el cargador, y la carcasa se acomodaba a eso. En la v3.0 el
sustrato sólo se pide a sí mismo: **cuatro tornillos y aire**. Todo lo demás
—dónde va la pantalla, dónde el capacitivo, dónde la celda— lo decide la
carcasa, y los módulos llegan hasta ahí clavados en sus tiras de pines y con
el cable que haga falta.

| Cota | Valor |
|---|---|
| Sustrato | **68 × 92 × 3,0 mm** |
| Postes de la carcasa | **M2**, Ø2,4 mm, a (3, 11), (65, 11), (3, 81) y (65, 81) desde la esquina inferior izquierda, mirando la cara de las canaletas |
| Aire hacia la cara de los módulos | **12 mm**: 4 de la SuperMini más su tira de pines, y lugar para los conectores que se claven |
| Aire hacia la cara de las canaletas | **6 mm** para la cinta, los 24 puentes y las soldaduras |
| Antena | 10 mm de aire arriba y a los costados, sin metal, y la muesca del sustrato despejada |
| Pantalla TFT 2,2" | módulo de **56 × 40 × 11 mm**; se clava en `J_TFT` y queda parada sobre el sustrato. Su ventana la define la carcasa |
| Capacitivo de suelo | **98 × 23 × 1,5 mm**, sale por abajo, junta abajo y el cable haciendo panza |
| Celda 18650 + portapilas | **75 × 21 × 19 mm**, parada, en compartimento **separado** |
| Cargador TP4056 + interruptor | fuera del sustrato, en su propio bolsillo; llegan al sustrato con dos cables a `J_PWR` |
| USB de la SuperMini y botón BOOT | accesibles abriendo la carcasa, no desde afuera |

> **Los cuatro tornillos son provisorios.** Están puestos donde no estorban
> al ruteo, no donde convenga a una carcasa que todavía no existe. Cuando
> Rocío modele las de Kip, Nori, Blink y Plum, lo más probable es que haya
> que moverlos: son cuatro números en `sustrato.tornillos` y un `make pcb`.
> Lo que **no** se puede mover sin pensarlo es meterlos en las esquinas: un
> agujero en una esquina, con su aire de 0,8 mm, tapa justo la franja por
> donde los dos rieles dan la vuelta, y eso se paga en puentes de masa.

> **Esto cambia lo que Rocío tiene modelado.** La placa es más grande que la
> v2.0 (68 × 92 contra 62 × 92), los postes se movieron y los tornillos
> volvieron a M2. Y sobre todo: **ya no hay ventana de pantalla en el
> sustrato**, así que la carcasa tiene que sostener el panel por su cuenta.
> Hay que avisarle antes de que modele sobre las viejas.

## Lo que se probó de verdad, y lo que no

**Probado (corre en CI, en cada commit):**

- La geometría entera: separaciones, anchos, conectividad de las 23 redes,
  bordes, tornillos, recortes, zona de antena, rótulos.
- Que ninguna canaleta pide más cinta de la que trae el rollo de 5 mm.
- Que `placa.h` y la netlist dicen lo mismo, pin por pin, en las dos placas.
- Que el STL sale de OpenSCAD sin errores y que los generados están al día.

**Sin probar, porque hace falta tener las piezas en la mano** — en orden, y
todo esto está en [armado.md](armado.md) y en la Fase 2 del
[roadmap](roadmap.md):

1. **Una pieza de prueba antes que el sustrato entero**: un cupón de
   40 × 40 mm con canaletas de 1,0 / 1,4 / 2,2 mm a 1,0 / 1,2 / 1,4 mm de
   profundidad, y su estampadora con `holgura_lateral` de 0,15 / 0,20 / 0,25.
   Ahí se contestan de una vez las tres preguntas que quedan abiertas del
   proceso: **hasta dónde se puede hundir la canaleta sin que la cinta se
   rompa en el piso** en vez de en el borde, **con cuánta luz lateral entra
   la nervadura** sin agarrar, y **cuánta lija hace falta** para que dos
   canaletas vecinas den abierto. Es una hora de impresora y evita imprimir
   cinco sustratos mal.
2. **Una unión soldada de prueba**: Sn42Bi58 sobre cinta pegada en PETG, y
   tirar. Si el PETG se marca, bajar la punta o pasar a remaches.
3. La huella de la SuperMini contra la placa real (paso 1 del armado): las
   dos filas de ocho a 2,54 mm y a 15,24 mm entre filas, y si la tira está
   corrida 0,76 mm del centro hacia el USB.
4. **El orden de los pines de cada módulo.** Es lo que hay que mirar antes de
   clavar nada, porque los clones cambian el orden entre lotes: el SHT21
   (VIN GND SCL SDA), el capacitivo (AOUT GND VCC), el TTP223 (IO GND VCC) y
   la TFT de 2,2" (VCC GND CS RESET DC SDI SCK LED SDO). Si alguno no
   coincide, se corrige la lista `pines` de ese componente en el JSON y se
   corre `make pcb`: la placa se reordena sola.
5. Qué es el pin LED de la TFT de 2,2": si ya trae resistencia en serie,
   `R11` se puebla con 0 Ω; si va directo a los LED, con 47–100 Ω.
6. Si el pin 5V de la SuperMini está unido a su VBUS.
7. Cuánto mide el capacitivo seco: si pasa de **2,5 V**, la lectura se
   satura en el ADC del C3 a 11 dB. Remedio: un divisor 100 k / 220 k
   soldado en los propios bornes de `J_SUELO` (hay lugar y los bornes tienen
   agujero), y el mismo número cargado en `RK_SUELO_DIVISOR_NUM/DEN` para que
   el firmware lo sepa.
8. El consumo en reposo medido, contra los 68 µA de la tabla.
9. El RSSI dentro de la carcasa.

## Preguntas abiertas para Iñaki

Todas están diseñadas en una dirección y el diseño sigue adelante con ella;
cambiar de idea es editar el JSON y correr `make pcb`.

1. **¿Un USB o dos?** Se diseñó con dos (el del cargador afuera, el de la
   SuperMini adentro) y la regla del interruptor. Recomendación: uno solo en
   la Fase 5, con multiplexor de alimentación.
2. **¿Celda reemplazable por el usuario?** Se diseñó con carga USB-C y celda
   de servicio. Recomendación: dejarlo así y corregir `decisiones.md`, que
   quedó viejo.
3. **¿La pantalla del producto pasa a ser la de 2,2"?** El sustrato de la
   v3.0 tiene una tira de **nueve** pines, que es la de la TFT de 2,2"
   ILI9341 240 × 320 —la que hay sobre la mesa—. La de 1,44" del producto
   tiene ocho y otro orden: **no entra en esta tira**. El firmware retiró el
   ILI9341 en la 0.6.0 y hay que volver a sumarlo como variante (es su bloque
   en `placa.h` y su clase en `pantalla.cpp`). Recomendación: mantener la de
   1,44" como producto y la de 2,2" como **banco**, y decidir cuál es el
   producto recién cuando haya carcasa. Si la respuesta es "la de 1,44"",
   esta tira pasa a ocho pines y es un cambio de tres líneas en el JSON.
4. **¿El sensor de aire pasa a ser el SHT21?** El firmware habla AHT20 en
   0x38; el módulo que hay es un SHT21/HTU21/Si7021 en 0x40, con otro
   protocolo. La tira de cuatro pines les sirve a los dos —VIN GND SCL SDA—
   así que el sustrato no toma partido, pero el firmware sí tiene que elegir.
   Recomendación: agregar el SHT21 como segundo driver seleccionable desde
   `platformio.ini`, no reemplazar al AHT20.
5. **¿Los tornillos van donde están?** Ver la nota de la sección anterior.

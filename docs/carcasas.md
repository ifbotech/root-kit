# Carcasas

Este documento es para quien modela. Tiene los números que la carcasa tiene
que respetar y nada más: la forma es decisión de arte, el envolvente es
decisión de física.

**La carcasa es el personaje.** La carcasa es el cuerpo (y la cabeza) del
Rooti; la pantalla sólo pone la cara. Si la silueta impresa no se distingue
de las otras cuatro desde el otro lado de una habitación, la cara no va a
salvarla — y si se distingue, la cara la completa.

## El envolvente

Todo va montado sobre tres piezas compradas. Estas medidas son las que hay que
respetar; el resto del volumen es libre.

| Pieza | Medida | Nota |
|---|---|---|
| Pantalla TFT 1,44" | módulo **28 × 37 mm**, PCB ~1,6 mm | Área activa **25,9 × 25,9 mm** |
| ESP32-C3 SuperMini | **22,5 × 18 × 4 mm** | La antena cerámica va en un borde |
| Portapilas 18650 | **75 × 21 × 19 mm** | Es la pieza que manda el tamaño |
| *(envolvente que usa el modelo)* | celda **23 × 76 × 21**, módulo **30 × 39 × 6** | las medidas de arriba más la holgura de montaje, con 1,6 mm de pared alrededor |
| Sensor capacitivo | **98 × 23 × 1,5 mm** | Sale por abajo, clavado en la tierra |

La celda es lo más grande de todo. Cualquier carcasa que quiera ser chica
tiene que resolver primero dónde va el 18650, y la respuesta natural es
**abajo, en vertical, haciendo de pie** — eso baja el centro de gravedad, que
en un objeto que vive clavado en tierra blanda no es un detalle estético.

### La ventana de la pantalla

| Cota | Valor | Por qué |
|---|---:|---|
| Apertura mínima | **26,5 × 26,5 mm** | Área activa + 0,3 mm por lado |
| Apertura máxima | **27,5 × 27,5 mm** | Más allá se ve el borde negro del panel |
| Profundidad al vidrio | **2,2 mm** | PCB + componentes traseros |
| Marco de apoyo | **1,5 mm** por lado | Donde apoya el módulo |

Si la apertura queda por debajo de 26,5 mm se come pixeles de la cara. Los
ojos viven entre el 30% y el 58% del alto, así que **un recorte de arriba se
come la frente y no molesta; uno de abajo se come la boca y sí.** Ante la
duda, correr la ventana 0,5 mm hacia arriba.

## Tolerancias para FDM

Medidas pensadas para una impresora de filamento común con boquilla de 0,4 mm.

| Qué | Holgura | Nota |
|---|---:|---|
| Encastre a presión | **0,20 mm** | Por cara, no en total |
| Encastre a rosca | **0,35 mm** | |
| Poste para tornillo M3 | **Ø2,4 mm** | El tornillo hace su rosca. Era M2/Ø1,7 hasta el sustrato v2.0 |
| Paso de cable | **Ø4 mm** mínimo | Dupont con funda |
| Pared mínima | **1,6 mm** | Cuatro perímetros a 0,4 |

**Sin soportes.** Cada modelo tiene que poder imprimirse en una sola pieza sin
soporte, o partido en dos piezas que encastren. Un soporte que hay que romper
con pinza en la cresta de sesenta unidades es media hora por unidad, y a esa
escala eso decide si el producto existe.

### Lo que el agua obliga

El aparato vive al lado de tierra que se riega. No hace falta que sea
estanco, pero sí:

- **Nada de aperturas mirando hacia arriba.** Ni la ranura del visor.
- **Goterón** en el borde inferior de la ventana: un labio de 0,8 mm que
  despega la gota del vidrio.
- **El compartimento de la celda no comparte volumen con la electrónica.**
  Un 18650 mojado es un incidente, no una falla.
- **La junta del sensor va abajo y con el cable haciendo panza**, para que el
  agua que corra por el cable gotee antes de llegar a la placa.

## El personaje y la carcasa son dos objetos

Al principio se intentó que fueran el mismo: que el modelo 3D de la app saliera
tal cual en STL y ésa fuera la carcasa. Salió mal, y conviene que quede
escrito. Atar el diseño del personaje a que se imprimiera sin soportes dejó
cinco cuerpos redondos, correctos y sin gracia: sin patitas separadas, sin
bracitos, sin sombrero volador. Un Rooti que no se puede querer no sirve.

Así que ahora son dos:

* el **personaje** (`root-lab/public/lib/rooti3d/formas.mjs`) se esculpe para
  verse bien. Es lo que gira en el teléfono;
* la **carcasa** —esto— tiene que alojar la celda, el módulo del TFT y la
  electrónica, apoyarse sin volcarse y salir de la impresora. Se modela en el
  CAD tomando del personaje la silueta y el carácter.

Para eso, `npm run carcasas` en root-lab escribe en [`carcasas/`](../carcasas/)
los cinco personajes en STL, **como referencia de forma**: para tenerlos a mano
mientras se modela, no para imprimirlos como aparato.

## Imprimir sin soportes: las reglas

Valen para las carcasas. El personaje de la app no las cumple, y no tiene
por qué: son dos objetos (ver más arriba).

| Regla | Valor | Por qué |
|---|---|---|
| **Voladizo máximo** | **45°** respecto de la vertical | lo que FDM imprime sin soporte con boquilla de 0,4 mm; un ala de sombrero plana (90°) no sale |
| **Base** | plana, al menos 45 % del ancho | apoya en la cama y la maceta no se vuelca |
| **Centro de gravedad** | en la mitad de abajo, sobre la base | la 18650 va parada, abajo, haciendo de pie |
| **Qué es la carcasa** | el cuerpo o la cabeza del Rooti | la pantalla no es un marco pegado: es una ventana del personaje |
| **Ventana del TFT** | el panel entra **desde atrás** y apoya en un marco; la abertura va **biselada a 45°** hacia afuera | el bisel no es voladizo (mira hacia arriba y hacia afuera) y no tapa pixeles en diagonal; el área activa es de 25,9 × 25,9 mm en el panel de 1,44" |

**Cómo se verifica.** A ojo y con el laminador, que para esto alcanza: se abre
el STL de la carcasa, se mira la vista previa de soportes y no tiene que
proponer ninguno. Lo que sí está automatizado es la parte del personaje
(`root-lab/test/rooti3d.test.mjs`): que las mallas estén cerradas y del
derecho, que apoyen en el piso y que tengan proporción de criatura.

Tres cosas que conviene tener a mano al modelar:

* **Un bulto redondo no se imprime**: la panza mirando al piso necesita
  soporte. Un cono de 45° hacia abajo, sí.
* **Un ala horizontal tampoco.** El sombrero del Champi, tal como está en la
  app, hay que resolverlo en la carcasa: o se abre a 45° o se parte en dos
  piezas que encastran.
* **La zona de la pantalla tiene que quedar plana**, porque el TFT es una
  plaquita rígida: nada de costillas ni curvas fuertes justo ahí.

## Los cinco Rooties

Los parámetros de cada cara están en `firmware/core/persona.c`, una fila por
Rooti con sus tres pieles. Las figuras, en
`root-lab/public/lib/rooti3d/formas.mjs`. Las dos son tablas de números: se
pueden ajustar sin tocar una línea de lógica.

| Rooti | Figura | Rasgo que manda | Tamaño del personaje (mm) |
|---|---|---|---|
| **Kip** | cabeza redonda y grande, cuerpito compacto | la cresta de tres rulos | 82 × 141 × 72 |
| **Nori** | cabeza ovalada, hombros marcados | el corte bob con flequillo | 86 × 116 × 80 |
| **Blink** | la cabeza es casi todo el bicho | los dos cuernitos, y adentro un ojo enorme | 87 × 133 × 74 |
| **Plum** | un solo volumen con forma de gota | el cabito con su hojita | 86 × 121 × 73 |

Son las medidas del PERSONAJE, no de la carcasa: la carcasa va a ser más
grande, porque adentro entra la celda.

Ninguna tiene un solo voladizo por encima de 45° ni una pieza que empiece en
el aire. El centro de masa es el de la carcasa vacía: con la celda puesta baja
todavía más.

**Las pieles no cambian la carcasa.** La rareza es de color y de adornos en
la pantalla y en la app; el cuerpo impreso es el mismo. Una edición especial
de filamento para las épicas queda como idea para más adelante.

### Qué hace distinguible a un Rooti

Dos reglas, y las dos salieron de mirar la lámina de las caras juntas:

1. **La silueta manda.** Se reconoce a tres metros, antes que cualquier
   detalle. Si dos Rooties tienen el mismo contorno, son el mismo Rooti con
   dos texturas.
2. **Un solo rasgo dominante por Rooti.** El Brote son las hojitas, el
   Pinchito el brazo que saluda, el Champi el sombrero. Un personaje con tres
   ideas buenas se lee peor que uno con una sola llevada al extremo.

## Cómo sabe el aparato qué carcasa lleva

**Se lo graban en fábrica.** La estación que ensambla la carcasa escribe el id
del Rooti en la NVS del aparato (`persona`), junto con su secreto. El aparato
lo informa en cada sincronización y la app lo reconoce apenas se vincula: el
usuario no declara nada. El cofre sortea después la piel (la rareza), que la
nube le manda al aparato y queda también en la NVS.

Si una placa no tiene persona grabada (prototipos, placas de desarrollo), la
nube le asigna siempre el mismo Rooti, elegido por su id.

**Lo que se descartó, y por qué.** La versión elegante es una **resistencia
dentro de la carcasa** leída por un divisor: se cambia la carcasa y la cara
cambia sola. Cuesta una resistencia, pero un pin de ADC, y el C3 SuperMini
tiene sus trece pines ocupados (ver [hardware.md](hardware.md#conexiones)). Es
candidata para la PCB propia de la Fase 5 del [roadmap](roadmap.md), donde el
C3 en módulo expone más pines.

## Antes de imprimir sesenta

Tres cosas que sólo se saben con una pieza en la mano:

1. **Una carcasa cruda, sin detalle, para verificar el encastre** de los tres
   componentes y el paso del cable. Es la que va a estar mal.
2. **La ventana contra el módulo real.** Los 0,3 mm de holgura por lado son
   teoría hasta que se apoya el vidrio.
3. **Una cara encendida adentro de la carcasa, de noche.** El ala del
   sombrero del Champi le tira sombra a la ventana, y las pieles son pastel:
   hay que confirmar que los ojos de cada piel siguen leyéndose a través del
   bisel. Si no, se oscurece el color `ojos` de esa piel en `persona.c` —un
   número— y listo.

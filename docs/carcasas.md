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
| Poste para tornillo M2 | **Ø1,7 mm** | El tornillo hace su rosca |
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

## Imprimir sin soportes: las reglas

Valen para las cinco carcasas y para las siluetas de los Rooties en la app,
que son las mismas (`root-lab/public/lib/cuerpo.mjs`).

| Regla | Valor | Por qué |
|---|---|---|
| **Voladizo máximo** | **45°** respecto de la vertical | lo que FDM imprime sin soporte con boquilla de 0,4 mm; un ala de sombrero plana (90°) no sale |
| **Base** | plana, al menos 30 % del ancho | apoya en la cama y la maceta no se vuelca |
| **Centro de gravedad** | en la mitad de abajo, sobre la base | la 18650 va parada, abajo, haciendo de pie |
| **Qué es la carcasa** | el cuerpo o la cabeza del Rooti | la pantalla no es un marco pegado: es una ventana del personaje |
| **Ventana del TFT** | el panel entra **desde atrás** y apoya en un marco; la abertura va **biselada a 45°** hacia afuera | el bisel no es voladizo (mira hacia arriba y hacia afuera) y no tapa pixeles en diagonal; el área activa es de 25,9 × 25,9 mm en el panel de 1,44" |

**Cómo se verifica.** Las siluetas se describen como puntos y se suavizan con
Catmull-Rom; `test/cuerpo.test.mjs` en root-lab muestrea la curva que se
dibuja (no los puntos) y mide el tramo que más mira hacia abajo: tiene que
dar 45° o menos. También verifica la base plana, que el centroide quede abajo
y sobre la base, y que la ventana con su bisel entre entera en la silueta.
Donde una curva suave se pasa (el ala del Champi), se usa una **esquina**: el
ala termina en un canto a 42,5° en vez de un redondeo a 56°.

## Los cinco Rooties

Los parámetros de cada cara están en `firmware/core/persona.c`, una fila por
Rooti con sus tres pieles, y se pueden ajustar sin tocar una línea de lógica.
Las siluetas, en `root-lab/public/lib/cuerpo.mjs`. Lo que sigue es la
intención; los números concretos viven ahí.

| Rooti | Silueta | Relieves | La cara que le hace juego | Voladizo |
|---|---|---|---|---:|
| **Brote** | semilla redonda con dos hojitas en V arriba | nervaduras en las hojas, hojitas-brazo a los costados, raíces-pie | ojos redondos enormes con brillos de cachorro, sin cejas | 37,7° |
| **Musgo** | domo bajo y ancho, el más estable | matas de musgo, manitos sobre la panza, un botón de flor arriba | ojos en medialuna "u u", boca de gato; calma | 35,6° |
| **Pinchito** | cactus columnar con un brazo en alto (saluda) y un bracito del otro lado | costillas verticales, espinas, flor arriba | ojos en arco "^ ^" que guiñan, sonrisa con dientecito | 38,6° |
| **Bulbo** | gota de cebolla con la punta arriba | espiral en la punta, collar de pétalos, gajos | ojos grandes con doble brillo, cejas flotantes, rubor suave | 31,4° |
| **Champi** | sombrero de hongo sobre un tallo | manchas del sombrero, laminillas debajo del ala | cejas finas, boca ":D" con lengua, pecas | 42,5° |

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

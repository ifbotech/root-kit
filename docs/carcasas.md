# Carcasas

Este documento es para quien modela. Tiene los números que la carcasa tiene
que respetar y nada más: la forma es decisión de arte, el envolvente es
decisión de física.

**La carcasa es el personaje.** La pantalla sólo pone la cara. Si la silueta
impresa no se distingue de las otras cinco desde el otro lado de una
habitación, la cara no va a salvarla — y si se distingue, la cara la completa.

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

## Los ocho modelos

Los parámetros de cada cara están en `firmware/core/persona.c`, una fila por
modelo, y se pueden ajustar sin tocar una línea de lógica. Lo que sigue es la
intención; los números concretos viven ahí.

| Modelo | Rareza | Silueta | La cara que le hace juego |
|---|---|---|---|
| **Cresta** | común | Mohicano de hojas puntiagudas, ancho arriba | Ojos angostos e inclinados, cejas despeinadas en tres trazos, dentadura, un colmillo |
| **Kawaii** | común | Melena con flequillo recto y dos hojas como coletas | Ojos rasgados que al sonreír son dos arcos `^ ^`, boca de gato, rubor, destellos |
| **Visor** | común | Cúpula lisa con una única ranura horizontal | Sin ojos: una banda cuya onda es la expresión, plana si está bien, dentada si hay alerta |
| **Ciclope** | raro | Una sola apertura circular grande, tipo ojo de buey | Un ojo enorme con pupila gigante que deriva sola; boca mínima |
| **Hongo** | raro | Sombrero que vuela por encima y da sombra a la pantalla | Párpados a media asta siempre, esporas subiendo |
| **Chico Malo** | común | *A definir con Rocío.* Idea: capucha o gorra hacia atrás con una hoja rebelde | Ojos angostos e inclinados, cejas gruesas y bajas, sonrisa de costado con un colmillo, una curita en el cachete. Rojos de brasa (paleta Chico Malo) |
| **Chica Chill** | común | *A definir con Rocío.* Idea: rodete con un lápiz clavado, o auriculares | Párpados relajados, anteojos redondos, cejas finas, sonrisa chica. Azules de medianoche (paleta Chica Chill) |
| **?????** | secreto | **Filamento translúcido**: se ve la placa por dentro | Ojos que no terminan de decidirse, estática |

### Por qué el secreto es translúcido y no dorado

Un dorado es un filamento más caro y una unidad que hay que separar en la
producción. El translúcido cuesta lo mismo que cualquier otro y hace algo que
ninguno de los otros cinco hace: **deja de ocultar el aparato y pasa a
exhibirlo.** El que le toca ve la placa, la celda y la pantalla desde afuera.
Es una diferencia de categoría, no de color, y eso es lo que un secreto tiene
que ser.

### Qué hace distinguible a un modelo

Dos reglas, y las dos salieron de mirar la lámina de las caras juntas:

1. **La silueta manda.** Se reconoce a tres metros, antes que cualquier
   detalle. Si dos modelos tienen el mismo contorno, son el mismo modelo con
   dos texturas.
2. **Un solo rasgo dominante por modelo.** Cresta es la cresta, Ciclope es el
   ojo, Hongo es el sombrero. Un modelo con tres ideas buenas se lee peor que
   uno con una sola llevada al extremo.

## Cómo sabe el aparato qué carcasa lleva

**Se lo graban en fábrica.** La estación que ensambla la carcasa escribe el id
del personaje en la NVS del aparato (`persona`), junto con su secreto. El
aparato lo informa en cada sincronización y el cofre de la app lo revela: el
usuario no declara nada, lo descubre.

Si una placa no tiene persona grabada (prototipos, placas de desarrollo), el
cofre tira con las probabilidades públicas y la nube se la asigna.

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
3. **Una cara encendida adentro de la carcasa, de noche.** El sombrero del
   Hongo le tira sombra a la pantalla a propósito, y hay que confirmar que la
   cara oscura de ese modelo sigue leyéndose. Si no, se sube el brillo de su
   paleta en `persona.c` —una línea— y listo.

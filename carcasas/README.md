# Las carcasas, en STL

<!-- GENERADO por root-lab/tools/rooties-stl.mjs. No editar a mano: los STL y
     esta tabla salen del mismo modelo que dibuja la app, así que para cambiar
     una figura se edita root-lab/public/lib/rooti3d/formas.mjs y se vuelve a
     correr el script. -->

Estos cinco archivos son los mismos Rooties que se ven girando en ROOTLAB. No
hay una versión "para imprimir" y otra "para la pantalla": es una sola malla,
en milímetros, y por eso lo que se prueba en cada commit
(`root-lab/test/rooti3d.test.mjs`) vale para el objeto que vas a tener en la
mano.

## Cómo se imprimen

Sin soportes, sin balsa y sin ajustes raros:

* **Orientación**: como vienen. La base plana apoya en la cama y ningún
  voladizo pasa de 45°.
* **Boquilla** 0,4 mm, **capa** 0,2 mm. Nada de la figura es más fino que
  2 mm, o sea cinco hilos.
* **Relleno** 15 % giroide. La celda va adentro: no hace falta más.
* **Perímetros** 3, que es lo que aguanta una caída de la mesa.
* **Material** PLA o PETG. El PETG aguanta mejor el sol de una ventana, que
  es donde va a vivir.

## Qué tiene que entrar adentro

| Pieza | Medida | Dónde |
| --- | --- | --- |
| Celda 18650 con portapilas | 23 × 76 × 21 mm | parada, desde 6 mm del piso, centrada y 6 mm hacia atrás |
| Módulo TFT 1,44" | 30 × 39 × 6 mm | detrás de la cara, a 2.2 mm del frente plano |
| Pared mínima | 1.6 mm | en todo el contorno |

La cama de referencia es de 220 × 250 × 220 mm (una Ender 3 o parecida).

## Los cinco

| Rooti | Archivo | Tamaño (mm) | Triángulos | Base | Centro de masa | Hueco del módulo |
| --- | --- | --- | --- | --- | --- | --- |
| Brote | `brote.stl` | 90.7 × 152.1 × 76 | 5140 | 56 % del ancho | 34 % del alto | 4.9 mm |
| Musgo | `musgo.stl` | 86.4 × 145.5 × 82.1 | 6232 | 67 % del ancho | 36 % del alto | 5.6 mm |
| Pinchito | `pinchito.stl` | 92.8 × 132.3 × 72.3 | 8020 | 48 % del ancho | 42 % del alto | 4.9 mm |
| Bulbo | `bulbo.stl` | 89.7 × 146.6 × 79 | 5108 | 54 % del ancho | 35 % del alto | 5.1 mm |
| Champi | `champi.stl` | 80 × 132 × 76 | 5312 | 69 % del ancho | 28 % del alto | 5.3 mm |

El **centro de masa** es de la carcasa vacía; con la celda puesta baja todavía
más, porque la celda es lo más pesado y va abajo. El **hueco del módulo** es
cuánto hay que rebajar el frente para que el TFT, que es una plaquita rígida y
plana, apoye derecho.

## Lo que falta por hacer a mano

Los STL son el cuerpo. Todavía hay que modelar, y se hace en el CAD del
hardware, no acá:

* la tapa de abajo con sus tornillos y el hueco del USB-C;
* los pilares del PCB y los agarres del portapilas;
* el pasaje de la sonda de tierra;
* los agujeros del sensor de luz y del de temperatura.

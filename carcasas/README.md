# Los Rooties en STL — referencia de forma

<!-- GENERADO por root-lab/tools/rooties-stl.mjs (npm run carcasas). Para
     cambiar una figura se edita root-lab/public/lib/rooti3d/formas.mjs. -->

**Esto no son las carcasas.** Son los cinco personajes tal como se ven en
ROOTLAB, exportados para tenerlos a mano mientras se modela el aparato.

El personaje y la carcasa son dos objetos distintos, a propósito:

* el **personaje** se esculpe para verse bien: tiene patitas separadas, brazos
  levantados y sombreros que vuelan. Varias de esas cosas no salen de una
  impresora sin soporte, y está bien que así sea;
* la **carcasa** tiene que alojar la celda 18650 parada, el módulo del TFT de
  1,44" y la electrónica, apoyarse sin volcarse y salir de la impresora. Se
  diseña aparte, en el CAD del hardware, tomando de acá la silueta y el
  carácter.

## Los cinco

| Rooti | Archivo | Tamaño (mm) | Triángulos |
| --- | --- | --- | --- |
| Brote | `brote.stl` | 83.6 × 129.2 × 62.9 | 27052 |
| Musgo | `musgo.stl` | 90.7 × 98.8 × 72.9 | 21696 |
| Pinchito | `pinchito.stl` | 93.4 × 106.3 × 61.9 | 25292 |
| Bulbo | `bulbo.stl` | 87.7 × 115.8 × 64.1 | 21776 |
| Champi | `champi.stl` | 87.1 × 118 × 83.1 | 32312 |

Las mallas son cerradas y con las normales hacia afuera (volumen con signo
positivo), así que un laminador las acepta sin reparaciones. Si querés
imprimir la figura como adorno, va con soportes y a 0,15 mm de capa.

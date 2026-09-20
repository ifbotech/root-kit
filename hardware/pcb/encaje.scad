// encaje.scad — la prueba de que la estampadora entra en el sustrato.
//
// Pone la estampadora dada vuelta, como se usa, apoyada a fondo sobre el
// sustrato, y calcula la INTERSECCION de las dos piezas. Si el diseño está
// bien, tiene que dar **vacía**: las nervaduras entran en sus canaletas sin
// tocar las paredes, el faldón envuelve el borde sin apretarlo, y las dos
// caras planas quedan separadas por est_sobresalir.
//
//   make pcb            la corre sola si hay OpenSCAD
//   python3 tools/pcb.py --encaje
//
// Cualquier sólido que aparezca acá es plástico contra plástico: la pieza no
// va a bajar hasta el fondo y la cinta no va a entrar en esa canaleta.
//
// Esta prueba existe porque la estampadora tiene un error posible que no se
// ve mirando el modelo: va **espejada en X**, porque se imprime con las
// nervaduras hacia arriba y se usa dada vuelta. Si alguna vez se toca
// `estampadora.scad` y se pierde ese espejo, la pieza sigue pareciendo
// correcta, imprime igual de bien, y no entra. Comprobado: volteada sobre el
// eje equivocado, esta misma intersección da 1254 facetas de choque.

use <sustrato.scad>
use <estampadora.scad>
include <generado/sustrato_datos.scad>

borde = est_faldon_holgura + est_faldon_pared;

// Apoyada a fondo: la punta de la nervadura llega al piso de la canaleta.
// Con la cinta puesta se queda 0,035 mm antes, que es lo que se quiere.
z_uso = sustrato_esp + est_espesor + est_sobresalir;

module estampadora_en_uso() {
    translate([sustrato_ancho + borde, -borde, z_uso])
        rotate([0, 180, 0])
            estampadora();
}

// Dos pruebas, elegidas con -D modo="..." desde tools/pcb.py:
//
//   "choque"    la interseccion de las dos piezas tiene que dar VACIA
//   "presencia" lo que la estampadora mete DENTRO de las canaletas tiene que
//               dar LLENO, y cubrir casi toda la placa
//
// La segunda existe porque la primera sola se puede aprobar por la razon
// equivocada: si las nervaduras desaparecieran, la interseccion tambien daria
// vacia y la prueba pasaria con una pieza que no sirve para nada.

modo = "choque";

// La franja de aire que hay dentro de las canaletas, entre el piso y la cara
// del sustrato. Ahi adentro tiene que haber punta de nervadura.
module franja_canaletas() {
    translate([0, 0, sustrato_esp - canaleta_prof + 0.05])
        linear_extrude(canaleta_prof - 0.10)
            square([sustrato_ancho, sustrato_alto]);
}

if (modo == "choque") {
    intersection() {
        sustrato();
        estampadora_en_uso();
    }
} else {
    intersection() {
        estampadora_en_uso();
        franja_canaletas();
    }
}

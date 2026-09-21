// cupon.scad — la pieza de prueba del proceso de la cinta.
//
// Media hora de impresora que contesta las tres preguntas que el sustrato
// v3.0 deja abiertas y que no se pueden contestar en la computadora:
//
//   1. HASTA DONDE SE PUEDE HUNDIR LA CANALETA. Cuanto mas profunda, mas se
//      estira la cinta sobre el borde y mas cerca queda de cortarse ahi
//      sola. Pero pasado un punto se rompe en el PISO, que es donde tiene
//      que conducir. El cupon trae tres profundidades: 1,0 / 1,2 / 1,4 mm.
//      El sustrato usa 1,2.
//   2. CON CUANTA LUZ ENTRA LA NERVADURA sin agarrar. Cuanto mas ajustada,
//      mejor marca la cinta contra el borde; demasiado y la estampadora no
//      baja. El cupon trae tres: 0,15 / 0,20 / 0,25 mm por lado, en tres
//      franjas horizontales que cruzan TODAS las canaletas, asi que una sola
//      prensada prueba las nueve combinaciones.
//   3. CUANTA LIJA HACE FALTA para que dos canaletas vecinas den abierto, y
//      si la pared minima de 0,8 mm sobrevive a esa lija. El ultimo par de
//      cada grupo esta separado por exactamente 0,8 mm.
//
// De yapa, la fila de ocho agujeros de abajo: es la huella de una tira de
// pines, a 2,54 mm de paso, y va de 1,3 a 2,0 mm de a una decima. Dice cual
// es el diametro mas chico por el que el pin entra SOLO, que no es el mismo
// que el diametro con el que el agujero se ve abierto.
//
// ---------------------------------------------------------------------------
// RESULTADO DE LA PRIMERA CORRIDA (21/09/2026, PETG, boquilla 0,6, capa 0,2)
//
//   Profundidad y luz: gano la combinacion del CENTRO, 1,2 mm de hondo con
//   0,20 mm de luz por lado. Con 1,0 la cinta no se marca lo suficiente
//   contra el borde; con 1,4 empieza a romperse en el piso. Con 0,15 la
//   nervadura agarra y la pieza no baja del todo; con 0,25 la cinta queda
//   floja contra el borde y la lija no la corta pareja. Los dos numeros
//   estan ahora en nucleo.json.
//
//   Agujeros: en esa corrida los ocho eran del MISMO diametro, 1,4 mm --no
//   habia gradiente, era un error de este archivo-- y por tres de ellos el
//   pin no entraba y por los otros cinco entraba perfecto. Ese es justamente
//   el hallazgo: a 1,4 mm nominal el agujero esta en el filo y lo decide la
//   variacion normal de la impresora, porque un agujero impreso sale dos o
//   tres decimas mas chico que el dibujado. El sustrato paso a 1,7 mm. La
//   fila de ahora, con gradiente de verdad, es la que confirma ese numero.
// ---------------------------------------------------------------------------
//
//   make cupon     exporta los dos STL
//
// Se imprimen los dos SIN SOPORTES, con la misma boquilla (0,6), la misma
// altura de capa (0,2) y el mismo material que el sustrato de verdad: si se
// prueba en PLA no se prueba nada, porque el PETG se estira distinto bajo la
// nervadura y se ablanda a otra temperatura.
//
// Los numeros de aca salen de hardware/pcb/nucleo.json y estan escritos a
// mano A PROPOSITO: este archivo es el que se toca para barrer valores, y no
// tiene que arrastrar al sustrato cada vez que se prueba algo.

// "sustrato" | "estampadora" | "choque" | "presencia"
//
// Los dos ultimos son la prueba de encaje, la misma que la placa grande:
//   choque     la interseccion de las dos piezas, apoyadas a fondo, tiene
//              que dar VACIA. Cualquier solido es plastico contra plastico.
//   presencia  lo que la estampadora mete DENTRO de las canaletas tiene que
//              dar LLENO. Sin esta, la primera se aprobaria por la razon
//              equivocada: si las nervaduras desaparecieran, la interseccion
//              tambien daria vacia.
pieza = "sustrato";

$fn = 48;
eps = 0.01;

// ------------------------------------------------------------ el cupon ---
cupon_ancho = 52.0;
cupon_alto  = 44.0;
cupon_esp   = 3.0;

// Las canaletas, de abajo hacia arriba en la pieza.
canaleta_y0 = 6.0;
canaleta_y1 = 38.0;

// Tres grupos, uno por profundidad. Dentro de cada grupo, cinco canaletas:
// las tres anchos del sustrato, y despues un par pegado a 0,8 mm, que es la
// pared minima. El ancho es el de la CANALETA (pista + 0,3 de holgura).
profundidades = [1.0, 1.2, 1.4];
anchos        = [1.3, 1.7, 2.5, 1.3, 1.3];   // pistas de 1,0 / 1,4 / 2,2
paredes       = [1.5, 1.5, 1.5, 0.8];        // entre canaleta y canaleta
grupo_sep     = 3.0;
margen_x      = 2.5;

function ancho_grupo() =
    anchos[0] + anchos[1] + anchos[2] + anchos[3] + anchos[4]
    + paredes[0] + paredes[1] + paredes[2] + paredes[3];

function x_grupo(g) = margen_x + g * (ancho_grupo() + grupo_sep);

// x del borde izquierdo de la canaleta i dentro del grupo g.
function x_canaleta(g, i) =
    x_grupo(g)
    + (i > 0 ? anchos[0] + paredes[0] : 0)
    + (i > 1 ? anchos[1] + paredes[1] : 0)
    + (i > 2 ? anchos[2] + paredes[2] : 0)
    + (i > 3 ? anchos[3] + paredes[3] : 0);

// La fila de agujeros de una tira de pines: mismo paso que una tira de
// verdad, y el diametro creciendo de a una decima. El primero es el mas
// chico. El sustrato usa 1,7, que es el quinto.
agujero_d0   = 1.3;
agujero_paso_d = 0.1;
agujero_paso = 2.54;
agujero_n    = 8;
agujero_y    = 2.6;
agujero_x0   = 10.0;

// ------------------------------------------------------- la estampadora ---
// Las mismas tres holguras laterales del JSON, en tres franjas que cruzan
// todas las canaletas. Cada franja es [y0, y1, holgura].
franjas = [
    [ 7.0, 16.0, 0.15],
    [18.0, 27.0, 0.20],
    [29.0, 37.0, 0.25],
];

est_sobresalir = 1.0;      // cuanto entra la nervadura mas alla de la cara
est_espesor    = 5.0;
est_faldon_alto    = 3.5;
est_faldon_pared   = 2.0;
est_faldon_holgura = 0.5;

borde_faldon = est_faldon_holgura + est_faldon_pared;

// --------------------------------------------------------- el sustrato ---
module canaletas_de(g) {
    // Todas las canaletas de un grupo, hundidas a SU profundidad.
    p = profundidades[g];
    translate([0, 0, cupon_esp - p])
        linear_extrude(p + eps)
            for (i = [0 : len(anchos) - 1])
                translate([x_canaleta(g, i), canaleta_y0])
                    square([anchos[i], canaleta_y1 - canaleta_y0]);
}

module agujeros_de_tira() {
    for (k = [0 : agujero_n - 1])
        translate([agujero_x0 + k * agujero_paso, agujero_y, -eps])
            cylinder(d = agujero_d0 + k * agujero_paso_d,
                     h = cupon_esp + 2 * eps);
    // Los dos extremos rotulados, para no tener que contar con el calibre.
    for (r = [[agujero_x0 - 4.2, agujero_d0],
              [agujero_x0 + (agujero_n - 1) * agujero_paso + 4.2,
               agujero_d0 + (agujero_n - 1) * agujero_paso_d]])
        translate([r[0], agujero_y, cupon_esp - 0.4])
            linear_extrude(0.4 + eps)
                text(str(r[1]), size = 2.0, halign = "center",
                     valign = "center", font = "Liberation Sans:style=Bold");
}

module rotulos_de_profundidad() {
    // Que grupo es cual, grabado 0,4 mm en la cara de las canaletas.
    for (g = [0 : len(profundidades) - 1])
        translate([x_grupo(g) + ancho_grupo() / 2, 40.8, cupon_esp - 0.4])
            linear_extrude(0.4 + eps)
                text(str(profundidades[g]), size = 2.4, halign = "center",
                     valign = "center", font = "Liberation Sans:style=Bold");
}

module cupon_sustrato() {
    difference() {
        linear_extrude(cupon_esp)
            offset(r = 1.5) offset(r = -1.5)
                square([cupon_ancho, cupon_alto]);
        for (g = [0 : len(profundidades) - 1]) canaletas_de(g);
        agujeros_de_tira();
        rotulos_de_profundidad();
    }
}

// ------------------------------------------------------ la estampadora ---
// Va ESPEJADA EN X, por lo mismo que la grande: se imprime con las
// nervaduras hacia arriba (unica forma de que salgan sin soportes) y se usa
// dada vuelta, y dar vuelta espeja. Aca importa de verdad: los tres grupos
// de profundidad no son simetricos, asi que sin el espejo la nervadura de
// 2,0 mm caeria en la canaleta de 1,4 de hondo y al reves.
module contorno_cupon(crecer = 0) {
    offset(r = crecer)
        offset(r = 1.5) offset(r = -1.5)
            square([cupon_ancho, cupon_alto]);
}

module nervaduras3d() {
    for (g = [0 : len(profundidades) - 1])
        for (f = franjas)
            for (i = [0 : len(anchos) - 1]) {
                a = anchos[i] - 2 * f[2];
                if (a > 0)
                    translate([x_canaleta(g, i) + f[2], f[0]])
                        linear_extrude(profundidades[g] + est_sobresalir)
                            square([a, f[1] - f[0]]);
            }
}

module cupon_estampadora() {
    difference() {
        union() {
            translate([cupon_ancho + borde_faldon, borde_faldon, 0])
                mirror([1, 0, 0]) {
                    // cuerpo
                    linear_extrude(est_espesor) contorno_cupon(borde_faldon);
                    // faldon: envuelve el borde del cupon y la centra sola
                    linear_extrude(est_espesor + est_faldon_alto)
                        difference() {
                            contorno_cupon(borde_faldon);
                            contorno_cupon(est_faldon_holgura);
                        }
                    translate([0, 0, est_espesor - eps]) nervaduras3d();
                }
        }
        // Que franja es cual, grabado en el dorso (la cara que queda arriba
        // cuando se usa). Va espejado para que se lea derecho ahi.
        for (f = franjas)
            translate([6.0, f[0] + (f[1] - f[0]) / 2, -eps])
                linear_extrude(0.4 + eps)
                    mirror([1, 0, 0])
                        text(str(f[2]), size = 3.0, halign = "center",
                             valign = "center",
                             font = "Liberation Sans:style=Bold");
    }
}

// ------------------------------------------------------------- el encaje ---
// Apoyada a fondo: la punta de la nervadura llega al piso de la canaleta y
// las dos caras planas quedan separadas por est_sobresalir.
z_uso = cupon_esp + est_espesor + est_sobresalir;

module cupon_estampadora_en_uso() {
    translate([cupon_ancho + borde_faldon, -borde_faldon, z_uso])
        rotate([0, 180, 0])
            cupon_estampadora();
}

// La franja de aire de cada grupo: entre el piso de SU canaleta y la cara.
// Un solo bloque no sirve, porque los tres grupos tienen profundidades
// distintas.
module franja_canaletas() {
    for (g = [0 : len(profundidades) - 1])
        translate([x_grupo(g) - 0.5, 0,
                   cupon_esp - profundidades[g] + 0.05])
            linear_extrude(profundidades[g] - 0.10)
                square([ancho_grupo() + 1.0, cupon_alto]);
}

if (pieza == "estampadora") {
    cupon_estampadora();
} else if (pieza == "choque") {
    intersection() {
        cupon_sustrato();
        cupon_estampadora_en_uso();
    }
} else if (pieza == "presencia") {
    intersection() {
        cupon_estampadora_en_uso();
        franja_canaletas();
    }
} else {
    cupon_sustrato();
}

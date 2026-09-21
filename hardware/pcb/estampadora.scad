// estampadora.scad — el negativo del sustrato, para meter la cinta de una vez.
//
// Las mismas canaletas del sustrato, pero en RELIEVE. Se apoya una hoja de
// cinta de cobre sobre el sustrato, se baja esta pieza encima y se aprieta:
// las nervaduras empujan la cinta al fondo de cada canaleta, todas juntas,
// en vez de pegar los 51 tramos uno por uno.
//
//   make pcb    regenera los datos y exporta los dos STL
//
// Tres cosas que hacen que funcione, y que son las que hay que entender si
// alguna vez se toca este archivo:
//
//   1. VA ESPEJADA EN X. Se imprime con las nervaduras hacia arriba (sin
//      soportes) y se usa dada vuelta, y dar vuelta espeja. Por eso todo el
//      modelo se construye dentro de un mirror([1,0,0]): un pad que en el
//      sustrato esta en x, acá se imprime en (ancho - x), y al voltear la
//      pieza vuelve a caer en x.
//   2. LA NERVADURA SOBRESALE MAS DE LO QUE HUNDE LA CANALETA. Con
//      est_sobresalir de más, al apretar la nervadura toca fondo y la cara
//      plana de la estampadora queda separada del sustrato: la cinta se pega
//      SOLO adentro de las canaletas y no sobre las paredes que las separan.
//      Si las dos caras se tocaran, la cinta quedaría pegada en todos lados y
//      habría que despegarla justo donde no hay que romperla. En la v3 este
//      número pasó de 0,4 a 1,0 mm: cuanto más entra la nervadura, más se
//      estira la cinta sobre el filo de la canaleta y más cerca queda de
//      cortarse sola ahí.
//   3. LA NERVADURA ES MAS FINA QUE LA CANALETA. est_holgura_lateral por lado
//      deja lugar para el espesor de la cinta doblada contra las dos paredes
//      (0,07 mm) más la tolerancia de impresión. En la v3 bajó de 0,25 a
//      0,15: es lo más cerca del corte limpio que se puede pedir con una
//      boquilla de 0,6. No se puede llegar al corte de verdad —una matriz de
//      corte para 0,06 mm de cobre pide unas micras de luz, y eso no sale de
//      una impresora FDM—, así que la cinta se marca acá y se termina de
//      separar LIJANDO la cara. Si la estampadora agarra y no baja, este es
//      el número a subir, de a 0,05.
//
// El faldón perimetral centra la pieza sobre el sustrato: no hay que apuntar
// a ojo. Entra 2,5 mm de los 3 mm de espesor del sustrato, así que nunca
// apoya en la mesa y la profundidad la sigue mandando la nervadura.

include <generado/sustrato_datos.scad>

$fn = 48;
eps = 0.01;

borde_faldon = est_faldon_holgura + est_faldon_pared;

// --------------------------------------------------------------- contornos
module contorno_sustrato(crecer = 0) {
    offset(r = crecer)
        offset(r = radio_borde) offset(r = -radio_borde)
            square([sustrato_ancho, sustrato_alto]);
}

// -------------------------------------------------------------- nervaduras
// El mismo trazado que las canaletas del sustrato, con el ancho reducido.
module tramo(a, b, ancho) {
    dx = b[0] - a[0];
    dy = b[1] - a[1];
    largo = sqrt(dx * dx + dy * dy);
    if (largo > eps) {
        translate([a[0], a[1]])
            rotate([0, 0, atan2(dy, dx)])
                translate([0, -ancho / 2])
                    square([largo, ancho]);
    }
}

module nervadura(ancho, puntos) {
    union() {
        for (i = [0 : len(puntos) - 2])
            tramo(puntos[i], puntos[i + 1], ancho);
        for (p = puntos)
            translate([p[0] - ancho / 2, p[1] - ancho / 2]) square([ancho, ancho]);
    }
}

// `extra` ensancha todo por igual: 0 para la punta, est_base_extra para el
// escalón de la base, que le da rigidez sin llegar a tocar el sustrato.
module nervaduras2d(extra = 0) {
    delta = canaleta_holg - 2 * est_holgura_lateral + 2 * extra;
    for (c = canaletas) nervadura(c[0] + delta, c[1]);
    for (p = pads)
        translate([p[0], p[1]]) rotate([0, 0, p[4]])
            translate([-(p[2] + delta) / 2, -(p[3] + delta) / 2])
                square([p[2] + delta, p[3] + delta]);
}

module nervaduras3d() {
    // Escalón de base, más ancho y bajo: cuando la pieza apoya queda por
    // encima de la cara del sustrato, así que ensancharlo no pega cinta.
    linear_extrude(est_base_alto) nervaduras2d(est_base_extra);
    // La punta, que es la que entra en la canaleta.
    linear_extrude(canaleta_prof + est_sobresalir) nervaduras2d(0);
}

// ------------------------------------------------------------------- pieza
module cuerpo() {
    linear_extrude(est_espesor) contorno_sustrato(borde_faldon);
}

module faldon() {
    linear_extrude(est_espesor + est_faldon_alto)
        difference() {
            contorno_sustrato(borde_faldon);
            contorno_sustrato(est_faldon_holgura);
        }
}

// Alivio sobre los recortes: ahí abajo no hay sustrato, y una cara plana
// empujaría la cinta al vacío y la arrugaría.
// El corte sale por encima de la cara para no dejar caras coplanares: dos
// superficies exactamente a la misma altura son ambiguas y se le notan al
// laminador antes que al ojo.
module alivio(x, y, a, h) {
    translate([x, y, est_espesor - est_relieve_hueco + 1])
        cube([a + 2, h + 2, 2], center = true);
}

module alivios() {
    for (r = recortes) alivio(r[0], r[1], r[2], r[3]);
}

// Todo lo que tiene que coincidir con el sustrato se construye en
// coordenadas del sustrato y se espeja de una vez. El translate deja la
// pieza apoyada en el primer cuadrante: mirror([1,0,0]) manda la x a
// [-(ancho+borde), borde], y el corrimiento la devuelve a [0, ancho+2*borde].
module estampadora() {
    translate([sustrato_ancho + borde_faldon, borde_faldon, 0])
        mirror([1, 0, 0]) {
            difference() {
                union() {
                    cuerpo();
                    faldon();
                }
                alivios();
            }
            translate([0, 0, est_espesor - eps]) nervaduras3d();
        }
}

module rotulo_dorso() {
    translate([(sustrato_ancho + 2 * borde_faldon) / 2,
               (sustrato_alto + 2 * borde_faldon) / 2, -eps])
        linear_extrude(0.4 + eps)
            mirror([1, 0, 0])
                text(est_rotulo, size = est_rotulo_tam, halign = "center",
                     valign = "center", font = "Liberation Sans:style=Bold");
}

difference() {
    estampadora();
    rotulo_dorso();
}

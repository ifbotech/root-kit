// sustrato.scad — el sustrato impreso de ROOTKIT.
//
// Una placa de PETG con canaletas en la cara de arriba: ahí va la cinta de
// cobre. Los módulos van del otro lado y sus patas pasan por los agujeros.
//
// Todo lo que cambia (medidas, canaletas, pads, recortes) sale de
// generado/sustrato_datos.scad, que lo escribe tools/pcb.py desde
// hardware/pcb/nucleo.json. Acá no hay ni un número del diseño: sólo la
// geometría que los convierte en plástico.
//
//   make pcb        regenera los datos y exporta el STL
//   openscad -o nucleo-sustrato.stl sustrato.scad
//
// Se imprime con la cara de las canaletas HACIA ARRIBA, sin soportes.
//
// LA CARA DE ABAJO ES UN PLANO. Ni un escalon, ni un bolsillo, ni una repisa:
// la cara que se imprime contra la cama es lisa de punta a punta, y todo lo
// que la atraviesa —ventana, muesca de la antena, agujeros— la atraviesa
// entera y recta. Habia dos cosas que no cumplian eso y las dos daban el
// mismo problema: un voladizo hacia adentro que la impresora tenia que
// puentear a ciegas sobre la primera capa. La repisa donde apoyaba la
// pantalla (ahora la sostiene el marco de la carcasa, que es lo que ya hacia)
// y el bolsillo del cargador (ahora el modulo apoya sobre la cara, 0,8 mm mas
// arriba). Si alguna vez se vuelve a hundir algo en esta cara, vuelve el
// problema.

include <generado/sustrato_datos.scad>

$fn = 48;
eps = 0.01;

// --------------------------------------------------------------- utilidades
// Un tramo de canaleta: rectángulo del ancho pedido, con un cuadrado en cada
// vértice para que las esquinas no queden mordidas.
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

module canaleta(ancho, puntos) {
    a = ancho + canaleta_holg;
    union() {
        for (i = [0 : len(puntos) - 2])
            tramo(puntos[i], puntos[i + 1], a);
        for (p = puntos)
            translate([p[0] - a / 2, p[1] - a / 2]) square([a, a]);
    }
}

module pad2d(p) {
    a = p[2] + canaleta_holg;
    h = p[3] + canaleta_holg;
    translate([p[0], p[1]]) rotate([0, 0, p[4]]) translate([-a / 2, -h / 2])
        square([a, h]);
}

// Todo el negativo de la cara de arriba, en 2D.
module surcos2d() {
    for (c = canaletas) canaleta(c[0], c[1]);
    for (p = pads) pad2d(p);
}

// ---------------------------------------------------------------- la placa
module cuerpo() {
    linear_extrude(sustrato_esp)
        offset(r = radio_borde) offset(r = -radio_borde)
            square([sustrato_ancho, sustrato_alto]);
}

module ventana_pasante() {
    // Recta y pasante, del mismo tamaño de arriba a abajo. La pantalla entra
    // desde la cara de los módulos y la aprieta el marco de la carcasa contra
    // el sustrato: es lo que ya la sostenía, la repisa no hacía falta.
    translate([ventana[0], ventana[1], sustrato_esp / 2])
        cube([ventana[2], ventana[3], sustrato_esp + 2 * eps], center = true);
}

module recortes_pasantes() {
    for (r = recortes)
        translate([r[0], r[1], sustrato_esp / 2])
            cube([r[2], r[3], sustrato_esp + 2 * eps], center = true);
}

module agujeros_de_pads() {
    for (p = pads)
        if (p[5] > 0)
            translate([p[0], p[1], -eps])
                cylinder(d = p[5], h = sustrato_esp + 2 * eps);
}

module agujeros_de_tornillos() {
    for (t = tornillos)
        translate([t[0], t[1], -eps])
            cylinder(d = t[2], h = sustrato_esp + 2 * eps);
}

module rotulos_grabados() {
    for (r = rotulos)
        translate([r[0], r[1], sustrato_esp - 0.4])
            rotate([0, 0, r[3]])
                linear_extrude(0.4 + eps)
                    text(r[4], size = r[2], halign = "center",
                         valign = "center", font = "Liberation Sans:style=Bold");
}

module sustrato() {
    difference() {
        cuerpo();
        // canaletas y pads, hundidos desde arriba
        translate([0, 0, sustrato_esp - canaleta_prof])
            linear_extrude(canaleta_prof + eps) surcos2d();
        ventana_pasante();
        recortes_pasantes();
        agujeros_de_pads();
        agujeros_de_tornillos();
        rotulos_grabados();
    }
}

sustrato();

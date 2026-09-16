/* Tests del motor gráfico. Lo importante acá no es que los píxeles sean
 * lindos —eso lo cubre test_render— sino que las primitivas no se salgan
 * del buffer: una primitiva sin recorte sobre un framebuffer en PSRAM no tira
 * excepción en el ESP32, corrompe lo que haya al lado. */
#include <stddef.h>
#include "rk_test.h"
#include "../gfx/fb.h"
#include "../gfx/font.h"
#include "../gfx/aa.h"

#define W 32
#define H 24
#define GUARD 64

/* Buffer con centinelas a ambos lados: si una primitiva se pasa de rango,
 * los centinelas cambian y el test lo ve. */
static rk_color_t g_mem[GUARD + W * H + GUARD];
static rk_fb_t    g_fb;

static void arena_init(void)
{
    int i;
    for (i = 0; i < GUARD + W * H + GUARD; i++) {
        g_mem[i] = 0xDEAD;
    }
    rk_fb_init(&g_fb, &g_mem[GUARD], W, H);
    rk_fb_clear(&g_fb, 0x0000);
}

static int guards_intactos(void)
{
    int i;
    for (i = 0; i < GUARD; i++) {
        if (g_mem[i] != 0xDEAD) { return 0; }
        if (g_mem[GUARD + W * H + i] != 0xDEAD) { return 0; }
    }
    return 1;
}

static int contar(rk_color_t c)
{
    int i, n = 0;
    for (i = 0; i < W * H; i++) {
        if (g_mem[GUARD + i] == c) { n++; }
    }
    return n;
}

static void test_color(void)
{
    /* RK_RGB tiene que producir RGB565 real, no algo parecido. */
    CHECK_HEX("negro",  0x0000, RK_RGB(0, 0, 0));
    CHECK_HEX("blanco", 0xFFFF, RK_RGB(255, 255, 255));
    CHECK_HEX("rojo",   0xF800, RK_RGB(255, 0, 0));
    CHECK_HEX("verde",  0x07E0, RK_RGB(0, 255, 0));
    CHECK_HEX("azul",   0x001F, RK_RGB(0, 0, 255));

    CHECK_HEX("mezcla en 0 devuelve el primero",  0xF800,
              rk_mix(0xF800, 0x001F, 0));
    CHECK_HEX("mezcla en 255 devuelve el segundo", 0x001F,
              rk_mix(0xF800, 0x001F, 255));
    CHECK_HEX("atenuar al maximo da negro", 0x0000, rk_dim(0xFFFF, 255));
    CHECK_HEX("atenuar en 0 no cambia nada", 0xABCD, rk_dim(0xABCD, 0));
}

static void test_recorte(void)
{
    arena_init();

    /* Todas las primitivas, con coordenadas deliberadamente imposibles. */
    rk_px(&g_fb, -5, -5, 0xFFFF);
    rk_px(&g_fb, W + 10, H + 10, 0xFFFF);
    rk_px(&g_fb, -1, 0, 0xFFFF);
    rk_px(&g_fb, 0, -1, 0xFFFF);
    rk_hline(&g_fb, -20, 5, 100, 0xFFFF);
    rk_hline(&g_fb, 5, -3, 10, 0xFFFF);
    rk_hline(&g_fb, 5, H + 3, 10, 0xFFFF);
    rk_vline(&g_fb, 5, -20, 100, 0xFFFF);
    rk_vline(&g_fb, -3, 5, 10, 0xFFFF);
    rk_fill_rect(&g_fb, -50, -50, 200, 200, 0xFFFF);
    rk_rect(&g_fb, -10, -10, 100, 100, 0xFFFF);
    rk_disc(&g_fb, 0, 0, 40, 0xFFFF);
    rk_disc(&g_fb, W, H, 30, 0xFFFF);
    rk_fill_round(&g_fb, -8, -8, 60, 60, 12, 0xFFFF);
    rk_vgradient(&g_fb, -10, -10, 100, 100, 0xF800, 0x001F);

    CHECK_TRUE("ninguna primitiva se sale del framebuffer", guards_intactos());
}

static void test_primitivas(void)
{
    arena_init();
    rk_fill_rect(&g_fb, 2, 3, 10, 5, 0x1234);
    CHECK_INT("fill_rect pinta ancho por alto", 50, contar(0x1234));

    arena_init();
    rk_hline(&g_fb, 0, 0, W, 0x4321);
    CHECK_INT("hline pinta el ancho entero", W, contar(0x4321));

    arena_init();
    rk_vline(&g_fb, 0, 0, H, 0x4321);
    CHECK_INT("vline pinta el alto entero", H, contar(0x4321));

    arena_init();
    rk_rect(&g_fb, 0, 0, 10, 10, 0x0F0F);
    CHECK_INT("rect pinta solo el perimetro", 36, contar(0x0F0F));

    arena_init();
    rk_fb_clear(&g_fb, 0x5555);
    CHECK_INT("clear pinta todo", W * H, contar(0x5555));

    /* El disco tiene que ser simétrico: una raíz entera mal redondeada da
     * un círculo con un lado más gordo, y a esta escala se nota. */
    arena_init();
    rk_disc(&g_fb, 16, 12, 8, 0x7777);
    {
        int y, asim = 0;
        for (y = 0; y < H; y++) {
            int izq = 0, der = 0, x;
            for (x = 0; x < 16; x++) {
                if (g_mem[GUARD + y * W + x] == 0x7777) { izq++; }
            }
            for (x = 17; x < W; x++) {
                if (g_mem[GUARD + y * W + x] == 0x7777) { der++; }
            }
            if (izq != der) { asim++; }
        }
        CHECK_INT("el disco es simetrico respecto del centro", 0, asim);
    }
}

static void test_seno_y_hash(void)
{
    int i, min = 999, max = -999;

    for (i = 0; i < 256; i++) {
        int v = rk_sin8((uint8_t)i);
        if (v < min) { min = v; }
        if (v > max) { max = v; }
    }
    CHECK_INT("el seno arranca en cero", 0, rk_sin8(0));
    CHECK_INT("cuarto de vuelta es el maximo", 127, rk_sin8(64));
    CHECK_INT("media vuelta vuelve a cero", 0, rk_sin8(128));
    CHECK_INT("tres cuartos es el minimo", -127, rk_sin8(192));
    CHECK_INT("nunca se pasa por arriba", 127, max);
    CHECK_INT("nunca se pasa por abajo", -127, min);

    /* El hash tiene que ser determinista y no degenerar: las partículas de
     * los efectos dependen de que reparta. */
    CHECK_INT("el hash es determinista", rk_hash(1234), rk_hash(1234));
    {
        int distintos = 0, j;
        uint16_t vistos[64];
        for (i = 0; i < 64; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 2654 + 17));
            int rep = 0;
            for (j = 0; j < distintos; j++) {
                if (vistos[j] == h) { rep = 1; break; }
            }
            if (!rep) { vistos[distintos++] = h; }
        }
        CHECK_TRUE("el hash no colapsa a pocos valores", distintos >= 60);
    }
}

static void test_tipografia(void)
{
    arena_init();

    CHECK_INT("cadena vacia mide cero", 0, rk_text_w("", 1));
    CHECK_INT("NULL mide cero",         0, rk_text_w(NULL, 1));
    CHECK_INT("un glifo mide 5",        5, rk_text_w("A", 1));
    CHECK_INT("dos glifos miden 11",   11, rk_text_w("AB", 1));
    CHECK_INT("a escala 2 mide el doble", 22, rk_text_w("AB", 2));
    /* Los caracteres desconocidos se saltean en vez de dibujar basura. */
    CHECK_INT("los desconocidos no ocupan", 5, rk_text_w("A\x01\x02", 1));
    CHECK_INT("minusculas cuentan igual",  11, rk_text_w("ab", 1));

    rk_text(&g_fb, -40, -40, "HOLA MUNDO", 0xFFFF, 1);
    rk_text(&g_fb, W + 20, H + 20, "HOLA MUNDO", 0xFFFF, 3);
    rk_text_center(&g_fb, W / 2, 0, "XXXXXXXXXXXXXXXXXXXX", 0xFFFF, 2);
    rk_text_shadow(&g_fb, -5, H - 2, "BORDE", 0xFFFF, 0x0000, 2);
    CHECK_TRUE("el texto fuera de rango no corrompe nada", guards_intactos());

    arena_init();
    rk_text(&g_fb, 1, 1, "I", 0xABCD, 1);
    CHECK_TRUE("la I dibuja algo", contar(0xABCD) > 0);
}

/* --------------------------------------------------- antialiasing ------ */
/* El estilo de ilustración depende de que el borde de cada forma sea una
 * mezcla y no un escalón. Estas pruebas fijan las tres cosas que lo hacen
 * funcionar: adentro es opaco, afuera no toca, y el borde es intermedio. */
static void test_aa_cobertura(void)
{
    rk_forma_t c = rk_circulo_q4(RK_Q4(16), RK_Q4(12), RK_Q4(8));
    rk_forma_t e = rk_elipse_q4(RK_Q4(16), RK_Q4(12), RK_Q4(12), RK_Q4(4));
    rk_forma_t k = rk_capsula_q4(RK_Q4(4), RK_Q4(12), RK_Q4(28), RK_Q4(12), RK_Q4(3));
    rk_forma_t a = rk_anillo_q4(RK_Q4(16), RK_Q4(12), RK_Q4(8), RK_Q4(2));
    rk_forma_t s = rk_semiplano_arriba_q4(RK_Q4(16), RK_Q4(12), 0);
    uint8_t borde;

    CHECK_INT("el centro del circulo es opaco", 255,
              rk_forma_cobertura(&c, RK_Q4(16), RK_Q4(12)));
    CHECK_INT("lejos del circulo no hay nada", 0,
              rk_forma_cobertura(&c, RK_Q4(30), RK_Q4(2)));
    borde = rk_forma_cobertura(&c, RK_Q4(24), RK_Q4(12));
    CHECK_TRUE("justo en el borde la cobertura es intermedia",
               borde > 60u && borde < 200u);

    CHECK_INT("la elipse cubre su centro", 255,
              rk_forma_cobertura(&e, RK_Q4(16), RK_Q4(12)));
    CHECK_INT("y no cubre arriba de su radio menor", 0,
              rk_forma_cobertura(&e, RK_Q4(16), RK_Q4(6)));
    CHECK_INT("pero si a lo ancho", 255,
              rk_forma_cobertura(&e, RK_Q4(26), RK_Q4(12)));

    CHECK_INT("la capsula cubre su eje", 255,
              rk_forma_cobertura(&k, RK_Q4(16), RK_Q4(12)));
    CHECK_INT("y sus puntas son redondas", 0,
              rk_forma_cobertura(&k, RK_Q4(1), RK_Q4(9)));

    CHECK_INT("el anillo esta hueco", 0,
              rk_forma_cobertura(&a, RK_Q4(16), RK_Q4(12)));
    CHECK_INT("y cubre su radio", 255,
              rk_forma_cobertura(&a, RK_Q4(24), RK_Q4(12)));

    CHECK_INT("el semiplano de arriba cubre arriba", 255,
              rk_forma_cobertura(&s, RK_Q4(16), RK_Q4(4)));
    CHECK_INT("y no cubre abajo", 0,
              rk_forma_cobertura(&s, RK_Q4(16), RK_Q4(20)));
    {
        rk_forma_t inv = rk_forma_invertida(s);
        CHECK_INT("invertido cubre lo contrario", 255,
                  rk_forma_cobertura(&inv, RK_Q4(16), RK_Q4(20)));
    }
    {
        /* Arista: cubre el lado donde está el punto de referencia, sin
         * importar el sentido en que se recorre. */
        rk_forma_t h1 = rk_semiplano_arista_q4(0, RK_Q4(10), RK_Q4(30), RK_Q4(10),
                                               RK_Q4(15), RK_Q4(20));
        rk_forma_t h2 = rk_semiplano_arista_q4(RK_Q4(30), RK_Q4(10), 0, RK_Q4(10),
                                               RK_Q4(15), RK_Q4(20));
        CHECK_INT("la arista cubre el lado del interior", 255,
                  rk_forma_cobertura(&h1, RK_Q4(15), RK_Q4(18)));
        CHECK_INT("recorrida al reves, tambien", 255,
                  rk_forma_cobertura(&h2, RK_Q4(15), RK_Q4(18)));
        CHECK_INT("y no cubre el otro lado", 0,
                  rk_forma_cobertura(&h2, RK_Q4(15), RK_Q4(2)));
    }

    CHECK_INT("raiz de 0", 0, (long)rk_isqrt64(0u));
    CHECK_INT("raiz de un cuadrado perfecto", 4096, (long)rk_isqrt64(16777216u));
    CHECK_INT("raiz entera redondea hacia abajo", 3, (long)rk_isqrt64(15u));
    CHECK_TRUE("raiz de un numero de 64 bits",
               rk_isqrt64(9223372030926249001ull) == 3037000499u);
}

static void test_aa_pintado(void)
{
    int i, mezclas = 0;

    /* Un circulo blanco sobre negro: tiene que haber pixeles intermedios en
     * el borde, que es exactamente lo que distingue ilustracion de pixel art. */
    arena_init();
    rk_aa_circulo(&g_fb, RK_Q4C(16), RK_Q4C(12), RK_Q4(7), 0xFFFF);
    for (i = 0; i < W * H; i++) {
        rk_color_t v = g_mem[GUARD + i];
        if (v != 0x0000 && v != 0xFFFF) { mezclas++; }
    }
    CHECK_TRUE("el borde del circulo tiene pixeles mezclados", mezclas >= 12);
    CHECK_HEX("el centro queda del color pleno", 0xFFFF,
              g_mem[GUARD + 12 * W + 16]);
    CHECK_HEX("la esquina no se toca", 0x0000, g_mem[GUARD + 0]);

    /* Un triangulo y un rombo, en cualquier orden de vertices. */
    arena_init();
    rk_aa_triangulo(&g_fb, RK_Q4(2), RK_Q4(20), RK_Q4(16), RK_Q4(2),
                    RK_Q4(30), RK_Q4(20), 0xF800, 255);
    CHECK_HEX("el triangulo cubre su interior", 0xF800,
              g_mem[GUARD + 15 * W + 16]);
    CHECK_HEX("y no la esquina de arriba", 0x0000, g_mem[GUARD + 1 * W + 2]);
    arena_init();
    rk_aa_triangulo(&g_fb, RK_Q4(30), RK_Q4(20), RK_Q4(16), RK_Q4(2),
                    RK_Q4(2), RK_Q4(20), 0xF800, 255);
    CHECK_HEX("con los vertices al reves da lo mismo", 0xF800,
              g_mem[GUARD + 15 * W + 16]);
    arena_init();
    rk_aa_rombo(&g_fb, RK_Q4C(16), RK_Q4C(12), RK_Q4(8), RK_Q4(8), 0x07E0, 255);
    CHECK_HEX("el rombo cubre su centro", 0x07E0, g_mem[GUARD + 12 * W + 16]);
    CHECK_HEX("y no sus esquinas", 0x0000, g_mem[GUARD + 5 * W + 10]);

    /* Recorte: nada se sale del buffer, ni con formas gigantes ni con
     * coordenadas negativas. */
    arena_init();
    rk_aa_circulo(&g_fb, RK_Q4(-5), RK_Q4(-5), RK_Q4(40), 0xFFFF);
    rk_aa_elipse(&g_fb, RK_Q4(W + 10), RK_Q4(H + 10), RK_Q4(30), RK_Q4(9), 0xFFFF);
    rk_aa_capsula(&g_fb, RK_Q4(-50), RK_Q4(-50), RK_Q4(90), RK_Q4(90), RK_Q4(6), 0xFFFF);
    rk_aa_arco(&g_fb, RK_Q4(0), RK_Q4(H), RK_Q4(40), RK_Q4(5), true, 0xFFFF);
    rk_aa_triangulo(&g_fb, RK_Q4(-40), 0, RK_Q4(90), RK_Q4(-9), 0, RK_Q4(90), 0xFFFF, 255);
    rk_aa_rombo(&g_fb, 0, 0, RK_Q4(90), RK_Q4(90), 0xFFFF, 128);
    {
        rk_forma_t f[2];
        f[0] = rk_semiplano_abajo_q4(0, RK_Q4(4), 20);
        f[1] = rk_forma_invertida(rk_circulo_q4(0, 0, RK_Q4(5)));
        rk_aa_pintar(&g_fb, f, 2, 0xFFFF, 255, -100, -100, 1000, 1000);
        rk_aa_pintar(&g_fb, f, 0, 0xFFFF, 255, 0, 0, W, H);
        rk_aa_pintar(NULL, f, 2, 0xFFFF, 255, 0, 0, W, H);
    }
    CHECK_TRUE("ninguna forma suavizada se sale del framebuffer",
               guards_intactos());

    /* Alfa: un rubor al 50% sobre negro no puede quedar pleno. */
    arena_init();
    {
        rk_forma_t f = rk_circulo_q4(RK_Q4C(16), RK_Q4C(12), RK_Q4(6));
        rk_aa_pintar(&g_fb, &f, 1, 0xFFFF, 128, 0, 0, W, H);
    }
    CHECK_TRUE("con alfa a la mitad el centro no es pleno",
               g_mem[GUARD + 12 * W + 16] != 0xFFFF &&
               g_mem[GUARD + 12 * W + 16] != 0x0000);
}

void suite_gfx(void)
{
    RK_SUITE("graficos");
    test_color();
    test_recorte();
    test_primitivas();
    test_seno_y_hash();
    test_tipografia();
    test_aa_cobertura();
    test_aa_pintado();
    RK_SUITE_END();
}

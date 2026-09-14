/* Tests del motor gráfico. Lo importante acá no es que los píxeles sean
 * lindos —eso lo cubre test_render— sino que las primitivas no se salgan
 * del buffer: un blit sin recorte sobre un framebuffer en PSRAM no tira
 * excepción en el ESP32, corrompe lo que haya al lado. */
#include <stddef.h>
#include "rk_test.h"
#include "../gfx/fb.h"
#include "../gfx/font.h"

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

static void test_sprites(void)
{
    static const rk_color_t pal[3] = { 0x0000, 0xF800, 0x001F };
    static const uint8_t idx[9]    = { 0, 1, 0,
                                       1, 2, 1,
                                       0, 1, 0 };
    rk_sprite_t s = { 3, 3, idx, pal, 3 };

    arena_init();
    rk_blit(&g_fb, &s, 5, 5);
    CHECK_INT("el indice 0 es transparente", 4, contar(0xF800));
    CHECK_INT("el indice 2 se pinta",        1, contar(0x001F));

    /* Blits completamente fuera del buffer, en las cuatro direcciones. */
    arena_init();
    rk_blit(&g_fb, &s, -10, -10);
    rk_blit(&g_fb, &s, W + 5, H + 5);
    rk_blit(&g_fb, &s, -2, 5);
    rk_blit(&g_fb, &s, W - 1, 5);
    rk_blit_tint(&g_fb, &s, -100, -100, 0xFFFF, 128);
    rk_blit_solid(&g_fb, &s, 1000, 1000, 0xFFFF);
    CHECK_TRUE("los blits fuera de rango no corrompen nada", guards_intactos());

    arena_init();
    rk_blit_solid(&g_fb, &s, 5, 5, 0x2222);
    CHECK_INT("blit_solid ignora la paleta", 5, contar(0x2222));

    arena_init();
    rk_fb_clear(&g_fb, 0x5555);
    rk_blit_tint(&g_fb, &s, 5, 5, 0x0000, 255);
    CHECK_INT("tinte al maximo lleva todo al color destino", 5, contar(0x0000));

    arena_init();
    rk_fb_clear(&g_fb, 0x5555);
    rk_blit_tint(&g_fb, &s, 5, 5, 0xFFFF, 0);
    CHECK_INT("tinte en cero equivale a un blit normal", 4, contar(0xF800));
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

void suite_gfx(void)
{
    RK_SUITE("graficos");
    test_color();
    test_recorte();
    test_primitivas();
    test_sprites();
    test_seno_y_hash();
    test_tipografia();
    RK_SUITE_END();
}

/* La pantalla del QR.
 *
 * La prueba que importa es la última: se lee el framebuffer módulo por
 * módulo y se compara con la matriz del QR. Si un corrimiento de un pixel
 * en el layout desalinea el dibujo, el QR se ve perfecto a simple vista y
 * ningún teléfono lo lee. Eso se descubre acá y no con la caja abierta.
 */
#include <stddef.h>
#include "rk_test.h"
#include "../ui/qr.h"
#include "../gfx/panel.h"

static rk_color_t g_chico[RK_MINI_PX + 64];
static rk_color_t g_grande[RK_PRIME_PX];

/* Recorre cada módulo del QR dibujado y cuenta los que no coinciden. */
static int errores_de_lectura(const rk_fb_t *fb, const rk_qr_t *q)
{
    int m = rk_qr_escala(q, fb->w, fb->h);
    int total = q->lado + 4;
    int lado_px = total * m;
    int x0 = (fb->w - lado_px) / 2;
    int y0 = -1, x, y, errores = 0;

    /* La tarjeta blanca: primera fila con blanco en la columna del centro. */
    for (y = 0; y < fb->h; y++) {
        if (fb->px[y * fb->w + fb->w / 2] == RK_QR_TARJETA) {
            y0 = y;
            break;
        }
    }
    if (y0 < 0) {
        return 9999;
    }
    for (y = 0; y < q->lado; y++) {
        for (x = 0; x < q->lado; x++) {
            int px = x0 + (2 + x) * m + m / 2;
            int py = y0 + (2 + y) * m + m / 2;
            bool oscuro = fb->px[py * fb->w + px] == RK_QR_TINTA;
            if (oscuro != rk_qr_modulo(q, x, y)) {
                errores++;
            }
        }
    }
    return errores;
}

static void test_qr(void)
{
    static rk_qr_t q;
    rk_fb_t fb;
    int i, sucio = 0;

    CHECK_TRUE("una URL de produccion se codifica",
               rk_qr_preparar(&q, "HTTPS://ROOTLAB.APP/V/K7Q2M9XA", "K7Q2M9XA"));
    CHECK_STR("el codigo se parte en dos grupos", "K7Q2-M9XA", q.codigo);
    CHECK_TRUE("entra en una version chica", q.lado >= 21 && q.lado <= 29);
    CHECK_TRUE("en el panel de 128 cada modulo mide 3 pixeles o mas",
               rk_qr_escala(&q, RK_MINI_W, RK_MINI_H) >= 3);
    CHECK_TRUE("en el de 240x320, 6 o mas",
               rk_qr_escala(&q, RK_PRIME_W, RK_PRIME_H) >= 6);

    for (i = 0; i < RK_MINI_PX + 64; i++) {
        g_chico[i] = 0xBEEFu;
    }
    rk_fb_init(&fb, &g_chico[32], RK_MINI_W, RK_MINI_H);
    rk_qr_draw(&fb, &q, RK_QR_EN_LINEA, 1234u);
    CHECK_INT("dibujado en 128 se lee modulo por modulo", 0, errores_de_lectura(&fb, &q));
    for (i = 0; i < 32; i++) {
        if (g_chico[i] != 0xBEEFu || g_chico[RK_MINI_PX + 32 + i] != 0xBEEFu) {
            sucio++;
        }
    }
    CHECK_INT("y no escribe fuera del framebuffer", 0, sucio);

    rk_fb_init(&fb, g_grande, RK_PRIME_W, RK_PRIME_H);
    rk_qr_draw(&fb, &q, RK_QR_PORTAL, 0u);
    CHECK_INT("dibujado en 240x320 tambien", 0, errores_de_lectura(&fb, &q));

    /* Una URL de desarrollo, con IP y puerto, sube de versión pero sigue
     * entrando legible en el panel chico. */
    CHECK_TRUE("una URL de red local se codifica",
               rk_qr_preparar(&q, "HTTP://192.168.100.200:8080/V/K7Q2M9XA", "K7Q2M9XA"));
    CHECK_TRUE("y en 128 sigue con modulos de 3",
               rk_qr_escala(&q, RK_MINI_W, RK_MINI_H) >= 3);
    rk_fb_init(&fb, &g_chico[32], RK_MINI_W, RK_MINI_H);
    rk_qr_draw(&fb, &q, RK_QR_CONECTANDO, 700u);
    CHECK_INT("y se lee", 0, errores_de_lectura(&fb, &q));

    /* La rayita de estado anima: dos instantes, dos cuadros. */
    {
        uint32_t a, b;
        int k;
        rk_qr_draw(&fb, &q, RK_QR_CONECTANDO, 100u);
        a = 0u;
        for (k = 0; k < RK_MINI_PX; k++) { a = a * 31u + fb.px[k]; }
        rk_qr_draw(&fb, &q, RK_QR_CONECTANDO, 800u);
        b = 0u;
        for (k = 0; k < RK_MINI_PX; k++) { b = b * 31u + fb.px[k]; }
        CHECK_TRUE("conectando se mueve", a != b);
    }

    /* Una URL imposible: no hay QR, pero el código sigue en pantalla. */
    {
        char larga[400];
        memset(larga, 'A', sizeof larga - 1u);
        larga[sizeof larga - 1u] = '\0';
        CHECK_TRUE("una URL que no entra no codifica", !rk_qr_preparar(&q, larga, "K7Q2M9XA"));
        rk_fb_init(&fb, &g_chico[32], RK_MINI_W, RK_MINI_H);
        rk_qr_draw(&fb, &q, RK_QR_PORTAL, 0u);
        CHECK_STR("pero guarda el codigo para mostrarlo", "K7Q2-M9XA", q.codigo);
        CHECK_INT("sin QR la escala es 0", 0, rk_qr_escala(&q, RK_MINI_W, RK_MINI_H));
    }

    rk_qr_draw(NULL, &q, RK_QR_PORTAL, 0u);
    rk_qr_draw(&fb, NULL, RK_QR_PORTAL, 0u);
    CHECK_TRUE("NULL no explota", !rk_qr_preparar(NULL, "X", "Y") && !rk_qr_modulo(NULL, 0, 0));
}

void suite_qr(void)
{
    RK_SUITE("pantalla del QR");
    test_qr();
    RK_SUITE_END();
}

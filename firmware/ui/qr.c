#include "qr.h"
#include "../gfx/font.h"
#include "../third_party/qrcodegen/qrcodegen.h"
#include <stddef.h>
#include <string.h>

/* Zona de silencio, en módulos. La norma pide cuatro; sobre una tarjeta
 * blanca rodeada de fondo oscuro dos alcanzan para cualquier lector de
 * teléfono, y en el panel de 128 esos dos módulos de diferencia son lo que
 * permite que cada módulo mida 3 pixeles y no 2. */
#define SILENCIO 2

#define COL_PORTAL   RK_RGB(255, 184,  64)
#define COL_CONECTA  RK_RGB( 90, 190, 255)
#define COL_LINEA    RK_RGB(110, 220, 120)
#define COL_CODIGO   RK_RGB(236, 238, 244)

bool rk_qr_preparar(rk_qr_t *q, const char *url, const char *codigo)
{
    uint8_t tmp[RK_QR_BUF];

    if (q == NULL) {
        return false;
    }
    memset(q, 0, sizeof *q);
    if (codigo != NULL) {
        /* "K7Q2M9XA" -> "K7Q2-M9XA": dos grupos se leen y se dictan mejor. */
        size_t n = strlen(codigo), i, k = 0u;
        for (i = 0; i < n && k + 1u < sizeof q->codigo; i++) {
            if (i == 4u && n == 8u) {
                q->codigo[k++] = '-';
            }
            q->codigo[k++] = codigo[i];
        }
        q->codigo[k] = '\0';
    }
    if (url == NULL || url[0] == '\0') {
        return false;
    }
    q->ok = qrcodegen_encodeText(url, tmp, q->buf, qrcodegen_Ecc_MEDIUM,
                                 qrcodegen_VERSION_MIN, RK_QR_VERSION_MAX,
                                 qrcodegen_Mask_AUTO, true);
    q->lado = q->ok ? qrcodegen_getSize(q->buf) : 0;
    return q->ok;
}

bool rk_qr_modulo(const rk_qr_t *q, int x, int y)
{
    if (q == NULL || !q->ok) {
        return false;
    }
    return qrcodegen_getModule(q->buf, x, y);
}

static int escala_texto(int w)
{
    return (w >= 200) ? 3 : 2;
}

int rk_qr_escala(const rk_qr_t *q, int w, int h)
{
    int total, ts, disponible_h, m;

    if (q == NULL || !q->ok || w <= 0 || h <= 0) {
        return 0;
    }
    total = q->lado + 2 * SILENCIO;
    ts = escala_texto(w);
    /* Arriba 3, abajo: separación, código, separación y la rayita. */
    disponible_h = h - 3 - (4 + RK_GLYPH_H * ts + 4 + 2 + 2);
    m = (w - 4) / total;
    if (disponible_h / total < m) {
        m = disponible_h / total;
    }
    return m < 1 ? 1 : m;
}

void rk_qr_draw(rk_fb_t *fb, const rk_qr_t *q, rk_qr_estado_t estado, uint32_t t_ms)
{
    int W, H, ts, m, total, lado_px, x0, y0, y_codigo, x, y;
    rk_color_t col;

    if (fb == NULL || fb->px == NULL) {
        return;
    }
    W = fb->w;
    H = fb->h;
    ts = escala_texto(W);
    rk_fb_clear(fb, RK_QR_FONDO);

    if (q == NULL || !q->ok) {
        /* Sin QR: el código solo, grande y centrado. */
        if (q != NULL && q->codigo[0] != '\0') {
            rk_text_center(fb, W / 2, H / 2 - (RK_GLYPH_H * ts) / 2, q->codigo,
                           COL_CODIGO, ts);
        }
        y_codigo = H;
    } else {
        m = rk_qr_escala(q, W, H);
        total = q->lado + 2 * SILENCIO;
        lado_px = total * m;
        x0 = (W - lado_px) / 2;
        /* Bloque QR + código centrado verticalmente en el panel. */
        {
            int bloque = lado_px + 4 + RK_GLYPH_H * ts;
            y0 = (H - 4 - bloque) / 2;
            if (y0 < 3) {
                y0 = 3;
            }
        }
        rk_fill_round(fb, x0, y0, lado_px, lado_px, m * 2, RK_QR_TARJETA);
        for (y = 0; y < q->lado; y++) {
            for (x = 0; x < q->lado; x++) {
                if (qrcodegen_getModule(q->buf, x, y)) {
                    rk_fill_rect(fb, x0 + (SILENCIO + x) * m, y0 + (SILENCIO + y) * m,
                                 m, m, RK_QR_TINTA);
                }
            }
        }
        y_codigo = y0 + lado_px + 4;
        rk_text_center(fb, W / 2, y_codigo, q->codigo, COL_CODIGO, ts);
        y_codigo += RK_GLYPH_H * ts;
    }

    /* La rayita de estado, al pie. */
    {
        int ancho = W / 3;
        int xr = (W - ancho) / 2;
        int yr = H - 3;
        if (yr < y_codigo + 1) {
            yr = y_codigo + 1;
        }
        if (yr + 2 > H) {
            return;
        }
        rk_fill_rect(fb, xr, yr, ancho, 2, rk_mix(RK_QR_FONDO, COL_CODIGO, 40));
        switch (estado) {
        case RK_QR_CONECTANDO: {
            /* Un tramo que barre de lado a lado. */
            int tramo = ancho / 4;
            int fase = (int)(t_ms % 1400u) * (ancho + tramo) / 1400 - tramo;
            int a = fase < 0 ? 0 : fase;
            int b = fase + tramo > ancho ? ancho : fase + tramo;
            if (b > a) {
                rk_fill_rect(fb, xr + a, yr, b - a, 2, COL_CONECTA);
            }
            break;
        }
        case RK_QR_EN_LINEA: {
            /* Late despacio: está vivo y esperando. */
            int s = rk_sin8((uint8_t)(t_ms / 5u)) + 127;
            col = rk_mix(rk_mix(RK_QR_FONDO, COL_LINEA, 90), COL_LINEA, (uint8_t)s);
            rk_fill_rect(fb, xr, yr, ancho, 2, col);
            break;
        }
        case RK_QR_PORTAL:
        default:
            rk_fill_rect(fb, xr, yr, ancho, 2, COL_PORTAL);
            break;
        }
    }
}

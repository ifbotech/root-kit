#include "aa.h"
#include <stddef.h>

/* Media franja de antialiasing, en Q4: medio pixel a cada lado del borde. */
#define MEDIA_BANDA  8
/* Margen de los atajos: un pixel entero. Con menos, los atajos de "adentro"
 * y "afuera" se meterían en la franja y el borde saldría duro. */
#define ATAJO        16

uint32_t rk_isqrt64(uint64_t v)
{
    uint64_t r = 0, bit = (uint64_t)1 << 62;

    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)r;
}

/* Distancia con signo (Q4, negativa adentro) a cobertura 0..255. */
static uint8_t cobertura_de(int64_t d)
{
    int64_t c = MEDIA_BANDA - d;
    if (c <= 0) {
        return 0;
    }
    if (c >= 2 * MEDIA_BANDA) {
        return 255;
    }
    return (uint8_t)(c * 255 / (2 * MEDIA_BANDA));
}

/* ------------------------------------------------------------- formas --- */
rk_forma_t rk_elipse_q4(int32_t cx, int32_t cy, int32_t rx, int32_t ry)
{
    rk_forma_t f = { 0 };
    f.tipo = RK_FORMA_ELIPSE;
    f.cx = cx; f.cy = cy; f.rx = rx; f.ry = ry;
    return f;
}

rk_forma_t rk_circulo_q4(int32_t cx, int32_t cy, int32_t r)
{
    return rk_elipse_q4(cx, cy, r, r);
}

rk_forma_t rk_capsula_q4(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         int32_t r)
{
    rk_forma_t f = { 0 };
    f.tipo = RK_FORMA_CAPSULA;
    f.cx = x0; f.cy = y0; f.x1 = x1; f.y1 = y1; f.rx = r;
    return f;
}

rk_forma_t rk_anillo_q4(int32_t cx, int32_t cy, int32_t r, int32_t grosor)
{
    rk_forma_t f = { 0 };
    f.tipo = RK_FORMA_ANILLO;
    f.cx = cx; f.cy = cy; f.rx = r; f.ry = grosor;
    return f;
}

/* rk_sin8 devuelve -127..127; en Q16 la unidad es 65536, así que el factor
 * es 65536/127 = 516. El error de módulo resultante es menor al 0,1% y no se
 * nota en un borde de un pixel. */
static rk_forma_t semiplano(int32_t cx, int32_t cy, int ang, bool abajo)
{
    rk_forma_t f = { 0 };
    int s = rk_sin8((uint8_t)ang);
    int c = rk_sin8((uint8_t)(ang + 64));
    f.tipo = RK_FORMA_SEMIPLANO;
    f.cx = cx;
    f.cy = cy;
    /* Normal que apunta hacia el lado NO cubierto. Para cubrir "arriba", la
     * normal apunta abajo: (-sen, cos) rotada por el ángulo de la recta. */
    f.nx = -s * 516;
    f.ny =  c * 516;
    if (abajo) {
        f.nx = -f.nx;
        f.ny = -f.ny;
    }
    return f;
}

rk_forma_t rk_semiplano_arriba_q4(int32_t cx, int32_t cy, int ang)
{
    return semiplano(cx, cy, ang, false);
}

rk_forma_t rk_semiplano_abajo_q4(int32_t cx, int32_t cy, int ang)
{
    return semiplano(cx, cy, ang, true);
}

rk_forma_t rk_semiplano_arista_q4(int32_t ax, int32_t ay, int32_t bx, int32_t by,
                                  int32_t ix, int32_t iy)
{
    rk_forma_t f = { 0 };
    int64_t dx = (int64_t)bx - ax, dy = (int64_t)by - ay;
    int64_t len = (int64_t)rk_isqrt64((uint64_t)(dx * dx + dy * dy));
    /* Producto cruz del punto interior respecto de la arista: su signo dice
     * de qué lado está el interior. La normal hacia AFUERA apunta al revés. */
    int64_t cruz = dx * ((int64_t)iy - ay) - dy * ((int64_t)ix - ax);
    int64_t sgn = cruz >= 0 ? 1 : -1;

    f.tipo = RK_FORMA_SEMIPLANO;
    f.cx = ax;
    f.cy = ay;
    if (len == 0) {
        /* Arista degenerada: no cubre nada. Se ancla la recta muy arriba con
         * la normal hacia abajo, así la distancia de cualquier pixel es
         * enorme y positiva. */
        f.cy = -(1 << 28);
        f.nx = 0;
        f.ny = 65536;
        return f;
    }
    f.nx = (int32_t)(-sgn * (-dy) * 65536 / len);
    f.ny = (int32_t)(-sgn * dx * 65536 / len);
    return f;
}

rk_forma_t rk_forma_invertida(rk_forma_t f)
{
    f.invertir = !f.invertir;
    return f;
}

/* --------------------------------------------------------- coberturas --- */
static uint8_t cob_elipse(const rk_forma_t *f, int32_t px, int32_t py)
{
    int64_t dx = (int64_t)px - f->cx;
    int64_t dy = (int64_t)py - f->cy;
    int64_t a = f->rx, b = f->ry;
    int64_t qx, qy, rmin, rmax, lo, hi, k0, hx, hy, h, d;
    uint64_t s;

    if (a <= 0 || b <= 0) {
        return 0;
    }
    /* q = (dx/a, dy/b) en Q16: el punto llevado al espacio donde la elipse es
     * el círculo unitario. */
    qx = (dx * 65536) / a;
    qy = (dy * 65536) / b;
    s = (uint64_t)(qx * qx + qy * qy);

    /* Atajos. La franja se mide en el eje mayor para "adentro" y en el menor
     * para "afuera", que es el lado conservador en los dos casos. */
    rmin = a < b ? a : b;
    rmax = a > b ? a : b;
    lo = 65536 - (ATAJO * 65536) / rmax;
    hi = 65536 + (ATAJO * 65536) / rmin;
    if (lo > 0 && s <= (uint64_t)(lo * lo)) {
        return 255;
    }
    if (s >= (uint64_t)(hi * hi)) {
        return 0;
    }

    /* Distancia aproximada al borde: (|q| - 1) / |grad|q||, con el gradiente
     * medido por pixel. Es exacta sobre los ejes y buena en el resto para una
     * franja de un pixel, que es todo lo que el antialiasing necesita. */
    k0 = (int64_t)rk_isqrt64(s);                    /* Q16 */
    hx = (qx * 16) / a;                              /* Q16 por pixel */
    hy = (qy * 16) / b;
    h = (int64_t)rk_isqrt64((uint64_t)(hx * hx + hy * hy));
    if (h == 0) {
        return 255;                                  /* el centro exacto */
    }
    d = ((k0 - 65536) * k0) / h;                     /* Q16 pixeles */
    return cobertura_de(d >> 12);                    /* a Q4 */
}

static uint8_t cob_semiplano(const rk_forma_t *f, int32_t px, int32_t py)
{
    int64_t dx = (int64_t)px - f->cx;
    int64_t dy = (int64_t)py - f->cy;
    return cobertura_de((dx * f->nx + dy * f->ny) >> 16);
}

static uint8_t cob_capsula(const rk_forma_t *f, int32_t px, int32_t py)
{
    int64_t abx = (int64_t)f->x1 - f->cx;
    int64_t aby = (int64_t)f->y1 - f->cy;
    int64_t apx = (int64_t)px - f->cx;
    int64_t apy = (int64_t)py - f->cy;
    int64_t len2 = abx * abx + aby * aby;
    int64_t t = 0, qx, qy, ex, ey, r = f->rx;
    uint64_t d2;

    if (len2 > 0) {
        t = ((apx * abx + apy * aby) * 65536) / len2;
        if (t < 0) { t = 0; }
        if (t > 65536) { t = 65536; }
    }
    qx = f->cx + (abx * t) / 65536;
    qy = f->cy + (aby * t) / 65536;
    ex = px - qx;
    ey = py - qy;
    d2 = (uint64_t)(ex * ex + ey * ey);

    if (r > ATAJO && d2 <= (uint64_t)((r - ATAJO) * (r - ATAJO))) {
        return 255;
    }
    if (d2 >= (uint64_t)((r + ATAJO) * (r + ATAJO))) {
        return 0;
    }
    return cobertura_de((int64_t)rk_isqrt64(d2) - r);
}

static uint8_t cob_anillo(const rk_forma_t *f, int32_t px, int32_t py)
{
    int64_t dx = (int64_t)px - f->cx;
    int64_t dy = (int64_t)py - f->cy;
    int64_t medio = f->ry / 2;
    int64_t fuera = f->rx + medio + ATAJO;
    int64_t dentro = f->rx - medio - ATAJO;
    uint64_t d2 = (uint64_t)(dx * dx + dy * dy);
    int64_t dist, d;

    if (d2 >= (uint64_t)(fuera * fuera)) {
        return 0;
    }
    if (dentro > 0 && d2 <= (uint64_t)(dentro * dentro)) {
        return 0;
    }
    dist = (int64_t)rk_isqrt64(d2);
    d = dist - f->rx;
    if (d < 0) {
        d = -d;
    }
    return cobertura_de(d - medio);
}

uint8_t rk_forma_cobertura(const rk_forma_t *f, int32_t px, int32_t py)
{
    uint8_t c;

    if (f == NULL) {
        return 0;
    }
    switch (f->tipo) {
    case RK_FORMA_ELIPSE:    c = cob_elipse(f, px, py);    break;
    case RK_FORMA_SEMIPLANO: c = cob_semiplano(f, px, py); break;
    case RK_FORMA_CAPSULA:   c = cob_capsula(f, px, py);   break;
    case RK_FORMA_ANILLO:    c = cob_anillo(f, px, py);    break;
    default:                 c = 0;                        break;
    }
    return f->invertir ? (uint8_t)(255 - c) : c;
}

/* ------------------------------------------------------------- cajas --- */
static int piso_px(int32_t q4)  { return (int)(q4 >= 0 ? q4 / 16 : -((-q4 + 15) / 16)); }
static int techo_px(int32_t q4) { return (int)(q4 >= 0 ? (q4 + 15) / 16 : -((-q4) / 16)); }

bool rk_forma_caja(const rk_forma_t *f, int *x0, int *y0, int *x1, int *y1)
{
    int32_t ax, ay, bx, by, r;

    if (f == NULL || f->invertir || f->tipo == RK_FORMA_SEMIPLANO) {
        return false;
    }
    switch (f->tipo) {
    case RK_FORMA_ELIPSE:
        ax = f->cx - f->rx; bx = f->cx + f->rx;
        ay = f->cy - f->ry; by = f->cy + f->ry;
        break;
    case RK_FORMA_CAPSULA:
        r  = f->rx;
        ax = (f->cx < f->x1 ? f->cx : f->x1) - r;
        bx = (f->cx > f->x1 ? f->cx : f->x1) + r;
        ay = (f->cy < f->y1 ? f->cy : f->y1) - r;
        by = (f->cy > f->y1 ? f->cy : f->y1) + r;
        break;
    case RK_FORMA_ANILLO:
        r  = f->rx + f->ry / 2;
        ax = f->cx - r; bx = f->cx + r;
        ay = f->cy - r; by = f->cy + r;
        break;
    default:
        return false;
    }
    /* Un pixel de más a cada lado: la franja de antialiasing sale del borde
     * geométrico y sin esto la última fila de medio tono queda afuera. */
    *x0 = piso_px(ax) - 1;
    *y0 = piso_px(ay) - 1;
    *x1 = techo_px(bx) + 1;
    *y1 = techo_px(by) + 1;
    return true;
}

/* ------------------------------------------------------------- pintar --- */
void rk_aa_pintar(rk_fb_t *fb, const rk_forma_t *formas, int n,
                  rk_color_t color, uint8_t alfa,
                  int clip_x0, int clip_y0, int clip_x1, int clip_y1)
{
    int x0, y0, x1, y1, i, x, y;

    if (fb == NULL || fb->px == NULL || formas == NULL || n <= 0 || alfa == 0) {
        return;
    }

    x0 = clip_x0 < 0 ? 0 : clip_x0;
    y0 = clip_y0 < 0 ? 0 : clip_y0;
    x1 = clip_x1 > fb->w ? fb->w : clip_x1;
    y1 = clip_y1 > fb->h ? fb->h : clip_y1;

    /* La intersección nunca es más grande que la forma acotada más chica. */
    for (i = 0; i < n; i++) {
        int bx0, by0, bx1, by1;
        if (rk_forma_caja(&formas[i], &bx0, &by0, &bx1, &by1)) {
            if (bx0 > x0) { x0 = bx0; }
            if (by0 > y0) { y0 = by0; }
            if (bx1 < x1) { x1 = bx1; }
            if (by1 < y1) { y1 = by1; }
        }
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (y = y0; y < y1; y++) {
        rk_color_t *fila = &fb->px[(size_t)y * (size_t)fb->w];
        int32_t py = RK_Q4C(y);
        for (x = x0; x < x1; x++) {
            int32_t px = RK_Q4C(x);
            uint8_t cob = 255;
            for (i = 0; i < n && cob > 0; i++) {
                uint8_t c = rk_forma_cobertura(&formas[i], px, py);
                if (c < cob) {
                    cob = c;
                }
            }
            if (cob == 0) {
                continue;
            }
            if (alfa != 255) {
                cob = (uint8_t)((uint16_t)cob * alfa / 255);
            }
            fila[x] = (cob == 255) ? color : rk_mix(fila[x], color, cob);
        }
    }
}

/* El color de un relleno en un punto. Para el lineal es la proyección sobre
 * el eje; para el radial, la distancia al centro. Los dos devuelven una
 * fracción de 0 a 255 que mezcla los dos colores. */
static rk_color_t color_en(const rk_relleno_t *r, int32_t px, int32_t py)
{
    int32_t t;

    if (r->tipo == RK_RELLENO_PLANO) {
        return r->a;
    }
    if (r->tipo == RK_RELLENO_RADIAL) {
        int32_t dx = px - r->x0, dy = py - r->y0;
        int32_t rad = r->x1 > 0 ? r->x1 : 1;
        /* Se compara en cuadrados hasta el final para no sacar dos raíces. */
        int64_t d2 = (int64_t)dx * dx + (int64_t)dy * dy;
        int64_t r2 = (int64_t)rad * rad;
        if (d2 >= r2) {
            return r->b;
        }
        t = (int32_t)(255 * rk_isqrt64(d2) / rad);
    } else {
        int32_t ex = r->x1 - r->x0, ey = r->y1 - r->y0;
        int64_t largo2 = (int64_t)ex * ex + (int64_t)ey * ey;
        int64_t proy;
        if (largo2 <= 0) {
            return r->a;
        }
        proy = (int64_t)(px - r->x0) * ex + (int64_t)(py - r->y0) * ey;
        t = (int32_t)(255 * proy / largo2);
        if (t < 0) { t = 0; }
        if (t > 255) { t = 255; }
    }
    return rk_mix(r->a, r->b, (uint8_t)t);
}

void rk_aa_pintar_relleno(rk_fb_t *fb, const rk_forma_t *formas, int n,
                          const rk_relleno_t *relleno, uint8_t alfa,
                          int clip_x0, int clip_y0, int clip_x1, int clip_y1)
{
    int x0, y0, x1, y1, i, x, y;

    if (fb == NULL || fb->px == NULL || formas == NULL || n <= 0 || alfa == 0
        || relleno == NULL) {
        return;
    }
    if (relleno->tipo == RK_RELLENO_PLANO) {
        rk_aa_pintar(fb, formas, n, relleno->a, alfa, clip_x0, clip_y0, clip_x1, clip_y1);
        return;
    }

    x0 = clip_x0 < 0 ? 0 : clip_x0;
    y0 = clip_y0 < 0 ? 0 : clip_y0;
    x1 = clip_x1 > fb->w ? fb->w : clip_x1;
    y1 = clip_y1 > fb->h ? fb->h : clip_y1;
    for (i = 0; i < n; i++) {
        int bx0, by0, bx1, by1;
        if (rk_forma_caja(&formas[i], &bx0, &by0, &bx1, &by1)) {
            if (bx0 > x0) { x0 = bx0; }
            if (by0 > y0) { y0 = by0; }
            if (bx1 < x1) { x1 = bx1; }
            if (by1 < y1) { y1 = by1; }
        }
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (y = y0; y < y1; y++) {
        rk_color_t *fila = &fb->px[(size_t)y * (size_t)fb->w];
        int32_t py = RK_Q4C(y);
        for (x = x0; x < x1; x++) {
            int32_t px = RK_Q4C(x);
            uint8_t cob = 255;
            rk_color_t col;
            for (i = 0; i < n && cob > 0; i++) {
                uint8_t c = rk_forma_cobertura(&formas[i], px, py);
                if (c < cob) {
                    cob = c;
                }
            }
            if (cob == 0) {
                continue;
            }
            if (alfa != 255) {
                cob = (uint8_t)((uint16_t)cob * alfa / 255);
            }
            col = color_en(relleno, px, py);
            fila[x] = (cob == 255) ? col : rk_mix(fila[x], col, cob);
        }
    }
}

void rk_aa_elipse(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t rx, int32_t ry,
                  rk_color_t c)
{
    rk_forma_t f = rk_elipse_q4(cx, cy, rx, ry);
    rk_aa_pintar(fb, &f, 1, c, 255, 0, 0, 1 << 15, 1 << 15);
}

void rk_aa_circulo(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t r, rk_color_t c)
{
    rk_aa_elipse(fb, cx, cy, r, r, c);
}

void rk_aa_capsula(rk_fb_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   int32_t r, rk_color_t c)
{
    rk_forma_t f = rk_capsula_q4(x0, y0, x1, y1, r);
    rk_aa_pintar(fb, &f, 1, c, 255, 0, 0, 1 << 15, 1 << 15);
}

void rk_aa_triangulo(rk_fb_t *fb, int32_t ax, int32_t ay, int32_t bx, int32_t by,
                     int32_t cx, int32_t cy, rk_color_t c, uint8_t alfa)
{
    rk_forma_t f[3];
    int32_t minx = ax, maxx = ax, miny = ay, maxy = ay;

    f[0] = rk_semiplano_arista_q4(ax, ay, bx, by, cx, cy);
    f[1] = rk_semiplano_arista_q4(bx, by, cx, cy, ax, ay);
    f[2] = rk_semiplano_arista_q4(cx, cy, ax, ay, bx, by);
    if (bx < minx) { minx = bx; } if (bx > maxx) { maxx = bx; }
    if (cx < minx) { minx = cx; } if (cx > maxx) { maxx = cx; }
    if (by < miny) { miny = by; } if (by > maxy) { maxy = by; }
    if (cy < miny) { miny = cy; } if (cy > maxy) { maxy = cy; }
    rk_aa_pintar(fb, f, 3, c, alfa,
                 piso_px(minx) - 1, piso_px(miny) - 1,
                 techo_px(maxx) + 1, techo_px(maxy) + 1);
}

void rk_aa_rombo(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t hw, int32_t hh,
                 rk_color_t c, uint8_t alfa)
{
    rk_forma_t f[4];

    f[0] = rk_semiplano_arista_q4(cx, cy - hh, cx + hw, cy, cx, cy);
    f[1] = rk_semiplano_arista_q4(cx + hw, cy, cx, cy + hh, cx, cy);
    f[2] = rk_semiplano_arista_q4(cx, cy + hh, cx - hw, cy, cx, cy);
    f[3] = rk_semiplano_arista_q4(cx - hw, cy, cx, cy - hh, cx, cy);
    rk_aa_pintar(fb, f, 4, c, alfa,
                 piso_px(cx - hw) - 1, piso_px(cy - hh) - 1,
                 techo_px(cx + hw) + 1, techo_px(cy + hh) + 1);
}

void rk_aa_arco(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t r, int32_t grosor,
                bool abajo, rk_color_t c)
{
    rk_forma_t f[2];
    int32_t medio = grosor / 2;

    f[0] = rk_anillo_q4(cx, cy, r, grosor);
    f[1] = abajo ? rk_semiplano_abajo_q4(cx, cy, 0)
                 : rk_semiplano_arriba_q4(cx, cy, 0);
    rk_aa_pintar(fb, f, 2, c, 255, 0, 0, 1 << 15, 1 << 15);
    /* Las puntas redondeadas: sin ellas el arco termina en un corte recto,
     * que es la marca de "dibujado por computadora" que este estilo evita. */
    rk_aa_circulo(fb, cx - r, cy, medio, c);
    rk_aa_circulo(fb, cx + r, cy, medio, c);
}

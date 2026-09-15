#include "gacha.h"
#include "../gfx/font.h"
#include "../art/adulto.h"
#include <stddef.h>

#define C_FONDO   RK_RGB( 10,  14,  12)
#define C_INK     RK_RGB(232, 240, 226)
#define C_INK_DIM RK_RGB(138, 152, 136)

rk_color_t rk_rarity_color(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:      return RK_RGB(198, 208, 196);   /* hueso   */
    case RK_RAR_RARO:       return RK_RGB( 72, 214, 190);   /* cian    */
    case RK_RAR_EPICO:      return RK_RGB(178, 122, 232);   /* violeta */
    case RK_RAR_LEGENDARIO: return RK_RGB(255, 190,  70);   /* oro     */
    default:                return C_INK_DIM;
    }
}

rk_gacha_fase_t rk_gacha_fase(uint32_t t_ms)
{
    if (t_ms < RK_GACHA_CAIDA)      { return RK_GACHA_FASE_CAIDA; }
    if (t_ms < RK_GACHA_TEMBLOR)    { return RK_GACHA_FASE_TEMBLOR; }
    if (t_ms < RK_GACHA_ESTALLIDO)  { return RK_GACHA_FASE_ESTALLIDO; }
    if (t_ms < RK_GACHA_REVELADO)   { return RK_GACHA_FASE_REVELADO; }
    return RK_GACHA_FASE_REPOSO;
}

bool rk_gacha_termino(uint32_t t_ms)
{
    return t_ms >= RK_GACHA_FIN;
}

/* --------------------------------------------------------- la capsula ---- */
/* Una bolita de gachapon: dos mitades, la de arriba translúcida y la de abajo
 * sólida, con una banda ecuatorial. `abertura` separa las mitades. */
static void capsula(rk_fb_t *fb, int cx, int cy, int r, int abertura,
                    rk_color_t tono)
{
    int i;

    /* mitad inferior */
    for (i = 0; i <= r; i++) {
        int w = 0, t = r * r - i * i;
        while ((w + 1) * (w + 1) <= t) { w++; }
        rk_hline(fb, cx - w, cy + i + abertura / 2, w * 2 + 1,
                 rk_mix(RK_RGB(40, 52, 46), tono, (uint8_t)(40 + i * 2)));
    }
    /* mitad superior, más clara: lee como plástico transparente */
    for (i = 0; i <= r; i++) {
        int w = 0, t = r * r - i * i;
        while ((w + 1) * (w + 1) <= t) { w++; }
        rk_hline(fb, cx - w, cy - i - abertura / 2, w * 2 + 1,
                 rk_mix(tono, RK_RGB(255, 255, 255), (uint8_t)(90 - i * 2)));
    }
    /* banda ecuatorial */
    rk_fill_rect(fb, cx - r, cy - abertura / 2 - 1, r * 2 + 1, 2,
                 rk_mix(tono, 0x0000, 90));
    rk_fill_rect(fb, cx - r, cy + abertura / 2 - 1, r * 2 + 1, 2,
                 rk_mix(tono, 0x0000, 90));
    /* brillo especular */
    rk_disc(fb, cx - r / 2, cy - r / 2 - abertura / 2, r / 6,
            RK_RGB(255, 255, 255));
}

/* Rayos de luz que escapan por la juntura antes de estallar. */
static void grietas(rk_fb_t *fb, int cx, int cy, int r, int fuerza,
                    rk_color_t tono, uint32_t t_ms)
{
    int i;
    for (i = 0; i < 7; i++) {
        uint16_t h = rk_hash((uint16_t)(i * 733u + 11u));
        int dx = (int)(h % (unsigned)(r * 2)) - r;
        int alto = 2 + (int)((h >> 5) % 5u) + fuerza / 12;
        int ph = rk_sin8((uint8_t)(t_ms / 5u + i * 30u));
        rk_vline(fb, cx + dx, cy - alto - ph / 60, alto * 2, tono);
    }
}

/* El estallido: destellos radiales, tantos y tan lejos como diga la rareza. */
static void destellos(rk_fb_t *fb, int cx, int cy, int n, int avance,
                      rk_color_t tono)
{
    int i;

    for (i = 0; i < n; i++) {
        uint16_t h  = rk_hash((uint16_t)(i * 1597u + 7u));
        int      ang = (int)(h % 256u);
        /* Cada destello sale con su propia velocidad: un frente perfectamente
         * circular se lee como un anillo y no como una explosion. */
        int      vel = 60 + (int)((h >> 4) % 70u);
        int      d   = avance * vel / 100;
        int      x   = cx + rk_sin8((uint8_t)(ang + 64)) * d / 127;
        int      y   = cy + rk_sin8((uint8_t)ang) * d / 127;
        int      sz;
        uint8_t  fade;

        /* Brillante casi todo el trayecto y recien apagandose al final. Antes
         * se apagaba desde el primer cuadro y el estallido no se veia. */
        if (avance < 130) {
            fade = 0;
            sz   = 3;
        } else {
            fade = (uint8_t)((avance - 130) * 255 / 70);
            sz   = (avance < 170) ? 2 : 1;
        }

        rk_fill_rect(fb, x - sz / 2, y - sz / 2, sz, sz,
                     rk_mix(tono, C_FONDO, fade));
        /* Una cruz de un pixel alrededor de los mas grandes: convierte un
         * cuadradito en un destello. */
        if (sz >= 3) {
            rk_color_t pale = rk_mix(tono, RK_RGB(255, 255, 255), 140);
            rk_px(fb, x - 2, y, pale);
            rk_px(fb, x + 2, y, pale);
            rk_px(fb, x, y - 2, pale);
            rk_px(fb, x, y + 2, pale);
        }
    }
}

/* ------------------------------------------------------------ pantalla --- */
void rk_gacha_draw(rk_fb_t *fb, const rk_companion_t *c, uint32_t t_ms)
{
    rk_rarity_t rar;
    rk_color_t  tono;
    int cx, cy, W, H, R, esc, i;

    if (fb == NULL) {
        return;
    }
    rar  = rk_rarity_of(c);
    tono = rk_rarity_color(rar);
    W = fb->w;
    H = fb->h;
    cx = W / 2;
    /* La cápsula se asienta a un tercio del alto desde arriba: deja abajo el
     * espacio del cartel, del nombre y del lema. Antes era una constante de
     * 108 px atada al lienzo viejo; ahora la ceremonia entra igual en el
     * Prime de 240x320 que en el Mini de 128x128. */
    cy = H * 34 / 100;
    R  = W / 7;                     /* radio de la cápsula                  */
    esc = (W >= 200) ? 2 : 1;       /* el adulto a 2x sólo entra en el Prime */

    /* Fondo: se aclara hacia el tono de la rareza a medida que avanza, lo que
     * anticipa el premio sin mostrarlo todavía. */
    {
        uint8_t carga = (uint8_t)(t_ms > RK_GACHA_ESTALLIDO ? 60u
                                  : t_ms * 60u / RK_GACHA_ESTALLIDO);
        rk_vgradient(fb, 0, 0, W, H,
                     rk_mix(C_FONDO, tono, carga / 3),
                     rk_mix(RK_RGB(22, 30, 26), tono, carga));
    }

    if (t_ms < RK_GACHA_CAIDA) {
        /* Cae desde arriba y rebota una vez. */
        uint32_t p = t_ms * 100u / RK_GACHA_CAIDA;
        int caida = (int)(cy * (100 - p) * (100 - p) / 10000);
        int rebote = (p > 80u) ? (int)(100u - p) / 2 : 0;
        capsula(fb, cx, cy - caida - rebote, R, 0, tono);

    } else if (t_ms < RK_GACHA_TEMBLOR) {
        uint32_t p = (t_ms - RK_GACHA_CAIDA) * 100u
                   / (RK_GACHA_TEMBLOR - RK_GACHA_CAIDA);
        /* El temblor se acelera: el intervalo del jitter se acorta con p. */
        int amp = 1 + (int)p / 25;
        int jx  = (int)((t_ms / (12u + (100u - p) / 6u)) % 2u) ? amp : -amp;
        grietas(fb, cx + jx, cy, R, (int)p, tono, t_ms);
        capsula(fb, cx + jx, cy, R, (int)p / 30, tono);

    } else if (t_ms < RK_GACHA_REVELADO) {
        uint32_t p = (t_ms - RK_GACHA_ESTALLIDO) * 100u
                   / (RK_GACHA_REVELADO - RK_GACHA_ESTALLIDO);
        if (t_ms < RK_GACHA_ESTALLIDO) { p = 0; }

        destellos(fb, cx, cy, rk_rarity_sparkles(rar), (int)p * 2, tono);

        /* Las dos mitades se van separando y saliendo de cuadro. */
        {
            int sep = (int)p * W / 80;
            capsula(fb, cx - sep / 2, cy - sep, R * 9 / 10, 0, tono);
            capsula(fb, cx + sep / 2, cy + sep, R * 9 / 10, 0, tono);
        }

        /* El simbionte sube desde abajo y crece. */
        if (p > 20u) {
            int sube = (int)(100u - (p > 100u ? 100u : p)) / 2;
            rk_adulto_draw(fb, cx, cy + sube, c, RK_MOOD_HAPPY, RK_SEV_OK,
                           esc, t_ms);
        }

    } else {
        /* Reposo: el bicho, su nombre, su rareza y su lema. Todo cuelga de
         * YR, la línea del cartel, que se calcula desde el BORDE INFERIOR del
         * bicho y no como fracción del alto: con una fracción fija el cartel
         * le quedaba encima de la panza en el Prime. */
        const int YR = cy + RK_ADULTO_H * esc / 2 + 6;
        int halo, halo0 = R * 14 / 10;
        for (halo = halo0; halo > halo0 / 2; halo -= 4) {
            rk_color_t h = rk_mix(C_FONDO, tono,
                                  (uint8_t)(30 - (halo0 - halo) / 2));
            rk_disc(fb, cx, cy - 4, halo, h);
        }
        /* Unos pocos destellos flotando: mantiene la escena viva sin ruido. */
        for (i = 0; i < rk_rarity_sparkles(rar) / 6; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 911u + 3u));
            int x = (int)(h % (unsigned)W);
            int y = H / 5 + (int)((h >> 5) % (unsigned)(H / 2));
            y += rk_sin8((uint8_t)(t_ms / 18u + i * 40u)) * 4 / 127;
            rk_px(fb, x, y, tono);
        }

        rk_adulto_draw(fb, cx, cy, c, RK_MOOD_HAPPY, RK_SEV_OK, esc, t_ms);

        /* Cartel de rareza. */
        {
            const char *rn = rk_rarity_name(rar);
            int w = rk_text_w(rn, 1) + 12;
            rk_fill_rect(fb, cx - w / 2, YR, w, 13, rk_mix(C_FONDO, tono, 45));
            rk_rect(fb, cx - w / 2, YR, w, 13, tono);
            rk_text_center(fb, cx, YR + 3, rn, tono, 1);
        }

        rk_text_center(fb, cx, YR + 20, c ? c->nombre : "???", C_INK, esc);

        if (c != NULL && c->lema != NULL) {
            /* El lema se parte en dos renglones por el espacio más cercano al
             * medio: a 160 px no entra de una y cortar por la mitad exacta
             * deja palabras rotas. */
            const char *s = c->lema;
            int len = 0, corte = -1, best = 1000, j;
            while (s[len] != '\0') { len++; }
            for (j = 0; j < len; j++) {
                if (s[j] == ' ') {
                    int d = (j > len / 2) ? j - len / 2 : len / 2 - j;
                    if (d < best) { best = d; corte = j; }
                }
            }
            if (YR + 54 >= H - 10) {
                /* No entra: en un panel chico el lema se omite antes que
                 * salirse de cuadro o pisar el pie. */
                corte = -2;
            }
            if (corte == -2) {
                /* nada */
            } else if (corte < 0 || rk_text_w(s, 1) <= W - 12) {
                rk_text_center(fb, cx, YR + 44, s, C_INK_DIM, 1);
            } else {
                char a[40], b[40];
                int n = corte < 39 ? corte : 39;
                for (j = 0; j < n; j++) { a[j] = s[j]; }
                a[n] = '\0';
                for (j = 0; j < 39 && s[corte + 1 + j] != '\0'; j++) {
                    b[j] = s[corte + 1 + j];
                }
                b[j] = '\0';
                rk_text_center(fb, cx, YR + 42, a, C_INK_DIM, 1);
                rk_text_center(fb, cx, YR + 52, b, C_INK_DIM, 1);
            }
        }

        if (rk_gacha_termino(t_ms) && ((t_ms / 600u) % 2u) == 0u) {
            rk_text_center(fb, cx, H - 14, "TOCA PARA CONTINUAR", C_INK_DIM, 1);
        }
    }

    /* Encabezado, siempre presente: dice qué está pasando. */
    rk_fill_rect(fb, 0, 0, W, 22, rk_mix(C_FONDO, 0x0000, 90));
    rk_text_center(fb, cx, 8, "HABITANTE DETECTADO", tono, 1);
}

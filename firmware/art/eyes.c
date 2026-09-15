#include "eyes.h"
#include "../gfx/fb.h"     /* rk_sin8, rk_hash */
#include "../gfx/font.h"   /* rk_text_w para el splash */
#include <stddef.h>

/* Geometría base de la cara. La OLED es de 128x32, así que los ojos son
 * anchos y bajos: hay mucho horizontal y muy poco vertical. */
#define CX_IZQ   44
#define CX_DER   84
#define CY       16

/* Cómo se comporta cada ojo en cada ánimo. Igual que en la Terminal, toda la
 * personalidad está concentrada en una tabla: agregar un estado es agregar
 * una fila. */
typedef enum {
    OJO_NORMAL = 0,
    OJO_FELIZ,        /* arco hacia arriba: ^ ^                */
    OJO_CANSADO,      /* párpado a media asta                  */
    OJO_ENORME,       /* pupila dilatada: susto o penumbra     */
    OJO_CERRADO,      /* línea curva                           */
    OJO_EQUIS,        /* X X                                   */
    OJO_RENDIJA,      /* entrecerrado desde arriba Y abajo     */
    OJO_ESPIRAL,      /* mareo                                 */
    OJO_RUIDO         /* estática: el Spore perdió el rumbo    */
} forma_ojo_t;

typedef struct {
    forma_ojo_t forma;
    int8_t  pupila_y;     /* desplazamiento vertical de la pupila      */
    uint8_t mira;         /* 1 = la pupila pasea sola                  */
    uint8_t tirita;       /* jitter horizontal en pixeles              */
    uint8_t parpadea;
    uint8_t respira;      /* el párpado sube y baja apenas             */
} cara_t;

static const cara_t CARAS[RK_MOOD_COUNT] = {
/* UNKNOWN     */ { OJO_ESPIRAL, 0, 0, 0, 0, 0 },
/* OFFLINE     */ { OJO_RUIDO,   0, 0, 0, 0, 0 },
/* SLEEPING    */ { OJO_CERRADO, 0, 0, 0, 0, 1 },
/* HAPPY       */ { OJO_FELIZ,   0, 1, 0, 1, 1 },
/* THIRSTY     */ { OJO_CANSADO, 2, 0, 0, 1, 0 },
/* DROWNING    */ { OJO_ENORME, -1, 0, 1, 0, 0 },
/* COLD        */ { OJO_RENDIJA, 0, 0, 2, 0, 0 },
/* HOT         */ { OJO_CANSADO, 2, 0, 0, 0, 1 },
/* SCORCHED    */ { OJO_EQUIS,   0, 0, 0, 0, 0 },
/* DARK        */ { OJO_ENORME,  0, 1, 0, 1, 0 },
/* PARCHED_AIR */ { OJO_NORMAL,  1, 1, 0, 1, 0 },
};

/* El ojo crece con la etapa. Es la señal de crecimiento más legible que hay
 * en monocromo: no hace falta ver el cuerpo para notar que el bicho maduró. */
static const struct { int rx, ry, sep; } TAMANO[RK_ETAPA_COUNT] = {
    { 10,  8,  0 },   /* ESPORA    */
    { 12,  9,  2 },   /* BROTE     */
    { 14, 11,  4 },   /* JOVEN     */
    { 16, 12,  6 },   /* MADURO    */
    { 17, 13,  8 },   /* ANCESTRAL */
};

/* ------------------------------------------------------------ un ojo ----- */
static void ojo(rk_mono_t *m, int cx, int cy, int rx, int ry,
                const cara_t *c, int px, int py, int cierre)
{
    int i;

    switch (c->forma) {

    case OJO_FELIZ:
        /* Dos arcos gruesos hacia arriba. Con un solo pixel de grosor a esta
         * escala el ojo feliz se lee como una raya y pierde toda la gracia. */
        rk_mono_arco(m, cx, cy + ry / 2, rx, ry, true, true);
        rk_mono_arco(m, cx, cy + ry / 2 + 1, rx, ry, true, true);
        rk_mono_arco(m, cx, cy + ry / 2 + 2, rx - 1, ry, true, true);
        break;

    case OJO_CERRADO:
        rk_mono_arco(m, cx, cy, rx, ry / 2, false, true);
        rk_mono_arco(m, cx, cy + 1, rx, ry / 2, false, true);
        break;

    case OJO_EQUIS:
        for (i = -rx + 2; i <= rx - 2; i++) {
            int y = (i * (ry - 1)) / (rx - 1);
            rk_mono_px(m, cx + i, cy + y, true);
            rk_mono_px(m, cx + i, cy - y, true);
            rk_mono_px(m, cx + i, cy + y + 1, true);
            rk_mono_px(m, cx + i, cy - y + 1, true);
        }
        break;

    case OJO_RENDIJA: {
        /* Entrecerrado parejo desde arriba y desde abajo: la forma que toma
         * la cara cuando hace frio, distinta del parpado caido del cansancio. */
        int h = ry / 2;
        int pr = rx / 3;
        rk_mono_fill_round(m, cx - rx, cy - h, rx * 2, h * 2, h, true);
        rk_mono_disc(m, cx + px / 2, cy, pr, false);
        rk_mono_px(m, cx + px / 2 - pr / 2, cy - pr / 2, true);
        break;
    }

    case OJO_ESPIRAL:
        /* Tres anillos concéntricos: a 32 pixeles de alto una espiral real no
         * se distingue, y el anillo lee igual de mareado. */
        for (i = 2; i <= rx; i += 3) {
            rk_mono_arco(m, cx, cy, i, (i * ry) / rx, true, true);
            rk_mono_arco(m, cx, cy, i, (i * ry) / rx, false, true);
        }
        break;

    case OJO_RUIDO:
        /* Estática determinista: el mismo instante da el mismo ruido, así se
         * puede testear por hash. */
        for (i = 0; i < rx * ry * 3; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 2711 + cx));
            int      x = (int)(h % (unsigned)(rx * 2)) - rx;
            int      y = (int)((h >> 7) % (unsigned)(ry * 2)) - ry;
            if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry) {
                rk_mono_px(m, cx + x, cy + y, (h & 0x40u) != 0u);
            }
        }
        rk_mono_arco(m, cx, cy, rx, ry, true, true);
        rk_mono_arco(m, cx, cy, rx, ry, false, true);
        break;

    case OJO_ENORME:
    case OJO_CANSADO:
    case OJO_NORMAL:
    default: {
        int pr = (c->forma == OJO_ENORME) ? (rx * 3) / 5 : rx / 2;
        int lim_x = rx - pr - 2, lim_y = ry - pr - 1;

        /* La pupila se queda adentro. Sin esto, con la pupila dilatada de
         * DROWNING el disco se come el borde y el ojo queda como una luna. */
        if (lim_x < 0) { lim_x = 0; }
        if (lim_y < 0) { lim_y = 0; }
        if (px >  lim_x) { px =  lim_x; }
        if (px < -lim_x) { px = -lim_x; }
        if (py >  lim_y) { py =  lim_y; }
        if (py < -lim_y) { py = -lim_y; }

        /* Blanco del ojo, pupila, y el párpado tapando desde arriba. */
        rk_mono_fill_round(m, cx - rx, cy - ry, rx * 2, ry * 2, ry, true);
        rk_mono_disc(m, cx + px, cy + py, pr, false);
        /* Un brillo de un pixel adentro de la pupila: es lo que separa un
         * agujero negro de un ojo. */
        rk_mono_px(m, cx + px - pr / 2, cy + py - pr / 2, true);

        if (cierre > 0) {
            rk_mono_fill_rect(m, cx - rx, cy - ry, rx * 2, cierre, false);
            rk_mono_arco(m, cx, cy - ry + cierre, rx, 2, false, true);
        }
        break;
    }
    }
}

/* -------------------------------------------------------------- cara ----- */
void rk_eyes_draw_stage(rk_mono_t *m, rk_mood_t mood, rk_stage_t etapa,
                        uint32_t t_ms)
{
    const cara_t *c;
    int rx, ry, sep, px = 0, py, cierre = 0, jitter = 0;

    if (m == NULL) {
        return;
    }
    if ((int)mood < 0 || mood >= RK_MOOD_COUNT) { mood = RK_MOOD_UNKNOWN; }
    if ((int)etapa < 0 || etapa >= RK_ETAPA_COUNT) { etapa = RK_ETAPA_ESPORA; }

    c   = &CARAS[mood];
    rx  = TAMANO[etapa].rx;
    ry  = TAMANO[etapa].ry;
    sep = TAMANO[etapa].sep;

    rk_mono_clear(m, false);

    /* La pupila pasea. Dos senoidales de período distinto dan un movimiento
     * que no se siente cíclico aunque lo sea. */
    if (c->mira) {
        px = rk_sin8((uint8_t)(t_ms / 37)) * (rx / 3) / 127;
        px += rk_sin8((uint8_t)(t_ms / 13)) * 1 / 127;
    }
    py = c->pupila_y;
    if (c->respira) {
        py += rk_sin8((uint8_t)(t_ms / 24)) * 1 / 127;
    }

    /* Parpadeo: 110 ms cada 3,7 s. El período es primo respecto del de la
     * mirada para que no se sincronicen y el bicho no quede robótico.
     *
     * Un parpadeo cierra el ojo sea cual sea su forma normal, así que se
     * sustituye la forma entera en vez de bajar el párpado: las formas de
     * arco —el ojo feliz, por ejemplo— no tienen párpado que bajar, y antes
     * de esto HAPPY declaraba parpadeo y nunca se le veía. */
    if (c->parpadea && (t_ms % 3700u) < 110u) {
        cara_t cerrado = *c;
        cerrado.forma = OJO_CERRADO;
        ojo(m, CX_IZQ - sep / 2, CY, rx, ry, &cerrado, 0, 0, 0);
        ojo(m, CX_DER + sep / 2, CY, rx, ry, &cerrado, 0, 0, 0);
        return;
    }
    if (c->forma == OJO_CANSADO) {
        /* Dos tercios y no la mitad: con el parpado a media asta el disco de
         * la pupila queda tapado entero y los tres estados cansados se
         * vuelven el mismo dibujo. Dejando asomar la pupila, la mirada baja
         * se lee como agotamiento. */
        cierre = (ry * 2) / 3;
    }

    if (c->tirita) {
        jitter = ((t_ms / 70u) % 2u) ? (int)c->tirita : -(int)c->tirita;
    }

    ojo(m, CX_IZQ - sep / 2 + jitter, CY, rx, ry, c, px, py, cierre);
    ojo(m, CX_DER + sep / 2 + jitter, CY, rx, ry, c, px, py, cierre);

    /* Zzz al costado cuando duerme: con los ojos cerrados es lo único que
     * distingue dormir de estar apagado. */
    if (mood == RK_MOOD_SLEEPING) {
        uint32_t f = (t_ms / 16u) % 200u;
        int zx = 108 + (int)f / 40;
        int zy = 20 - (int)f / 12;
        int s  = 1 + (int)f / 120;
        rk_mono_hline(m, zx, zy, 4 * s, true);
        rk_mono_hline(m, zx, zy + 3 * s, 4 * s, true);
        rk_mono_px(m, zx + 2 * s, zy + s, true);
        rk_mono_px(m, zx + s, zy + 2 * s, true);
    }

    /* Burbujas subiendo cuando se ahoga. Sin esto DROWNING y DARK comparten
     * el ojo dilatado y, de reojo, la maceta encharcada y la que esta a
     * oscuras se ven igual — que son dos problemas opuestos. */
    if (mood == RK_MOOD_DROWNING) {
        int k;
        for (k = 0; k < 4; k++) {
            uint16_t hh = rk_hash((uint16_t)(k * 617u + 5u));
            int bx = 6 + (int)(hh % 14u) + (k % 2) * 104;
            int by = 28 - (int)((t_ms / 26u + (uint32_t)k * 31u) % 28u);
            int r  = 1 + (int)((hh >> 6) % 2u);
            rk_mono_hline(m, bx - r, by, r * 2 + 1, true);
            rk_mono_px(m, bx, by - r, true);
            rk_mono_px(m, bx, by + r, true);
        }
    }

    /* Gotas de sudor cuando hace calor: salen de arriba y caen por el
     * costado. Es lo que separa "cansado por calor" de "cansado por sed". */
    if (mood == RK_MOOD_HOT) {
        int k;
        for (k = 0; k < 2; k++) {
            int gy = 2 + (int)((t_ms / 22u + (uint32_t)k * 13u) % 26u);
            int gx = (k == 0) ? 16 : 110;
            rk_mono_px(m, gx, gy, true);
            rk_mono_px(m, gx, gy + 1, true);
            rk_mono_px(m, gx - 1, gy + 1, true);
            rk_mono_px(m, gx + 1, gy + 1, true);
            rk_mono_px(m, gx, gy + 2, true);
        }
    }

    /* Una gota que cae cuando tiene sed. Un solo detalle, pero convierte una
     * cara cansada en un pedido concreto. */
    if (mood == RK_MOOD_THIRSTY) {
        int gy = 6 + (int)((t_ms / 30u) % 22u);
        rk_mono_px(m, 20, gy, true);
        rk_mono_px(m, 20, gy + 1, true);
        rk_mono_px(m, 19, gy + 1, true);
        rk_mono_px(m, 21, gy + 1, true);
    }
}

void rk_eyes_draw(rk_mono_t *m, rk_mood_t mood, uint32_t t_ms)
{
    rk_eyes_draw_stage(m, mood, RK_ETAPA_JOVEN, t_ms);
}

void rk_eyes_splash(rk_mono_t *m, const char *nombre, rk_stage_t etapa)
{
    int w;

    if (m == NULL) {
        return;
    }
    if ((int)etapa < 0 || etapa >= RK_ETAPA_COUNT) { etapa = RK_ETAPA_ESPORA; }

    rk_mono_clear(m, false);
    w = rk_text_w(nombre ? nombre : "?", 1);
    rk_mono_text(m, (RK_OLED_W - w) / 2, 6, nombre ? nombre : "?", true);

    w = rk_text_w(rk_stage_name(etapa), 1);
    rk_mono_text(m, (RK_OLED_W - w) / 2, 18, rk_stage_name(etapa), true);

    rk_mono_hline(m, 20, 15, RK_OLED_W - 40, true);
}

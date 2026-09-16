#include "persona.h"
#include <stddef.h>
#include <string.h>

/* Los seis modelos de la primera tanda.
 *
 * Están pensados para ser distintos DOS VECES: como silueta impresa a un
 * metro de distancia, y como cara en 128x128. Un modelo que se distingue
 * sólo en la cara no justifica una carcasa nueva, y uno que se distingue
 * sólo en la silueta deja la pantalla haciendo de relleno.
 *
 * Las medidas de los rasgos van en CENTÉSIMAS DEL ANCHO DEL PANEL, no en
 * pixeles: así la misma tabla sirve para 128x128 y para cualquier panel
 * futuro sin volver a tocar un número.
 */
const rk_persona_t rk_persona_table[] = {
{
    "cresta", "Cresta", "carcasas/cresta.stl",
    "No te va a agradecer. Igual regala.",
    RK_RAR_COMUN,
    /* Ojos angostos e inclinados hacia adentro, cejas despeinadas que caen
     * sobre ellos, boca con dientes. Todo el modelo empuja hacia el centro
     * de la cara, que es lo que el ojo humano lee como enojo. */
    RK_OJOS_FIEROS, 15, 11, 21, -12,
    RK_CEJA_DESPEINADA, -16, 13,
    RK_BOCA_DIENTES, 30,
    RK_ADORNO_COLMILLO,
    RK_RGB( 18,  26,  18), RK_RGB( 34,  52,  28),
    RK_RGB( 10,  14,  10), RK_RGB(226, 240, 214),
    RK_RGB(150, 214,  64), RK_RGB(178, 255,  72)
},
{
    "kawaii", "Kawaii", "carcasas/kawaii.stl",
    "Te quiere aunque la olvides. Eso es peor.",
    RK_RAR_COMUN,
    /* Ojos rasgados y arqueados, sin cejas, boca de gato, rubor y brillos.
     * Sin cejas a propósito: son el rasgo que más agresividad puede meter,
     * y acá no hay ninguna que meter. */
    RK_OJOS_RASGADOS, 15, 11, 22, 6,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_GATO, 20,
    RK_ADORNO_BRILLOS | RK_ADORNO_RUBOR,
    RK_RGB( 42,  22,  36), RK_RGB( 88,  44,  70),
    RK_RGB( 40,  16,  30), RK_RGB(255, 246, 250),
    RK_RGB(255, 140, 190), RK_RGB(255, 190, 224)
},
{
    "visor", "Visor", "carcasas/visor.stl",
    "Registra. No opina.",
    RK_RAR_COMUN,
    /* Sin ojos: una sola banda horizontal cuya ONDA es la expresión. Es el
     * modelo que prueba que el rig no depende de tener dos ojos, y el más
     * barato de imprimir: una cúpula lisa con una ranura. */
    RK_OJOS_VISOR, 34, 7, 0, 0,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_ONDA, 22,
    RK_ADORNO_SCANLINE,
    RK_RGB(  8,  14,  18), RK_RGB( 14,  30,  38),
    RK_RGB(  4,   8,  10), RK_RGB(180, 250, 255),
    RK_RGB( 64, 226, 232), RK_RGB( 64, 226, 232)
},
{
    "ciclope", "Ciclope", "carcasas/ciclope.stl",
    "Mira una sola cosa. La mira mucho.",
    RK_RAR_RARO,
    /* Un ojo enorme y centrado. La carcasa es una sola apertura circular,
     * tipo ojo de buey, y la pupila gigante hace casi toda la actuación. */
    RK_OJOS_UNICO, 30, 28, 0, 0,
    RK_CEJA_FINA, -4, 10,
    RK_BOCA_CHICA, 10,
    0u,
    RK_RGB( 32,  24,  10), RK_RGB( 68,  48,  16),
    RK_RGB( 16,  12,   6), RK_RGB(255, 244, 222),
    RK_RGB(255, 178,  46), RK_RGB(255, 214, 120)
},
{
    "hongo", "Hongo", "carcasas/hongo.stl",
    "Duerme. Crece igual.",
    RK_RAR_RARO,
    /* Párpados siempre a media asta y esporas subiendo. La carcasa es un
     * sombrero que vuela por encima de la pantalla y le da sombra, así que
     * la cara se diseñó oscura para que la sombra no la mate. */
    RK_OJOS_PESADOS, 16, 13, 22, 0,
    RK_CEJA_FINA, 6, 13,
    RK_BOCA_LINEA, 18,
    RK_ADORNO_ESPORAS,
    RK_RGB( 30,  22,  40), RK_RGB( 58,  42,  76),
    RK_RGB( 18,  12,  26), RK_RGB(238, 232, 246),
    RK_RGB(186, 150, 236), RK_RGB(212, 178, 255)
},
{
    "glitch", "?????", "carcasas/glitch.stl",
    "No estaba en la caja. Igual salio.",
    RK_RAR_SECRETO,
    /* El secreto. Se imprime en filamento translúcido, que no cuesta un peso
     * más y hace que la placa se vea por dentro: la carcasa deja de ocultar
     * al aparato y pasa a exhibirlo. La cara es la misma idea —ojos que no
     * terminan de decidirse— con estática encima. */
    RK_OJOS_REDONDOS, 16, 15, 23, 0,
    RK_CEJA_FINA, 0, 12,
    RK_BOCA_ONDA, 26,
    RK_ADORNO_ESTATICA | RK_ADORNO_BRILLOS,
    RK_RGB( 10,  10,  14), RK_RGB( 26,  20,  36),
    RK_RGB(196, 200, 214), RK_RGB(250, 250, 255),
    RK_RGB( 90, 255, 210), RK_RGB(255,  80, 180)
},
};

const int rk_persona_count =
    (int)(sizeof(rk_persona_table) / sizeof(rk_persona_table[0]));

const rk_persona_t *rk_persona_find(const char *id)
{
    int i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0; i < rk_persona_count; i++) {
        if (strcmp(rk_persona_table[i].id, id) == 0) {
            return &rk_persona_table[i];
        }
    }
    return NULL;
}

const rk_persona_t *rk_persona_at(int idx)
{
    if (idx < 0 || idx >= rk_persona_count) {
        return NULL;
    }
    return &rk_persona_table[idx];
}

int rk_persona_index(const rk_persona_t *p)
{
    int i;

    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < rk_persona_count; i++) {
        if (&rk_persona_table[i] == p) {
            return i;
        }
    }
    for (i = 0; i < rk_persona_count; i++) {
        if (strcmp(rk_persona_table[i].id, p->id) == 0) {
            return i;
        }
    }
    return -1;
}

const char *rk_rarity_name(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:   return "COMUN";
    case RK_RAR_RARO:    return "RARO";
    case RK_RAR_SECRETO: return "SECRETO";
    default:             return "?";
    }
}

rk_color_t rk_rarity_color(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:   return RK_RGB(198, 208, 196);   /* hueso   */
    case RK_RAR_RARO:    return RK_RGB( 72, 214, 190);   /* cian    */
    case RK_RAR_SECRETO: return RK_RGB(255, 190,  70);   /* oro     */
    default:             return RK_RGB(138, 152, 136);
    }
}

int rk_rarity_sparkles(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:   return 14;
    case RK_RAR_RARO:    return 40;
    case RK_RAR_SECRETO: return 90;
    default:             return 8;
    }
}

int rk_rarity_count(rk_rarity_t r)
{
    int i, n = 0;

    for (i = 0; i < rk_persona_count; i++) {
        if (rk_persona_table[i].rareza == r) {
            n++;
        }
    }
    return n;
}

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
 * Las medidas de los rasgos van en CENTÉSIMAS DEL LADO CORTO DEL PANEL, no en
 * pixeles: así la misma tabla sirve para la pantalla de 1,44" (128x128) y
 * para la de 2,2" (240x320) sin volver a tocar un número.
 *
 * ESTILO: ilustración plana, formas redondas y grandes, sin contorno negro,
 * bordes suavizados. Es la dirección de arte del tipo Duolingo. Los números
 * de esta tabla son un punto de partida para que la artista ajuste: cambiar
 * una proporción o un color es editar una fila, no tocar código.
 */
const rk_persona_t rk_persona_table[] = {
{
    "cresta", "Cresta", "carcasas/cresta.stl",
    "No te va a agradecer. Igual regala.",
    RK_RAR_COMUN,
    /* Ojos grandes con el párpado de arriba cortado en diagonal hacia la
     * nariz, cejas gruesas en mechón y una boca con dientes. Todo el modelo
     * empuja hacia el centro de la cara, que es lo que el ojo humano lee
     * como enojo aunque el ánimo sea bueno. */
    RK_OJOS_FIEROS, 15, 14, 20, 14,
    RK_CEJA_DESPEINADA, -12, 7,
    RK_BOCA_DIENTES, 11,
    RK_ADORNO_COLMILLO,
    RK_RGB( 98, 197,  54), RK_RGB( 72, 160,  38),
    RK_RGB( 33,  52,  24), RK_RGB(255, 255, 255),
    RK_RGB( 60, 120,  40), RK_RGB(255, 110, 140)
},
{
    "kawaii", "Kawaii", "carcasas/kawaii.stl",
    "Te quiere aunque la olvides. Eso es peor.",
    RK_RAR_COMUN,
    /* Ojos enormes con pupila grande y brillo doble, sin cejas, boca de
     * gato, rubor y destellos. Sin cejas a propósito: son el rasgo que más
     * agresividad puede meter, y acá no hay ninguna que meter. */
    RK_OJOS_RASGADOS, 16, 16, 21, 0,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_GATO, 6,
    RK_ADORNO_BRILLOS | RK_ADORNO_RUBOR,
    RK_RGB(255, 168, 208), RK_RGB(255, 112, 168),
    RK_RGB( 84,  32,  64), RK_RGB(255, 255, 255),
    RK_RGB(140,  80, 200), RK_RGB(255, 255, 255)
},
{
    "visor", "Visor", "carcasas/visor.stl",
    "Registra. No opina.",
    RK_RAR_COMUN,
    /* Sin ojos de verdad: una franja oscura con dos luces adentro, y la
     * forma de esas luces es toda la expresión. Sin boca: la visera de la
     * carcasa hace ese trabajo. */
    RK_OJOS_VISOR, 38, 15, 0, 0,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_NINGUNA, 0,
    RK_ADORNO_SCANLINE,
    RK_RGB( 58,  82, 106), RK_RGB( 44,  64,  84),
    RK_RGB( 18,  26,  36), RK_RGB(225, 250, 255),
    RK_RGB( 70, 215, 255), RK_RGB( 70, 215, 255)
},
{
    "ciclope", "Ciclope", "carcasas/ciclope.stl",
    "Mira una sola cosa. La mira mucho.",
    RK_RAR_RARO,
    /* Un ojo enorme y centrado con iris de color. La carcasa es una sola
     * apertura circular, tipo ojo de buey, y la pupila hace casi toda la
     * actuación. */
    RK_OJOS_UNICO, 27, 27, 0, 0,
    RK_CEJA_GRUESA, -4, 6,
    RK_BOCA_CHICA, 6,
    0u,
    RK_RGB(255, 168,  56), RK_RGB(236, 132,  28),
    RK_RGB( 46,  30,  12), RK_RGB(255, 255, 255),
    RK_RGB( 60, 150, 235), RK_RGB(255, 110, 140)
},
{
    "hongo", "Hongo", "carcasas/hongo.stl",
    "Duerme. Crece igual.",
    RK_RAR_RARO,
    /* Párpados siempre a media asta y esporas subiendo. La carcasa es un
     * sombrero que vuela por encima de la pantalla y le da sombra, así que
     * la cara se pensó con mucho contraste para que la sombra no la mate. */
    RK_OJOS_PESADOS, 15, 14, 20, 0,
    RK_CEJA_FINA, 8, 7,
    RK_BOCA_LINEA, 7,
    RK_ADORNO_ESPORAS,
    RK_RGB(186, 142, 242), RK_RGB(156, 112, 214),
    RK_RGB( 48,  28,  74), RK_RGB(255, 255, 255),
    RK_RGB(110,  70, 170), RK_RGB(250, 242, 255)
},
{
    "glitch", "?????", "carcasas/glitch.stl",
    "No estaba en la caja. Igual salio.",
    RK_RAR_SECRETO,
    /* El secreto. Se imprime en filamento translúcido, que no cuesta un peso
     * más y hace que la placa se vea por dentro. La cara es la misma idea:
     * ojos que no terminan de decidir de qué color son, con estática. */
    RK_OJOS_REDONDOS, 16, 16, 21, 0,
    RK_CEJA_FINA, 0, 7,
    RK_BOCA_ONDA, 9,
    RK_ADORNO_ESTATICA,
    RK_RGB( 28,  28,  38), RK_RGB( 44,  44,  58),
    RK_RGB(  8,   8,  14), RK_RGB(245, 248, 255),
    RK_RGB( 40, 240, 220), RK_RGB(255,  60, 170)
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

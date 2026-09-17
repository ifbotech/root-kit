#include "persona.h"
#include <stddef.h>
#include <string.h>

/* Los cinco Rooties botánicos.
 *
 * Distintos DOS VECES: como silueta impresa a un metro (una semilla con dos
 * hojitas, una cúpula de musgo, un cactus ovalado, un bulbo en gota, un
 * honguito con sombrero) y como cara en 128x128. Las siluetas y sus reglas
 * de impresión están en docs/carcasas.md; el cuerpo que dibuja la app, en
 * root-lab/public/lib/cuerpo.mjs.
 *
 * Los números de cada fila son un punto de partida para que la artista
 * ajuste: cambiar una proporción o un color es editar una fila.
 *
 * Formato de cada piel, para que tools/sincronizar-firmware.mjs la lea:
 *
 *   { "nombre", RK_HEX(fondo), RK_HEX(ojos), RK_HEX(piel), RK_HEX(rubor), adornos }
 */
const rk_persona_t rk_persona_table[] = {
{
    "brote", "Brote", "carcasas/brote.stl",
    "Todo le parece nuevo. Sobre todo vos.",
    /* El brote curioso: una semilla con dos hojitas. Ojos redondos y
     * enormes con los brillos espejados de un cachorro, una sonrisa chica y
     * dos chapitas de rubor redondas. Sin cejas: nada que le quite
     * inocencia. */
    RK_OJOS_REDONDOS, RK_BRILLO_CACHORRO, 14, 15, 22, -4,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_SUAVE, 7, 19,
    RK_MEJILLA_CIRCULO,
    {
        { "Hoja Nueva", RK_HEX(0xE8F5E9), RK_HEX(0x1B5E20), RK_HEX(0xA5D6A7), RK_HEX(0xFF8A80), 0u },
        { "Lavanda", RK_HEX(0xF3E5F5), RK_HEX(0x4A148C), RK_HEX(0xCE93D8), RK_HEX(0xEA80FC), RK_ADORNO_BRILLOS },
        { "Flor de Cerezo Dorada", RK_HEX(0xFFF8E1), RK_HEX(0xE65100), RK_HEX(0xFFE082), RK_HEX(0xFF5252), RK_ADORNO_CORONA | RK_ADORNO_BRILLOS },
    }
},
{
    "musgo", "Musgo", "carcasas/musgo.stl",
    "No hay apuro. Nunca hubo.",
    /* La esfera serena: una cúpula de musgo tipo almohadón. Ojos de media
     * luna con los párpados relajados, boca de gato y un rubor ancho y
     * bajito. Es el que menos se inmuta: sus ánimos se leen en los
     * párpados. */
    RK_OJOS_MEDIALUNA, RK_BRILLO_SIMPLE, 15, 13, 22, -2,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_GATO, 6, 18,
    RK_MEJILLA_HORIZONTAL,
    {
        { "Musgo", RK_HEX(0xF1F8E9), RK_HEX(0x33691E), RK_HEX(0xC5E1A5), RK_HEX(0xAED581), 0u },
        { "Glaciar", RK_HEX(0xE0F7FA), RK_HEX(0x006064), RK_HEX(0x80DEEA), RK_HEX(0x4DD0E1), RK_ADORNO_BRILLOS },
        { "Otoño Tostado", RK_HEX(0xFBE9E7), RK_HEX(0xBF360C), RK_HEX(0xFFAB91), RK_HEX(0xFF7043), RK_ADORNO_CORONA },
    }
},
{
    "pinchito", "Pinchito", "carcasas/pinchito.stl",
    "¡Hola! ¿Ya regaste? ¡Hola!",
    /* El cactus entusiasta: un óvalo con nervaduras suaves y una flor al
     * costado. Contento, los ojos quedan en arco "^ ^" y cada tanto guiña,
     * uno y después el otro. Sonríe con un dientito y las mejillas le
     * brillan. */
    RK_OJOS_ARCO, RK_BRILLO_SIMPLE, 13, 14, 21, -5,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_DIENTECITO, 8, 19,
    RK_MEJILLA_BRILLO,
    {
        { "Desierto", RK_HEX(0xE8F5E9), RK_HEX(0x2E7D32), RK_HEX(0xFFF176), RK_HEX(0xFF80AB), 0u },
        { "Melocotón", RK_HEX(0xFCE4EC), RK_HEX(0x880E4F), RK_HEX(0xF8BBD0), RK_HEX(0xFF4081), RK_ADORNO_BRILLOS },
        { "Medianoche Neón", RK_HEX(0xECEFF1), RK_HEX(0x0D47A1), RK_HEX(0x90CAF9), RK_HEX(0xFFD600), RK_ADORNO_AURA | RK_ADORNO_LUCES },
    }
},
{
    "bulbo", "Bulbo", "carcasas/bulbo.stl",
    "Sueña con flores que todavía no existen.",
    /* El soñador mágico: una gota que termina en espiral. Ojos enormes,
     * estilo Café, con dos puntos de luz; cejas redondeadas que flotan
     * separadas del ojo. La boca es mínima para que manden los ojos. */
    RK_OJOS_REDONDOS, RK_BRILLO_DOBLE, 17, 18, 23, -3,
    RK_CEJA_FLOTANTE, 6, 9,
    RK_BOCA_SUAVE, 5, 23,
    RK_MEJILLA_SUAVE,
    {
        { "Limonada", RK_HEX(0xFFFDE7), RK_HEX(0x827717), RK_HEX(0xFFF59D), RK_HEX(0xFFAB91), 0u },
        { "Lila Místico", RK_HEX(0xEDE7F6), RK_HEX(0x311B92), RK_HEX(0xB39DDB), RK_HEX(0xB388FF), RK_ADORNO_BRILLOS },
        { "Galáctico", RK_HEX(0xE8EAF6), RK_HEX(0x1A237E), RK_HEX(0x7986CB), RK_HEX(0xFF4081), RK_ADORNO_AURA | RK_ADORNO_BRILLOS },
    }
},
{
    "champi", "Champi", "carcasas/champi.stl",
    "Tiene hambre. Y sed. Y ganas de charlar.",
    /* El honguito glotón: un tallo macizo bajo un sombrero cónico que le
     * hace de visera a la pantalla. Ojos ovalados y altos, cejas finas, una
     * boca abierta en "D" y pecas. Como el sombrero le da sombra, la cara es
     * la de más contraste. */
    RK_OJOS_REDONDOS, RK_BRILLO_SIMPLE, 12, 16, 21, -3,
    RK_CEJA_FINA, 4, 8,
    RK_BOCA_D, 9, 21,
    RK_MEJILLA_PECAS,
    {
        { "Bosque", RK_HEX(0xEFEBE9), RK_HEX(0x3E2723), RK_HEX(0xD7CCC8), RK_HEX(0xFF8A80), 0u },
        { "Amanita Rosa", RK_HEX(0xFCE4EC), RK_HEX(0xAD1457), RK_HEX(0xF48FB1), RK_HEX(0xFFCDD2), RK_ADORNO_BRILLOS },
        { "Bioluminiscente", RK_HEX(0xE0F2F1), RK_HEX(0x004D40), RK_HEX(0x80CBC4), RK_HEX(0x69F0AE), RK_ADORNO_AURA | RK_ADORNO_LUCES },
    }
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

const rk_piel_t *rk_persona_piel(const rk_persona_t *p, int rareza)
{
    if (p == NULL) {
        return NULL;
    }
    if (rareza < 0 || rareza >= (int)RK_RAREZA_COUNT) {
        rareza = (int)RK_RAREZA_COMUN;
    }
    return &p->pieles[rareza];
}

const char *rk_rareza_id(rk_rareza_t r)
{
    switch (r) {
    case RK_RAREZA_COMUN: return "comun";
    case RK_RAREZA_RARA:  return "raro";
    case RK_RAREZA_EPICA: return "epico";
    default:              return "?";
    }
}

const char *rk_rareza_nombre(rk_rareza_t r)
{
    switch (r) {
    case RK_RAREZA_COMUN: return "COMUN";
    case RK_RAREZA_RARA:  return "RARA";
    case RK_RAREZA_EPICA: return "EPICA";
    default:              return "?";
    }
}

int rk_rareza_parse(const char *id)
{
    int r;

    if (id == NULL) {
        return -1;
    }
    for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
        if (strcmp(id, rk_rareza_id((rk_rareza_t)r)) == 0) {
            return r;
        }
    }
    return -1;
}

rk_color_t rk_rareza_color(rk_rareza_t r)
{
    switch (r) {
    case RK_RAREZA_COMUN: return RK_RGB(198, 208, 196);   /* hueso   */
    case RK_RAREZA_RARA:  return RK_RGB(186, 140, 255);   /* lila    */
    case RK_RAREZA_EPICA: return RK_RGB(255, 190,  70);   /* oro     */
    default:              return RK_RGB(138, 152, 136);
    }
}

int rk_rareza_destellos(rk_rareza_t r)
{
    switch (r) {
    case RK_RAREZA_COMUN: return 14;
    case RK_RAREZA_RARA:  return 40;
    case RK_RAREZA_EPICA: return 90;
    default:              return 8;
    }
}

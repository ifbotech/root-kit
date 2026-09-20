#include "persona.h"
#include <stddef.h>
#include <string.h>

/* Los cinco Rooties botánicos.
 *
 * Distintos DOS VECES: como figura a un metro (una semilla con dos hojas, un
 * almohadón de musgo con esporas, un cactus barril con flor, una cebolla con
 * un brote, un hongo con sombrero) y como cara en 128x128. Los cuerpos en 3D
 * —y la figura que se imprime— están en root-lab/public/lib/rooti3d/formas.mjs;
 * las reglas de impresión, en docs/carcasas.md; de dónde sale cada uno y qué
 * lo separa de sus referencias, en docs/rooties.md.
 *
 * Los números de cada fila son un punto de partida para que la artista
 * ajuste: cambiar una proporción o un color es editar una fila.
 *
 * Formato de cada piel, para que tools/sincronizar-firmware.mjs la lea:
 *
 *   { "nombre", RK_HEX(fondo), RK_HEX(ojos), RK_HEX(piel), RK_HEX(rubor), RK_HEX(acento), adornos }
 *
 * `fondo` y `piel` van iguales: la cara se pinta sobre el cuerpo (persona.h).
 */
const rk_persona_t rk_persona_table[] = {
{
    "brote", "Brote", "carcasas/brote.stl",
    "Todo le parece nuevo. Sobre todo vos.",
    /* El brote curioso: una semilla gordita, ancha abajo, de la que salen
     * dos cotiledones redondos en V. Ojos redondos y enormes con los
     * brillos espejados de un cachorro, una sonrisa chica y dos chapitas de
     * rubor redondas. Sin cejas: nada que le quite inocencia. */
    RK_OJOS_REDONDOS, RK_BRILLO_CACHORRO, 14, 15, 22, -4,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_SUAVE, 7, 19,
    RK_MEJILLA_CIRCULO,
    {
        { "Brote Tierno", RK_HEX(0xD4F26E), RK_HEX(0x2A2140), RK_HEX(0xD4F26E), RK_HEX(0xFF7DA6), RK_HEX(0x3DBF6B), 0u },
        { "Cereza", RK_HEX(0xFFB3D0), RK_HEX(0x4A1530), RK_HEX(0xFFB3D0), RK_HEX(0xFF6F9E), RK_HEX(0xE8457A), RK_ADORNO_BRILLOS },
        { "Sol Dorado", RK_HEX(0xFFDA5C), RK_HEX(0x3A2015), RK_HEX(0xFFDA5C), RK_HEX(0xFF5E6C), RK_HEX(0xFF8A2B), RK_ADORNO_CORONA | RK_ADORNO_BRILLOS },
    }
},
{
    "musgo", "Musgo", "carcasas/musgo.stl",
    "No hay apuro. Nunca hubo.",
    /* La esfera serena: un almohadón de musgo con flecos en capas, del que
     * asoman dos esporofitos —los tallitos con cápsula que tiene el musgo de
     * verdad—. Ojos de media luna con los párpados relajados, boca de gato y
     * un rubor ancho y bajito. Es el que menos se inmuta: sus ánimos se leen
     * en los párpados. */
    RK_OJOS_MEDIALUNA, RK_BRILLO_SIMPLE, 15, 13, 22, -2,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_GATO, 6, 18,
    RK_MEJILLA_HORIZONTAL,
    {
        { "Musgo", RK_HEX(0x74DDB5), RK_HEX(0x113329), RK_HEX(0x74DDB5), RK_HEX(0xFF8FA0), RK_HEX(0xFF9A3C), 0u },
        { "Glaciar", RK_HEX(0x94DEFF), RK_HEX(0x0F2E4A), RK_HEX(0x94DEFF), RK_HEX(0xFF9EC8), RK_HEX(0x3F6BFF), RK_ADORNO_BRILLOS },
        { "Aurora", RK_HEX(0xFFA9DC), RK_HEX(0x3D1238), RK_HEX(0xFFA9DC), RK_HEX(0xFF5FA8), RK_HEX(0xFFE066), RK_ADORNO_AURA | RK_ADORNO_LUCES },
    }
},
{
    "pinchito", "Pinchito", "carcasas/pinchito.stl",
    "¡Hola! ¿Ya regaste? ¡Hola!",
    /* El cactus entusiasta: un barril con costillas y pinchitos, una flor
     * arriba y un brazo levantado que saluda. Contento, los ojos quedan en
     * arco "^ ^" y cada tanto guiña, uno y después el otro. Sonríe con un
     * dientito y las mejillas le brillan. */
    RK_OJOS_ARCO, RK_BRILLO_SIMPLE, 13, 14, 21, -5,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_DIENTECITO, 8, 19,
    RK_MEJILLA_BRILLO,
    {
        { "Desierto", RK_HEX(0x8FE27A), RK_HEX(0x16361C), RK_HEX(0x8FE27A), RK_HEX(0xFF7FB0), RK_HEX(0xFF4FA0), 0u },
        { "Atardecer", RK_HEX(0xFFB47C), RK_HEX(0x4A1E14), RK_HEX(0xFFB47C), RK_HEX(0xFF6A8A), RK_HEX(0xE8447F), RK_ADORNO_BRILLOS },
        { "Neón", RK_HEX(0x9CAEFF), RK_HEX(0x161B55), RK_HEX(0x9CAEFF), RK_HEX(0xFF6FD8), RK_HEX(0xFF4FE0), RK_ADORNO_AURA | RK_ADORNO_LUCES },
    }
},
{
    "bulbo", "Bulbo", "carcasas/bulbo.stl",
    "Sueña con flores que todavía no existen.",
    /* El soñador mágico: un bulbo de cebolla con gajos, que termina en una
     * punta de la que sale un brote, y raicitas por patas. Ojos enormes,
     * estilo Café, con dos puntos de luz; cejas redondeadas que flotan
     * separadas del ojo. La boca es mínima para que manden los ojos. */
    RK_OJOS_REDONDOS, RK_BRILLO_DOBLE, 17, 18, 23, -3,
    RK_CEJA_FLOTANTE, 6, 9,
    RK_BOCA_SUAVE, 5, 23,
    RK_MEJILLA_SUAVE,
    {
        { "Lavanda", RK_HEX(0xC8A4FF), RK_HEX(0x2A1450), RK_HEX(0xC8A4FF), RK_HEX(0xFF86C8), RK_HEX(0x6FDB7E), 0u },
        { "Menta", RK_HEX(0x8AECD2), RK_HEX(0x0E3A32), RK_HEX(0x8AECD2), RK_HEX(0xFF8FB0), RK_HEX(0xFF7AA0), RK_ADORNO_BRILLOS },
        { "Galáctico", RK_HEX(0x9C9CFF), RK_HEX(0x15114A), RK_HEX(0x9C9CFF), RK_HEX(0xFF6FC0), RK_HEX(0xFFD84D), RK_ADORNO_AURA | RK_ADORNO_BRILLOS },
    }
},
{
    "champi", "Champi", "carcasas/champi.stl",
    "Tiene hambre. Y sed. Y ganas de charlar.",
    /* El honguito glotón: un tallo macizo con anillo, bajo un sombrero de
     * campana que le hace de visera a la cara. La cara va en el tallo, que es
     * claro; el sombrero es el acento. Ojos ovalados y altos, cejas finas,
     * una boca abierta en "D" y pecas. */
    RK_OJOS_REDONDOS, RK_BRILLO_SIMPLE, 12, 16, 21, -3,
    RK_CEJA_FINA, 4, 8,
    RK_BOCA_D, 9, 21,
    RK_MEJILLA_PECAS,
    {
        { "Amanita", RK_HEX(0xFFE8CB), RK_HEX(0x3A1E14), RK_HEX(0xFFE8CB), RK_HEX(0xFF8A7A), RK_HEX(0xFF5A4F), 0u },
        { "Violeta", RK_HEX(0xF2E5FF), RK_HEX(0x2A1850), RK_HEX(0xF2E5FF), RK_HEX(0xFF8FC8), RK_HEX(0x9B6BFF), RK_ADORNO_BRILLOS },
        { "Bioluminiscente", RK_HEX(0xDBFFF3), RK_HEX(0x0E3A33), RK_HEX(0xDBFFF3), RK_HEX(0xFF7FB2), RK_HEX(0x22D9A8), RK_ADORNO_AURA | RK_ADORNO_LUCES },
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

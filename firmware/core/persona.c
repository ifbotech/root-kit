#include "persona.h"
#include <stddef.h>
#include <string.h>

/* Los cuatro Rooties, tal como los dibujó Rocío.
 *
 * Distintos DOS VECES: como figura a un metro (una cresta de tres rulos, un
 * corte bob con flequillo, dos cuernitos sobre un ojo enorme, una gota con
 * cabito) y como cara en 128x128. Los cuerpos en 3D están en
 * root-lab/public/lib/rooti3d/formas.mjs; quién es cada uno y de dónde sale,
 * en docs/rooties.md.
 *
 * Los números de cada fila son el punto de partida para que la artista
 * ajuste: cambiar una proporción o un color es editar una fila.
 *
 * LAS TRES PIELES COMPARTEN LA PALETA
 *
 * La paleta es del personaje, no de la rareza: es parte de quién es. Lo que
 * cambia de común a rara y a épica es el ACABADO, elegido para que vaya con
 * su carácter —el fuego con el piloto, el acero y el cristal con la crítica,
 * el oro con el cíclope, el aura con la empática—. Dentro de la misma familia
 * de colores sí se mueven los tonos, para que las tres se distingan quietas;
 * lo que las vuelve un premio es el movimiento.
 *
 * Formato de cada piel, para que tools/sincronizar-firmware.mjs la lea:
 *
 *   { "nombre", RK_HEX(fondo), RK_HEX(ojos), RK_HEX(piel), RK_HEX(rubor), RK_HEX(acento), adornos }
 *
 * `fondo` y `piel` van iguales: la cara se pinta sobre el cuerpo (persona.h).
 */
const rk_persona_t rk_persona_table[] = {
{
    "kip", "Kip", "carcasas/kip.stl",
    "Si sale mal, por lo menos sale rápido.",
    /* EL PILOTO AUDAZ. Cresta de tres rulos, cejas negras tupidas y ojos
     * rasgados de corte angular: con esas cejas, medio grado de inclinación
     * ya es una actitud. La boca sube de un lado nomás —media sonrisa de
     * quien acaba de clavar una pirueta— y el rubor es de carrera, no de
     * timidez. Paleta Fiery Red Sunset. */
    RK_OJOS_RASGADOS, RK_BRILLO_SIMPLE, 15, 14, 23, -4,
    RK_CEJA_GRUESA, -8, 7,
    RK_BOCA_LADEADA, 10, 23,
    RK_MEJILLA_CIRCULO,
    {
        { "Naranja Piloto", RK_HEX(0xFAA307), RK_HEX(0x03071E), RK_HEX(0xFAA307), RK_HEX(0xFF4D4D), RK_HEX(0xD00000), 0u },
        { "Ascua", RK_HEX(0xFFBA08), RK_HEX(0x03071E), RK_HEX(0xFFBA08), RK_HEX(0xFF4D4D), RK_HEX(0xD00000), RK_ADORNO_BRILLOS },
        { "Llamarada", RK_HEX(0xFFBA08), RK_HEX(0x03071E), RK_HEX(0xFFBA08), RK_HEX(0xD00000), RK_HEX(0xD00000), RK_ADORNO_FUEGO | RK_ADORNO_BRILLOS },
    }
},
{
    "nori", "Nori", "carcasas/nori.stl",
    "Lo estás haciendo bien. Por ahora.",
    /* LA CRÍTICA SOFISTICADA. Corte bob recto con flequillo pulcro, pecas y
     * ojos almendrados de esquinas rectificadas con la pupila grande: mira
     * de frente y juzga en silencio. La boca es corta y casi recta, porque
     * una sonrisa entera le sacaría el filo. Paleta Deep Sea Blue; el rostro
     * es un azul claro de la misma familia, para que el ojo marino se lea.
     * El rubor es azul, no rosa: no se sonroja, se enfría. */
    RK_OJOS_ALMENDRA, RK_BRILLO_DOBLE, 16, 16, 22, -3,
    RK_CEJA_FINA, -4, 9,
    RK_BOCA_SOBRIA, 9, 23,
    RK_MEJILLA_PECAS,
    {
        { "Azul Marea", RK_HEX(0xB8D0EA), RK_HEX(0x023E7D), RK_HEX(0xB8D0EA), RK_HEX(0x0466C8), RK_HEX(0x023E7D), 0u },
        { "Acero", RK_HEX(0xCBDEF2), RK_HEX(0x023E7D), RK_HEX(0xCBDEF2), RK_HEX(0x0353A4), RK_HEX(0x979DAC), RK_ADORNO_METAL },
        { "Cristal", RK_HEX(0xCBDEF2), RK_HEX(0x023E7D), RK_HEX(0xCBDEF2), RK_HEX(0x0466C8), RK_HEX(0x0353A4), RK_ADORNO_CRISTAL | RK_ADORNO_BRILLOS },
    }
},
{
    "blink", "Blink", "carcasas/blink.stl",
    "¡Todo increíble! ¿Cuál era el problema?",
    /* EL CÍCLOPE OPTIMISTA. Dos cuernitos redondeados y UN ojo enorme con
     * iris de bronce, que se abre de par en par cuando algo le gusta —o sea,
     * casi siempre—. Vive en su propio plano positivo y siempre sale ileso;
     * cuando algo no le cierra no se asusta: ladea la cabeza. Los dientes de
     * sierra son su única mueca, y dura poco. Paleta Royal Gold & Saffron. */
    RK_OJOS_UNICO, RK_BRILLO_DOBLE, 30, 29, 0, -2,
    RK_CEJA_GRUESA, 0, 8,
    RK_BOCA_SIERRA, 11, 27,
    RK_MEJILLA_CIRCULO,
    {
        { "Sol", RK_HEX(0xFFE169), RK_HEX(0x6B4A0B), RK_HEX(0xFFE169), RK_HEX(0xEDC531), RK_HEX(0xC9A227), 0u },
        { "Mostaza", RK_HEX(0xFAD643), RK_HEX(0x6B4A0B), RK_HEX(0xFAD643), RK_HEX(0xEDC531), RK_HEX(0xC9A227), RK_ADORNO_BRILLOS },
        { "Oro Real", RK_HEX(0xFFE169), RK_HEX(0x6B4A0B), RK_HEX(0xFFE169), RK_HEX(0xEDC531), RK_HEX(0xC9A227), RK_ADORNO_ORO | RK_ADORNO_CORONA },
    }
},
{
    "plum", "Plum", "carcasas/plum.stl",
    "Te extrañó, y estuviste todo el tiempo acá.",
    /* LA BERENJENITA EMPÁTICA. Cuerpo de gota con su cabito, ojos grandes y
     * húmedos con el brillo espejado de un cachorro, y rubores malva. No es
     * asustadiza: es tímida y leal. Cuando la planta está en apuros pone
     * ojos de súplica y se le caen los hombros, y da culpa no regar. Paleta
     * Vivid Nightfall. */
    RK_OJOS_REDONDOS, RK_BRILLO_CACHORRO, 17, 18, 22, -3,
    RK_CEJA_FINA, 6, 9,
    RK_BOCA_SUAVE, 7, 23,
    RK_MEJILLA_SUAVE,
    {
        { "Malva", RK_HEX(0xE0AAFF), RK_HEX(0x10002B), RK_HEX(0xE0AAFF), RK_HEX(0xC77DFF), RK_HEX(0x5A189A), 0u },
        { "Amatista", RK_HEX(0xC77DFF), RK_HEX(0x10002B), RK_HEX(0xC77DFF), RK_HEX(0x9D4EDD), RK_HEX(0x5A189A), RK_ADORNO_BRILLOS | RK_ADORNO_AURA },
        { "Nocturna", RK_HEX(0xC77DFF), RK_HEX(0x10002B), RK_HEX(0xC77DFF), RK_HEX(0xE0AAFF), RK_HEX(0x7B2CBF), RK_ADORNO_AURA | RK_ADORNO_LUCES },
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

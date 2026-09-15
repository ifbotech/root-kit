#include "companion.h"
#include <stddef.h>
#include <string.h>

/* Un habitante por especie. Los nombres siguen la convención del universo:
 * un archivo ejecutable, una biblioteca, un comprimido. Son procesos que
 * corren en la maceta. */
const rk_companion_t rk_companion_table[] = {
    { "tuga",  "Tuga.exe",   "monstera",
      "Lenta, testaruda, sobrevive a todo." },
    { "myco",  "Myco.zip",   "pothos",
      "Se propaga en silencio. Ya esta en tres macetas." },
    { "sable", "Sable.bin",  "sansevieria",
      "No pide nada. No perdona nada." },
    { "zam",   "Zam.sys",    "zamioculcas",
      "Funciona en modo seguro desde hace meses." },
    { "spine", "Spine.dll",  "cactus",
      "Guarda agua y rencores." },
    { "vera",  "Vera.sh",    "aloe",
      "Se repara sola. A veces demasiado." },
    { "filo",  "Filo.tar",   "filodendro",
      "Extiende ramas como quien abre pestanas." },
    { "fern",  "Fern.log",   "helecho",
      "Registra cada dia seco y te lo recuerda." },
    { "orqui", "Orqui.key",  "orquidea",
      "Florece cuando quiere. No cuando vos queres." },
    { "cala",  "Cala.gif",   "calathea",
      "Se mueve de noche. Nadie sabe adonde va." },
    { "lyra",  "Lyra.iso",   "ficus-lyrata",
      "Imagen completa de un arbol que te va a odiar." },
    { "bonz",  "Bonz.root",  "bonsai",
      "Un siglo comprimido en veinte centimetros." },
};

const int rk_companion_count =
    (int)(sizeof(rk_companion_table) / sizeof(rk_companion_table[0]));

const rk_companion_t *rk_companion_find(const char *id)
{
    int i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0; i < rk_companion_count; i++) {
        if (strcmp(rk_companion_table[i].id, id) == 0) {
            return &rk_companion_table[i];
        }
    }
    return NULL;
}

int rk_companion_index(const rk_companion_t *c)
{
    int i;

    if (c == NULL) {
        return -1;
    }
    for (i = 0; i < rk_companion_count; i++) {
        if (&rk_companion_table[i] == c) {
            return i;
        }
    }
    /* Puede venir una copia y no un puntero a la tabla: se compara por id. */
    for (i = 0; i < rk_companion_count; i++) {
        if (strcmp(rk_companion_table[i].id, c->id) == 0) {
            return i;
        }
    }
    return -1;
}

const rk_companion_t *rk_companion_for_species(const char *especie)
{
    int i;

    if (especie == NULL) {
        return NULL;
    }
    for (i = 0; i < rk_companion_count; i++) {
        if (strcmp(rk_companion_table[i].especie, especie) == 0) {
            return &rk_companion_table[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------- rareza ---- */
rk_rarity_t rk_rarity_from_difficulty(uint8_t dificultad)
{
    if (dificultad >= 80u) { return RK_RAR_LEGENDARIO; }
    if (dificultad >= 60u) { return RK_RAR_EPICO; }
    if (dificultad >= 30u) { return RK_RAR_RARO; }
    return RK_RAR_COMUN;
}

rk_rarity_t rk_rarity_of(const rk_companion_t *c)
{
    const rk_species_t *sp;

    if (c == NULL) {
        return RK_RAR_COMUN;
    }
    sp = rk_species_find(c->especie);
    if (sp == NULL) {
        return RK_RAR_COMUN;
    }
    return rk_rarity_from_difficulty(sp->dificultad);
}

const char *rk_rarity_name(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:       return "COMUN";
    case RK_RAR_RARO:        return "RARO";
    case RK_RAR_EPICO:       return "EPICO";
    case RK_RAR_LEGENDARIO:  return "LEGENDARIO";
    default:                 return "?";
    }
}

int rk_rarity_sparkles(rk_rarity_t r)
{
    switch (r) {
    case RK_RAR_COMUN:       return 10;
    case RK_RAR_RARO:        return 24;
    case RK_RAR_EPICO:       return 48;
    case RK_RAR_LEGENDARIO:  return 90;
    default:                 return 8;
    }
}

/* ------------------------------------------------------------ vinculo ---- */
/* Días SANOS acumulados que hacen falta para entrar en cada etapa. El umbral
 * es de días sanos y no de días transcurridos: una planta abandonada tiene un
 * simbionte que no crece, y ahí está toda la mecánica de vínculo. */
static const uint16_t UMBRAL[RK_ETAPA_COUNT] = { 0u, 7u, 30u, 90u, 180u };

void rk_bond_init(rk_bond_t *b)
{
    if (b == NULL) {
        return;
    }
    b->dias_vividos = 0;
    b->dias_sanos   = 0;
    b->racha        = 0;
    b->mejor_racha  = 0;
}

void rk_bond_dia(rk_bond_t *b, bool sano)
{
    if (b == NULL) {
        return;
    }
    if (b->dias_vividos < 0xFFFFu) {
        b->dias_vividos++;
    }
    if (sano) {
        if (b->dias_sanos < 0xFFFFu) {
            b->dias_sanos++;
        }
        if (b->racha < 0xFFFFu) {
            b->racha++;
        }
        if (b->racha > b->mejor_racha) {
            b->mejor_racha = b->racha;
        }
    } else {
        /* La racha se corta, pero los días sanos acumulados NO se pierden.
         * Castigar el olvido borrando meses de cuidado convierte un mal día
         * en motivo para abandonar el producto. */
        b->racha = 0;
    }
}

rk_stage_t rk_stage_from_bond(const rk_bond_t *b)
{
    int i;

    if (b == NULL) {
        return RK_ETAPA_ESPORA;
    }
    for (i = RK_ETAPA_COUNT - 1; i > 0; i--) {
        if (b->dias_sanos >= UMBRAL[i]) {
            return (rk_stage_t)i;
        }
    }
    return RK_ETAPA_ESPORA;
}

const char *rk_stage_name(rk_stage_t e)
{
    switch (e) {
    case RK_ETAPA_ESPORA:     return "ESPORA";
    case RK_ETAPA_BROTE:      return "BROTE";
    case RK_ETAPA_JOVEN:      return "JOVEN";
    case RK_ETAPA_MADURO:     return "MADURO";
    case RK_ETAPA_ANCESTRAL:  return "ANCESTRAL";
    default:                  return "?";
    }
}

uint16_t rk_stage_faltan(const rk_bond_t *b)
{
    rk_stage_t e;

    if (b == NULL) {
        return UMBRAL[RK_ETAPA_BROTE];
    }
    e = rk_stage_from_bond(b);
    if (e >= RK_ETAPA_COUNT - 1) {
        return 0;
    }
    return (uint16_t)(UMBRAL[e + 1] - b->dias_sanos);
}

uint8_t rk_stage_progreso(const rk_bond_t *b)
{
    rk_stage_t e;
    uint16_t desde, hasta, span;

    if (b == NULL) {
        return 0;
    }
    e = rk_stage_from_bond(b);
    if (e >= RK_ETAPA_COUNT - 1) {
        return 100;
    }
    desde = UMBRAL[e];
    hasta = UMBRAL[e + 1];
    span  = (uint16_t)(hasta - desde);
    if (span == 0u) {
        return 100;
    }
    return (uint8_t)(((uint32_t)(b->dias_sanos - desde) * 100u) / span);
}

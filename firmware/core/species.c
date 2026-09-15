#include "species.h"
#include <stddef.h>
#include <string.h>

/* Rangos aproximados a partir de guías de cuidado corrientes. NO son datos
 * calibrados: sirven para desarrollar y demostrar, y hay que reemplazarlos
 * por el dataset real antes de que esto salga a la calle.
 *
 * La última columna es la dificultad hortícola, de la que sale la rareza del
 * simbionte. Está ordenada de más fácil a más difícil a propósito: leerla de
 * corrido debería dar la misma sensación que tiene cualquiera que haya
 * matado un ficus. */
const rk_species_t rk_species_table[] = {
    /* id              nombre                     suelo    temp dC    HR   lux min  lux max  dif */
    { "sansevieria",  "Lengua de suegra",         8,  35,  150, 320,  30,     800,   30000,  10 },
    { "pothos",       "Potus",                   20,  55,  170, 300,  40,     500,   12000,  15 },
    { "zamioculcas",  "Zamioculca",              10,  40,  160, 300,  30,     400,   15000,  18 },
    { "cactus",       "Cactus / suculenta",       5,  25,  100, 380,  20,    5000,   80000,  25 },
    { "aloe",         "Aloe vera",                8,  30,  130, 350,  25,    3000,   50000,  28 },
    { "monstera",     "Monstera deliciosa",      25,  60,  180, 300,  50,    1000,   15000,  45 },
    { "filodendro",   "Filodendro",              25,  58,  180, 300,  50,     900,   14000,  40 },
    { "helecho",      "Helecho de Boston",       45,  80,  160, 260,  70,     600,    8000,  70 },
    { "orquidea",     "Orquídea phalaenopsis",   30,  60,  180, 290,  60,    1200,   10000,  75 },
    { "calathea",     "Calathea",                40,  70,  180, 280,  70,     800,    9000,  78 },
    { "ficus-lyrata", "Ficus lyrata",            25,  55,  180, 270,  50,    2000,   20000,  85 },
    { "bonsai",       "Bonsái de olmo",          30,  60,  150, 270,  55,    3000,   25000,  92 },
};

const int rk_species_count =
    (int)(sizeof(rk_species_table) / sizeof(rk_species_table[0]));

const rk_species_t *rk_species_find(const char *id)
{
    int i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0; i < rk_species_count; i++) {
        if (strcmp(rk_species_table[i].id, id) == 0) {
            return &rk_species_table[i];
        }
    }
    return NULL;
}

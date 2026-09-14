#include "species.h"
#include <stddef.h>
#include <string.h>

/* Valores de semilla, aproximados a partir de guías de cuidado corrientes.
 * NO son datos calibrados: sirven para desarrollar y demostrar, y hay que
 * reemplazarlos por el dataset real antes de que esto salga a la calle. */
const rk_species_t rk_species_table[] = {
    {
        "monstera", "Monstera deliciosa",
        25, 60,          /* suelo %      */
        180, 300,        /* 18,0 – 30,0 °C */
        50,              /* HR mínima %  */
        1000, 15000      /* lux          */
    },
    {
        "pothos", "Potus (Epipremnum aureum)",
        20, 55,
        170, 300,
        40,
        500, 12000
    },
    {
        "sansevieria", "Lengua de suegra",
        8, 35,
        150, 320,
        30,
        800, 30000
    },
    {
        "ficus-lyrata", "Ficus lyrata",
        25, 55,
        180, 270,
        50,
        2000, 20000
    },
    {
        "cactus", "Cactus / suculenta",
        5, 25,
        100, 380,
        20,
        5000, 80000
    },
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

/* species.h — rangos de confort por especie.
 *
 * Esta tabla es la que después reemplaza el dataset de Mi Flora / HHCC
 * (unas 3.500 especies con mínimos y máximos de humedad, luz, temperatura
 * y fertilidad). Las cinco entradas de species.c son sólo semilla para
 * poder desarrollar el simbionte antes de tener la base real cargada.
 */
#ifndef ROOTKIT_SPECIES_H
#define ROOTKIT_SPECIES_H

#include <stdint.h>

typedef struct {
    const char *id;           /* clave estable, la que guarda el Hub  */
    const char *nombre;       /* lo que se muestra en pantalla        */

    uint8_t  soil_min;        /* % de humedad de suelo                */
    uint8_t  soil_max;

    int16_t  temp_min_dc;     /* décimas de grado                     */
    int16_t  temp_max_dc;

    uint8_t  rh_min;          /* % de humedad relativa del aire       */

    uint32_t lux_min;         /* iluminancia diurna deseable          */
    uint32_t lux_max;
} rk_species_t;

extern const rk_species_t rk_species_table[];
extern const int          rk_species_count;

/* Devuelve NULL si el id no está en la tabla. */
const rk_species_t *rk_species_find(const char *id);

#endif /* ROOTKIT_SPECIES_H */

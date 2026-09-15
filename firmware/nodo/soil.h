/* soil.h — lectura cruda del sensor capacitivo a porcentaje de humedad.
 *
 * El sensor capacitivo v1.2/v2.0 entrega una tensión que BAJA cuando sube la
 * humedad: en aire seco marca alto, sumergido marca bajo. De ahí que la
 * calibración guarde dry_raw > wet_raw y el porcentaje se calcule invertido.
 *
 * Dos cosas que no son opcionales en este módulo:
 *
 *  - La calibración es por unidad. Dos sensores del mismo lote difieren
 *    fácil un 15% en los extremos, así que un mapeo fijo garantiza que la
 *    mitad de los Spores mientan.
 *  - Toda lectura pasa por un control de plausibilidad. Un sensor
 *    desconectado, en corto o con el cable cortado por la humedad entrega
 *    valores perfectamente representables pero físicamente imposibles, y sin
 *    este filtro el simbionte reacciona con total convicción a un cable
 *    suelto.
 */
#ifndef ROOTKIT_SOIL_H
#define ROOTKIT_SOIL_H

#include <stdint.h>
#include <stdbool.h>

/* Límites físicos del ADC del ESP32-C3 con divisor 2:3 y atenuación 11 dB.
 * Fuera de esta ventana no hay suelo posible: hay un problema eléctrico. */
#define RK_SOIL_RAW_FLOOR   150u   /* por debajo: corto o entrada a masa   */
#define RK_SOIL_RAW_CEIL   4000u   /* por encima: sensor desconectado      */

/* Separación mínima entre los dos puntos de calibración. Si el usuario
 * calibra con la sonda apenas húmeda en vez de sumergida, el rango queda
 * tan chico que el ruido del ADC se amplifica hasta volver inútil la lectura. */
#define RK_SOIL_CAL_MIN_SPAN 300u

typedef struct {
    uint16_t dry_raw;   /* lectura en aire, la más alta      */
    uint16_t wet_raw;   /* lectura sumergido, la más baja    */
} rk_soil_cal_t;

/* Calibración de fábrica: sirve para arrancar y para que un Spore sin
 * calibrar dé algo razonable, pero marca la trama como no calibrada. */
extern const rk_soil_cal_t RK_SOIL_CAL_DEFAULT;

bool rk_soil_cal_valid(const rk_soil_cal_t *cal);

/* Devuelve 0..100, o -1 si la lectura es implausible. */
int rk_soil_pct(const rk_soil_cal_t *cal, uint16_t raw);

/* Mediana de N lecturas. El capacitivo tiene picos de ruido de un par de
 * decenas de cuentas, y la mediana los descarta sin el retardo que
 * introduciría un promedio móvil. N impar, máximo 9. */
uint16_t rk_soil_median(const uint16_t *samples, int n);

#endif /* ROOTKIT_SOIL_H */

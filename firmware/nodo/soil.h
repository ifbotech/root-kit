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
 *    mitad de los nodos mientan.
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

/* Calibración de fábrica: sirve para arrancar y para que un nodo sin
 * calibrar dé algo razonable, pero marca la trama como no calibrada. */
extern const rk_soil_cal_t RK_SOIL_CAL_DEFAULT;

bool rk_soil_cal_valid(const rk_soil_cal_t *cal);

/* Devuelve 0..100, o -1 si la lectura es implausible. */
int rk_soil_pct(const rk_soil_cal_t *cal, uint16_t raw);

/* Mediana de N lecturas. El capacitivo tiene picos de ruido de un par de
 * decenas de cuentas, y la mediana los descarta sin el retardo que
 * introduciría un promedio móvil. N impar, máximo 9. */
uint16_t rk_soil_median(const uint16_t *samples, int n);

/* ---------------------------------------------------------- el riego ----
 *
 * EL AGUA QUE SE ESCURRE
 *
 * En tierra compactada o hidrofóbica (la de una planta que estuvo semanas
 * seca) el agua no entra: baja por los costados de la maceta y sale por el
 * plato. El sensor lo cuenta con claridad: la humedad SUBE de golpe (el agua
 * pasa por al lado de la sonda) y en media hora VUELVE casi adonde estaba.
 * Un riego que empapó también baja después, pero de a poco, durante días.
 *
 * Este detector mira sólo eso: una subida de al menos RK_RIEGO_SALTO_PCT
 * puntos en RK_RIEGO_SUBIDA_S o menos, seguida de perder más de
 * RK_RIEGO_PERDIDA_PCT por ciento de lo ganado dentro de RK_RIEGO_OBSERVA_S.
 * Si pasa, deja la bandera RK_FALLA_ESCURRE en la telemetría durante
 * RK_RIEGO_AVISO_S, para que la nube no anote un riego que no fue y la app
 * explique qué hacer (regar de a poco, en dos o tres veces).
 *
 * Es de estado chico a propósito: sobrevive al deep sleep en la memoria RTC
 * de la placa. La subida se mide entre dos muestras, así que con el muestreo
 * cada cinco minutos un riego se ve entre una lectura y la siguiente.
 */
#define RK_RIEGO_SALTO_PCT      25u   /* subida mínima para ser un riego       */
#define RK_RIEGO_SUBIDA_S      300u   /* ...en como mucho este tiempo          */
#define RK_RIEGO_OBSERVA_S    1800u   /* cuánto se mira después del pico       */
#define RK_RIEGO_PERDIDA_PCT    70u   /* perder tanto de lo ganado = escurrió  */
#define RK_RIEGO_AVISO_S     21600u   /* la bandera dura 6 h                   */

typedef enum {
    RK_RIEGO_NADA = 0,      /* la tierra hace lo suyo                      */
    RK_RIEGO_SUBIENDO,      /* hubo una subida brusca: se está observando  */
    RK_RIEGO_EMPAPO,        /* la subida se sostuvo: riego de verdad       */
    RK_RIEGO_ESCURRIO       /* la subida se fue enseguida: el agua escurrió */
} rk_riego_evento_t;

/* Todo en cero es un detector recién iniciado. */
typedef struct {
    bool     hay_ancla;
    uint8_t  ancla_pct;     /* la tierra antes de la subida                */
    uint32_t ancla_s;
    bool     observando;
    uint8_t  pico_pct;      /* lo más alto que llegó la subida             */
    uint32_t pico_s;
    bool     escurrio;      /* el último riego se escurrió                 */
    uint32_t escurrio_s;
} rk_riego_t;

void rk_riego_iniciar(rk_riego_t *r);

/* Una lectura más. `t_s` es el reloj monótono del aparato. */
rk_riego_evento_t rk_riego_paso(rk_riego_t *r, uint8_t soil_pct, uint32_t t_s);

/* ¿Hay que marcar RK_FALLA_ESCURRE en esta lectura? Verdadero durante
 * RK_RIEGO_AVISO_S después de un escurrimiento, o hasta un riego que empape. */
bool rk_riego_escurriendo(const rk_riego_t *r, uint32_t t_s);

#endif /* ROOTKIT_SOIL_H */

/* power.h — estado de carga y presupuesto energético de un nodo a batería.
 *
 * Vive acá y no en core/ porque es lo único del sistema que depende de que
 * haya una celda. Los números de autonomía de docs/hardware.md salen de acá.
 *
 * El modelo de consumo vive acá, en código testeado, y no en una planilla:
 * las cifras de autonomía que se publican salen de correr estas funciones,
 * de modo que si alguien cambia un parámetro del firmware el número de la
 * documentación cambia con él en vez de quedar desactualizado en silencio.
 */
#ifndef ROOTKIT_POWER_H
#define ROOTKIT_POWER_H

#include <stdint.h>
#include <stdbool.h>

/* Debajo de esto avisamos; debajo del corte el nodo se duerme para siempre
 * en vez de arrastrar la celda por debajo del umbral seguro del litio. */
#define RK_BATT_WARN_MV   3450u
#define RK_BATT_CUTOFF_MV 3150u

/* Estado de carga a partir de la tensión en reposo. Vale sólo con la radio
 * apagada: bajo carga la caída IR da varios cientos de milivolts y el
 * porcentaje saldría muy pesimista. */
uint8_t rk_batt_pct(uint16_t mv);
bool    rk_batt_is_low(uint16_t mv);
bool    rk_batt_is_critical(uint16_t mv);

/* Perfil de consumo.
 *
 * Los costos por evento van en NANOamperios-hora, no en micro: una medición
 * cuesta 0,25 uAh, y en enteros de microamperios-hora eso se redondea a cero
 * y desaparece del modelo. Con nAh los tres terminos conviven sin perder
 * resolucion y sin meter punto flotante, que el firmware no quiere.
 *
 *   una medicion  ~   250 nAh  (150 ms a 6 mA)
 *   una emision   ~ 28000 nAh  (1 s a 100 mA, con IP estatica y BSSID cacheado)
 *   dormir un dia ~ 40 uA * 24 h = 960000 nAh
 */
typedef struct {
    uint32_t sleep_ua;        /* corriente continua en deep sleep, en uA   */
    uint32_t measure_nah;     /* por medición: sensores + ADC + I2C        */
    uint32_t tx_nah;          /* por transmisión: asociación + envío       */
    uint32_t measures_per_day;
    uint32_t tx_per_day;
} rk_power_profile_t;

/* Consumo diario en nanoamperios-hora. */
uint32_t rk_power_daily_nah(const rk_power_profile_t *p);

/* Autonomía en días para una celda de `capacity_mah`, descontando
 * `derate_pct` por envejecimiento, frío y autodescarga. */
uint32_t rk_power_days(const rk_power_profile_t *p,
                       uint32_t capacity_mah, uint8_t derate_pct);

/* Qué fracción del consumo diario se va en dormir, en porcentaje. Es el
 * número que dice si conviene seguir optimizando el firmware o si ya hay que
 * ir a buscar microamperios al hardware. */
uint8_t rk_power_sleep_share(const rk_power_profile_t *p);

/* Perfiles de referencia usados en los tests y en la documentación. */
extern const rk_power_profile_t RK_PROFILE_INGENUO;   /* sin optimizar     */
extern const rk_power_profile_t RK_PROFILE_FIJO;      /* 15 min, siempre tx */
extern const rk_power_profile_t RK_PROFILE_ADAPTATIVO;

#endif /* ROOTKIT_POWER_H */

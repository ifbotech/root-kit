#include "power.h"
#include <stddef.h>

/* Curva de descarga de una 18650 de litio a corriente baja. A los microamperios
 * que consume un Mini la curva es prácticamente la de circuito abierto, que
 * es justo la condición en la que medimos: siempre con la radio apagada. */
static const struct { uint16_t mv; uint8_t pct; } CURVA[] = {
    { 4200, 100 }, { 4100,  92 }, { 4000,  85 }, { 3900,  76 },
    { 3800,  66 }, { 3700,  55 }, { 3600,  43 }, { 3500,  30 },
    { 3400,  18 }, { 3300,   9 }, { 3200,   3 }, { 3000,   0 },
};
#define NCURVA ((int)(sizeof(CURVA) / sizeof(CURVA[0])))

uint8_t rk_batt_pct(uint16_t mv)
{
    int i;

    if (mv >= CURVA[0].mv) {
        return 100;
    }
    if (mv <= CURVA[NCURVA - 1].mv) {
        return 0;
    }
    for (i = 1; i < NCURVA; i++) {
        if (mv >= CURVA[i].mv) {
            /* Interpolación lineal entre los dos puntos que lo encierran. */
            uint32_t span_mv  = (uint32_t)(CURVA[i - 1].mv - CURVA[i].mv);
            uint32_t span_pct = (uint32_t)(CURVA[i - 1].pct - CURVA[i].pct);
            uint32_t above    = (uint32_t)(mv - CURVA[i].mv);
            return (uint8_t)(CURVA[i].pct + (above * span_pct + span_mv / 2u) / span_mv);
        }
    }
    return 0;
}

bool rk_batt_is_low(uint16_t mv)      { return mv < RK_BATT_WARN_MV; }
bool rk_batt_is_critical(uint16_t mv) { return mv < RK_BATT_CUTOFF_MV; }

/* ---------------------------------------------------------- presupuesto -- */
/* Los tres perfiles corresponden a decisiones concretas de firmware, y la
 * diferencia entre ellos es el argumento entero a favor del muestreo
 * adaptativo:
 *
 *   INGENUO     LED de alimentación puesto, DHCP en cada despertar, scan
 *               completo de Wi-Fi, transmite siempre. Es lo que sale si uno
 *               no piensa el consumo.
 *   FIJO        LED desoldado, IP estática y BSSID cacheado, pero mide y
 *               transmite cada 15 minutos sin excepción.
 *   ADAPTATIVO  Desacopla medir de transmitir: mide cada 5 minutos, que es
 *               barato, y sólo transmite cuando algo cambió o cuando toca
 *               el latido de dos horas.
 */
const rk_power_profile_t RK_PROFILE_INGENUO = {
    300u,       /* 300 uA durmiendo: el LED de power de una placa clon    */
    400u,       /* medición sin apagar el sensor entre lecturas            */
    83000u,     /* 3 s a ~100 mA: asociación completa mas DHCP             */
    96u, 96u
};
/* Nota de calibracion: los tres perfiles comparten el costo por medicion y
 * por emision; lo que cambia es cuantas veces al dia ocurre cada cosa y el
 * consumo en reposo. Las cuentas de tx_per_day del perfil adaptativo salen
 * de la simulacion de una semana de test_nodo.c, no de una estimacion. */

const rk_power_profile_t RK_PROFILE_FIJO = {
    40u,        /* LED fuera, LDO de bajo reposo                           */
    250u,       /* sensor alimentado desde GPIO, apagado entre lecturas    */
    28000u,     /* 1 s a ~100 mA: IP estatica y BSSID en memoria RTC       */
    96u, 96u
};

const rk_power_profile_t RK_PROFILE_ADAPTATIVO = {
    40u,
    250u,
    28000u,
    288u,       /* mide cada 5 min de promedio                              */
    31u         /* medido: 220 emisiones en la semana simulada              */
};

uint32_t rk_power_daily_nah(const rk_power_profile_t *p)
{
    if (p == NULL) {
        return 0;
    }
    /* sleep_ua esta en microamperios: por 24 h da uAh, y por 1000 da nAh. */
    return p->sleep_ua * 24u * 1000u
         + p->measure_nah * p->measures_per_day
         + p->tx_nah * p->tx_per_day;
}

uint32_t rk_power_days(const rk_power_profile_t *p,
                       uint32_t capacity_mah, uint8_t derate_pct)
{
    uint32_t daily = rk_power_daily_nah(p);
    uint64_t usable_nah;

    if (daily == 0u) {
        return 0;
    }
    if (derate_pct > 90u) {
        derate_pct = 90u;
    }
    /* mAh -> nAh son seis ordenes de magnitud: 2200 mAh no entran en 32 bits. */
    usable_nah = (uint64_t)capacity_mah * 1000000ull
               * (uint64_t)(100u - derate_pct) / 100ull;
    return (uint32_t)(usable_nah / (uint64_t)daily);
}

uint8_t rk_power_sleep_share(const rk_power_profile_t *p)
{
    uint32_t daily = rk_power_daily_nah(p);

    if (daily == 0u) {
        return 0;
    }
    return (uint8_t)(((uint64_t)p->sleep_ua * 24u * 1000u * 100u) / daily);
}

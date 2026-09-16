#include "soil.h"
#include <stddef.h>
#include <string.h>

/* Valores medidos sobre un capacitivo v2.0 alimentado desde el LDO de 3,3 V,
 * con divisor 2:3 a la entrada del ADC. Son un punto de partida: cada unidad
 * se recalibra en producción y el resultado va a NVS. */
const rk_soil_cal_t RK_SOIL_CAL_DEFAULT = { 2650u, 1180u };

bool rk_soil_cal_valid(const rk_soil_cal_t *cal)
{
    if (cal == NULL) {
        return false;
    }
    if (cal->dry_raw <= cal->wet_raw) {
        return false;                      /* invertida: calibración mal hecha */
    }
    if ((uint16_t)(cal->dry_raw - cal->wet_raw) < RK_SOIL_CAL_MIN_SPAN) {
        return false;
    }
    if (cal->dry_raw > RK_SOIL_RAW_CEIL || cal->wet_raw < RK_SOIL_RAW_FLOOR) {
        return false;
    }
    return true;
}

int rk_soil_pct(const rk_soil_cal_t *cal, uint16_t raw)
{
    uint32_t span, above;

    if (!rk_soil_cal_valid(cal)) {
        return -1;
    }
    if (raw < RK_SOIL_RAW_FLOOR || raw > RK_SOIL_RAW_CEIL) {
        return -1;
    }

    /* Tolerancia del 20% del rango por fuera de los puntos de calibración:
     * tierra más seca que el aire de calibración o un sustrato muy conductivo
     * son posibles, pero el doble del rango ya no. */
    span = (uint32_t)(cal->dry_raw - cal->wet_raw);
    if ((uint32_t)raw > (uint32_t)cal->dry_raw + span / 5u) {
        return -1;
    }
    if ((uint32_t)raw + span / 5u < (uint32_t)cal->wet_raw) {
        return -1;
    }

    if (raw >= cal->dry_raw) {
        return 0;
    }
    if (raw <= cal->wet_raw) {
        return 100;
    }
    above = (uint32_t)(cal->dry_raw - raw);
    return (int)((above * 100u + span / 2u) / span);   /* redondeo al más cercano */
}

uint16_t rk_soil_median(const uint16_t *samples, int n)
{
    uint16_t tmp[9];
    int i, j;
    uint16_t k;

    if (samples == NULL || n <= 0) {
        return 0;
    }
    if (n > 9) {
        n = 9;
    }
    for (i = 0; i < n; i++) {
        tmp[i] = samples[i];
    }
    /* Inserción: con n <= 9 es más rápido y más chico que cualquier otra cosa. */
    for (i = 1; i < n; i++) {
        k = tmp[i];
        for (j = i - 1; j >= 0 && tmp[j] > k; j--) {
            tmp[j + 1] = tmp[j];
        }
        tmp[j + 1] = k;
    }
    return tmp[n / 2];
}

/* ---------------------------------------------------------- el riego ---- */
void rk_riego_iniciar(rk_riego_t *r)
{
    if (r != NULL) {
        memset(r, 0, sizeof *r);
    }
}

rk_riego_evento_t rk_riego_paso(rk_riego_t *r, uint8_t soil_pct, uint32_t t_s)
{
    if (r == NULL) {
        return RK_RIEGO_NADA;
    }
    if (soil_pct > 100u) {
        soil_pct = 100u;
    }
    if (!r->hay_ancla) {
        r->hay_ancla = true;
        r->ancla_pct = soil_pct;
        r->ancla_s = t_s;
        return RK_RIEGO_NADA;
    }

    if (r->observando) {
        uint32_t ganado = (uint32_t)(r->pico_pct - r->ancla_pct);

        if (soil_pct > r->pico_pct) {
            /* Sigue subiendo: el riego todavía no terminó. */
            r->pico_pct = soil_pct;
            r->pico_s = t_s;
            return RK_RIEGO_SUBIENDO;
        }
        if ((uint32_t)(r->pico_pct - soil_pct) * 100u >= ganado * RK_RIEGO_PERDIDA_PCT
            && t_s - r->pico_s <= RK_RIEGO_OBSERVA_S) {
            r->observando = false;
            r->escurrio = true;
            r->escurrio_s = t_s;
            r->ancla_pct = soil_pct;
            r->ancla_s = t_s;
            return RK_RIEGO_ESCURRIO;
        }
        if (t_s - r->pico_s > RK_RIEGO_OBSERVA_S) {
            /* Media hora después sigue mojada: empapó. Y un riego que empapa
             * cancela el aviso del que se escurrió. */
            r->observando = false;
            r->escurrio = false;
            r->ancla_pct = soil_pct;
            r->ancla_s = t_s;
            return RK_RIEGO_EMPAPO;
        }
        return RK_RIEGO_SUBIENDO;
    }

    if (soil_pct >= r->ancla_pct + RK_RIEGO_SALTO_PCT && t_s - r->ancla_s <= RK_RIEGO_SUBIDA_S) {
        r->observando = true;
        r->pico_pct = soil_pct;
        r->pico_s = t_s;
        return RK_RIEGO_SUBIENDO;
    }
    /* El ancla sigue a la tierra: el punto más bajo reciente. Si la humedad
     * sube despacio (menos de un salto cada cinco minutos), el ancla la sigue
     * y nunca hay "riego": una subida lenta no es un balde de agua. */
    if (soil_pct <= r->ancla_pct || t_s - r->ancla_s > RK_RIEGO_SUBIDA_S) {
        r->ancla_pct = soil_pct;
        r->ancla_s = t_s;
    }
    return RK_RIEGO_NADA;
}

bool rk_riego_escurriendo(const rk_riego_t *r, uint32_t t_s)
{
    if (r == NULL || !r->escurrio) {
        return false;
    }
    return t_s - r->escurrio_s <= RK_RIEGO_AVISO_S;
}

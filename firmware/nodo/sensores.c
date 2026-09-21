#include "sensores.h"
#include <stddef.h>
#include <string.h>

uint8_t rk_crc8_aht(const uint8_t *b, int n)
{
    uint8_t crc = 0xFFu;
    int i, k;

    for (i = 0; i < n; i++) {
        crc ^= b[i];
        for (k = 0; k < 8; k++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x31u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

bool rk_aht20_convertir(const uint8_t b[7], int16_t *temp_dc, uint8_t *rh_pct)
{
    uint32_t h, t;
    int32_t dc;

    if (b == NULL) {
        return false;
    }
    if (b[0] & 0x80u) {
        return false;                     /* todavía midiendo */
    }
    if (rk_crc8_aht(b, 6) != b[6]) {
        return false;
    }
    h = ((uint32_t)b[1] << 12) | ((uint32_t)b[2] << 4) | ((uint32_t)b[3] >> 4);
    t = (((uint32_t)b[3] & 0x0Fu) << 16) | ((uint32_t)b[4] << 8) | (uint32_t)b[5];

    /* RH = h / 2^20 * 100 ; T = t / 2^20 * 200 - 50. Con redondeo. */
    if (rh_pct != NULL) {
        uint32_t rh = (h * 100u + (1u << 19)) >> 20;
        *rh_pct = (uint8_t)(rh > 100u ? 100u : rh);
    }
    dc = (int32_t)(((uint64_t)t * 2000u + (1u << 19)) >> 20) - 500;
    if (temp_dc != NULL) {
        *temp_dc = (int16_t)dc;
    }
    return true;
}

uint8_t rk_crc8_sht2x(const uint8_t *b, int n)
{
    uint8_t crc = 0x00u;                  /* el del AHT20 arranca en 0xFF */
    int i, k;

    for (i = 0; i < n; i++) {
        crc ^= b[i];
        for (k = 0; k < 8; k++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x31u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

bool rk_sht21_convertir(const uint8_t b[6], int16_t *temp_dc, uint8_t *rh_pct)
{
    uint32_t st, srh;
    int32_t cent, rh10;

    if (b == NULL) {
        return false;
    }
    if (rk_crc8_sht2x(b, 2) != b[2] || rk_crc8_sht2x(&b[3], 2) != b[5]) {
        return false;
    }
    /* Los dos bits de abajo son de estado, no de medicion. */
    st = (((uint32_t)b[0] << 8) | (uint32_t)b[1]) & 0xFFFCu;
    srh = (((uint32_t)b[3] << 8) | (uint32_t)b[4]) & 0xFFFCu;

    /* Se hace la cuenta en centesimas y recien al final se redondea a
     * decimas: en decimas directas, 25,0 C salia 24,9. */
    cent = (int32_t)(((uint64_t)17572u * st) >> 16) - 4685;
    if (temp_dc != NULL) {
        *temp_dc = (int16_t)((cent + (cent >= 0 ? 5 : -5)) / 10);
    }
    rh10 = (int32_t)(((uint64_t)1250u * srh) >> 16) - 60;
    if (rh_pct != NULL) {
        int32_t pct = (rh10 + 5) / 10;
        if (pct < 0) {
            pct = 0;                      /* la formula da -6 % en el cero */
        }
        if (pct > 100) {
            pct = 100;                    /* y 118 % en el tope            */
        }
        *rh_pct = (uint8_t)pct;
    }
    return true;
}

uint32_t rk_bh1750_lux(uint16_t cuenta, uint8_t mtreg)
{
    if (mtreg == 0u) {
        mtreg = 69u;
    }
    /* cuenta / 1,2 * 69 / mtreg = cuenta * 5 * 69 / (6 * mtreg) */
    return (uint32_t)(((uint64_t)cuenta * 345u + (uint64_t)(3u * mtreg)) / (6u * (uint32_t)mtreg));
}

uint8_t rk_crc8_maxim(const uint8_t *b, int n)
{
    uint8_t crc = 0u;
    int i, k;

    for (i = 0; i < n; i++) {
        uint8_t v = b[i];
        for (k = 0; k < 8; k++) {
            uint8_t mezcla = (uint8_t)((crc ^ v) & 0x01u);
            crc >>= 1;
            if (mezcla) {
                crc ^= 0x8Cu;
            }
            v >>= 1;
        }
    }
    return crc;
}

bool rk_ds18b20_convertir(const uint8_t sp[9], int16_t *temp_dc)
{
    int16_t raw;
    int32_t dc;
    int i, unos = 0;

    if (sp == NULL) {
        return false;
    }
    for (i = 0; i < 9; i++) {
        if (sp[i] == 0xFFu) {
            unos++;
        }
    }
    if (unos == 9) {
        return false;                     /* nadie en la línea */
    }
    if (rk_crc8_maxim(sp, 8) != sp[8]) {
        return false;
    }
    raw = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);
    if (raw == 0x0550) {
        return false;                     /* 85,0 °C: el valor de encendido */
    }
    /* 1/16 °C -> décimas, redondeando al más cercano también en negativo. */
    dc = (int32_t)raw * 10;
    dc = (dc >= 0) ? (dc + 8) / 16 : -((-dc + 8) / 16);
    if (temp_dc != NULL) {
        *temp_dc = (int16_t)dc;
    }
    return true;
}

uint16_t rk_riel_mv(uint16_t adc_mv, uint32_t r_arriba, uint32_t r_abajo)
{
    uint32_t mv;
    if (r_abajo == 0u) {
        return 0u;
    }
    mv = (uint32_t)(((uint64_t)adc_mv * (r_arriba + r_abajo) + r_abajo / 2u) / r_abajo);
    return (uint16_t)(mv > 65535u ? 65535u : mv);
}

bool rk_riel_usb(uint16_t riel_mv)
{
    return riel_mv >= RK_RIEL_USB_MV;
}

void rk_sensores_telemetria(const rk_crudos_t *c, const rk_soil_cal_t *cal,
                            rk_telemetry_t *out)
{
    uint16_t riel;

    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof *out);
    out->valid = true;
    out->suelo_dc = RK_TEMP_NO_HAY;
    out->fallas = RK_FALLA_SUELO | RK_FALLA_AIRE | RK_FALLA_LUZ | RK_FALLA_SONDA;
    if (c == NULL) {
        return;
    }

    if (c->n_suelo > 0u) {
        int n = c->n_suelo > RK_SUELO_MUESTRAS ? RK_SUELO_MUESTRAS : c->n_suelo;
        uint16_t med;
        int pct;
        if ((n & 1) == 0) {
            n--;                          /* la mediana quiere impares */
        }
        med = rk_soil_median(c->suelo, n);
        pct = rk_soil_pct(cal != NULL ? cal : &RK_SOIL_CAL_DEFAULT, med);
        out->suelo_raw = med;
        if (pct >= 0) {
            out->soil_pct = (uint8_t)pct;
            out->fallas &= (uint8_t)~RK_FALLA_SUELO;
        }
    }

    /* El aire lo puede haber medido cualquiera de los dos sensores: el que
     * este poblado en los crudos es el que hay en la placa. Para todo lo de
     * mas arriba --el animo, la nube, el historial-- son lo mismo. */
    if ((c->sht_leido && rk_sht21_convertir(c->sht, &out->temp_dc, &out->rh_pct))
        || (c->aht_leido
            && rk_aht20_convertir(c->aht, &out->temp_dc, &out->rh_pct))) {
        out->fallas &= (uint8_t)~RK_FALLA_AIRE;
    } else {
        out->temp_dc = 0;
        out->rh_pct = 0u;
    }

    if (c->bh_leido) {
        out->lux = rk_bh1750_lux(c->bh1750, c->bh_mtreg);
        out->fallas &= (uint8_t)~RK_FALLA_LUZ;
    }

    if (c->ds_leido && rk_ds18b20_convertir(c->ds, &out->suelo_dc)) {
        out->fallas &= (uint8_t)~RK_FALLA_SONDA;
    } else {
        out->suelo_dc = RK_TEMP_NO_HAY;
    }

    riel = rk_riel_mv(c->riel_adc_mv, c->r_arriba, c->r_abajo);
    out->usb = rk_riel_usb(riel);
    /* Enchufado, el riel mide VBUS y no dice nada de la celda. Se reporta 0
     * ("no se sabe") en vez de un 100 % que sería mentira. */
    out->batt_mv = out->usb ? 0u : riel;
}

#include "historial.h"
#include <string.h>

#define MAGIA   0x31484B52u   /* "RKH1" */
#define VERSION 1u

uint32_t rk_crc32(const uint8_t *b, size_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    int k;

    for (i = 0; i < n; i++) {
        crc ^= b[i];
        for (k = 0; k < 8; k++) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    return ~crc;
}

void rk_historial_iniciar(rk_historial_t *h)
{
    if (h != NULL) {
        memset(h, 0, sizeof *h);
    }
}

uint8_t rk_registro_fallas(const rk_registro_t *r)
{
    if (r == NULL) {
        return 0u;
    }
    return (uint8_t)((r->banderas >> 4) | ((r->banderas & 0x08u) ? RK_FALLA_ESCURRE : 0u));
}

uint8_t rk_registro_severidad(const rk_registro_t *r)
{
    return r == NULL ? 0u : (uint8_t)((r->banderas >> 1) & 0x03u);
}

rk_registro_t rk_registro_desde(const rk_telemetry_t *t, uint32_t reloj_s,
                                uint8_t animo, uint8_t severidad)
{
    rk_registro_t r;

    memset(&r, 0, sizeof r);
    r.reloj_s = reloj_s;
    r.animo = animo;
    r.banderas = (uint8_t)((severidad & 0x03u) << 1);
    if (t == NULL) {
        r.suelo_pct = RK_HIST_SIN_DATO_U8;
        r.hr_pct = RK_HIST_SIN_DATO_U8;
        r.temp_dc = RK_TEMP_NO_HAY;
        r.suelo_dc = RK_TEMP_NO_HAY;
        r.lux = RK_HIST_SIN_LUX;
        return r;
    }
    r.suelo_pct = (t->fallas & RK_FALLA_SUELO) ? RK_HIST_SIN_DATO_U8 : t->soil_pct;
    r.hr_pct    = (t->fallas & RK_FALLA_AIRE) ? RK_HIST_SIN_DATO_U8 : t->rh_pct;
    r.temp_dc   = (t->fallas & RK_FALLA_AIRE) ? RK_TEMP_NO_HAY : t->temp_dc;
    r.suelo_dc  = (t->fallas & RK_FALLA_SONDA) ? RK_TEMP_NO_HAY : t->suelo_dc;
    r.lux       = (t->fallas & RK_FALLA_LUZ) ? RK_HIST_SIN_LUX : t->lux;
    r.bat_mv    = t->batt_mv;
    if (t->fallas & RK_FALLA_ESCURRE) {
        r.banderas |= 0x08u;
    }
    r.suelo_raw = t->suelo_raw;
    r.banderas  = (uint8_t)(r.banderas | (t->usb ? 1u : 0u) | ((t->fallas & 0x0Fu) << 4));
    return r;
}

void rk_historial_agregar(rk_historial_t *h, const rk_registro_t *r)
{
    if (h == NULL || r == NULL) {
        return;
    }
    if (h->cuenta < RK_HIST_CAP) {
        h->reg[(h->inicio + h->cuenta) % RK_HIST_CAP] = *r;
        h->cuenta++;
    } else {
        h->reg[h->inicio] = *r;
        h->inicio = (uint16_t)((h->inicio + 1u) % RK_HIST_CAP);
        h->perdidos++;
    }
}

const rk_registro_t *rk_historial_ver(const rk_historial_t *h, uint16_t i)
{
    if (h == NULL || i >= h->cuenta) {
        return NULL;
    }
    return &h->reg[(h->inicio + i) % RK_HIST_CAP];
}

void rk_historial_descartar(rk_historial_t *h, uint16_t n)
{
    if (h == NULL) {
        return;
    }
    if (n >= h->cuenta) {
        h->inicio = 0u;
        h->cuenta = 0u;
        return;
    }
    h->inicio = (uint16_t)((h->inicio + n) % RK_HIST_CAP);
    h->cuenta = (uint16_t)(h->cuenta - n);
}

/* ------------------------------------------------------------ bytes ------ */
static void p16(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }
static void p32(uint8_t *b, uint32_t v)
{
    b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8);
    b[2] = (uint8_t)(v >> 16); b[3] = (uint8_t)(v >> 24);
}
static uint16_t g16(const uint8_t *b) { return (uint16_t)(b[0] | (b[1] << 8)); }
static uint32_t g32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

size_t rk_historial_serializar(const rk_historial_t *h, uint8_t *buf, size_t cap)
{
    size_t n, i, k;

    if (h == NULL || buf == NULL) {
        return 0u;
    }
    n = RK_HIST_CAB_LEN + (size_t)h->cuenta * RK_HIST_REG_LEN + 4u;
    if (n > cap) {
        return 0u;
    }
    p32(buf, MAGIA);
    p16(buf + 4, VERSION);
    p16(buf + 6, h->cuenta);
    p32(buf + 8, h->perdidos);
    p16(buf + 12, RK_HIST_REG_LEN);
    p16(buf + 14, 0u);
    k = RK_HIST_CAB_LEN;
    for (i = 0; i < h->cuenta; i++) {
        const rk_registro_t *r = rk_historial_ver(h, (uint16_t)i);
        p32(buf + k, r->reloj_s);
        p32(buf + k + 4, r->lux);
        p16(buf + k + 8, (uint16_t)r->temp_dc);
        p16(buf + k + 10, (uint16_t)r->suelo_dc);
        p16(buf + k + 12, r->bat_mv);
        p16(buf + k + 14, r->suelo_raw);
        buf[k + 16] = r->suelo_pct;
        buf[k + 17] = r->hr_pct;
        buf[k + 18] = r->animo;
        buf[k + 19] = r->banderas;
        k += RK_HIST_REG_LEN;
    }
    p32(buf + k, rk_crc32(buf, k));
    return n;
}

bool rk_historial_cargar(rk_historial_t *h, const uint8_t *buf, size_t len)
{
    uint16_t cuenta, i;
    size_t k;

    if (h == NULL) {
        return false;
    }
    rk_historial_iniciar(h);
    if (buf == NULL || len < RK_HIST_CAB_LEN + 4u) {
        return false;
    }
    if (g32(buf) != MAGIA || g16(buf + 4) != VERSION || g16(buf + 12) != RK_HIST_REG_LEN) {
        return false;
    }
    cuenta = g16(buf + 6);
    if (cuenta > RK_HIST_CAP ||
        len != RK_HIST_CAB_LEN + (size_t)cuenta * RK_HIST_REG_LEN + 4u) {
        return false;
    }
    k = RK_HIST_CAB_LEN + (size_t)cuenta * RK_HIST_REG_LEN;
    if (rk_crc32(buf, k) != g32(buf + k)) {
        return false;
    }
    h->perdidos = g32(buf + 8);
    k = RK_HIST_CAB_LEN;
    for (i = 0; i < cuenta; i++) {
        rk_registro_t *r = &h->reg[i];
        r->reloj_s   = g32(buf + k);
        r->lux       = g32(buf + k + 4);
        r->temp_dc   = (int16_t)g16(buf + k + 8);
        r->suelo_dc  = (int16_t)g16(buf + k + 10);
        r->bat_mv    = g16(buf + k + 12);
        r->suelo_raw = g16(buf + k + 14);
        r->suelo_pct = buf[k + 16];
        r->hr_pct    = buf[k + 17];
        r->animo     = buf[k + 18];
        r->banderas  = buf[k + 19];
        k += RK_HIST_REG_LEN;
    }
    h->cuenta = cuenta;
    return true;
}

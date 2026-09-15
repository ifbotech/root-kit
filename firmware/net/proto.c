#include "proto.h"
#include <string.h>

/* ------------------------------------------------------------- CRC16 ----- */
/* CRC16-CCITT (polinomio 0x1021, inicial 0xFFFF). Implementación por bits:
 * son 32 bytes por trama como máximo y unos pocos ciclos por byte, así que no justifica
 * los 512 bytes de flash de una tabla. */
uint16_t rk_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    size_t   i;
    int      b;

    if (data == NULL) {
        return crc;
    }
    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (b = 0; b < 8; b++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((uint16_t)(crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* --------------------------------------------------------- lux codec ---- */
/* 4 bits de exponente y 12 de mantisa: valor = mantisa << exponente.
 * Resolución de 1 lux hasta 4095 —donde está el umbral de noche y toda la
 * luz de interior— y techo de 4095<<15, muy por encima del sol directo. */
uint16_t rk_lux_encode(uint32_t lux)
{
    uint16_t exp = 0;

    while (lux > 0x0FFFu && exp < 15u) {
        lux >>= 1;
        exp++;
    }
    if (lux > 0x0FFFu) {
        lux = 0x0FFFu;          /* saturamos en vez de envolver */
    }
    return (uint16_t)((exp << 12) | (uint16_t)lux);
}

uint32_t rk_lux_decode(uint16_t code)
{
    uint32_t mant = (uint32_t)(code & 0x0FFFu);
    uint32_t exp  = (uint32_t)((code >> 12) & 0x0Fu);
    return mant << exp;
}

/* ------------------------------------------------------ lectura/escritura */
static void put_u16(uint8_t *b, uint16_t v)
{
    b[0] = (uint8_t)(v & 0xFFu);
    b[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static uint16_t get_u16(const uint8_t *b)
{
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static int header(uint8_t *buf, uint8_t type, const uint8_t *id)
{
    buf[0] = RK_PROTO_MAGIC;
    buf[1] = RK_PROTO_VERSION;
    buf[2] = type;
    buf[3] = 0;                     /* flags, lo pisa quien corresponda */
    memcpy(&buf[4], id, 6);
    return 10;
}

/* Valida cabecera y CRC. Devuelve el tipo, o un error negativo. */
static int verify(const uint8_t *buf, size_t len, size_t expected)
{
    uint16_t crc;

    if (buf == NULL || len < 3u) {
        return RK_PROTO_E_TRUNCATED;
    }
    if (buf[0] != RK_PROTO_MAGIC) {
        return RK_PROTO_E_MAGIC;
    }
    if (buf[1] != RK_PROTO_VERSION) {
        return RK_PROTO_E_VERSION;
    }
    if (expected != 0u && len < expected) {
        return RK_PROTO_E_TRUNCATED;
    }
    if (expected != 0u) {
        crc = rk_crc16(buf, expected - 2u);
        if (crc != get_u16(&buf[expected - 2u])) {
            return RK_PROTO_E_CRC;
        }
    }
    return (int)buf[2];
}

/* ------------------------------------------------------------ telemetría - */
int rk_proto_encode_telemetry(uint8_t *buf, size_t cap,
                              const rk_telemetry_pkt_t *p)
{
    int n;

    if (buf == NULL || p == NULL) {
        return RK_PROTO_E_ARG;
    }
    if (cap < (size_t)RK_TELEMETRY_LEN) {
        return RK_PROTO_E_LEN;
    }

    n = header(buf, RK_PKT_TELEMETRY, p->id);
    buf[3] = p->flags;
    put_u16(&buf[n], p->seq);                       n += 2;   /* 10 */
    buf[n++] = p->soil_pct;                                   /* 12 */
    put_u16(&buf[n], (uint16_t)p->temp_dc);         n += 2;   /* 13 */
    buf[n++] = p->rh_pct;                                     /* 15 */
    put_u16(&buf[n], rk_lux_encode(p->lux));        n += 2;   /* 16 */
    put_u16(&buf[n], p->batt_mv);                   n += 2;   /* 18 */
    put_u16(&buf[n], p->soil_raw);                  n += 2;   /* 20 */
    buf[n++] = RK_ESTADO(p->mood, p->severity);               /* 22 */
    buf[n++] = p->etapa;                                      /* 23 */
    put_u16(&buf[n], rk_crc16(buf, (size_t)n));     n += 2;   /* 24 */

    return n;   /* 26 */
}

int rk_proto_decode_telemetry(const uint8_t *buf, size_t len,
                              rk_telemetry_pkt_t *out)
{
    int t;

    if (out == NULL) {
        return RK_PROTO_E_ARG;
    }
    t = verify(buf, len, (size_t)RK_TELEMETRY_LEN);
    if (t < 0) {
        return t;
    }
    if (t != (int)RK_PKT_TELEMETRY) {
        return RK_PROTO_E_TYPE;
    }

    memcpy(out->id, &buf[4], 6);
    out->flags    = buf[3];
    out->seq      = get_u16(&buf[10]);
    out->soil_pct = buf[12];
    out->temp_dc  = (int16_t)get_u16(&buf[13]);
    out->rh_pct   = buf[15];
    out->lux      = rk_lux_decode(get_u16(&buf[16]));
    out->batt_mv  = get_u16(&buf[18]);
    out->soil_raw = get_u16(&buf[20]);
    out->mood     = RK_ESTADO_MOOD(buf[22]);
    out->severity = RK_ESTADO_SEV(buf[22]);
    out->etapa    = buf[23];
    return RK_PROTO_OK;
}

/* ----------------------------------------------------------------- hello - */
int rk_proto_encode_hello(uint8_t *buf, size_t cap, const rk_hello_pkt_t *p)
{
    int n;

    if (buf == NULL || p == NULL) {
        return RK_PROTO_E_ARG;
    }
    if (cap < (size_t)RK_HELLO_LEN) {
        return RK_PROTO_E_LEN;
    }
    n = header(buf, RK_PKT_HELLO, p->id);           /* n == 10 */
    buf[n++] = p->hw_rev;                                     /* 10 */
    buf[n++] = p->fw_major;                                   /* 11 */
    buf[n++] = p->fw_minor;                                   /* 12 */
    put_u16(&buf[n], p->boot_count);                n += 2;   /* 13 */
    buf[n++] = p->role;                                       /* 15 */
    put_u16(&buf[n], rk_crc16(buf, (size_t)n));     n += 2;   /* 16 */
    return n;   /* 18 */
}

int rk_proto_decode_hello(const uint8_t *buf, size_t len, rk_hello_pkt_t *out)
{
    int t;

    if (out == NULL) {
        return RK_PROTO_E_ARG;
    }
    t = verify(buf, len, (size_t)RK_HELLO_LEN);
    if (t < 0) {
        return t;
    }
    if (t != (int)RK_PKT_HELLO) {
        return RK_PROTO_E_TYPE;
    }
    memcpy(out->id, &buf[4], 6);
    out->hw_rev     = buf[10];
    out->fw_major   = buf[11];
    out->fw_minor   = buf[12];
    out->boot_count = get_u16(&buf[13]);
    out->role       = buf[15];
    return RK_PROTO_OK;
}

/* ---------------------------------------------------------------- config - */
/* Layout (32 bytes):
 *   0..9  cabecera con id y flags
 *  10..11 interval_s
 *  12..13 soil_dry_raw
 *  14..15 soil_wet_raw
 *  16     soil_min
 *  17     soil_max
 *  18..19 temp_min_dc
 *  20..21 temp_max_dc
 *  22     rh_min
 *  23     comp_idx        (0xFF: todavía sin simbionte asignado)
 *  24     etapa
 *  25..26 lux_min         (codificado con la misma mantisa+exponente)
 *  27..28 lux_max
 *  29     reservado
 *  30..31 crc
 *
 * Los dos límites de luz se codifican con rk_lux_encode en vez de ir en 32
 * bits crudos: el códec ya existe, ya está testeado, y con 12 bits de
 * mantisa el error relativo arriba de 4.095 lux es menor al 0,03%, muy por
 * debajo de la tolerancia del propio BH1750.
 */
int rk_proto_encode_config(uint8_t *buf, size_t cap, const rk_config_pkt_t *p)
{
    int n;

    if (buf == NULL || p == NULL) {
        return RK_PROTO_E_ARG;
    }
    if (cap < (size_t)RK_CONFIG_LEN) {
        return RK_PROTO_E_LEN;
    }
    n = header(buf, RK_PKT_CONFIG, p->id);          /* n == 10 */
    buf[3] = p->flags;
    put_u16(&buf[n], p->interval_s);                n += 2;   /* 10 */
    put_u16(&buf[n], p->soil_dry_raw);              n += 2;   /* 12 */
    put_u16(&buf[n], p->soil_wet_raw);              n += 2;   /* 14 */
    buf[n++] = p->soil_min;                                   /* 16 */
    buf[n++] = p->soil_max;                                   /* 17 */
    put_u16(&buf[n], (uint16_t)p->temp_min_dc);     n += 2;   /* 18 */
    put_u16(&buf[n], (uint16_t)p->temp_max_dc);     n += 2;   /* 20 */
    buf[n++] = p->rh_min;                                     /* 22 */
    buf[n++] = p->comp_idx;                                   /* 23 */
    buf[n++] = p->etapa;                                      /* 24 */
    put_u16(&buf[n], rk_lux_encode(p->lux_min));    n += 2;   /* 25 */
    put_u16(&buf[n], rk_lux_encode(p->lux_max));    n += 2;   /* 27 */
    buf[n++] = 0;                                             /* 29 reservado */
    put_u16(&buf[n], rk_crc16(buf, (size_t)n));     n += 2;   /* 30 */
    return n;   /* 32 */
}

int rk_proto_decode_config(const uint8_t *buf, size_t len,
                           rk_config_pkt_t *out)
{
    int t;

    if (out == NULL) {
        return RK_PROTO_E_ARG;
    }
    t = verify(buf, len, (size_t)RK_CONFIG_LEN);
    if (t < 0) {
        return t;
    }
    if (t != (int)RK_PKT_CONFIG) {
        return RK_PROTO_E_TYPE;
    }
    memcpy(out->id, &buf[4], 6);
    out->flags        = buf[3];
    out->interval_s   = get_u16(&buf[10]);
    out->soil_dry_raw = get_u16(&buf[12]);
    out->soil_wet_raw = get_u16(&buf[14]);
    out->soil_min     = buf[16];
    out->soil_max     = buf[17];
    out->temp_min_dc  = (int16_t)get_u16(&buf[18]);
    out->temp_max_dc  = (int16_t)get_u16(&buf[20]);
    out->rh_min       = buf[22];
    out->comp_idx     = buf[23];
    out->etapa        = buf[24];
    out->lux_min      = rk_lux_decode(get_u16(&buf[25]));
    out->lux_max      = rk_lux_decode(get_u16(&buf[27]));
    return RK_PROTO_OK;
}

/* -------------------------------------------------------------- varios --- */
int rk_proto_peek_type(const uint8_t *buf, size_t len)
{
    return verify(buf, len, 0u);
}

bool rk_seq_is_new(uint16_t last, uint16_t seq)
{
    /* Diferencia con signo en 16 bits: positiva significa futuro. Esto
     * sobrevive al envolvimiento, que con un Mini que manda cada 15 minutos
     * ocurre cada dos años largos, pero también tras un reinicio del contador. */
    int16_t delta = (int16_t)((uint16_t)(seq - last));
    return delta > 0;
}

const char *rk_proto_strerror(int err)
{
    switch (err) {
    case RK_PROTO_OK:          return "ok";
    case RK_PROTO_E_TRUNCATED: return "trama truncada";
    case RK_PROTO_E_MAGIC:     return "magic invalido";
    case RK_PROTO_E_VERSION:   return "version no soportada";
    case RK_PROTO_E_CRC:       return "CRC no coincide";
    case RK_PROTO_E_TYPE:      return "tipo inesperado";
    case RK_PROTO_E_LEN:       return "buffer insuficiente";
    case RK_PROTO_E_ARG:       return "argumento invalido";
    default:                   return "error desconocido";
    }
}

#include "rk_test.h"
#include "../net/proto.h"

static const uint8_t ID[6] = { 0xA4, 0xCF, 0x12, 0x9B, 0x40, 0x11 };

static rk_telemetry_pkt_t sample(void)
{
    rk_telemetry_pkt_t p;
    memcpy(p.id, ID, 6);
    p.seq      = 4211;
    p.flags    = RK_FLAG_CALIBRATED;
    p.soil_pct = 42;
    p.temp_dc  = 236;
    p.rh_pct   = 57;
    p.lux      = 6250;
    p.batt_mv  = 3912;
    p.soil_raw = 1930;
    return p;
}

static void test_crc(void)
{
    /* Vector canónico de CRC16-CCITT (init 0xFFFF): "123456789" -> 0x29B1.
     * Sin este ancla, un cambio en la implementación rompe la compatibilidad
     * con cualquier decodificador escrito en otro lenguaje sin que nadie se
     * entere. */
    const uint8_t v[] = "123456789";
    CHECK_HEX("CRC16-CCITT del vector canonico", 0x29B1u, rk_crc16(v, 9));
    CHECK_HEX("CRC de buffer vacio es el inicial", 0xFFFFu, rk_crc16(v, 0));
    CHECK_HEX("CRC de NULL no explota", 0xFFFFu, rk_crc16(NULL, 9));
}

static void test_lux_codec(void)
{
    /* Los valores chicos tienen que ser exactos: el umbral de noche está en
     * 15 lux y toda la luz de interior vive debajo de 4096. */
    uint32_t exactos[] = { 0u, 1u, 15u, 100u, 999u, 4095u };
    uint32_t grandes[] = { 4096u, 10000u, 40000u, 100000u, 262140u };
    int i;
    char lbl[64];

    for (i = 0; i < (int)(sizeof exactos / sizeof exactos[0]); i++) {
        snprintf(lbl, sizeof lbl, "lux %lu ida y vuelta exacto",
                 (unsigned long)exactos[i]);
        CHECK_INT(lbl, exactos[i], rk_lux_decode(rk_lux_encode(exactos[i])));
    }
    /* Arriba de 4095 se acepta pérdida, pero acotada: nunca peor que 1/4096. */
    for (i = 0; i < (int)(sizeof grandes / sizeof grandes[0]); i++) {
        uint32_t v = grandes[i];
        uint32_t r = rk_lux_decode(rk_lux_encode(v));
        snprintf(lbl, sizeof lbl, "lux %lu con error < 0,1%%", (unsigned long)v);
        CHECK_NEAR(lbl, v, r, (long)(v / 1000u) + 1);
    }
    CHECK_INT("lux enorme satura sin envolver", 4095u << 15,
              rk_lux_decode(rk_lux_encode(0xFFFFFFFFu)));
}

static void test_telemetry_roundtrip(void)
{
    uint8_t buf[RK_PKT_MAX];
    rk_telemetry_pkt_t in = sample(), out;
    int n;

    n = rk_proto_encode_telemetry(buf, sizeof buf, &in);
    CHECK_INT("la trama de telemetria mide 24 bytes", RK_TELEMETRY_LEN, n);
    CHECK_INT("decodifica sin error", RK_PROTO_OK,
              rk_proto_decode_telemetry(buf, (size_t)n, &out));

    CHECK_INT("id byte 0", ID[0], out.id[0]);
    CHECK_INT("id byte 5", ID[5], out.id[5]);
    CHECK_INT("seq",      in.seq,      out.seq);
    CHECK_INT("flags",    in.flags,    out.flags);
    CHECK_INT("soil",     in.soil_pct, out.soil_pct);
    CHECK_INT("temp_dc",  in.temp_dc,  out.temp_dc);
    CHECK_INT("rh",       in.rh_pct,   out.rh_pct);
    CHECK_INT("lux",      in.lux,      out.lux);
    CHECK_INT("batt_mv",  in.batt_mv,  out.batt_mv);
    CHECK_INT("soil_raw", in.soil_raw, out.soil_raw);
}

static void test_temperaturas_negativas(void)
{
    /* Un balcón en Buenos Aires no baja de cero, pero uno en Bariloche sí, y
     * el complemento a dos mal manejado es un clásico. */
    uint8_t buf[RK_PKT_MAX];
    rk_telemetry_pkt_t in = sample(), out;
    int16_t casos[] = { -1, -55, -150, -400, 0, 450 };
    int i;
    char lbl[64];

    for (i = 0; i < (int)(sizeof casos / sizeof casos[0]); i++) {
        in.temp_dc = casos[i];
        rk_proto_encode_telemetry(buf, sizeof buf, &in);
        rk_proto_decode_telemetry(buf, RK_TELEMETRY_LEN, &out);
        snprintf(lbl, sizeof lbl, "temperatura %d decimas sobrevive", casos[i]);
        CHECK_INT(lbl, casos[i], out.temp_dc);
    }
}

static void test_rechazos(void)
{
    uint8_t buf[RK_PKT_MAX];
    rk_telemetry_pkt_t in = sample(), out;
    uint8_t copia[RK_PKT_MAX];
    int i, corruptas = 0;

    rk_proto_encode_telemetry(buf, sizeof buf, &in);

    CHECK_INT("trama truncada se rechaza", RK_PROTO_E_TRUNCATED,
              rk_proto_decode_telemetry(buf, 12u, &out));

    memcpy(copia, buf, sizeof copia);
    copia[0] = 'X';
    CHECK_INT("magic invalido se rechaza", RK_PROTO_E_MAGIC,
              rk_proto_decode_telemetry(copia, RK_TELEMETRY_LEN, &out));

    memcpy(copia, buf, sizeof copia);
    copia[1] = 99;
    CHECK_INT("version futura se rechaza", RK_PROTO_E_VERSION,
              rk_proto_decode_telemetry(copia, RK_TELEMETRY_LEN, &out));

    memcpy(copia, buf, sizeof copia);
    copia[2] = RK_PKT_HELLO;
    CHECK_INT("tipo equivocado se rechaza", RK_PROTO_E_CRC,
              rk_proto_decode_telemetry(copia, RK_TELEMETRY_LEN, &out));

    CHECK_INT("buffer chico al codificar", RK_PROTO_E_LEN,
              rk_proto_encode_telemetry(buf, 10u, &in));
    CHECK_INT("puntero nulo al codificar", RK_PROTO_E_ARG,
              rk_proto_encode_telemetry(NULL, 24u, &in));

    /* Un solo bit dado vuelta en cualquier posición tiene que caer: es la
     * garantía que justifica gastar dos bytes en el CRC. */
    for (i = 0; i < RK_TELEMETRY_LEN * 8; i++) {
        memcpy(copia, buf, sizeof copia);
        copia[i / 8] ^= (uint8_t)(1u << (i % 8));
        if (rk_proto_decode_telemetry(copia, RK_TELEMETRY_LEN, &out) != RK_PROTO_OK) {
            corruptas++;
        }
    }
    CHECK_INT("los 192 flips de un bit se detectan", RK_TELEMETRY_LEN * 8, corruptas);
}

static void test_hello_config(void)
{
    uint8_t buf[RK_PKT_MAX];
    rk_hello_pkt_t  h = { { 0 }, 2, 1, 4, 37 }, ho;
    rk_config_pkt_t c = { { 0 }, 600, 2650, 1180, RK_FLAG_CALIBRATED }, co;
    int n;

    memcpy(h.id, ID, 6);
    memcpy(c.id, ID, 6);

    n = rk_proto_encode_hello(buf, sizeof buf, &h);
    CHECK_INT("hello mide 18 bytes", RK_HELLO_LEN, n);
    CHECK_INT("hello decodifica", RK_PROTO_OK,
              rk_proto_decode_hello(buf, (size_t)n, &ho));
    CHECK_INT("hello hw_rev",     h.hw_rev,     ho.hw_rev);
    CHECK_INT("hello fw_major",   h.fw_major,   ho.fw_major);
    CHECK_INT("hello fw_minor",   h.fw_minor,   ho.fw_minor);
    CHECK_INT("hello boot_count", h.boot_count, ho.boot_count);

    n = rk_proto_encode_config(buf, sizeof buf, &c);
    CHECK_INT("config mide 18 bytes", RK_CONFIG_LEN, n);
    CHECK_INT("config decodifica", RK_PROTO_OK,
              rk_proto_decode_config(buf, (size_t)n, &co));
    CHECK_INT("config interval",  c.interval_s,   co.interval_s);
    CHECK_INT("config dry_raw",   c.soil_dry_raw, co.soil_dry_raw);
    CHECK_INT("config wet_raw",   c.soil_wet_raw, co.soil_wet_raw);
    CHECK_INT("config flags",     c.flags,        co.flags);

    CHECK_INT("peek reconoce el tipo", RK_PKT_CONFIG,
              rk_proto_peek_type(buf, (size_t)n));
}

static void test_secuencias(void)
{
    CHECK_TRUE("1 es posterior a 0",      rk_seq_is_new(0, 1));
    CHECK_TRUE("100 es posterior a 99",   rk_seq_is_new(99, 100));
    CHECK_TRUE("el mismo no es nuevo",   !rk_seq_is_new(500, 500));
    CHECK_TRUE("uno viejo no es nuevo",  !rk_seq_is_new(500, 499));
    /* Lo que de verdad importa: el envolvimiento a los 65535. */
    CHECK_TRUE("0 es posterior a 65535",  rk_seq_is_new(65535, 0));
    CHECK_TRUE("3 es posterior a 65534",  rk_seq_is_new(65534, 3));
    CHECK_TRUE("65530 no revive tras wrap", !rk_seq_is_new(5, 65530));
}

void suite_proto(void)
{
    RK_SUITE("protocolo");
    test_crc();
    test_lux_codec();
    test_telemetry_roundtrip();
    test_temperaturas_negativas();
    test_rechazos();
    test_hello_config();
    test_secuencias();
    RK_SUITE_END();
}

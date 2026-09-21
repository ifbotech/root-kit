/* Sensores e historial: de los bytes del bus a lo que se guarda y se manda.
 *
 * Las conversiones se prueban con los valores de las hojas de datos, porque
 * son el tipo de error que no se ve: un corrimiento de bits mal puesto da una
 * temperatura perfectamente creíble y perfectamente falsa.
 */
#include <stddef.h>
#include <string.h>
#include "rk_test.h"
#include "../nodo/sensores.h"
#include "../nodo/historial.h"
#include "../core/mood.h"
#include "../core/species.h"

/* Arma la lectura del AHT20 para una temperatura y humedad dadas. */
static void aht(uint8_t b[7], uint32_t h20, uint32_t t20)
{
    b[0] = 0x1Cu;                                   /* calibrado, libre */
    b[1] = (uint8_t)(h20 >> 12);
    b[2] = (uint8_t)(h20 >> 4);
    b[3] = (uint8_t)(((h20 & 0xFu) << 4) | ((t20 >> 16) & 0xFu));
    b[4] = (uint8_t)(t20 >> 8);
    b[5] = (uint8_t)t20;
    b[6] = rk_crc8_aht(b, 6);
}

static void test_aht20(void)
{
    uint8_t b[7];
    int16_t t = 0;
    uint8_t rh = 0;
    static const uint8_t BEEF[2] = { 0xBE, 0xEF };

    /* Vector de referencia del CRC-8 de Sensirion/Aosong. */
    CHECK_HEX("crc8 de 0xBEEF", 0x92, rk_crc8_aht(BEEF, 2));

    aht(b, 1u << 19, 393216u);                      /* 50 %, 25,0 °C */
    CHECK_TRUE("una lectura buena convierte", rk_aht20_convertir(b, &t, &rh));
    CHECK_INT("25,0 grados", 250, t);
    CHECK_INT("50 %", 50, rh);

    aht(b, 0u, 0u);
    rk_aht20_convertir(b, &t, &rh);
    CHECK_INT("el piso es -50 grados", -500, t);
    CHECK_INT("y 0 %", 0, rh);

    aht(b, 0xFFFFFu, 0xFFFFFu);
    rk_aht20_convertir(b, &t, &rh);
    CHECK_INT("el techo es 150 grados", 1500, t);
    CHECK_INT("y 100 %", 100, rh);

    aht(b, 1u << 19, 393216u);
    b[4] ^= 0x01u;
    CHECK_TRUE("un bit dado vuelta no pasa el CRC", !rk_aht20_convertir(b, &t, &rh));
    aht(b, 1u << 19, 393216u);
    b[0] |= 0x80u;
    b[6] = rk_crc8_aht(b, 6);
    CHECK_TRUE("ocupado no es una lectura", !rk_aht20_convertir(b, &t, &rh));
    CHECK_TRUE("NULL no explota", !rk_aht20_convertir(NULL, &t, &rh));
}

/* Arma la lectura de seis bytes del SHT21 para dos palabras crudas. */
static void sht(uint8_t b[6], uint16_t st, uint16_t srh)
{
    b[0] = (uint8_t)(st >> 8);
    b[1] = (uint8_t)st;
    b[2] = rk_crc8_sht2x(b, 2);
    b[3] = (uint8_t)(srh >> 8);
    b[4] = (uint8_t)srh;
    b[5] = rk_crc8_sht2x(&b[3], 2);
}

static void test_sht21(void)
{
    uint8_t b[6];
    int16_t t = 0;
    uint8_t rh = 0;
    static const uint8_t BEEF[2] = { 0xBE, 0xEF };

    /* Mismo polinomio que el AHT20 pero arrancando en 0x00: si alguien
     * reusara rk_crc8_aht, este numero seria 0x92 y nada andaria. */
    CHECK_HEX("crc8 del SHT2x de 0xBEEF", 0x13, rk_crc8_sht2x(BEEF, 2));

    /* Los dos ejemplos de la hoja de datos del SHT21/HTU21D. */
    sht(b, 0x68ACu, 0x7C80u);
    CHECK_TRUE("una lectura buena convierte", rk_sht21_convertir(b, &t, &rh));
    CHECK_INT("25,0 grados", 250, t);
    CHECK_INT("55 %", 55, rh);

    /* Los dos bits de abajo son de estado y no cambian la medicion. */
    sht(b, 0x68ACu | 0x3u, 0x7C80u | 0x2u);
    rk_sht21_convertir(b, &t, &rh);
    CHECK_INT("los bits de estado no ensucian la temperatura", 250, t);
    CHECK_INT("ni la humedad", 55, rh);

    sht(b, 0u, 0u);
    rk_sht21_convertir(b, &t, &rh);
    CHECK_INT("el piso es -46,9 grados", -469, t);
    CHECK_INT("y la humedad no baja de 0 %", 0, rh);

    sht(b, 0xFFFCu, 0xFFFCu);
    rk_sht21_convertir(b, &t, &rh);
    CHECK_INT("el techo es 128,9 grados", 1289, t);
    CHECK_INT("y la humedad no pasa de 100 %", 100, rh);

    sht(b, 0x68ACu, 0x7C80u);
    b[1] ^= 0x10u;
    CHECK_TRUE("un bit dado vuelta no pasa el CRC", !rk_sht21_convertir(b, &t, &rh));
    sht(b, 0x68ACu, 0x7C80u);
    b[4] ^= 0x10u;
    CHECK_TRUE("y tampoco del lado de la humedad",
               !rk_sht21_convertir(b, &t, &rh));
    CHECK_TRUE("NULL no explota", !rk_sht21_convertir(NULL, &t, &rh));

    /* Y la telemetria tiene que salir igual con uno u otro sensor. */
    {
        rk_crudos_t c;
        rk_telemetry_t tel;
        memset(&c, 0, sizeof c);
        sht(c.sht, 0x68ACu, 0x7C80u);
        c.sht_leido = true;
        rk_sensores_telemetria(&c, NULL, &tel);
        CHECK_INT("la telemetria toma la temperatura del SHT21", 250, tel.temp_dc);
        CHECK_INT("y su humedad", 55, tel.rh_pct);
        CHECK_TRUE("y no marca falla de aire",
                   (tel.fallas & RK_FALLA_AIRE) == 0u);
    }
}

static void test_bh1750_y_riel(void)
{
    CHECK_INT("12000 cuentas son 10000 lux", 10000, (long)rk_bh1750_lux(12000u, 69u));
    CHECK_INT("0 es 0", 0, (long)rk_bh1750_lux(0u, 69u));
    CHECK_INT("MTreg 0 se trata como el de fabrica", 10000, (long)rk_bh1750_lux(12000u, 0u));
    CHECK_INT("con MTreg 31 la misma cuenta es mas luz", 22258,
              (long)rk_bh1750_lux(12000u, 31u));
    CHECK_INT("saturado a MTreg 69, redondeado", 54613, (long)rk_bh1750_lux(65535u, 69u));

    CHECK_INT("divisor 1:1", 4200, rk_riel_mv(2100u, 470000u, 470000u));
    CHECK_INT("divisor sin resistencia de abajo", 0, rk_riel_mv(2100u, 1u, 0u));
    CHECK_TRUE("4,2 V es bateria", !rk_riel_usb(4200u));
    CHECK_TRUE("4,6 V es USB", rk_riel_usb(4600u));
}

static void test_ds18b20(void)
{
    static const uint8_t ROM[8] = { 0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xA2 };
    uint8_t sp[9] = { 0 };
    int16_t t = 0;

    /* El ejemplo de la nota de aplicación 27 de Maxim. */
    CHECK_HEX("crc maxim del ejemplo de la nota 27", 0xA2, rk_crc8_maxim(ROM, 7));

    sp[0] = 0x91; sp[1] = 0x01; sp[2] = 0x4B; sp[3] = 0x46; sp[4] = 0x7F;
    sp[5] = 0xFF; sp[6] = 0x0C; sp[7] = 0x10; sp[8] = rk_crc8_maxim(sp, 8);
    CHECK_TRUE("+25,0625 convierte", rk_ds18b20_convertir(sp, &t));
    CHECK_INT("redondea a 25,1", 251, t);

    sp[0] = 0x5E; sp[1] = 0xFF; sp[8] = rk_crc8_maxim(sp, 8);
    rk_ds18b20_convertir(sp, &t);
    CHECK_INT("-10,125 redondea a -10,1", -101, t);

    sp[0] = 0xF8; sp[1] = 0xFF; sp[8] = rk_crc8_maxim(sp, 8);
    rk_ds18b20_convertir(sp, &t);
    CHECK_INT("-0,5", -5, t);

    sp[0] = 0x50; sp[1] = 0x05; sp[8] = rk_crc8_maxim(sp, 8);
    CHECK_TRUE("85,0 es el valor de encendido, no una lectura",
               !rk_ds18b20_convertir(sp, &t));

    sp[0] = 0x91; sp[1] = 0x01; sp[8] = (uint8_t)(rk_crc8_maxim(sp, 8) ^ 0x5Au);
    CHECK_TRUE("CRC malo se descarta", !rk_ds18b20_convertir(sp, &t));
    memset(sp, 0xFF, sizeof sp);
    CHECK_TRUE("linea suelta se descarta", !rk_ds18b20_convertir(sp, &t));
}

static rk_crudos_t crudos_buenos(void)
{
    rk_crudos_t c;
    int i;

    memset(&c, 0, sizeof c);
    for (i = 0; i < 5; i++) {
        c.suelo[i] = (uint16_t)(2200u + (uint16_t)(i * 7));
    }
    c.suelo[2] = 3990u;                              /* un pico de ruido */
    c.n_suelo = 5u;
    aht(c.aht, 1u << 19, 393216u);
    c.aht_leido = true;
    c.bh1750 = 12000u;
    c.bh_mtreg = 69u;
    c.bh_leido = true;
    c.ds_leido = false;
    c.riel_adc_mv = 1950u;
    c.r_arriba = 470000u;
    c.r_abajo = 470000u;
    return c;
}

static void test_telemetria(void)
{
    rk_crudos_t c = crudos_buenos();
    rk_telemetry_t t;

    rk_sensores_telemetria(&c, NULL, &t);
    CHECK_TRUE("la lectura es valida", t.valid);
    CHECK_INT("sin sonda solo falla la sonda", RK_FALLA_SONDA, t.fallas);
    CHECK_INT("temperatura del aire", 250, t.temp_dc);
    CHECK_INT("luz", 10000, (long)t.lux);
    CHECK_INT("bateria por el riel", 3900, t.batt_mv);
    CHECK_TRUE("sin USB", !t.usb);
    CHECK_TRUE("la mediana ignora el pico de ruido", t.suelo_raw < 2300u);
    CHECK_INT("sin sonda la tierra queda sin dato", RK_TEMP_NO_HAY, t.suelo_dc);

    c.aht_leido = false;
    c.riel_adc_mv = 2300u;                           /* 4,6 V: USB */
    c.n_suelo = 4u;                                  /* par: usa 3 */
    rk_sensores_telemetria(&c, NULL, &t);
    CHECK_TRUE("sin AHT20 falla el aire", (t.fallas & RK_FALLA_AIRE) != 0u);
    CHECK_TRUE("enchufado se detecta", t.usb);
    CHECK_INT("y la bateria queda desconocida, no al 100 %", 0, t.batt_mv);

    c = crudos_buenos();
    c.n_suelo = 0u;
    c.bh_leido = false;
    rk_sensores_telemetria(&c, NULL, &t);
    CHECK_TRUE("sin suelo ni luz, las dos fallan",
               (t.fallas & (RK_FALLA_SUELO | RK_FALLA_LUZ)) == (RK_FALLA_SUELO | RK_FALLA_LUZ));

    rk_sensores_telemetria(NULL, NULL, &t);
    CHECK_INT("sin crudos falla todo", 0x0F, t.fallas);
    rk_sensores_telemetria(&c, NULL, NULL);
    CHECK_TRUE("sin salida no explota", true);
}

/* Un sensor caído no puede inventar un problema. */
static void test_animo_con_fallas(void)
{
    const rk_species_t *sp = rk_species_find("monstera");
    rk_mood_state_t st;
    rk_telemetry_t t;
    rk_verdict_t v;

    memset(&t, 0, sizeof t);
    t.valid = true;
    t.soil_pct = 45;
    t.temp_dc = 0;          /* lo que queda cuando el AHT20 no contesta */
    t.rh_pct = 0;
    t.lux = 5000;
    t.fallas = RK_FALLA_AIRE;

    rk_mood_state_init(&st);
    v = rk_mood_eval(&st, sp, &t);
    CHECK_STR("sin AHT20 no tiene frio ni aire seco", "HAPPY", rk_mood_name(v.mood));

    t.fallas = RK_FALLA_SUELO;
    t.soil_pct = 0;
    t.temp_dc = 230;
    t.rh_pct = 60;
    rk_mood_state_init(&st);
    v = rk_mood_eval(&st, sp, &t);
    CHECK_STR("sin sensor de suelo no tiene sed", "HAPPY", rk_mood_name(v.mood));

    t.fallas = RK_FALLA_LUZ;
    t.soil_pct = 45;
    t.lux = 0;
    rk_mood_state_init(&st);
    v = rk_mood_eval(&st, sp, &t);
    CHECK_STR("sin sensor de luz no le falta luz", "HAPPY", rk_mood_name(v.mood));

    t.fallas = RK_FALLA_SONDA;
    t.soil_pct = 10;
    rk_mood_state_init(&st);
    v = rk_mood_eval(&st, sp, &t);
    CHECK_STR("sin sonda la sed se sigue viendo", "THIRSTY", rk_mood_name(v.mood));
}

static void test_historial(void)
{
    static rk_historial_t h, otro;
    static uint8_t buf[RK_HIST_BYTES_MAX];
    rk_telemetry_t t;
    rk_registro_t r;
    size_t n;
    uint32_t i;

    memset(&t, 0, sizeof t);
    t.valid = true;
    t.soil_pct = 40;
    t.temp_dc = -35;
    t.rh_pct = 70;
    t.lux = 120000u;
    t.batt_mv = 3800u;
    t.suelo_dc = 190;
    t.usb = true;
    t.fallas = RK_FALLA_LUZ;

    r = rk_registro_desde(&t, 77u, RK_MOOD_COLD, RK_SEV_URGENT);
    CHECK_INT("la severidad queda en las banderas", RK_SEV_URGENT, rk_registro_severidad(&r));
    CHECK_INT("la falla de luz queda como sin dato", (long)RK_HIST_SIN_LUX, (long)r.lux);
    CHECK_INT("temperatura negativa", -35, r.temp_dc);
    CHECK_TRUE("usb en la bandera", (r.banderas & 1u) != 0u);
    CHECK_INT("fallas en el nibble alto", RK_FALLA_LUZ, r.banderas >> 4);
    CHECK_INT("y se leen enteras", RK_FALLA_LUZ, rk_registro_fallas(&r));
    CHECK_TRUE("sin escurrimiento el bit 3 queda libre", (r.banderas & 0x08u) == 0u);

    t.fallas = RK_FALLA_LUZ | RK_FALLA_ESCURRE;
    r = rk_registro_desde(&t, 77u, RK_MOOD_COLD, RK_SEV_URGENT);
    CHECK_TRUE("el escurrimiento va en el bit 3", (r.banderas & 0x08u) != 0u);
    CHECK_INT("y no pisa las fallas de sensor", RK_FALLA_LUZ, r.banderas >> 4);
    CHECK_INT("la lectura junta las dos", RK_FALLA_LUZ | RK_FALLA_ESCURRE, rk_registro_fallas(&r));
    CHECK_INT("la severidad no se toca", RK_SEV_URGENT, rk_registro_severidad(&r));
    t.fallas = RK_FALLA_LUZ;

    rk_historial_iniciar(&h);
    CHECK_TRUE("vacio no tiene nada que ver", rk_historial_ver(&h, 0u) == NULL);
    for (i = 0; i < RK_HIST_CAP + 5u; i++) {
        r = rk_registro_desde(&t, i, RK_MOOD_HAPPY, RK_SEV_OK);
        rk_historial_agregar(&h, &r);
    }
    CHECK_INT("lleno no crece", RK_HIST_CAP, h.cuenta);
    CHECK_INT("y cuenta lo que piso", 5, (long)h.perdidos);
    CHECK_INT("el mas viejo es el sexto", 5, (long)rk_historial_ver(&h, 0u)->reloj_s);
    CHECK_INT("el mas nuevo es el ultimo", (long)RK_HIST_CAP + 4,
              (long)rk_historial_ver(&h, (uint16_t)(RK_HIST_CAP - 1u))->reloj_s);

    rk_historial_descartar(&h, 10u);
    CHECK_INT("descartar confirmadas", RK_HIST_CAP - 10u, h.cuenta);
    CHECK_INT("el nuevo mas viejo", 15, (long)rk_historial_ver(&h, 0u)->reloj_s);

    n = rk_historial_serializar(&h, buf, sizeof buf);
    CHECK_TRUE("serializa", n > 0u);
    CHECK_TRUE("carga lo mismo", rk_historial_cargar(&otro, buf, n));
    CHECK_INT("misma cuenta", h.cuenta, otro.cuenta);
    CHECK_INT("mismo orden", 15, (long)rk_historial_ver(&otro, 0u)->reloj_s);
    CHECK_INT("mismos datos", -35, rk_historial_ver(&otro, 7u)->temp_dc);
    CHECK_INT("mismas perdidas", 5, (long)otro.perdidos);

    buf[40] ^= 0x10u;
    CHECK_TRUE("un byte corrupto invalida el archivo", !rk_historial_cargar(&otro, buf, n));
    CHECK_INT("y deja el historial vacio", 0, otro.cuenta);
    buf[40] ^= 0x10u;
    CHECK_TRUE("cortado a la mitad no carga", !rk_historial_cargar(&otro, buf, n / 2u));
    CHECK_TRUE("sin lugar no serializa", rk_historial_serializar(&h, buf, 100u) == 0u);

    rk_historial_descartar(&h, 60000u);
    CHECK_INT("descartar de mas vacia", 0, h.cuenta);
    n = rk_historial_serializar(&h, buf, sizeof buf);
    CHECK_TRUE("un historial vacio tambien se guarda y se carga",
               n > 0u && rk_historial_cargar(&otro, buf, n) && otro.cuenta == 0u);
    CHECK_HEX("crc32 de referencia", 0xCBF43926u,
              rk_crc32((const uint8_t *)"123456789", 9u));
}

void suite_sensores(void)
{
    RK_SUITE("sensores e historial");
    test_aht20();
    test_sht21();
    test_bh1750_y_riel();
    test_ds18b20();
    test_telemetria();
    test_animo_con_fallas();
    test_historial();
    RK_SUITE_END();
}

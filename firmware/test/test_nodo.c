#include "rk_test.h"
#include "../nodo/soil.h"
#include "../nodo/power.h"
#include "../nodo/sampler.h"
#include "../gfx/fb.h"   /* rk_sin8, para el dia simulado */

/* ------------------------------------------------------------- suelo ----- */
static void test_soil_calibracion(void)
{
    rk_soil_cal_t ok  = { 2650u, 1180u };
    rk_soil_cal_t inv = { 1180u, 2650u };   /* alguien invirtió los puntos */
    rk_soil_cal_t est = { 1400u, 1300u };   /* calibró con la sonda húmeda */
    rk_soil_cal_t abs = { 4500u, 1180u };   /* fuera del rango del ADC     */

    CHECK_TRUE("calibracion sana es valida",       rk_soil_cal_valid(&ok));
    CHECK_TRUE("calibracion invertida se rechaza", !rk_soil_cal_valid(&inv));
    CHECK_TRUE("rango muy chico se rechaza",       !rk_soil_cal_valid(&est));
    CHECK_TRUE("fuera del ADC se rechaza",         !rk_soil_cal_valid(&abs));
    CHECK_TRUE("NULL se rechaza",                  !rk_soil_cal_valid(NULL));
    CHECK_TRUE("la calibracion de fabrica es valida",
               rk_soil_cal_valid(&RK_SOIL_CAL_DEFAULT));
}

static void test_soil_conversion(void)
{
    rk_soil_cal_t cal = { 2600u, 1200u };   /* rango de 1400 cuentas */

    /* El capacitivo marca ALTO en seco: más cuentas significa menos agua. */
    CHECK_INT("en el punto seco da 0",    0,   rk_soil_pct(&cal, 2600u));
    CHECK_INT("en el punto humedo da 100", 100, rk_soil_pct(&cal, 1200u));
    CHECK_INT("el punto medio da 50",      50,  rk_soil_pct(&cal, 1900u));
    CHECK_NEAR("a un cuarto del rango",    25,  rk_soil_pct(&cal, 2250u), 1);
    CHECK_NEAR("a tres cuartos",           75,  rk_soil_pct(&cal, 1550u), 1);

    /* Fuera de los puntos pero dentro de la tolerancia: satura, no miente. */
    CHECK_INT("mas seco que el aire satura en 0",   0,
              rk_soil_pct(&cal, 2700u));
    CHECK_INT("mas humedo que el agua satura en 100", 100,
              rk_soil_pct(&cal, 1150u));
}

static void test_soil_fallas(void)
{
    rk_soil_cal_t cal = { 2600u, 1200u };
    rk_soil_cal_t mala = { 100u, 50u };

    /* Estos son los casos que, sin filtro, hacen que el simbionte reaccione
     * con total convicción a un cable suelto. */
    CHECK_INT("sensor desconectado se detecta", -1, rk_soil_pct(&cal, 4095u));
    CHECK_INT("entrada en corto se detecta",    -1, rk_soil_pct(&cal, 0u));
    CHECK_INT("lectura absurdamente alta",      -1, rk_soil_pct(&cal, 3200u));
    CHECK_INT("lectura absurdamente baja",      -1, rk_soil_pct(&cal, 800u));
    CHECK_INT("calibracion invalida no convierte", -1,
              rk_soil_pct(&mala, 1500u));
    CHECK_INT("calibracion NULL no convierte", -1, rk_soil_pct(NULL, 1500u));
}

static void test_soil_mediana(void)
{
    /* Un pico aislado de ruido no debe mover la lectura: es exactamente lo
     * que hace el capacitivo cuando la radio transmite cerca. */
    uint16_t con_pico[5] = { 1900u, 1905u, 3900u, 1898u, 1902u };
    uint16_t ordenada[3] = { 10u, 20u, 30u };
    uint16_t una[1]      = { 777u };

    CHECK_INT("la mediana ignora el pico", 1902u, rk_soil_median(con_pico, 5));
    CHECK_INT("mediana de tres",           20u,   rk_soil_median(ordenada, 3));
    CHECK_INT("mediana de una sola",       777u,  rk_soil_median(una, 1));
    CHECK_INT("n invalido devuelve 0",     0u,    rk_soil_median(ordenada, 0));
    CHECK_INT("NULL devuelve 0",           0u,    rk_soil_median(NULL, 3));
}

/* ------------------------------------------------------------ bateria ---- */
static void test_bateria(void)
{
    CHECK_INT("celda llena",        100, rk_batt_pct(4200u));
    CHECK_INT("por encima de llena",100, rk_batt_pct(4300u));
    CHECK_INT("celda vacia",          0, rk_batt_pct(3000u));
    CHECK_INT("por debajo de vacia",  0, rk_batt_pct(2800u));
    CHECK_INT("punto exacto de tabla", 55, rk_batt_pct(3700u));
    CHECK_NEAR("interpola entre puntos", 60, rk_batt_pct(3750u), 2);
    /* Monotonía: sin esto un ruido de ADC puede hacer "subir" la batería. */
    {
        int mv, prev = -1, fallas = 0;
        for (mv = 2800; mv <= 4300; mv += 10) {
            int p = rk_batt_pct((uint16_t)mv);
            if (p < prev) { fallas++; }
            prev = p;
        }
        CHECK_INT("la curva es monotona creciente", 0, fallas);
    }
    CHECK_TRUE("3,4 V es baja",       rk_batt_is_low(3400u));
    CHECK_TRUE("3,9 V no es baja",   !rk_batt_is_low(3900u));
    CHECK_TRUE("3,1 V es critica",    rk_batt_is_critical(3100u));
    CHECK_TRUE("3,4 V no es critica",!rk_batt_is_critical(3400u));
}

/* Este test es también la fuente de las cifras que aparecen en la
 * documentación: si alguien cambia un parámetro del firmware, acá se rompe. */
static void test_presupuesto_energetico(void)
{
    uint32_t d_ing = rk_power_days(&RK_PROFILE_INGENUO,    2200u, 20u);
    uint32_t d_fij = rk_power_days(&RK_PROFILE_FIJO,       2200u, 20u);
    uint32_t d_ada = rk_power_days(&RK_PROFILE_ADAPTATIVO, 2200u, 20u);

    printf("         autonomia con 18650 de 2200 mAh, 20%% de castigo:\n");
    printf("           ingenuo    %4lu dias  (%6lu uAh/dia, %2u%% durmiendo)\n",
           (unsigned long)d_ing,
           (unsigned long)(rk_power_daily_nah(&RK_PROFILE_INGENUO) / 1000u),
           rk_power_sleep_share(&RK_PROFILE_INGENUO));
    printf("           fijo       %4lu dias  (%6lu uAh/dia, %2u%% durmiendo)\n",
           (unsigned long)d_fij,
           (unsigned long)(rk_power_daily_nah(&RK_PROFILE_FIJO) / 1000u),
           rk_power_sleep_share(&RK_PROFILE_FIJO));
    printf("           adaptativo %4lu dias  (%6lu uAh/dia, %2u%% durmiendo)\n",
           (unsigned long)d_ada,
           (unsigned long)(rk_power_daily_nah(&RK_PROFILE_ADAPTATIVO) / 1000u),
           rk_power_sleep_share(&RK_PROFILE_ADAPTATIVO));

    CHECK_TRUE("el perfil ingenuo no llega a 6 meses", d_ing < 180u);
    CHECK_TRUE("el perfil fijo pasa el ano",           d_fij > 365u);
    CHECK_TRUE("el adaptativo casi duplica al fijo",   d_ada * 10u > d_fij * 18u);
    CHECK_TRUE("el adaptativo pasa los dos anos",      d_ada > 730u);

    /* Conclusión de ingeniería, y la razón de que este test imprima:
     * en el perfil adaptativo dormir y transmitir quedan casi empatados
     * (~50/50). O sea que seguir puliendo el firmware ya rinde poco: el
     * próximo microamperio hay que ir a buscarlo al hardware, desoldando
     * el LED de alimentación y eligiendo un LDO de bajo reposo. */
    CHECK_TRUE("con adaptativo dormir pasa a ser el termino mas grande",
               rk_power_sleep_share(&RK_PROFILE_ADAPTATIVO) >= 45u);
    CHECK_TRUE("pero no domina: la radio sigue pesando",
               rk_power_sleep_share(&RK_PROFILE_ADAPTATIVO) <= 70u);
    CHECK_INT("perfil NULL devuelve 0", 0, rk_power_daily_nah(NULL));
    CHECK_INT("dias con perfil NULL es 0", 0, rk_power_days(NULL, 2200u, 20u));
}

/* ---------------------------------------------------------- muestreador -- */
static rk_telemetry_t tp(uint8_t soil, int16_t temp, uint8_t rh, uint32_t lux)
{
    rk_telemetry_t p;
    memset(&p, 0, sizeof p);
    p.soil_pct = soil;
    p.temp_dc  = temp;
    p.rh_pct   = rh;
    p.lux      = lux;
    p.valid    = true;
    p.batt_mv  = 3900;
    return p;
}

static void test_sampler(void)
{
    const rk_species_t *sp = rk_species_find("monstera");
    rk_sampler_t s;
    rk_sampler_decision_t d;
    rk_telemetry_t t;
    uint32_t up = 0;
    int i;

    rk_sampler_init(&s, NULL);

    t = tp(45, 235, 60, 5000);
    d = rk_sampler_step(&s, &t, up, sp);
    CHECK_TRUE("la primera lectura siempre se transmite", d.transmit);
    CHECK_INT("motivo: primera lectura", RK_TX_PRIMERA, d.reason);
    CHECK_INT("la secuencia arranca en 1", 1, d.seq);

    /* Lectura idéntica: no hay nada que contar. */
    up += 300;
    d = rk_sampler_step(&s, &t, up, sp);
    CHECK_TRUE("una lectura identica no se transmite", !d.transmit);

    /* Cambio por debajo de la banda muerta: tampoco. */
    up += 300;
    t = tp(47, 236, 62, 5200);
    d = rk_sampler_step(&s, &t, up, sp);
    CHECK_TRUE("un cambio menor a la banda muerta no se transmite",
               !d.transmit);

    /* Cambio real. */
    up += 300;
    t = tp(52, 236, 62, 5200);
    d = rk_sampler_step(&s, &t, up, sp);
    CHECK_TRUE("un cambio de 7 puntos de suelo se transmite", d.transmit);
    CHECK_INT("motivo: cambio de lectura", RK_TX_CAMBIO, d.reason);

    /* Cruce de umbral: aunque el movimiento sea de un punto, cambia la cara
     * del simbionte y eso tiene que llegar sí o sí. */
    rk_sampler_init(&s, NULL);
    t = tp(26, 235, 60, 5000);           /* monstera: soil_min = 25 */
    rk_sampler_step(&s, &t, 0, sp);
    t = tp(24, 235, 60, 5000);
    d = rk_sampler_step(&s, &t, 300, sp);
    CHECK_TRUE("un cruce de umbral se transmite siempre", d.transmit);
    CHECK_INT("motivo: cruce de umbral", RK_TX_UMBRAL, d.reason);

    /* Latido: sin novedades, pero la Terminal no debe darlo por muerto. */
    rk_sampler_init(&s, NULL);
    t = tp(45, 235, 60, 5000);
    rk_sampler_step(&s, &t, 0, sp);
    d = rk_sampler_step(&s, &t, 7200, sp);
    CHECK_TRUE("a las 2 h sin novedad manda latido", d.transmit);
    CHECK_INT("motivo: latido", RK_TX_LATIDO, d.reason);

    /* Batería: el cambio de estado se avisa aunque todo lo demás siga igual. */
    rk_sampler_init(&s, NULL);
    t = tp(45, 235, 60, 5000);
    rk_sampler_step(&s, &t, 0, sp);
    t.batt_mv = 3300;                    /* debajo de RK_BATT_WARN_MV */
    d = rk_sampler_step(&s, &t, 600, sp);
    CHECK_TRUE("pasar a bateria baja se transmite", d.transmit);
    CHECK_INT("motivo: bateria", RK_TX_BATERIA, d.reason);

    /* Adaptación del período: quieto y lejos de umbrales, se estira. */
    rk_sampler_init(&s, NULL);
    t = tp(45, 235, 60, 5000);           /* bien al medio de todos los rangos */
    up = 0;
    for (i = 0; i < 30; i++) {
        d = rk_sampler_step(&s, &t, up, sp);
        up += d.sleep_s;
    }
    CHECK_INT("con todo quieto llega al techo de 30 min",
              RK_SAMPLER_DEFAULT.max_interval_s, d.sleep_s);

    /* Cerca de un umbral, vuelve al piso. */
    t = tp(27, 235, 60, 5000);           /* a 2 puntos de soil_min = 25 */
    d = rk_sampler_step(&s, &t, up, sp);
    CHECK_INT("cerca de un umbral baja al piso de 2 min",
              RK_SAMPLER_DEFAULT.min_interval_s, d.sleep_s);

    CHECK_TRUE("sampler NULL no explota",
               !rk_sampler_step(NULL, &t, 0, sp).transmit);
    CHECK_TRUE("telemetria NULL no explota",
               !rk_sampler_step(&s, NULL, 0, sp).transmit);
}

/* Simulación de una semana: es la prueba que valida de punta a punta que el
 * ahorro prometido existe con un patrón realista y no sólo en casos armados. */
static void test_sampler_una_semana(void)
{
    const rk_species_t *sp = rk_species_find("monstera");
    rk_sampler_t s;
    rk_sampler_decision_t d;
    uint32_t up = 0, tx_adaptativo = 0, medidas = 0;
    uint32_t tx_fijo;
    int soil_x10 = 600;

    rk_sampler_init(&s, NULL);
    while (up < 7u * 86400u) {
        rk_telemetry_t t;
        int fase = (int)((up % 86400u) * 256u / 86400u);
        int sol  = rk_sin8((uint8_t)fase);

        /* La tierra se seca ~4 puntos por día y se riega al llegar a 28%. */
        soil_x10 -= 1;
        if (soil_x10 < 280) { soil_x10 = 620; }

        t = tp((uint8_t)(soil_x10 / 10),
               (int16_t)(235 + sol * 30 / 127),
               (uint8_t)(58 - sol * 8 / 127),
               sol > 0 ? (uint32_t)(sol * 6000 / 127) : 0u);

        d = rk_sampler_step(&s, &t, up, sp);
        medidas++;
        if (d.transmit) { tx_adaptativo++; }
        up += d.sleep_s;
    }

    tx_fijo = 7u * 96u;   /* cada 15 min durante una semana */
    printf("         una semana simulada: %lu mediciones, %lu transmisiones\n",
           (unsigned long)medidas, (unsigned long)tx_adaptativo);
    printf("           contra %lu transmisiones de un firmware de 15 min fijos\n",
           (unsigned long)tx_fijo);
    printf("           ahorro de radio: %lu%%\n",
           (unsigned long)(100u - tx_adaptativo * 100u / tx_fijo));

    CHECK_TRUE("transmite menos que el firmware de intervalo fijo",
               tx_adaptativo < tx_fijo);
    CHECK_TRUE("ahorra al menos la mitad de las transmisiones",
               tx_adaptativo * 2u < tx_fijo);
    /* Cota por abajo: si transmitiera casi nada estaría perdiendo eventos. */
    CHECK_TRUE("igual transmite el latido y los cambios",
               tx_adaptativo >= 7u * 12u);
}

void suite_nodo(void)
{
    RK_SUITE("nodo");
    test_soil_calibracion();
    test_soil_conversion();
    test_soil_fallas();
    test_soil_mediana();
    test_bateria();
    test_presupuesto_energetico();
    test_sampler();
    test_sampler_una_semana();
    RK_SUITE_END();
}

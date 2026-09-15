/* El kit: el roster del Prime y el enlace con los Minis.
 *
 * Dos cosas se fijan acá, y las dos son reglas de arquitectura más que
 * detalles de implementación:
 *
 *   1. UN KIT TIENE EXACTAMENTE UN PRIME. Sin esa garantía, dos nodos se
 *      pelean por servir el Hub y el usuario ve dos sistemas.
 *   2. EL ÁNIMO SE EVALÚA UNA SOLA VEZ, EN EL NODO DUEÑO DE LA MACETA.
 *      El Prime copia lo que recibe y no vuelve a llamar al evaluador. El
 *      test lo verifica de la única forma que no se puede falsear: mandando
 *      una telemetría cuyos números dirían una cosa y cuyo ánimo dice otra,
 *      y comprobando que el Prime respeta el ánimo.
 */
#include <stddef.h>
#include <string.h>
#include "rk_test.h"
#include "../core/node.h"
#include "../net/link.h"

/* ------------------------------------------------------------- roster -- */
static void test_alta(void)
{
    rk_roster_t r;
    const rk_species_t *mon = rk_species_find("monstera");

    rk_roster_init(&r);
    CHECK_INT("un kit vacio no tiene nodos", 0, r.count);
    CHECK_INT("un kit vacio no tiene Prime", -1, rk_roster_prime(&r));

    CHECK_INT("el primer nodo entra", 0,
              rk_roster_add(&r, "MONSTERA", RK_ROLE_PRIME, mon));
    CHECK_INT("y es el Prime", 0, rk_roster_prime(&r));
    CHECK_INT("el segundo nodo entra", 1,
              rk_roster_add(&r, "POTUS", RK_ROLE_MINI,
                            rk_species_find("pothos")));

    /* Un segundo Prime tiene que rebotar. */
    CHECK_INT("un segundo Prime se rechaza", -1,
              rk_roster_add(&r, "OTRA", RK_ROLE_PRIME, mon));
    CHECK_INT("y el kit sigue con dos nodos", 2, r.count);

    CHECK_INT("un nombre vacio se rechaza", -1,
              rk_roster_add(&r, "", RK_ROLE_MINI, mon));
    CHECK_INT("un nombre nulo se rechaza", -1,
              rk_roster_add(&r, NULL, RK_ROLE_MINI, mon));
    CHECK_INT("un rol invalido se rechaza", -1,
              rk_roster_add(&r, "X", (rk_role_t)9, mon));
    CHECK_INT("un roster nulo se rechaza", -1,
              rk_roster_add(NULL, "X", RK_ROLE_MINI, mon));

    /* El simbionte sale de la especie, sin intervención de nadie. */
    CHECK_TRUE("el Prime recibio su simbionte", r.nodes[0].comp != NULL);
    CHECK_STR("y es el de la monstera", "Tuga.exe", r.nodes[0].comp->nombre);
    CHECK_TRUE("el Mini recibio el suyo", r.nodes[1].comp != NULL);
    CHECK_STR("y es el del potus", "Myco.zip", r.nodes[1].comp->nombre);

    /* Sin especie, sin simbionte: es el estado mientras la foto se procesa. */
    CHECK_INT("un nodo sin especie tambien entra", 2,
              rk_roster_add(&r, "SIN ID", RK_ROLE_MINI, NULL));
    CHECK_TRUE("pero no tiene simbionte todavia", r.nodes[2].comp == NULL);
}

static void test_tope(void)
{
    rk_roster_t r;
    const rk_species_t *sp = rk_species_find("cactus");
    int i;

    rk_roster_init(&r);
    for (i = 0; i < RK_MAX_NODES; i++) {
        CHECK_INT("entra un nodo mas", i,
                  rk_roster_add(&r, "N", i == 0 ? RK_ROLE_PRIME : RK_ROLE_MINI,
                                sp));
    }
    CHECK_INT("el kit lleno se queda lleno", -1,
              rk_roster_add(&r, "SOBRA", RK_ROLE_MINI, sp));
    CHECK_INT("y sigue con el maximo", RK_MAX_NODES, r.count);
}

static void test_nombre_largo(void)
{
    rk_roster_t r;
    rk_roster_init(&r);
    /* 30 caracteres en un campo de 18: tiene que truncar y terminar en NUL,
     * no desbordar. */
    rk_roster_add(&r, "UNA MACETA CON NOMBRE LARGUISIMO", RK_ROLE_PRIME,
                  rk_species_find("aloe"));
    CHECK_INT("el nombre se trunca a 17 caracteres", 17,
              (int)strlen(r.nodes[0].nombre));
    CHECK_TRUE("y queda terminado", r.nodes[0].nombre[17] == '\0');
}

static void test_enlace(void)
{
    rk_roster_t r;
    rk_roster_init(&r);
    rk_roster_add(&r, "A", RK_ROLE_PRIME, rk_species_find("monstera"));

    CHECK_INT("sin telemetria el enlace es NUNCA", RK_LINK_NUNCA,
              rk_node_link(&r.nodes[0]));
    CHECK_INT("un nodo nulo tambien", RK_LINK_NUNCA, rk_node_link(NULL));

    r.nodes[0].tel.valid = true;
    r.nodes[0].tel.age_s = 60u;
    CHECK_INT("recien hablo: VIVO", RK_LINK_VIVO, rk_node_link(&r.nodes[0]));

    r.nodes[0].tel.age_s = RK_LINK_TIBIO_S;
    CHECK_INT("justo en el umbral: TIBIO", RK_LINK_TIBIO,
              rk_node_link(&r.nodes[0]));
    r.nodes[0].tel.age_s = RK_LINK_TIBIO_S - 1u;
    CHECK_INT("un segundo antes: todavia VIVO", RK_LINK_VIVO,
              rk_node_link(&r.nodes[0]));

    r.nodes[0].tel.age_s = RK_LINK_CAIDO_S;
    CHECK_INT("pasado el techo: CAIDO", RK_LINK_CAIDO,
              rk_node_link(&r.nodes[0]));

    CHECK_STR("el enlace tiene nombre", "en linea", rk_link_name(RK_LINK_VIVO));
    CHECK_STR("y el rol tambien", "PRIME", rk_role_name(RK_ROLE_PRIME));
    CHECK_STR("y el Mini", "MINI", rk_role_name(RK_ROLE_MINI));
}

static void test_peor(void)
{
    rk_roster_t r;
    rk_roster_init(&r);
    rk_roster_add(&r, "A", RK_ROLE_PRIME, rk_species_find("monstera"));
    rk_roster_add(&r, "B", RK_ROLE_MINI, rk_species_find("pothos"));
    rk_roster_add(&r, "C", RK_ROLE_MINI, rk_species_find("cactus"));

    CHECK_INT("un kit vacio no tiene peor", -1, rk_roster_peor(NULL));

    r.nodes[0].verdict.severity = RK_SEV_OK;
    r.nodes[1].verdict.severity = RK_SEV_WATCH;
    r.nodes[2].verdict.severity = RK_SEV_URGENT;
    CHECK_INT("gana el urgente", 2, rk_roster_peor(&r));

    r.nodes[2].verdict.severity = RK_SEV_WATCH;
    r.nodes[1].tel.age_s = 100u;
    r.nodes[2].tel.age_s = 900u;
    CHECK_INT("a igual severidad gana el mas callado", 2, rk_roster_peor(&r));
    r.nodes[1].tel.age_s = 9000u;
    CHECK_INT("y cambia si el otro se calla mas", 1, rk_roster_peor(&r));
}

static void test_cerrar_dia(void)
{
    rk_roster_t r;
    rk_roster_init(&r);
    rk_roster_add(&r, "A", RK_ROLE_PRIME, rk_species_find("monstera"));
    r.nodes[0].tel.valid = true;
    r.nodes[0].tel.age_s = 60u;
    r.nodes[0].verdict.severity = RK_SEV_OK;

    rk_roster_cerrar_dia(&r);
    CHECK_INT("un dia bueno suma dia sano", 1, r.nodes[0].bond.dias_sanos);
    CHECK_INT("y dia vivido", 1, r.nodes[0].bond.dias_vividos);

    r.nodes[0].verdict.severity = RK_SEV_URGENT;
    rk_roster_cerrar_dia(&r);
    CHECK_INT("un dia urgente no suma sano", 1, r.nodes[0].bond.dias_sanos);
    CHECK_INT("pero si vivido", 2, r.nodes[0].bond.dias_vividos);
    CHECK_INT("y corta la racha", 0, r.nodes[0].bond.racha);

    /* Un nodo caído no cuenta como día sano: si no sabemos cómo estuvo, no
     * se premia. Es la regla que impide que desenchufar un Mini haga crecer
     * al simbionte gratis. */
    r.nodes[0].verdict.severity = RK_SEV_OK;
    r.nodes[0].tel.age_s = RK_LINK_CAIDO_S + 1u;
    rk_roster_cerrar_dia(&r);
    CHECK_INT("un nodo caido no suma dia sano", 1, r.nodes[0].bond.dias_sanos);
}

/* ------------------------------------------------------------- enlace -- */
static void kit_de_dos(rk_roster_t *r)
{
    rk_roster_init(r);
    rk_roster_add(r, "PRIME", RK_ROLE_PRIME, rk_species_find("monstera"));
    rk_roster_add(r, "MINI",  RK_ROLE_MINI,  rk_species_find("bonsai"));
}

static void test_config_lleva_los_umbrales(void)
{
    rk_roster_t r;
    rk_config_pkt_t cfg, ida;
    rk_soil_cal_t cal = { 2900u, 1200u };
    uint8_t buf[RK_PKT_MAX];
    const rk_species_t *bon = rk_species_find("bonsai");
    rk_species_t sp;
    const rk_companion_t *comp = NULL;
    rk_stage_t etapa = RK_ETAPA_ESPORA;
    int n;

    kit_de_dos(&r);

    CHECK_TRUE("la config sale del nodo",
               rk_link_config_from_node(&r.nodes[1], 300u, &cal, &cfg));
    CHECK_TRUE("un nodo nulo no da config",
               !rk_link_config_from_node(NULL, 300u, &cal, &cfg));

    /* Un nodo sin especie no puede evaluarse solo, asi que no se configura. */
    r.nodes[1].sp = NULL;
    CHECK_TRUE("sin especie no hay config",
               !rk_link_config_from_node(&r.nodes[1], 300u, &cal, &cfg));
    r.nodes[1].sp = bon;
    rk_link_config_from_node(&r.nodes[1], 300u, &cal, &cfg);

    /* Ida y vuelta por el cable. */
    n = rk_proto_encode_config(buf, sizeof buf, &cfg);
    CHECK_INT("la config son 32 bytes", RK_CONFIG_LEN, n);
    CHECK_INT("y vuelve entera", RK_PROTO_OK,
              rk_proto_decode_config(buf, (size_t)n, &ida));

    CHECK_INT("sobrevive soil_min",    bon->soil_min,    ida.soil_min);
    CHECK_INT("sobrevive soil_max",    bon->soil_max,    ida.soil_max);
    CHECK_INT("sobrevive temp_min_dc", bon->temp_min_dc, ida.temp_min_dc);
    CHECK_INT("sobrevive temp_max_dc", bon->temp_max_dc, ida.temp_max_dc);
    CHECK_INT("sobrevive rh_min",      bon->rh_min,      ida.rh_min);
    CHECK_INT("sobrevive la calibracion seca",  2900, ida.soil_dry_raw);
    CHECK_INT("sobrevive la calibracion mojada", 1200, ida.soil_wet_raw);
    CHECK_TRUE("y queda marcada como calibrada",
               (ida.flags & RK_FLAG_CALIBRATED) != 0u);

    /* La luz viaja comprimida: se acepta el error del codec, no un valor
     * cualquiera. Con 12 bits de mantisa el error relativo es < 0,05%. */
    CHECK_NEAR("sobrevive lux_min", (long)bon->lux_min, (long)ida.lux_min,
               (long)bon->lux_min / 1000 + 1);
    CHECK_NEAR("sobrevive lux_max", (long)bon->lux_max, (long)ida.lux_max,
               (long)bon->lux_max / 1000 + 1);

    CHECK_INT("y el simbionte que le toco",
              rk_companion_index(r.nodes[1].comp), ida.comp_idx);

    /* El Mini reconstruye su especie y evalua con ella. */
    rk_link_apply_config(&ida, &sp, &comp, &etapa);
    CHECK_INT("el Mini recupera soil_min", bon->soil_min, sp.soil_min);
    CHECK_INT("el Mini recupera temp_max", bon->temp_max_dc, sp.temp_max_dc);
    CHECK_TRUE("y sabe quien vive ahi", comp != NULL);
    CHECK_STR("y es el mismo bicho", "Bonz.root", comp->nombre);
    CHECK_INT("y en que etapa esta", RK_ETAPA_ESPORA, (int)etapa);

    /* Un indice de simbionte fuera de tabla no puede indexar memoria ajena. */
    ida.comp_idx = RK_COMP_NINGUNO;
    rk_link_apply_config(&ida, &sp, &comp, &etapa);
    CHECK_TRUE("un indice invalido deja el nodo sin simbionte", comp == NULL);
    ida.etapa = 200u;
    rk_link_apply_config(&ida, &sp, &comp, &etapa);
    CHECK_INT("una etapa invalida cae en espora", RK_ETAPA_ESPORA, (int)etapa);

    /* Sin calibración válida se manda la de fábrica, sin marcarla. */
    cal.dry_raw = 100u;   /* rango absurdo */
    cal.wet_raw = 90u;
    rk_link_config_from_node(&r.nodes[1], 300u, &cal, &cfg);
    CHECK_TRUE("una calibracion invalida no se manda",
               (cfg.flags & RK_FLAG_CALIBRATED) == 0u);
    CHECK_INT("y se cae a la de fabrica", RK_SOIL_CAL_DEFAULT.dry_raw,
              cfg.soil_dry_raw);
}

/* LA REGLA: el Prime no recalcula. Se le manda una telemetria cuyos numeros
 * gritarian THIRSTY y cuyo campo de animo dice HAPPY. Si el Prime respeta el
 * animo, la regla se cumple; si lo recalcula, el test falla. */
static void test_el_prime_no_recalcula(void)
{
    rk_roster_t r;
    rk_telemetry_pkt_t p;
    rk_node_t *n;

    kit_de_dos(&r);

    memset(&p, 0, sizeof p);
    memcpy(p.id, r.nodes[1].id, 6);
    p.soil_pct = 2;            /* tierra seca: el evaluador diria THIRSTY */
    p.temp_dc  = 220;
    p.rh_pct   = 60;
    p.lux      = 5000;
    p.batt_mv  = 3900;
    p.mood     = (uint8_t)RK_MOOD_HAPPY;
    p.severity = (uint8_t)RK_SEV_OK;
    p.etapa    = (uint8_t)RK_ETAPA_JOVEN;

    n = rk_link_ingest(&r, &p, 0u);
    CHECK_TRUE("la telemetria encontro su nodo", n != NULL);
    CHECK_INT("el Prime respeta el animo del nodo", RK_MOOD_HAPPY,
              (int)n->verdict.mood);
    CHECK_INT("y su severidad", RK_SEV_OK, (int)n->verdict.severity);
    CHECK_STR("y la frase sale del animo recibido", "estoy perfecta",
              n->verdict.reason);
    CHECK_INT("los datos crudos tambien llegan", 2, (int)n->tel.soil_pct);
    CHECK_INT("y la lectura queda fresca", 0, (int)n->tel.age_s);

    /* Un id que no esta en el kit se descarta: un vecino con un ROOTKIT no
     * puede escribir en el nuestro. */
    p.id[5] = 0x7Fu;
    CHECK_TRUE("una telemetria ajena se descarta",
               rk_link_ingest(&r, &p, 0u) == NULL);
    CHECK_TRUE("y un roster nulo tampoco explota",
               rk_link_ingest(NULL, &p, 0u) == NULL);
    CHECK_TRUE("ni un paquete nulo",
               rk_link_ingest(&r, NULL, 0u) == NULL);
}

static void test_envejecer(void)
{
    rk_roster_t r;
    kit_de_dos(&r);

    r.nodes[1].tel.valid = true;
    r.nodes[1].tel.age_s = 0u;

    rk_link_envejecer(&r, 3600u);
    CHECK_INT("la lectura envejece", 3600, (int)r.nodes[1].tel.age_s);
    CHECK_INT("y sigue viva", RK_LINK_VIVO, rk_node_link(&r.nodes[1]));

    rk_link_envejecer(&r, RK_LINK_CAIDO_S);
    CHECK_INT("hasta que cae", RK_LINK_CAIDO, rk_node_link(&r.nodes[1]));

    /* Un nodo que nunca hablo no envejece: no hay nada que envejecer. */
    CHECK_INT("un nodo sin telemetria no envejece", 0,
              (int)r.nodes[0].tel.age_s);

    /* Y la suma satura en vez de envolver: 136 anios de silencio siguen
     * siendo silencio, no una lectura recien llegada. */
    r.nodes[1].tel.age_s = 0xFFFFFF00u;
    rk_link_envejecer(&r, 0x1000u);
    CHECK_INT("la antiguedad satura", RK_LINK_CAIDO, rk_node_link(&r.nodes[1]));
}

/* El camino completo: el Mini mide, evalua, emite; el Prime recibe. */
static void test_vuelta_completa(void)
{
    rk_roster_t prime_kit, mini_kit;
    rk_config_pkt_t cfg;
    rk_telemetry_pkt_t tx, rx;
    rk_species_t sp;
    const rk_companion_t *comp = NULL;
    rk_stage_t etapa;
    uint8_t buf[RK_PKT_MAX];
    rk_node_t *n;
    int len;

    /* --- lado Prime: arma la config --- */
    kit_de_dos(&prime_kit);
    CHECK_TRUE("el Prime arma la config",
               rk_link_config_from_node(&prime_kit.nodes[1], 300u, NULL, &cfg));
    len = rk_proto_encode_config(buf, sizeof buf, &cfg);
    CHECK_TRUE("y la codifica", len == RK_CONFIG_LEN);

    /* --- lado Mini: la aplica, mide y evalua --- */
    {
        rk_config_pkt_t recibida;
        CHECK_INT("el Mini la decodifica", RK_PROTO_OK,
                  rk_proto_decode_config(buf, (size_t)len, &recibida));
        rk_link_apply_config(&recibida, &sp, &comp, &etapa);
    }

    rk_roster_init(&mini_kit);
    rk_roster_add(&mini_kit, "MINI", RK_ROLE_PRIME, NULL);
    mini_kit.nodes[0].sp   = &sp;
    mini_kit.nodes[0].comp = comp;
    memcpy(mini_kit.nodes[0].id, prime_kit.nodes[1].id, 6);

    mini_kit.nodes[0].tel.valid    = true;
    mini_kit.nodes[0].tel.age_s    = 30u;
    mini_kit.nodes[0].tel.soil_pct = 4;       /* muy por debajo del bonsai */
    mini_kit.nodes[0].tel.temp_dc  = 210;
    mini_kit.nodes[0].tel.rh_pct   = 60;
    mini_kit.nodes[0].tel.lux      = 6000;
    mini_kit.nodes[0].tel.batt_mv  = 3820;
    rk_roster_eval(&mini_kit);

    CHECK_INT("el Mini evalua con los umbrales que le llegaron",
              RK_MOOD_THIRSTY, (int)mini_kit.nodes[0].verdict.mood);

    /* --- lado Mini: emite --- */
    rk_link_telemetry_from_node(&mini_kit.nodes[0], 7u, 3100u, 0u, &tx);
    len = rk_proto_encode_telemetry(buf, sizeof buf, &tx);
    CHECK_INT("la telemetria son 26 bytes", RK_TELEMETRY_LEN, len);

    /* --- lado Prime: recibe --- */
    CHECK_INT("el Prime la decodifica", RK_PROTO_OK,
              rk_proto_decode_telemetry(buf, (size_t)len, &rx));
    n = rk_link_ingest(&prime_kit, &rx, 0u);
    CHECK_TRUE("y la aplica al nodo correcto", n != NULL);
    CHECK_STR("que es el Mini", "MINI", n->nombre);
    CHECK_INT("con el animo que evaluo el Mini", RK_MOOD_THIRSTY,
              (int)n->verdict.mood);
    CHECK_INT("y sus datos crudos", 4, (int)n->tel.soil_pct);
    CHECK_INT("y su celda", 3820, (int)n->tel.batt_mv);
}

void suite_link(void)
{
    RK_SUITE("kit y enlace");
    test_alta();
    test_tope();
    test_nombre_largo();
    test_enlace();
    test_peor();
    test_cerrar_dia();
    test_config_lleva_los_umbrales();
    test_el_prime_no_recalcula();
    test_envejecer();
    test_vuelta_completa();
    RK_SUITE_END();
}

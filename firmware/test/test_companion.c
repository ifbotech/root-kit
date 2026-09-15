/* Tests de la colección: rarezas, asignación determinista, crecimiento del
 * vínculo y la ceremonia de apertura. */
#include <stddef.h>
#include "rk_test.h"
#include "../core/companion.h"
#include "../ui/gacha.h"
#include "../gfx/panel.h"

/* ------------------------------------------------------------- catalogo -- */
static void test_catalogo(void)
{
    int i, j;
    int sin_especie = 0, sin_lema = 0, duplicados = 0;

    CHECK_TRUE("hay al menos diez simbiontes", rk_companion_count >= 10);

    for (i = 0; i < rk_companion_count; i++) {
        const rk_companion_t *c = &rk_companion_table[i];
        if (rk_species_find(c->especie) == NULL) { sin_especie++; }
        if (c->lema == NULL || c->lema[0] == '\0') { sin_lema++; }
        for (j = i + 1; j < rk_companion_count; j++) {
            if (strcmp(c->especie, rk_companion_table[j].especie) == 0) {
                duplicados++;
            }
        }
    }
    CHECK_INT("ningun simbionte apunta a una especie inexistente", 0, sin_especie);
    CHECK_INT("todos tienen lema", 0, sin_lema);
    CHECK_INT("ninguna especie tiene dos simbiontes", 0, duplicados);

    /* Biyectivo: tantos simbiontes como especies, y todos alcanzables. */
    CHECK_INT("un simbionte por especie", rk_species_count, rk_companion_count);
    {
        int huerfanas = 0;
        for (i = 0; i < rk_species_count; i++) {
            if (rk_companion_for_species(rk_species_table[i].id) == NULL) {
                huerfanas++;
            }
        }
        CHECK_INT("ninguna especie se queda sin habitante", 0, huerfanas);
    }

    CHECK_TRUE("buscar por id funciona",
               rk_companion_find("tuga") != NULL);
    CHECK_TRUE("un id inexistente devuelve NULL",
               rk_companion_find("no-existe") == NULL);
    CHECK_TRUE("NULL no explota", rk_companion_find(NULL) == NULL);
    CHECK_TRUE("especie NULL no explota",
               rk_companion_for_species(NULL) == NULL);
}

/* --------------------------------------------------------------- rareza -- */
static void test_rareza(void)
{
    CHECK_INT("dificultad 0 es comun",      RK_RAR_COMUN,
              rk_rarity_from_difficulty(0));
    CHECK_INT("dificultad 29 sigue comun",  RK_RAR_COMUN,
              rk_rarity_from_difficulty(29));
    CHECK_INT("dificultad 30 pasa a raro",  RK_RAR_RARO,
              rk_rarity_from_difficulty(30));
    CHECK_INT("dificultad 59 sigue raro",   RK_RAR_RARO,
              rk_rarity_from_difficulty(59));
    CHECK_INT("dificultad 60 es epico",     RK_RAR_EPICO,
              rk_rarity_from_difficulty(60));
    CHECK_INT("dificultad 79 sigue epico",  RK_RAR_EPICO,
              rk_rarity_from_difficulty(79));
    CHECK_INT("dificultad 80 es legendario", RK_RAR_LEGENDARIO,
              rk_rarity_from_difficulty(80));
    CHECK_INT("dificultad 100 es legendario", RK_RAR_LEGENDARIO,
              rk_rarity_from_difficulty(100));

    /* La rareza sale del trabajo real que da la planta. Un potus perdona
     * todo; un bonsai no perdona nada. */
    CHECK_INT("el potus da un comun", RK_RAR_COMUN,
              rk_rarity_of(rk_companion_find("myco")));
    CHECK_INT("la monstera da un raro", RK_RAR_RARO,
              rk_rarity_of(rk_companion_find("tuga")));
    CHECK_INT("la orquidea da un epico", RK_RAR_EPICO,
              rk_rarity_of(rk_companion_find("orqui")));
    CHECK_INT("el bonsai da un legendario", RK_RAR_LEGENDARIO,
              rk_rarity_of(rk_companion_find("bonz")));

    CHECK_INT("NULL cae en comun sin romper", RK_RAR_COMUN, rk_rarity_of(NULL));

    /* La escalada de destellos tiene que ser estrictamente creciente: es la
     * única señal visual de que un legendario vale más que un común. */
    CHECK_TRUE("los destellos escalan con la rareza",
               rk_rarity_sparkles(RK_RAR_COMUN) <
               rk_rarity_sparkles(RK_RAR_RARO) &&
               rk_rarity_sparkles(RK_RAR_RARO) <
               rk_rarity_sparkles(RK_RAR_EPICO) &&
               rk_rarity_sparkles(RK_RAR_EPICO) <
               rk_rarity_sparkles(RK_RAR_LEGENDARIO));

    /* Que la pirámide tenga forma de pirámide: si hubiera tantos legendarios
     * como comunes, el escalón más alto dejaría de sentirse alto. */
    {
        int cuenta[RK_RAR_COUNT] = { 0, 0, 0, 0 };
        int i;
        for (i = 0; i < rk_companion_count; i++) {
            cuenta[rk_rarity_of(&rk_companion_table[i])]++;
        }
        printf("         pirámide de rarezas: %d comunes, %d raros, "
               "%d epicos, %d legendarios\n",
               cuenta[RK_RAR_COMUN], cuenta[RK_RAR_RARO],
               cuenta[RK_RAR_EPICO], cuenta[RK_RAR_LEGENDARIO]);
        CHECK_TRUE("hay al menos un simbionte de cada rareza",
                   cuenta[0] > 0 && cuenta[1] > 0 && cuenta[2] > 0 && cuenta[3] > 0);
        CHECK_TRUE("los legendarios son los menos frecuentes",
                   cuenta[RK_RAR_LEGENDARIO] < cuenta[RK_RAR_COMUN]);
    }
}

/* El test que sostiene la posición legal del producto: el resultado NO
 * depende del azar, sólo de la especie. */
static void test_determinismo(void)
{
    int i, k, distintos = 0;

    for (i = 0; i < rk_species_count; i++) {
        const rk_companion_t *primero =
            rk_companion_for_species(rk_species_table[i].id);
        for (k = 0; k < 50; k++) {
            if (rk_companion_for_species(rk_species_table[i].id) != primero) {
                distintos++;
            }
        }
    }
    CHECK_INT("la misma especie siempre revela el mismo simbionte", 0, distintos);
}

/* ------------------------------------------------------------- vinculo --- */
static void test_vinculo(void)
{
    rk_bond_t b;
    int i;

    rk_bond_init(&b);
    CHECK_INT("arranca como espora", RK_ETAPA_ESPORA, rk_stage_from_bond(&b));
    CHECK_INT("faltan 7 dias sanos para brotar", 7, rk_stage_faltan(&b));
    CHECK_INT("progreso inicial en cero", 0, rk_stage_progreso(&b));

    for (i = 0; i < 7; i++) { rk_bond_dia(&b, true); }
    CHECK_INT("a los 7 dias sanos brota", RK_ETAPA_BROTE, rk_stage_from_bond(&b));
    CHECK_INT("la racha va en 7", 7, b.racha);

    for (i = 0; i < 23; i++) { rk_bond_dia(&b, true); }
    CHECK_INT("a los 30 es joven", RK_ETAPA_JOVEN, rk_stage_from_bond(&b));

    for (i = 0; i < 60; i++) { rk_bond_dia(&b, true); }
    CHECK_INT("a los 90 madura", RK_ETAPA_MADURO, rk_stage_from_bond(&b));

    for (i = 0; i < 90; i++) { rk_bond_dia(&b, true); }
    CHECK_INT("a los 180 es ancestral", RK_ETAPA_ANCESTRAL,
              rk_stage_from_bond(&b));
    CHECK_INT("en la ultima etapa no falta nada", 0, rk_stage_faltan(&b));
    CHECK_INT("y el progreso esta completo", 100, rk_stage_progreso(&b));
}

static void test_vinculo_abandono(void)
{
    rk_bond_t b;
    int i;

    /* Lo central de la mecánica: el tiempo solo no hace crecer al simbionte. */
    rk_bond_init(&b);
    for (i = 0; i < 100; i++) { rk_bond_dia(&b, false); }
    CHECK_INT("cien dias de abandono no lo hacen crecer", RK_ETAPA_ESPORA,
              rk_stage_from_bond(&b));
    CHECK_INT("pero los dias vividos se cuentan igual", 100, b.dias_vividos);
    CHECK_INT("y los sanos siguen en cero", 0, b.dias_sanos);

    /* Un mal día corta la racha pero NO borra lo acumulado. Castigar el
     * olvido con meses de progreso convierte un descuido en motivo para
     * abandonar el producto. */
    rk_bond_init(&b);
    for (i = 0; i < 40; i++) { rk_bond_dia(&b, true); }
    CHECK_INT("cuarenta dias sanos, racha de 40", 40, b.racha);
    rk_bond_dia(&b, false);
    CHECK_INT("un mal dia corta la racha", 0, b.racha);
    CHECK_INT("pero conserva la mejor racha", 40, b.mejor_racha);
    CHECK_INT("y no pierde los dias sanos", 40, b.dias_sanos);
    CHECK_INT("asi que sigue siendo joven", RK_ETAPA_JOVEN,
              rk_stage_from_bond(&b));

    CHECK_TRUE("NULL no explota en ninguna funcion del vinculo",
               rk_stage_from_bond(NULL) == RK_ETAPA_ESPORA &&
               rk_stage_progreso(NULL) == 0);
    rk_bond_init(NULL);
    rk_bond_dia(NULL, true);
    CHECK_TRUE("init y dia con NULL tampoco", true);
}

/* ----------------------------------------------------------- ceremonia --- */
static void test_ceremonia(void)
{
    CHECK_INT("arranca cayendo", RK_GACHA_FASE_CAIDA, rk_gacha_fase(0));
    CHECK_INT("despues tiembla", RK_GACHA_FASE_TEMBLOR,
              rk_gacha_fase(RK_GACHA_CAIDA + 10u));
    CHECK_INT("luego estalla", RK_GACHA_FASE_ESTALLIDO,
              rk_gacha_fase(RK_GACHA_TEMBLOR + 10u));
    CHECK_INT("despues revela", RK_GACHA_FASE_REVELADO,
              rk_gacha_fase(RK_GACHA_ESTALLIDO + 10u));
    CHECK_INT("y queda en reposo", RK_GACHA_FASE_REPOSO,
              rk_gacha_fase(RK_GACHA_REVELADO + 10u));

    CHECK_TRUE("no termina antes de tiempo", !rk_gacha_termino(RK_GACHA_FIN - 1u));
    CHECK_TRUE("termina cuando corresponde", rk_gacha_termino(RK_GACHA_FIN));

    /* La ceremonia entera dura menos de seis segundos. Más que eso y el
     * usuario que registra tres macetas seguidas la empieza a saltear. */
    CHECK_TRUE("la ceremonia dura menos de 6 segundos", RK_GACHA_FIN < 6000u);

    /* Cada rareza tiene su color y ninguno se repite: si dos rarezas se ven
     * igual, el sistema de rarezas no existe. */
    {
        int i, j, repetidos = 0;
        for (i = 0; i < RK_RAR_COUNT; i++) {
            for (j = i + 1; j < RK_RAR_COUNT; j++) {
                if (rk_rarity_color((rk_rarity_t)i) ==
                    rk_rarity_color((rk_rarity_t)j)) {
                    repetidos++;
                }
            }
        }
        CHECK_INT("cada rareza tiene su color", 0, repetidos);
    }

    /* Dibujar con puntero nulo, con simbionte nulo y en cualquier instante
     * no debe romper: la ceremonia corre justo después de una identificación
     * por IA, que es donde más cosas pueden fallar. */
    {
        static rk_color_t pp[RK_PRIME_PX];
        static rk_color_t pm[RK_MINI_PX];
        rk_fb_t fp, fm;
        uint32_t t;
        rk_fb_init(&fp, pp, RK_PRIME_W, RK_PRIME_H);
        rk_fb_init(&fm, pm, RK_MINI_W, RK_MINI_H);
        rk_gacha_draw(NULL, NULL, 0u);
        for (t = 0u; t < 7000u; t += 97u) {
            rk_gacha_draw(&fp, NULL, t);
            rk_gacha_draw(&fp, rk_companion_find("bonz"), t);
            /* La ceremonia tambien tiene que entrar en el Mini: el brote de
             * una maceta nueva se revela ahi si el Prime esta ocupado. */
            rk_gacha_draw(&fm, rk_companion_find("bonz"), t);
        }
        CHECK_TRUE("la ceremonia completa se dibuja sin romper", true);
    }
}

void suite_companion(void)
{
    RK_SUITE("coleccion");
    test_catalogo();
    test_rareza();
    test_determinismo();
    test_vinculo();
    test_vinculo_abandono();
    test_ceremonia();
    RK_SUITE_END();
}

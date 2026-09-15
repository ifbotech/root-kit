/* Genera test/golden.h con los hashes de referencia de las dos pantallas.
 *
 * Se corre a mano cuando un cambio visual es intencional:
 *
 *     make golden
 *
 * El diff del archivo generado muestra exactamente qué pantallas cambiaron y
 * en qué panel, que es justo la revisión que uno quiere hacer antes de
 * commitear arte. Si sólo se mueve la columna del Mini, el rig del Prime
 * quedó intacto y no hace falta volver a mirarlo.
 */
#include <stdio.h>
#include "golden_util.h"

static const char *MOOD_ID[RK_MOOD_COUNT] = {
    "RK_MOOD_UNKNOWN", "RK_MOOD_OFFLINE", "RK_MOOD_SLEEPING", "RK_MOOD_HAPPY",
    "RK_MOOD_THIRSTY", "RK_MOOD_DROWNING", "RK_MOOD_COLD", "RK_MOOD_HOT",
    "RK_MOOD_SCORCHED", "RK_MOOD_DARK", "RK_MOOD_PARCHED_AIR",
};

int main(int argc, char **argv)
{
    const char *path = (argc >= 2) ? argv[1] : "test/golden.h";
    FILE *f = fopen(path, "w");
    int m;

    if (f == NULL) {
        fprintf(stderr, "no pude escribir %s\n", path);
        return 1;
    }

    fprintf(f, "/* Generado por `make golden` - no editar a mano.\n");
    fprintf(f, " *\n");
    fprintf(f, " * Hash FNV-1a del framebuffer de cada panel para cada estado de\n");
    fprintf(f, " * animo, con el kit fijo de golden_util.c. Si un cambio de codigo\n");
    fprintf(f, " * altera cualquier pixel, el test de la suite correspondiente lo\n");
    fprintf(f, " * marca: \"render\" para el Prime de %dx%d y \"mini\" para el de\n",
            RK_PRIME_W, RK_PRIME_H);
    fprintf(f, " * %dx%d.\n", RK_MINI_W, RK_MINI_H);
    fprintf(f, " */\n");
    fprintf(f, "#ifndef ROOTKIT_GOLDEN_H\n#define ROOTKIT_GOLDEN_H\n\n");
    fprintf(f, "#include \"../core/mood.h\"\n\n");
    fprintf(f, "typedef struct {\n");
    fprintf(f, "    rk_mood_t mood;\n");
    fprintf(f, "    uint32_t  t_ms;\n");
    fprintf(f, "    uint32_t  prime;\n");
    fprintf(f, "    uint32_t  mini;\n");
    fprintf(f, "} rk_golden_t;\n\n");
    fprintf(f, "static const rk_golden_t RK_GOLDEN[] = {\n");

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        uint32_t p = rk_golden_prime((rk_mood_t)m, RK_GOLDEN_T_MS);
        uint32_t i = rk_golden_mini((rk_mood_t)m, RK_GOLDEN_T_MS);
        fprintf(f, "    { %-20s, %5uu, 0x%08XU, 0x%08XU },\n",
                MOOD_ID[m], RK_GOLDEN_T_MS, p, i);
    }

    fprintf(f, "};\n\n");
    fprintf(f, "#define RK_GOLDEN_COUNT "
               "((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))\n\n");
    fprintf(f, "#endif /* ROOTKIT_GOLDEN_H */\n");
    fclose(f);

    printf("golden.h regenerado: %d animos x 2 paneles\n", RK_MOOD_COUNT);
    return 0;
}

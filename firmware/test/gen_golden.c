/* Genera test/golden.h con los hashes de referencia de cada pantalla.
 *
 * Se corre a mano cuando un cambio visual es intencional:
 *
 *     make golden
 *
 * El diff del archivo generado muestra exactamente qué pantallas cambiaron,
 * que es justo la revisión que uno quiere hacer antes de commitear arte.
 */
#include <stdio.h>
#include "golden_util.h"

int main(int argc, char **argv)
{
    const char *path = (argc >= 2) ? argv[1] : "test/golden.h";
    FILE *f = fopen(path, "w");
    int m;

    if (f == NULL) {
        fprintf(stderr, "no pude escribir %s\n", path);
        return 1;
    }

    fprintf(f, "/* Generado por `make golden` — no editar a mano.\n");
    fprintf(f, " *\n");
    fprintf(f, " * Hash FNV-1a del framebuffer de 160x240 para cada estado de animo,\n");
    fprintf(f, " * con el escenario fijo de golden_util.c. Si un cambio de codigo\n");
    fprintf(f, " * altera cualquier pixel, el test de la suite \"render\" lo marca.\n");
    fprintf(f, " */\n");
    fprintf(f, "#ifndef ROOTKIT_GOLDEN_H\n#define ROOTKIT_GOLDEN_H\n\n");
    fprintf(f, "#include \"../core/mood.h\"\n\n");
    fprintf(f, "typedef struct {\n");
    fprintf(f, "    rk_mood_t mood;\n");
    fprintf(f, "    uint32_t  t_ms;\n");
    fprintf(f, "    uint32_t  hash;\n");
    fprintf(f, "} rk_golden_t;\n\n");
    fprintf(f, "static const rk_golden_t RK_GOLDEN[] = {\n");

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        uint32_t h = rk_golden_render((rk_mood_t)m, RK_GOLDEN_T_MS);
        fprintf(f, "    { %-20s, %5uu, 0x%08XU },\n",
                (m == RK_MOOD_UNKNOWN)     ? "RK_MOOD_UNKNOWN" :
                (m == RK_MOOD_OFFLINE)     ? "RK_MOOD_OFFLINE" :
                (m == RK_MOOD_SLEEPING)    ? "RK_MOOD_SLEEPING" :
                (m == RK_MOOD_HAPPY)       ? "RK_MOOD_HAPPY" :
                (m == RK_MOOD_THIRSTY)     ? "RK_MOOD_THIRSTY" :
                (m == RK_MOOD_DROWNING)    ? "RK_MOOD_DROWNING" :
                (m == RK_MOOD_COLD)        ? "RK_MOOD_COLD" :
                (m == RK_MOOD_HOT)         ? "RK_MOOD_HOT" :
                (m == RK_MOOD_SCORCHED)    ? "RK_MOOD_SCORCHED" :
                (m == RK_MOOD_DARK)        ? "RK_MOOD_DARK" :
                                             "RK_MOOD_PARCHED_AIR",
                RK_GOLDEN_T_MS, h);
    }

    fprintf(f, "};\n\n");
    fprintf(f, "#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))\n\n");
    fprintf(f, "#endif /* ROOTKIT_GOLDEN_H */\n");
    fclose(f);

    printf("golden.h regenerado con %d cuadros de referencia\n", RK_MOOD_COUNT);
    return 0;
}

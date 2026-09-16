/* Genera test/golden.h con los hashes de referencia de cada modelo en cada
 * ánimo.
 *
 * Se corre a mano cuando un cambio visual es intencional:
 *
 *     make golden
 *
 * El diff del archivo generado muestra exactamente qué caras cambiaron. Como
 * la tabla está ordenada por modelo, un diff que toca once filas seguidas
 * dice "se movió un modelo" y uno que toca una columna dice "se movió un
 * ánimo en todos": las dos son revisiones distintas y conviene distinguirlas
 * de un vistazo.
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
    int p, m;

    if (f == NULL) {
        fprintf(stderr, "no pude escribir %s\n", path);
        return 1;
    }

    fprintf(f, "/* Generado por `make golden` - no editar a mano.\n");
    fprintf(f, " *\n");
    fprintf(f, " * Hash FNV-1a del framebuffer de %dx%d para cada MODELO en\n",
            RK_MINI_W, RK_MINI_H);
    fprintf(f, " * cada ANIMO, con el nodo fijo de golden_util.c. Si un cambio\n");
    fprintf(f, " * de codigo altera cualquier pixel, la suite \"cara\" lo marca.\n");
    fprintf(f, " */\n");
    fprintf(f, "#ifndef ROOTKIT_GOLDEN_H\n#define ROOTKIT_GOLDEN_H\n\n");
    fprintf(f, "#include \"../core/mood.h\"\n\n");
    fprintf(f, "typedef struct {\n");
    fprintf(f, "    int       persona;\n");
    fprintf(f, "    rk_mood_t mood;\n");
    fprintf(f, "    uint32_t  t_ms;\n");
    fprintf(f, "    uint32_t  hash;\n");
    fprintf(f, "} rk_golden_t;\n\n");
    fprintf(f, "static const rk_golden_t RK_GOLDEN[] = {\n");

    for (p = 0; p < rk_persona_count; p++) {
        fprintf(f, "    /* %s */\n", rk_persona_at(p)->nombre);
        for (m = 0; m < RK_MOOD_COUNT; m++) {
            uint32_t h = rk_golden_cara(p, (rk_mood_t)m, RK_GOLDEN_T_MS);
            fprintf(f, "    { %d, %-20s, %5uu, 0x%08XU },\n",
                    p, MOOD_ID[m], RK_GOLDEN_T_MS, h);
        }
    }

    fprintf(f, "};\n\n");
    fprintf(f, "#define RK_GOLDEN_COUNT "
               "((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))\n\n");
    fprintf(f, "#endif /* ROOTKIT_GOLDEN_H */\n");
    fclose(f);

    printf("golden.h regenerado: %d modelos x %d animos = %d caras\n",
           rk_persona_count, RK_MOOD_COUNT, rk_persona_count * RK_MOOD_COUNT);
    return 0;
}

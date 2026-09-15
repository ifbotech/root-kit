/* Generado por `make golden` - no editar a mano.
 *
 * Hash FNV-1a del framebuffer de cada panel para cada estado de
 * animo, con el kit fijo de golden_util.c. Si un cambio de codigo
 * altera cualquier pixel, el test de la suite correspondiente lo
 * marca: "render" para el Prime de 240x320 y "mini" para el de
 * 128x128.
 */
#ifndef ROOTKIT_GOLDEN_H
#define ROOTKIT_GOLDEN_H

#include "../core/mood.h"

typedef struct {
    rk_mood_t mood;
    uint32_t  t_ms;
    uint32_t  prime;
    uint32_t  mini;
} rk_golden_t;

static const rk_golden_t RK_GOLDEN[] = {
    { RK_MOOD_UNKNOWN     ,  1200u, 0x4A68DBB0U, 0x37B10605U },
    { RK_MOOD_OFFLINE     ,  1200u, 0xCB665FBEU, 0x1B2E2298U },
    { RK_MOOD_SLEEPING    ,  1200u, 0xF4561BF3U, 0x77C525A7U },
    { RK_MOOD_HAPPY       ,  1200u, 0xF9635CABU, 0x9D619DE2U },
    { RK_MOOD_THIRSTY     ,  1200u, 0xF641F833U, 0x0FF033B8U },
    { RK_MOOD_DROWNING    ,  1200u, 0x3BD5868EU, 0xD7FC7C23U },
    { RK_MOOD_COLD        ,  1200u, 0x8C4141FCU, 0x2DC41841U },
    { RK_MOOD_HOT         ,  1200u, 0xC22BA7A6U, 0x369A37F2U },
    { RK_MOOD_SCORCHED    ,  1200u, 0x38D0B272U, 0xF3C8CC2AU },
    { RK_MOOD_DARK        ,  1200u, 0xFC2A526AU, 0x7D42F7B5U },
    { RK_MOOD_PARCHED_AIR ,  1200u, 0x1117BBC3U, 0x613F45F6U },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

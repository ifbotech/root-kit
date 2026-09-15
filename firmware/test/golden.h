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
    { RK_MOOD_UNKNOWN     ,  1200u, 0xCA87009AU, 0x37B10605U },
    { RK_MOOD_OFFLINE     ,  1200u, 0xB4590238U, 0x1B2E2298U },
    { RK_MOOD_SLEEPING    ,  1200u, 0xBF64BF9FU, 0x77C525A7U },
    { RK_MOOD_HAPPY       ,  1200u, 0x34615309U, 0x9D619DE2U },
    { RK_MOOD_THIRSTY     ,  1200u, 0x7A5C9945U, 0x0FF033B8U },
    { RK_MOOD_DROWNING    ,  1200u, 0x86605EF9U, 0xD7FC7C23U },
    { RK_MOOD_COLD        ,  1200u, 0x295561BAU, 0x2DC41841U },
    { RK_MOOD_HOT         ,  1200u, 0xF12FE3BAU, 0x369A37F2U },
    { RK_MOOD_SCORCHED    ,  1200u, 0x07397D74U, 0xF3C8CC2AU },
    { RK_MOOD_DARK        ,  1200u, 0xF995DE38U, 0x7D42F7B5U },
    { RK_MOOD_PARCHED_AIR ,  1200u, 0x25785F47U, 0x613F45F6U },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

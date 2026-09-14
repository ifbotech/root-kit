/* Generado por `make golden` — no editar a mano.
 *
 * Hash FNV-1a del framebuffer de 160x240 para cada estado de animo,
 * con el escenario fijo de golden_util.c. Si un cambio de codigo
 * altera cualquier pixel, el test de la suite "render" lo marca.
 */
#ifndef ROOTKIT_GOLDEN_H
#define ROOTKIT_GOLDEN_H

#include "../core/mood.h"

typedef struct {
    rk_mood_t mood;
    uint32_t  t_ms;
    uint32_t  hash;
} rk_golden_t;

static const rk_golden_t RK_GOLDEN[] = {
    { RK_MOOD_UNKNOWN     ,  1200u, 0xF613678BU },
    { RK_MOOD_OFFLINE     ,  1200u, 0xA589451FU },
    { RK_MOOD_SLEEPING    ,  1200u, 0x1DDE2EACU },
    { RK_MOOD_HAPPY       ,  1200u, 0xAADC0E6CU },
    { RK_MOOD_THIRSTY     ,  1200u, 0xE7580F68U },
    { RK_MOOD_DROWNING    ,  1200u, 0xFE4EE25CU },
    { RK_MOOD_COLD        ,  1200u, 0x90E2FD16U },
    { RK_MOOD_HOT         ,  1200u, 0xE0123241U },
    { RK_MOOD_SCORCHED    ,  1200u, 0x44357435U },
    { RK_MOOD_DARK        ,  1200u, 0xB0A6874EU },
    { RK_MOOD_PARCHED_AIR ,  1200u, 0x90DE16E1U },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

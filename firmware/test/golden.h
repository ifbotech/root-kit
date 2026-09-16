/* Generado por `make golden` - no editar a mano.
 *
 * Hash FNV-1a del framebuffer de 128x128 para cada MODELO en
 * cada ANIMO, con el nodo fijo de golden_util.c. Si un cambio
 * de codigo altera cualquier pixel, la suite "cara" lo marca.
 */
#ifndef ROOTKIT_GOLDEN_H
#define ROOTKIT_GOLDEN_H

#include "../core/mood.h"

typedef struct {
    int       persona;
    rk_mood_t mood;
    uint32_t  t_ms;
    uint32_t  hash;
} rk_golden_t;

static const rk_golden_t RK_GOLDEN[] = {
    /* Cresta */
    { 0, RK_MOOD_UNKNOWN     ,  1200u, 0x904D53D8U },
    { 0, RK_MOOD_OFFLINE     ,  1200u, 0x3CD7383FU },
    { 0, RK_MOOD_SLEEPING    ,  1200u, 0x99B63CBEU },
    { 0, RK_MOOD_HAPPY       ,  1200u, 0x5BA171F9U },
    { 0, RK_MOOD_THIRSTY     ,  1200u, 0x04EF6472U },
    { 0, RK_MOOD_DROWNING    ,  1200u, 0x46BF7DF9U },
    { 0, RK_MOOD_COLD        ,  1200u, 0x773B02AAU },
    { 0, RK_MOOD_HOT         ,  1200u, 0xF7D66740U },
    { 0, RK_MOOD_SCORCHED    ,  1200u, 0x65705ACDU },
    { 0, RK_MOOD_DARK        ,  1200u, 0x57EB71C3U },
    { 0, RK_MOOD_PARCHED_AIR ,  1200u, 0xA9ADA1F5U },
    /* Kawaii */
    { 1, RK_MOOD_UNKNOWN     ,  1200u, 0x2C51F97EU },
    { 1, RK_MOOD_OFFLINE     ,  1200u, 0x2B35965FU },
    { 1, RK_MOOD_SLEEPING    ,  1200u, 0xB992B202U },
    { 1, RK_MOOD_HAPPY       ,  1200u, 0xA05360BFU },
    { 1, RK_MOOD_THIRSTY     ,  1200u, 0x5ACC52F9U },
    { 1, RK_MOOD_DROWNING    ,  1200u, 0xCB31E768U },
    { 1, RK_MOOD_COLD        ,  1200u, 0x67043BF2U },
    { 1, RK_MOOD_HOT         ,  1200u, 0xAE43D2FDU },
    { 1, RK_MOOD_SCORCHED    ,  1200u, 0xC369A99FU },
    { 1, RK_MOOD_DARK        ,  1200u, 0x50582B32U },
    { 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x1BC7585FU },
    /* Visor */
    { 2, RK_MOOD_UNKNOWN     ,  1200u, 0x730E862CU },
    { 2, RK_MOOD_OFFLINE     ,  1200u, 0xCF353698U },
    { 2, RK_MOOD_SLEEPING    ,  1200u, 0x8DB0AEEAU },
    { 2, RK_MOOD_HAPPY       ,  1200u, 0xC7C30308U },
    { 2, RK_MOOD_THIRSTY     ,  1200u, 0x9B654B72U },
    { 2, RK_MOOD_DROWNING    ,  1200u, 0x04A71676U },
    { 2, RK_MOOD_COLD        ,  1200u, 0x4A7C6DE6U },
    { 2, RK_MOOD_HOT         ,  1200u, 0x13915672U },
    { 2, RK_MOOD_SCORCHED    ,  1200u, 0xF57EF01DU },
    { 2, RK_MOOD_DARK        ,  1200u, 0x0CDEB12FU },
    { 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x09DE27E4U },
    /* Ciclope */
    { 3, RK_MOOD_UNKNOWN     ,  1200u, 0xF6E65F42U },
    { 3, RK_MOOD_OFFLINE     ,  1200u, 0xF8BF2713U },
    { 3, RK_MOOD_SLEEPING    ,  1200u, 0x0229C0F1U },
    { 3, RK_MOOD_HAPPY       ,  1200u, 0xFAF1F4F4U },
    { 3, RK_MOOD_THIRSTY     ,  1200u, 0xFD8D636CU },
    { 3, RK_MOOD_DROWNING    ,  1200u, 0x9B70E30CU },
    { 3, RK_MOOD_COLD        ,  1200u, 0xD9FC90D3U },
    { 3, RK_MOOD_HOT         ,  1200u, 0x52C616DEU },
    { 3, RK_MOOD_SCORCHED    ,  1200u, 0x545E4FF2U },
    { 3, RK_MOOD_DARK        ,  1200u, 0x1244CF28U },
    { 3, RK_MOOD_PARCHED_AIR ,  1200u, 0x55A6DD31U },
    /* Hongo */
    { 4, RK_MOOD_UNKNOWN     ,  1200u, 0xA11124DDU },
    { 4, RK_MOOD_OFFLINE     ,  1200u, 0x7F1675DCU },
    { 4, RK_MOOD_SLEEPING    ,  1200u, 0x63FA2DAAU },
    { 4, RK_MOOD_HAPPY       ,  1200u, 0x91DE0819U },
    { 4, RK_MOOD_THIRSTY     ,  1200u, 0x7A389B20U },
    { 4, RK_MOOD_DROWNING    ,  1200u, 0xC67ABAC4U },
    { 4, RK_MOOD_COLD        ,  1200u, 0x8BD49894U },
    { 4, RK_MOOD_HOT         ,  1200u, 0xF0093208U },
    { 4, RK_MOOD_SCORCHED    ,  1200u, 0x32610020U },
    { 4, RK_MOOD_DARK        ,  1200u, 0xFE9C64F6U },
    { 4, RK_MOOD_PARCHED_AIR ,  1200u, 0xF6DA4C30U },
    /* ????? */
    { 5, RK_MOOD_UNKNOWN     ,  1200u, 0xD0848485U },
    { 5, RK_MOOD_OFFLINE     ,  1200u, 0xE42EAF4BU },
    { 5, RK_MOOD_SLEEPING    ,  1200u, 0x6D81D6E3U },
    { 5, RK_MOOD_HAPPY       ,  1200u, 0x7BBF3035U },
    { 5, RK_MOOD_THIRSTY     ,  1200u, 0xDB8D55C2U },
    { 5, RK_MOOD_DROWNING    ,  1200u, 0x86CA6741U },
    { 5, RK_MOOD_COLD        ,  1200u, 0x4F8FD222U },
    { 5, RK_MOOD_HOT         ,  1200u, 0x760D6573U },
    { 5, RK_MOOD_SCORCHED    ,  1200u, 0x3EB86A4BU },
    { 5, RK_MOOD_DARK        ,  1200u, 0x04148A98U },
    { 5, RK_MOOD_PARCHED_AIR ,  1200u, 0x3ED11785U },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

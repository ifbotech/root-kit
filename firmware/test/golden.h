/* Generado por `make golden` - no editar a mano.
 *
 * Hash FNV-1a del framebuffer de 128x128 para cada ROOTI, en
 * cada PIEL y cada ANIMO, con el nodo fijo de golden_util.c. Si un cambio
 * de codigo altera cualquier pixel, la suite "cara" lo marca.
 */
#ifndef ROOTKIT_GOLDEN_H
#define ROOTKIT_GOLDEN_H

#include "../core/mood.h"

typedef struct {
    int       persona;
    int       rareza;
    rk_mood_t mood;
    uint32_t  t_ms;
    uint32_t  hash;
} rk_golden_t;

static const rk_golden_t RK_GOLDEN[] = {
    /* Kip, COMUN */
    { 0, 0, RK_MOOD_UNKNOWN     ,  1200u, 0xD14E039EU },
    { 0, 0, RK_MOOD_OFFLINE     ,  1200u, 0x5DE0AB82U },
    { 0, 0, RK_MOOD_SLEEPING    ,  1200u, 0x249A0151U },
    { 0, 0, RK_MOOD_HAPPY       ,  1200u, 0x93660E4BU },
    { 0, 0, RK_MOOD_THIRSTY     ,  1200u, 0x3FE27168U },
    { 0, 0, RK_MOOD_DROWNING    ,  1200u, 0x4ADCFDA4U },
    { 0, 0, RK_MOOD_COLD        ,  1200u, 0xF367DA41U },
    { 0, 0, RK_MOOD_HOT         ,  1200u, 0xBA16D1F1U },
    { 0, 0, RK_MOOD_SCORCHED    ,  1200u, 0x8D43E63DU },
    { 0, 0, RK_MOOD_DARK        ,  1200u, 0xF31DFF9BU },
    { 0, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x701CDC86U },
    /* Kip, RARA */
    { 0, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x4C171480U },
    { 0, 1, RK_MOOD_OFFLINE     ,  1200u, 0xC9E88640U },
    { 0, 1, RK_MOOD_SLEEPING    ,  1200u, 0xD299237AU },
    { 0, 1, RK_MOOD_HAPPY       ,  1200u, 0xB4B46763U },
    { 0, 1, RK_MOOD_THIRSTY     ,  1200u, 0x890D6402U },
    { 0, 1, RK_MOOD_DROWNING    ,  1200u, 0x71672737U },
    { 0, 1, RK_MOOD_COLD        ,  1200u, 0x10B47B1EU },
    { 0, 1, RK_MOOD_HOT         ,  1200u, 0xCB2648E0U },
    { 0, 1, RK_MOOD_SCORCHED    ,  1200u, 0x40C31AC8U },
    { 0, 1, RK_MOOD_DARK        ,  1200u, 0x731EF7CDU },
    { 0, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0xC853998AU },
    /* Kip, EPICA */
    { 0, 2, RK_MOOD_UNKNOWN     ,  1200u, 0xD583B6B0U },
    { 0, 2, RK_MOOD_OFFLINE     ,  1200u, 0x90C9BD53U },
    { 0, 2, RK_MOOD_SLEEPING    ,  1200u, 0xCBB6A640U },
    { 0, 2, RK_MOOD_HAPPY       ,  1200u, 0x35BAD242U },
    { 0, 2, RK_MOOD_THIRSTY     ,  1200u, 0x5EFFBAF4U },
    { 0, 2, RK_MOOD_DROWNING    ,  1200u, 0x84476154U },
    { 0, 2, RK_MOOD_COLD        ,  1200u, 0x4782CE12U },
    { 0, 2, RK_MOOD_HOT         ,  1200u, 0xD45757E2U },
    { 0, 2, RK_MOOD_SCORCHED    ,  1200u, 0xE1AFFD5CU },
    { 0, 2, RK_MOOD_DARK        ,  1200u, 0x473A69EAU },
    { 0, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x13FBFB71U },
    /* Nori, COMUN */
    { 1, 0, RK_MOOD_UNKNOWN     ,  1200u, 0x30E43240U },
    { 1, 0, RK_MOOD_OFFLINE     ,  1200u, 0xA1FC277AU },
    { 1, 0, RK_MOOD_SLEEPING    ,  1200u, 0x806074B7U },
    { 1, 0, RK_MOOD_HAPPY       ,  1200u, 0x31B6B90AU },
    { 1, 0, RK_MOOD_THIRSTY     ,  1200u, 0x9EF5D44AU },
    { 1, 0, RK_MOOD_DROWNING    ,  1200u, 0xE31D2A7EU },
    { 1, 0, RK_MOOD_COLD        ,  1200u, 0x115F07DBU },
    { 1, 0, RK_MOOD_HOT         ,  1200u, 0x0757DDAEU },
    { 1, 0, RK_MOOD_SCORCHED    ,  1200u, 0xECF18525U },
    { 1, 0, RK_MOOD_DARK        ,  1200u, 0xD63A52F1U },
    { 1, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x41CAAC67U },
    /* Nori, RARA */
    { 1, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x95D9FC24U },
    { 1, 1, RK_MOOD_OFFLINE     ,  1200u, 0xC99C20C3U },
    { 1, 1, RK_MOOD_SLEEPING    ,  1200u, 0xCDC6176BU },
    { 1, 1, RK_MOOD_HAPPY       ,  1200u, 0x30377311U },
    { 1, 1, RK_MOOD_THIRSTY     ,  1200u, 0x7AD0F789U },
    { 1, 1, RK_MOOD_DROWNING    ,  1200u, 0xB7B9B217U },
    { 1, 1, RK_MOOD_COLD        ,  1200u, 0xA234DDAFU },
    { 1, 1, RK_MOOD_HOT         ,  1200u, 0x2A71E6A2U },
    { 1, 1, RK_MOOD_SCORCHED    ,  1200u, 0xB3EE2E36U },
    { 1, 1, RK_MOOD_DARK        ,  1200u, 0x37505207U },
    { 1, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0xC69B3006U },
    /* Nori, EPICA */
    { 1, 2, RK_MOOD_UNKNOWN     ,  1200u, 0x939B413AU },
    { 1, 2, RK_MOOD_OFFLINE     ,  1200u, 0xBC1E9F8FU },
    { 1, 2, RK_MOOD_SLEEPING    ,  1200u, 0x885357F0U },
    { 1, 2, RK_MOOD_HAPPY       ,  1200u, 0x6423DDA2U },
    { 1, 2, RK_MOOD_THIRSTY     ,  1200u, 0x1B73A1CFU },
    { 1, 2, RK_MOOD_DROWNING    ,  1200u, 0xF7E49DF1U },
    { 1, 2, RK_MOOD_COLD        ,  1200u, 0xCEB7B5F5U },
    { 1, 2, RK_MOOD_HOT         ,  1200u, 0xA9910DE1U },
    { 1, 2, RK_MOOD_SCORCHED    ,  1200u, 0x5CAD0631U },
    { 1, 2, RK_MOOD_DARK        ,  1200u, 0x5C579869U },
    { 1, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x572C1AF2U },
    /* Blink, COMUN */
    { 2, 0, RK_MOOD_UNKNOWN     ,  1200u, 0x28E4F52AU },
    { 2, 0, RK_MOOD_OFFLINE     ,  1200u, 0xC97F7425U },
    { 2, 0, RK_MOOD_SLEEPING    ,  1200u, 0xF6ABC00AU },
    { 2, 0, RK_MOOD_HAPPY       ,  1200u, 0x6E74B1C6U },
    { 2, 0, RK_MOOD_THIRSTY     ,  1200u, 0x6C4936FDU },
    { 2, 0, RK_MOOD_DROWNING    ,  1200u, 0x531683EBU },
    { 2, 0, RK_MOOD_COLD        ,  1200u, 0x9C593290U },
    { 2, 0, RK_MOOD_HOT         ,  1200u, 0xA7A7AD09U },
    { 2, 0, RK_MOOD_SCORCHED    ,  1200u, 0x94FD5E6AU },
    { 2, 0, RK_MOOD_DARK        ,  1200u, 0x2BB50E54U },
    { 2, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x5ECB7FFCU },
    /* Blink, RARA */
    { 2, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x6401C55BU },
    { 2, 1, RK_MOOD_OFFLINE     ,  1200u, 0x4832C971U },
    { 2, 1, RK_MOOD_SLEEPING    ,  1200u, 0x511F5A4AU },
    { 2, 1, RK_MOOD_HAPPY       ,  1200u, 0xF668DA8EU },
    { 2, 1, RK_MOOD_THIRSTY     ,  1200u, 0x2332464CU },
    { 2, 1, RK_MOOD_DROWNING    ,  1200u, 0x1F19B3ECU },
    { 2, 1, RK_MOOD_COLD        ,  1200u, 0x8ACA3B5FU },
    { 2, 1, RK_MOOD_HOT         ,  1200u, 0xE08BAE3EU },
    { 2, 1, RK_MOOD_SCORCHED    ,  1200u, 0x3B0E9207U },
    { 2, 1, RK_MOOD_DARK        ,  1200u, 0xC3CF59FFU },
    { 2, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x1573D1A9U },
    /* Blink, EPICA */
    { 2, 2, RK_MOOD_UNKNOWN     ,  1200u, 0x3C11E503U },
    { 2, 2, RK_MOOD_OFFLINE     ,  1200u, 0xF0C32CB9U },
    { 2, 2, RK_MOOD_SLEEPING    ,  1200u, 0x55454F01U },
    { 2, 2, RK_MOOD_HAPPY       ,  1200u, 0x837CEDDCU },
    { 2, 2, RK_MOOD_THIRSTY     ,  1200u, 0x2ADC023AU },
    { 2, 2, RK_MOOD_DROWNING    ,  1200u, 0x4393F324U },
    { 2, 2, RK_MOOD_COLD        ,  1200u, 0xFF41A685U },
    { 2, 2, RK_MOOD_HOT         ,  1200u, 0x23219F06U },
    { 2, 2, RK_MOOD_SCORCHED    ,  1200u, 0x1116B14EU },
    { 2, 2, RK_MOOD_DARK        ,  1200u, 0xB6853846U },
    { 2, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0xAA4D135AU },
    /* Plum, COMUN */
    { 3, 0, RK_MOOD_UNKNOWN     ,  1200u, 0x0F6A95E2U },
    { 3, 0, RK_MOOD_OFFLINE     ,  1200u, 0x9E067DDDU },
    { 3, 0, RK_MOOD_SLEEPING    ,  1200u, 0x34E65DC7U },
    { 3, 0, RK_MOOD_HAPPY       ,  1200u, 0x6FDDD230U },
    { 3, 0, RK_MOOD_THIRSTY     ,  1200u, 0x0B8331A3U },
    { 3, 0, RK_MOOD_DROWNING    ,  1200u, 0xC8903AAFU },
    { 3, 0, RK_MOOD_COLD        ,  1200u, 0x15639242U },
    { 3, 0, RK_MOOD_HOT         ,  1200u, 0xB37007C8U },
    { 3, 0, RK_MOOD_SCORCHED    ,  1200u, 0xE9DF46C8U },
    { 3, 0, RK_MOOD_DARK        ,  1200u, 0x1D727122U },
    { 3, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0xAC67CE44U },
    /* Plum, RARA */
    { 3, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x266A66A7U },
    { 3, 1, RK_MOOD_OFFLINE     ,  1200u, 0x128569B4U },
    { 3, 1, RK_MOOD_SLEEPING    ,  1200u, 0x0D0EDCABU },
    { 3, 1, RK_MOOD_HAPPY       ,  1200u, 0xA7069F13U },
    { 3, 1, RK_MOOD_THIRSTY     ,  1200u, 0x2BDC5DC6U },
    { 3, 1, RK_MOOD_DROWNING    ,  1200u, 0x5C0933E9U },
    { 3, 1, RK_MOOD_COLD        ,  1200u, 0xB85A5BFAU },
    { 3, 1, RK_MOOD_HOT         ,  1200u, 0x5B741848U },
    { 3, 1, RK_MOOD_SCORCHED    ,  1200u, 0x96C610C9U },
    { 3, 1, RK_MOOD_DARK        ,  1200u, 0xED1DC8C7U },
    { 3, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x90588249U },
    /* Plum, EPICA */
    { 3, 2, RK_MOOD_UNKNOWN     ,  1200u, 0xE60B6227U },
    { 3, 2, RK_MOOD_OFFLINE     ,  1200u, 0x4048A2EDU },
    { 3, 2, RK_MOOD_SLEEPING    ,  1200u, 0x6BEADA23U },
    { 3, 2, RK_MOOD_HAPPY       ,  1200u, 0xF7F33D7DU },
    { 3, 2, RK_MOOD_THIRSTY     ,  1200u, 0x85C9CA37U },
    { 3, 2, RK_MOOD_DROWNING    ,  1200u, 0x4E11A6C0U },
    { 3, 2, RK_MOOD_COLD        ,  1200u, 0x305F25D1U },
    { 3, 2, RK_MOOD_HOT         ,  1200u, 0x6769D2E5U },
    { 3, 2, RK_MOOD_SCORCHED    ,  1200u, 0x9C599B1DU },
    { 3, 2, RK_MOOD_DARK        ,  1200u, 0xF22A5CEDU },
    { 3, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x492AC9AEU },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

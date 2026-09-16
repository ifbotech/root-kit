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
    { 0, RK_MOOD_UNKNOWN     ,  1200u, 0xCB07AC30U },
    { 0, RK_MOOD_OFFLINE     ,  1200u, 0x9997736AU },
    { 0, RK_MOOD_SLEEPING    ,  1200u, 0x54BA9421U },
    { 0, RK_MOOD_HAPPY       ,  1200u, 0xA49EFF33U },
    { 0, RK_MOOD_THIRSTY     ,  1200u, 0x182BE899U },
    { 0, RK_MOOD_DROWNING    ,  1200u, 0x55D78A7EU },
    { 0, RK_MOOD_COLD        ,  1200u, 0x5A144051U },
    { 0, RK_MOOD_HOT         ,  1200u, 0xCE8318CBU },
    { 0, RK_MOOD_SCORCHED    ,  1200u, 0xB621F05BU },
    { 0, RK_MOOD_DARK        ,  1200u, 0x933A8AC0U },
    { 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x5DB5A200U },
    /* Kawaii */
    { 1, RK_MOOD_UNKNOWN     ,  1200u, 0x91756AB2U },
    { 1, RK_MOOD_OFFLINE     ,  1200u, 0xFF8A52B1U },
    { 1, RK_MOOD_SLEEPING    ,  1200u, 0x72353901U },
    { 1, RK_MOOD_HAPPY       ,  1200u, 0xB6CCA321U },
    { 1, RK_MOOD_THIRSTY     ,  1200u, 0x64143FFEU },
    { 1, RK_MOOD_DROWNING    ,  1200u, 0x3D742281U },
    { 1, RK_MOOD_COLD        ,  1200u, 0x71F3BC34U },
    { 1, RK_MOOD_HOT         ,  1200u, 0xB1707DA4U },
    { 1, RK_MOOD_SCORCHED    ,  1200u, 0xBF901628U },
    { 1, RK_MOOD_DARK        ,  1200u, 0x107C5A71U },
    { 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x0D1B3865U },
    /* Visor */
    { 2, RK_MOOD_UNKNOWN     ,  1200u, 0x124E3F27U },
    { 2, RK_MOOD_OFFLINE     ,  1200u, 0x8DDCAD73U },
    { 2, RK_MOOD_SLEEPING    ,  1200u, 0xBA8AFC9CU },
    { 2, RK_MOOD_HAPPY       ,  1200u, 0xA4944C2FU },
    { 2, RK_MOOD_THIRSTY     ,  1200u, 0x2FE3E2D5U },
    { 2, RK_MOOD_DROWNING    ,  1200u, 0xBAC488F9U },
    { 2, RK_MOOD_COLD        ,  1200u, 0x5D73DE7CU },
    { 2, RK_MOOD_HOT         ,  1200u, 0x4D0068A4U },
    { 2, RK_MOOD_SCORCHED    ,  1200u, 0xAD84F2F7U },
    { 2, RK_MOOD_DARK        ,  1200u, 0x5837F7E7U },
    { 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x74FEF243U },
    /* Ciclope */
    { 3, RK_MOOD_UNKNOWN     ,  1200u, 0x4120C53DU },
    { 3, RK_MOOD_OFFLINE     ,  1200u, 0x7C102456U },
    { 3, RK_MOOD_SLEEPING    ,  1200u, 0xB95A2409U },
    { 3, RK_MOOD_HAPPY       ,  1200u, 0x9FCC2018U },
    { 3, RK_MOOD_THIRSTY     ,  1200u, 0xE5030B92U },
    { 3, RK_MOOD_DROWNING    ,  1200u, 0xA59D1B63U },
    { 3, RK_MOOD_COLD        ,  1200u, 0x1878AEAFU },
    { 3, RK_MOOD_HOT         ,  1200u, 0x8F40D4A5U },
    { 3, RK_MOOD_SCORCHED    ,  1200u, 0x2ED88944U },
    { 3, RK_MOOD_DARK        ,  1200u, 0xF4DF828BU },
    { 3, RK_MOOD_PARCHED_AIR ,  1200u, 0x085E1437U },
    /* Hongo */
    { 4, RK_MOOD_UNKNOWN     ,  1200u, 0xC4A0EFB8U },
    { 4, RK_MOOD_OFFLINE     ,  1200u, 0x17D188BFU },
    { 4, RK_MOOD_SLEEPING    ,  1200u, 0x1EC54A2EU },
    { 4, RK_MOOD_HAPPY       ,  1200u, 0x1F112DE0U },
    { 4, RK_MOOD_THIRSTY     ,  1200u, 0x1343B9DCU },
    { 4, RK_MOOD_DROWNING    ,  1200u, 0x77EF78A1U },
    { 4, RK_MOOD_COLD        ,  1200u, 0xB2C0AD50U },
    { 4, RK_MOOD_HOT         ,  1200u, 0x29079E62U },
    { 4, RK_MOOD_SCORCHED    ,  1200u, 0x640F631CU },
    { 4, RK_MOOD_DARK        ,  1200u, 0x87AE79B6U },
    { 4, RK_MOOD_PARCHED_AIR ,  1200u, 0xEADE08CFU },
    /* ????? */
    { 5, RK_MOOD_UNKNOWN     ,  1200u, 0x08A7D889U },
    { 5, RK_MOOD_OFFLINE     ,  1200u, 0xAA3751DAU },
    { 5, RK_MOOD_SLEEPING    ,  1200u, 0xB1F6A4D4U },
    { 5, RK_MOOD_HAPPY       ,  1200u, 0xFF48A1F8U },
    { 5, RK_MOOD_THIRSTY     ,  1200u, 0x9FE1D1E7U },
    { 5, RK_MOOD_DROWNING    ,  1200u, 0x65A41856U },
    { 5, RK_MOOD_COLD        ,  1200u, 0x83C6BAF5U },
    { 5, RK_MOOD_HOT         ,  1200u, 0xADBD5124U },
    { 5, RK_MOOD_SCORCHED    ,  1200u, 0x89CFD7EAU },
    { 5, RK_MOOD_DARK        ,  1200u, 0xFB20F8C1U },
    { 5, RK_MOOD_PARCHED_AIR ,  1200u, 0xEF12789FU },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

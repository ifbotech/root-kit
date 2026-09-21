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
    { 0, 0, RK_MOOD_UNKNOWN     ,  1200u, 0xB902EEBDU },
    { 0, 0, RK_MOOD_OFFLINE     ,  1200u, 0x9C2F9CCBU },
    { 0, 0, RK_MOOD_SLEEPING    ,  1200u, 0xB8FDFF82U },
    { 0, 0, RK_MOOD_HAPPY       ,  1200u, 0x62DC6909U },
    { 0, 0, RK_MOOD_THIRSTY     ,  1200u, 0x948AC6C0U },
    { 0, 0, RK_MOOD_DROWNING    ,  1200u, 0x41959D06U },
    { 0, 0, RK_MOOD_COLD        ,  1200u, 0x65260B85U },
    { 0, 0, RK_MOOD_HOT         ,  1200u, 0x7230FBE1U },
    { 0, 0, RK_MOOD_SCORCHED    ,  1200u, 0xD881E614U },
    { 0, 0, RK_MOOD_DARK        ,  1200u, 0x1DE67BC1U },
    { 0, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0xBCE192B4U },
    /* Kip, RARA */
    { 0, 1, RK_MOOD_UNKNOWN     ,  1200u, 0xB73B4AA6U },
    { 0, 1, RK_MOOD_OFFLINE     ,  1200u, 0x88453EE7U },
    { 0, 1, RK_MOOD_SLEEPING    ,  1200u, 0x0F5EC108U },
    { 0, 1, RK_MOOD_HAPPY       ,  1200u, 0x100FF736U },
    { 0, 1, RK_MOOD_THIRSTY     ,  1200u, 0x216B2FDEU },
    { 0, 1, RK_MOOD_DROWNING    ,  1200u, 0x3BBB290DU },
    { 0, 1, RK_MOOD_COLD        ,  1200u, 0xBFA3910BU },
    { 0, 1, RK_MOOD_HOT         ,  1200u, 0x4588716EU },
    { 0, 1, RK_MOOD_SCORCHED    ,  1200u, 0x96AC7210U },
    { 0, 1, RK_MOOD_DARK        ,  1200u, 0x4D87E40AU },
    { 0, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x0905A1C1U },
    /* Kip, EPICA */
    { 0, 2, RK_MOOD_UNKNOWN     ,  1200u, 0x748DBDFAU },
    { 0, 2, RK_MOOD_OFFLINE     ,  1200u, 0xF7E4FD7BU },
    { 0, 2, RK_MOOD_SLEEPING    ,  1200u, 0x39321D91U },
    { 0, 2, RK_MOOD_HAPPY       ,  1200u, 0xA3EC9A64U },
    { 0, 2, RK_MOOD_THIRSTY     ,  1200u, 0xC932B498U },
    { 0, 2, RK_MOOD_DROWNING    ,  1200u, 0x271A3BC7U },
    { 0, 2, RK_MOOD_COLD        ,  1200u, 0x5752337FU },
    { 0, 2, RK_MOOD_HOT         ,  1200u, 0x6105692BU },
    { 0, 2, RK_MOOD_SCORCHED    ,  1200u, 0xDF1259EAU },
    { 0, 2, RK_MOOD_DARK        ,  1200u, 0x2A2DDEA7U },
    { 0, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x1C1C1D2CU },
    /* Nori, COMUN */
    { 1, 0, RK_MOOD_UNKNOWN     ,  1200u, 0x80ADD847U },
    { 1, 0, RK_MOOD_OFFLINE     ,  1200u, 0x779C97BBU },
    { 1, 0, RK_MOOD_SLEEPING    ,  1200u, 0xC3BED986U },
    { 1, 0, RK_MOOD_HAPPY       ,  1200u, 0xC2AB4DB8U },
    { 1, 0, RK_MOOD_THIRSTY     ,  1200u, 0x845A494BU },
    { 1, 0, RK_MOOD_DROWNING    ,  1200u, 0xE7FD0E08U },
    { 1, 0, RK_MOOD_COLD        ,  1200u, 0x4629AD54U },
    { 1, 0, RK_MOOD_HOT         ,  1200u, 0x05E66D87U },
    { 1, 0, RK_MOOD_SCORCHED    ,  1200u, 0x32160D37U },
    { 1, 0, RK_MOOD_DARK        ,  1200u, 0xB891EF8FU },
    { 1, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x547F4931U },
    /* Nori, RARA */
    { 1, 1, RK_MOOD_UNKNOWN     ,  1200u, 0xF1D74200U },
    { 1, 1, RK_MOOD_OFFLINE     ,  1200u, 0x9CB31269U },
    { 1, 1, RK_MOOD_SLEEPING    ,  1200u, 0xF28A29B7U },
    { 1, 1, RK_MOOD_HAPPY       ,  1200u, 0x3DA89EFEU },
    { 1, 1, RK_MOOD_THIRSTY     ,  1200u, 0x8D232D0FU },
    { 1, 1, RK_MOOD_DROWNING    ,  1200u, 0xC43F1DB5U },
    { 1, 1, RK_MOOD_COLD        ,  1200u, 0x568A810DU },
    { 1, 1, RK_MOOD_HOT         ,  1200u, 0x123A1C6FU },
    { 1, 1, RK_MOOD_SCORCHED    ,  1200u, 0xDD0BC1AFU },
    { 1, 1, RK_MOOD_DARK        ,  1200u, 0x17F75393U },
    { 1, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0xC6F418DFU },
    /* Nori, EPICA */
    { 1, 2, RK_MOOD_UNKNOWN     ,  1200u, 0x8E80746FU },
    { 1, 2, RK_MOOD_OFFLINE     ,  1200u, 0x1CC2A107U },
    { 1, 2, RK_MOOD_SLEEPING    ,  1200u, 0xB36BCFFDU },
    { 1, 2, RK_MOOD_HAPPY       ,  1200u, 0xD221E992U },
    { 1, 2, RK_MOOD_THIRSTY     ,  1200u, 0xC3311B0AU },
    { 1, 2, RK_MOOD_DROWNING    ,  1200u, 0xEB3E3A38U },
    { 1, 2, RK_MOOD_COLD        ,  1200u, 0xB13DBD81U },
    { 1, 2, RK_MOOD_HOT         ,  1200u, 0xF834CF81U },
    { 1, 2, RK_MOOD_SCORCHED    ,  1200u, 0x38DA687CU },
    { 1, 2, RK_MOOD_DARK        ,  1200u, 0x50A05F13U },
    { 1, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x222E4971U },
    /* Blink, COMUN */
    { 2, 0, RK_MOOD_UNKNOWN     ,  1200u, 0x9E70166DU },
    { 2, 0, RK_MOOD_OFFLINE     ,  1200u, 0xCFAEA44DU },
    { 2, 0, RK_MOOD_SLEEPING    ,  1200u, 0xF3A7FBA2U },
    { 2, 0, RK_MOOD_HAPPY       ,  1200u, 0x22761F73U },
    { 2, 0, RK_MOOD_THIRSTY     ,  1200u, 0x36EA66F9U },
    { 2, 0, RK_MOOD_DROWNING    ,  1200u, 0x37C6C5CCU },
    { 2, 0, RK_MOOD_COLD        ,  1200u, 0x4B870005U },
    { 2, 0, RK_MOOD_HOT         ,  1200u, 0x5CB8E1BAU },
    { 2, 0, RK_MOOD_SCORCHED    ,  1200u, 0xE0880AA3U },
    { 2, 0, RK_MOOD_DARK        ,  1200u, 0xC1101DDDU },
    { 2, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0xD6E17262U },
    /* Blink, RARA */
    { 2, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x8BACD381U },
    { 2, 1, RK_MOOD_OFFLINE     ,  1200u, 0xD82B8826U },
    { 2, 1, RK_MOOD_SLEEPING    ,  1200u, 0x8101E78AU },
    { 2, 1, RK_MOOD_HAPPY       ,  1200u, 0x629AD480U },
    { 2, 1, RK_MOOD_THIRSTY     ,  1200u, 0x4559A52CU },
    { 2, 1, RK_MOOD_DROWNING    ,  1200u, 0x3370F6E5U },
    { 2, 1, RK_MOOD_COLD        ,  1200u, 0x01EED545U },
    { 2, 1, RK_MOOD_HOT         ,  1200u, 0x017EECD2U },
    { 2, 1, RK_MOOD_SCORCHED    ,  1200u, 0x664BF09AU },
    { 2, 1, RK_MOOD_DARK        ,  1200u, 0xDF4BCFA0U },
    { 2, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0xDE3917C7U },
    /* Blink, EPICA */
    { 2, 2, RK_MOOD_UNKNOWN     ,  1200u, 0xF2F86EAFU },
    { 2, 2, RK_MOOD_OFFLINE     ,  1200u, 0xF843B34AU },
    { 2, 2, RK_MOOD_SLEEPING    ,  1200u, 0x6A1AB241U },
    { 2, 2, RK_MOOD_HAPPY       ,  1200u, 0xCE31F012U },
    { 2, 2, RK_MOOD_THIRSTY     ,  1200u, 0x66C3AA85U },
    { 2, 2, RK_MOOD_DROWNING    ,  1200u, 0x666D49D0U },
    { 2, 2, RK_MOOD_COLD        ,  1200u, 0x391B4BB5U },
    { 2, 2, RK_MOOD_HOT         ,  1200u, 0x947A124BU },
    { 2, 2, RK_MOOD_SCORCHED    ,  1200u, 0xFACC5807U },
    { 2, 2, RK_MOOD_DARK        ,  1200u, 0x3AABACF3U },
    { 2, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0x95F1CD50U },
    /* Plum, COMUN */
    { 3, 0, RK_MOOD_UNKNOWN     ,  1200u, 0xF62DCC27U },
    { 3, 0, RK_MOOD_OFFLINE     ,  1200u, 0x0FA021A9U },
    { 3, 0, RK_MOOD_SLEEPING    ,  1200u, 0x56093D97U },
    { 3, 0, RK_MOOD_HAPPY       ,  1200u, 0x7929BCA7U },
    { 3, 0, RK_MOOD_THIRSTY     ,  1200u, 0xBC6B16FCU },
    { 3, 0, RK_MOOD_DROWNING    ,  1200u, 0xD9F88CCEU },
    { 3, 0, RK_MOOD_COLD        ,  1200u, 0x1C31E719U },
    { 3, 0, RK_MOOD_HOT         ,  1200u, 0x4B86E32BU },
    { 3, 0, RK_MOOD_SCORCHED    ,  1200u, 0x586C44DCU },
    { 3, 0, RK_MOOD_DARK        ,  1200u, 0x53E96249U },
    { 3, 0, RK_MOOD_PARCHED_AIR ,  1200u, 0x35138AB2U },
    /* Plum, RARA */
    { 3, 1, RK_MOOD_UNKNOWN     ,  1200u, 0x3F6531AAU },
    { 3, 1, RK_MOOD_OFFLINE     ,  1200u, 0xFE33E58AU },
    { 3, 1, RK_MOOD_SLEEPING    ,  1200u, 0xF4912C1AU },
    { 3, 1, RK_MOOD_HAPPY       ,  1200u, 0x38F33EEBU },
    { 3, 1, RK_MOOD_THIRSTY     ,  1200u, 0xDE792082U },
    { 3, 1, RK_MOOD_DROWNING    ,  1200u, 0xFBB3324FU },
    { 3, 1, RK_MOOD_COLD        ,  1200u, 0xDF98E698U },
    { 3, 1, RK_MOOD_HOT         ,  1200u, 0xE24A2B52U },
    { 3, 1, RK_MOOD_SCORCHED    ,  1200u, 0x85E88000U },
    { 3, 1, RK_MOOD_DARK        ,  1200u, 0xE89857E2U },
    { 3, 1, RK_MOOD_PARCHED_AIR ,  1200u, 0x01175C97U },
    /* Plum, EPICA */
    { 3, 2, RK_MOOD_UNKNOWN     ,  1200u, 0x89545172U },
    { 3, 2, RK_MOOD_OFFLINE     ,  1200u, 0x5705346DU },
    { 3, 2, RK_MOOD_SLEEPING    ,  1200u, 0x21304AACU },
    { 3, 2, RK_MOOD_HAPPY       ,  1200u, 0x5DED4DC0U },
    { 3, 2, RK_MOOD_THIRSTY     ,  1200u, 0x30DD172BU },
    { 3, 2, RK_MOOD_DROWNING    ,  1200u, 0x86D51FA6U },
    { 3, 2, RK_MOOD_COLD        ,  1200u, 0x076FFE97U },
    { 3, 2, RK_MOOD_HOT         ,  1200u, 0x3DFE3FEFU },
    { 3, 2, RK_MOOD_SCORCHED    ,  1200u, 0x95254223U },
    { 3, 2, RK_MOOD_DARK        ,  1200u, 0x5BBB8D4CU },
    { 3, 2, RK_MOOD_PARCHED_AIR ,  1200u, 0xF32DB12DU },
};

#define RK_GOLDEN_COUNT ((int)(sizeof(RK_GOLDEN) / sizeof(RK_GOLDEN[0])))

#endif /* ROOTKIT_GOLDEN_H */

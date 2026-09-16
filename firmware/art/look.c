#include "look.h"
#include <stddef.h>

/* ------------------------------------------------------------ tintes ----- */
#define TINT_FRIO     RK_RGB( 90, 150, 235)
#define TINT_CALOR    RK_RGB(240, 110,  70)
#define TINT_SOL      RK_RGB(255, 210, 120)
#define TINT_SED      RK_RGB(190, 150,  90)
#define TINT_AGUA     RK_RGB( 60, 120, 210)
#define TINT_MUERTO   RK_RGB(110, 120, 130)

/* Toda la personalidad del bicho está concentrada en esta tabla. */
static const rk_look_t LOOKS[RK_MOOD_COUNT] = {
/*               ojo            boca           tinte        amt amp vel shv dim blink */
/* UNKNOWN     */ { RK_OJO_DIZZY,  RK_BOCA_FLAT,  0,            0,   1, 30, 0,   0, false },
/* OFFLINE     */ { RK_OJO_GLITCH, RK_BOCA_FLAT,  TINT_MUERTO, 120,  0,  0, 0,  60, false },
/* SLEEPING    */ { RK_OJO_BLINK,  RK_BOCA_FLAT,  0,            0,   3, 14, 0,  95, false },
/* HAPPY       */ { RK_OJO_OPEN,   RK_BOCA_SMILE, 0,            0,   4, 42, 0,   0, true  },
/* THIRSTY     */ { RK_OJO_SLEEPY, RK_BOCA_FROWN, TINT_SED,    70,   1, 22, 0,   0, true  },
/* DROWNING    */ { RK_OJO_WIDE,   RK_BOCA_OPEN,  TINT_AGUA,   85,   3, 64, 0,  25, false },
/* COLD        */ { RK_OJO_SLEEPY, RK_BOCA_WAVY,  TINT_FRIO,   90,   1, 26, 1,   0, false },
/* HOT         */ { RK_OJO_SLEEPY, RK_BOCA_PANT,  TINT_CALOR,  75,   2, 70, 0,   0, false },
/* SCORCHED    */ { RK_OJO_DEAD,   RK_BOCA_FROWN, TINT_SOL,   100,   1, 18, 0,   0, false },
/* DARK        */ { RK_OJO_WIDE,   RK_BOCA_FLAT,  0,            0,   1, 20, 0, 110, true  },
/* PARCHED_AIR */ { RK_OJO_OPEN,   RK_BOCA_WAVY,  TINT_SED,    40,   2, 34, 0,   0, true  },
};

const rk_look_t *rk_look(rk_mood_t mood)
{
    if ((int)mood < 0 || mood >= RK_MOOD_COUNT) {
        mood = RK_MOOD_UNKNOWN;
    }
    return &LOOKS[mood];
}

bool rk_look_parpadea(const rk_look_t *lk, uint32_t t_ms)
{
    return lk != NULL && lk->blinks && (t_ms % 3400u) < 120u;
}

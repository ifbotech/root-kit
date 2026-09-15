#include "panel.h"

const rk_panel_info_t rk_panel[RK_PANEL_COUNT] = {
    { "PRIME", RK_PRIME_W, RK_PRIME_H, RK_PRIME_PITCH_UM,
      RK_PRIME_ART, RK_PRIME_TEXT },
    { "MINI",  RK_MINI_W,  RK_MINI_H,  RK_MINI_PITCH_UM,
      RK_MINI_ART,  RK_MINI_TEXT  },
};

int rk_panel_decimas_mm(rk_panel_t p, int px)
{
    if ((int)p < 0 || p >= RK_PANEL_COUNT || px < 0) {
        return 0;
    }
    /* px * um/px = um; a décimas de mm hay que dividir por 100. Se redondea
     * al más cercano en vez de truncar: con 64 px en el Mini la diferencia
     * entre 12,9 y 12,8 mm decide si un test pasa o no. */
    return (px * rk_panel[p].pitch_um + 50) / 100;
}

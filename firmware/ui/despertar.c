#include "despertar.h"
#include "../art/face.h"
#include <stddef.h>

/* Los dos intentos de abrir los ojos, como tramos de (inicio, fin, desde,
 * hasta). Entre tramo y tramo el valor se interpola con una curva suave; el
 * último valor se mantiene. */
typedef struct {
    uint16_t t0, t1;
    uint8_t  desde, hasta;
} tramo_t;

static const tramo_t TRAMOS[] = {
    {  900, 1250, 100,  45 },   /* primer intento: se abre a medias       */
    { 1250, 1450,  45,  85 },   /* se le vuelven a cerrar                 */
    { 1450, 1900,  85,   0 },   /* y ahora sí                             */
};

/* Suavizado entero: 3p^2 - 2p^3 con p en 0..256. */
static int32_t suave(int32_t p)
{
    return (p * p * (768 - 2 * p)) >> 16;
}

uint8_t rk_despertar_cierre(uint32_t t_ms)
{
    size_t i;

    if (t_ms < TRAMOS[0].t0) {
        return 100u;
    }
    for (i = 0; i < sizeof TRAMOS / sizeof TRAMOS[0]; i++) {
        const tramo_t *k = &TRAMOS[i];
        if (t_ms < k->t1) {
            int32_t p = (int32_t)((t_ms - k->t0) * 256u / (uint32_t)(k->t1 - k->t0));
            int32_t s = suave(p);
            return (uint8_t)(k->desde + ((int32_t)k->hasta - k->desde) * s / 256);
        }
    }
    return 0u;
}

bool rk_despertar_termino(uint32_t t_ms)
{
    return t_ms >= RK_DESP_FIN_MS;
}

void rk_despertar_draw(rk_fb_t *fb, const rk_persona_t *p, uint32_t t_ms)
{
    if (fb == NULL || fb->px == NULL) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }

    if (t_ms < RK_DESP_NEGRO_MS) {
        /* Un latido lento de la piel sobre negro: algo está por despertar. */
        int pulso = rk_sin8((uint8_t)(t_ms * 256u / RK_DESP_NEGRO_MS - 64u)) + 127;
        rk_fb_clear(fb, rk_mix(RK_RGB(0, 0, 0), p->fondo, (uint8_t)(pulso / 6)));
        return;
    }

    /* Contento desde el principio: con los párpados abajo la sonrisa se lee
     * como un sueño lindo, y cuando los ojos se abren ya está puesta. */
    {
        rk_mood_t m = RK_MOOD_HAPPY;
        uint8_t cierre = rk_despertar_cierre(t_ms);

        rk_face_draw_cierre(fb, p, m, RK_SEV_OK, 0u, cierre, t_ms);

        if (t_ms < RK_DESP_PIEL_MS) {
            /* La piel entra desde negro: se oscurece el cuadro entero. */
            uint8_t oscuro = (uint8_t)(255u - (t_ms - RK_DESP_NEGRO_MS) * 255u
                                       / (RK_DESP_PIEL_MS - RK_DESP_NEGRO_MS));
            int i, n = fb->w * fb->h;
            for (i = 0; i < n; i++) {
                fb->px[i] = rk_dim(fb->px[i], oscuro);
            }
        }
    }
}

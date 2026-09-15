/* Generado por tools/gen_art.py — no editar a mano.
 *
 * Dos escalas del mismo organismo: el ADULTO de 96x72 que vive en el
 * Prime, y el BROTE de 32x32 que vive en cada Mini. Doce paletas, una
 * por simbionte, en el mismo orden que rk_companion_table.
 */
#ifndef ROOTKIT_SPRITES_H
#define ROOTKIT_SPRITES_H

#include "../gfx/fb.h"

/* Una forma es un mapa de índices sin paleta. La paleta la pone quien
 * dibuja, según de qué simbionte se trate, y así los doce comparten
 * los mismos bytes de arte. */
typedef struct {
    uint8_t        w;
    uint8_t        h;
    const uint8_t *idx;
} rk_shape_t;

/* Arma el sprite que espera rk_blit, atando forma y paleta. */
rk_sprite_t rk_shape_sprite(const rk_shape_t *s, const rk_color_t *pal);

#define RK_PAL_LEN        13
#define RK_PAL_COUNT      12
extern const rk_color_t RK_PAL[RK_PAL_COUNT][RK_PAL_LEN];

/* ------------------------------------------------------- adulto -- */
#define RK_ADULTO_W       96
#define RK_ADULTO_H       72
#define RK_AD_HEAD_CX     48
#define RK_AD_HEAD_CY     53
#define RK_AD_EYE_DX      11
#define RK_AD_EYE_DY      4
#define RK_AD_MOUTH_DY    13

extern const rk_shape_t rk_adulto_body;
extern const rk_shape_t rk_ad_eye_blink;
extern const rk_shape_t rk_ad_eye_dead;
extern const rk_shape_t rk_ad_eye_dizzy;
extern const rk_shape_t rk_ad_eye_glitch;
extern const rk_shape_t rk_ad_eye_happy;
extern const rk_shape_t rk_ad_eye_open;
extern const rk_shape_t rk_ad_eye_sleepy;
extern const rk_shape_t rk_ad_eye_wide;
extern const rk_shape_t rk_ad_mouth_flat;
extern const rk_shape_t rk_ad_mouth_frown;
extern const rk_shape_t rk_ad_mouth_open;
extern const rk_shape_t rk_ad_mouth_pant;
extern const rk_shape_t rk_ad_mouth_smile;
extern const rk_shape_t rk_ad_mouth_wavy;

/* -------------------------------------------------------- brote -- */
#define RK_BROTE_W        32
#define RK_BROTE_H        32
#define RK_BR_HEAD_CX     16
#define RK_BR_HEAD_CY     20
#define RK_BR_EYE_DX      5
#define RK_BR_EYE_DY      1
#define RK_BR_MOUTH_DY    5

/* Un cuerpo por simbionte: mismo esqueleto, copete y punteado propios. */
extern const rk_shape_t rk_brote_body[RK_PAL_COUNT];
extern const rk_shape_t rk_br_eye_blink;
extern const rk_shape_t rk_br_eye_dead;
extern const rk_shape_t rk_br_eye_dizzy;
extern const rk_shape_t rk_br_eye_glitch;
extern const rk_shape_t rk_br_eye_happy;
extern const rk_shape_t rk_br_eye_open;
extern const rk_shape_t rk_br_eye_sleepy;
extern const rk_shape_t rk_br_eye_wide;
extern const rk_shape_t rk_br_mouth_flat;
extern const rk_shape_t rk_br_mouth_frown;
extern const rk_shape_t rk_br_mouth_open;
extern const rk_shape_t rk_br_mouth_pant;
extern const rk_shape_t rk_br_mouth_smile;
extern const rk_shape_t rk_br_mouth_wavy;

#endif /* ROOTKIT_SPRITES_H */

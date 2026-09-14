/* Generado por tools/gen_art.py — no editar a mano. */
#ifndef ROOTKIT_TUGA_DATA_H
#define ROOTKIT_TUGA_DATA_H

#include "../gfx/fb.h"

/* Anclas de la cara, en coordenadas del sprite del cuerpo. */
#define RK_TUGA_W        96
#define RK_TUGA_H        72
#define RK_HEAD_CX       48
#define RK_HEAD_CY       53
#define RK_EYE_DX        11
#define RK_EYE_DY        4
#define RK_MOUTH_DY      13

extern const rk_color_t RK_TUGA_PAL[13];

extern const rk_sprite_t rk_tuga_body;
extern const rk_sprite_t rk_eye_blink;
extern const rk_sprite_t rk_eye_dead;
extern const rk_sprite_t rk_eye_dizzy;
extern const rk_sprite_t rk_eye_glitch;
extern const rk_sprite_t rk_eye_happy;
extern const rk_sprite_t rk_eye_open;
extern const rk_sprite_t rk_eye_sleepy;
extern const rk_sprite_t rk_eye_wide;
extern const rk_sprite_t rk_mouth_flat;
extern const rk_sprite_t rk_mouth_frown;
extern const rk_sprite_t rk_mouth_open;
extern const rk_sprite_t rk_mouth_pant;
extern const rk_sprite_t rk_mouth_smile;
extern const rk_sprite_t rk_mouth_wavy;

#endif /* ROOTKIT_TUGA_DATA_H */

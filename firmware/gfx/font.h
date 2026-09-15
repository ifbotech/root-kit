/* font.h — tipografía de mapa de bits 5x7, sólo mayúsculas.
 *
 * Mayúsculas a propósito: es la convención de las terminales de los ochenta
 * y de la interfaz de los juegos de 16 bits, y de paso reduce el set a algo
 * que se puede dibujar y revisar a mano.
 */
#ifndef ROOTKIT_FONT_H
#define ROOTKIT_FONT_H

#include "fb.h"

#define RK_GLYPH_W  5
#define RK_GLYPH_H  7
#define RK_TRACK    1   /* separación entre glifos, en pixeles lógicos */

/* Dibuja el texto y devuelve el ancho consumido. Las minúsculas se
 * convierten a mayúsculas; los caracteres desconocidos se saltean. */
int rk_text(rk_fb_t *fb, int x, int y, const char *s, rk_color_t c, int scale);

/* Ancho en pixeles que ocuparía el texto, sin dibujar nada. */
int rk_text_w(const char *s, int scale);

/* Centra el texto alrededor de cx. */
int rk_text_center(rk_fb_t *fb, int cx, int y, const char *s,
                   rk_color_t c, int scale);

/* Texto con sombra dura un pixel abajo a la derecha: sobre fondos con
 * gradiente mejora muchísimo la legibilidad y cuesta casi nada. */
int rk_text_shadow(rk_fb_t *fb, int x, int y, const char *s,
                   rk_color_t c, rk_color_t shadow, int scale);

/* ¿Está encendido el pixel (col, row) del glifo? Lo usa el framebuffer
 * monocromo del Spore, que no puede reutilizar rk_text porque escribe en un
 * formato de páginas distinto. Compartir los glifos evita mantener dos
 * tipografías que se desincronizan. */
bool rk_glyph_pixel(char ch, int col, int row);

#endif /* ROOTKIT_FONT_H */

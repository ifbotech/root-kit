/* mono.h — framebuffer de 1 bit para la OLED del Spore.
 *
 * SSD1306 de 128x32 por I2C, que es la pantalla que va en cada maceta. El
 * formato del buffer es el que el chip espera directo: páginas de 8 filas,
 * un byte por columna, el bit 0 arriba. Así el volcado es un memcpy y no una
 * conversión.
 *
 *   buf[pagina * 128 + x], bit (y & 7)
 *
 * Son 512 bytes. Cabe en cualquier lado, incluida la memoria RTC del ESP32-C3
 * si alguna vez conviene conservar la cara entre despertares.
 *
 * Por qué monocromo y no la TFT redonda a color: la OLED comparte el bus I2C
 * que el Spore ya tiene para el AHT21 y el BH1750 —cero pines nuevos— cuesta
 * la mitad, y sobre todo consume la sexta parte. Con la OLED el Spore
 * conserva 785 días de autonomía; con la TFT redonda cae a 436, y encendida
 * de forma permanente a dos.
 */
#ifndef ROOTKIT_MONO_H
#define ROOTKIT_MONO_H

#include <stdint.h>
#include <stdbool.h>

#define RK_OLED_W  128
#define RK_OLED_H   32
#define RK_OLED_BYTES (RK_OLED_W * RK_OLED_H / 8)

typedef struct {
    uint8_t *buf;     /* RK_OLED_BYTES, en formato de páginas del SSD1306 */
    int      w;
    int      h;
} rk_mono_t;

void rk_mono_init(rk_mono_t *m, uint8_t *buf, int w, int h);
void rk_mono_clear(rk_mono_t *m, bool encendido);
void rk_mono_px(rk_mono_t *m, int x, int y, bool on);
bool rk_mono_get(const rk_mono_t *m, int x, int y);

void rk_mono_hline(rk_mono_t *m, int x, int y, int w, bool on);
void rk_mono_vline(rk_mono_t *m, int x, int y, int h, bool on);
void rk_mono_rect(rk_mono_t *m, int x, int y, int w, int h, bool on);
void rk_mono_fill_rect(rk_mono_t *m, int x, int y, int w, int h, bool on);
/* Rectángulo con esquinas redondeadas: la forma base de un ojo. */
void rk_mono_fill_round(rk_mono_t *m, int x, int y, int w, int h, int r, bool on);
void rk_mono_disc(rk_mono_t *m, int cx, int cy, int r, bool on);
/* Arco superior o inferior de una elipse, de un pixel de grosor. Es lo que
 * dibuja un ojo feliz o un párpado cerrado. */
void rk_mono_arco(rk_mono_t *m, int cx, int cy, int rx, int ry,
                  bool arriba, bool on);

/* Texto con la misma tipografía 5x7 de la Terminal, para que el Spore y la
 * pantalla grande se vean del mismo producto. */
int  rk_mono_text(rk_mono_t *m, int x, int y, const char *s, bool on);

/* Cuántos pixeles están encendidos. En una OLED eso es corriente: se usa en
 * los tests para verificar que ninguna cara se pase del presupuesto. */
int  rk_mono_encendidos(const rk_mono_t *m);

#endif /* ROOTKIT_MONO_H */

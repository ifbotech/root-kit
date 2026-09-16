/* qr.h — la pantalla del QR. Lo primero que se ve al sacar la maceta de la
 * caja, y lo que vuelve a aparecer cada vez que se desvincula.
 *
 * QUÉ HAY EN PANTALLA
 *
 *   - El QR, lo más grande que entra, sobre una tarjeta blanca. Lleva a la
 *     app: https://.../V/<CÓDIGO>.
 *   - El código, debajo, en letras grandes y partido en dos grupos de
 *     cuatro. Es el plan B cuando la cámara no enfoca, y el plan A en
 *     iPhone cuando la app ya instalada no comparte almacenamiento con el
 *     Safari que leyó el QR: ahí se tipea.
 *   - Una rayita abajo que dice, sin palabras, en qué anda el aparato:
 *     ámbar quieta = esperando que le pasen el wifi; celeste que barre =
 *     conectándose; verde que late = en línea, esperando a la app.
 *
 * Nada más. Ni instrucciones, ni logo: la caja y la app explican; la
 * pantalla sólo tiene que ser escaneable desde el primer segundo.
 *
 * POR QUÉ SE CODIFICA UNA SOLA VEZ
 *
 * Armar el QR (Reed-Solomon, elegir máscara) cuesta del orden de un
 * milisegundo; dibujarlo, casi nada. La URL cambia sólo cuando cambia la
 * época, así que rk_qr_preparar() se llama una vez y cada cuadro sólo pinta.
 */
#ifndef ROOTKIT_QR_H
#define ROOTKIT_QR_H

#include <stdbool.h>
#include <stdint.h>
#include "../gfx/fb.h"

/* Versión máxima admitida: 6 son 41x41 módulos, que alcanzan para una URL
 * de ~120 caracteres alfanuméricos con corrección media. En el panel de
 * 128 una versión 6 da módulos de 2 pixeles, que es el límite legible. */
#define RK_QR_VERSION_MAX  6
#define RK_QR_BUF          ((((RK_QR_VERSION_MAX) * 4 + 17) * ((RK_QR_VERSION_MAX) * 4 + 17) + 7) / 8 + 1)

typedef enum {
    RK_QR_PORTAL = 0,     /* esperando que le pasen el wifi               */
    RK_QR_CONECTANDO,     /* probando la red guardada                     */
    RK_QR_EN_LINEA        /* reportándose, esperando que lo reclamen      */
} rk_qr_estado_t;

typedef struct {
    bool    ok;
    int     lado;              /* módulos por lado, sin zona de silencio  */
    uint8_t buf[RK_QR_BUF];
    char    codigo[10];
} rk_qr_t;

/* Codifica la URL. Devuelve false si no entra en la versión máxima; en ese
 * caso la pantalla muestra sólo el código, que igual alcanza para vincular. */
bool rk_qr_preparar(rk_qr_t *q, const char *url, const char *codigo);

/* ¿Está encendido el módulo (x, y)? Fuera de rango, apagado. */
bool rk_qr_modulo(const rk_qr_t *q, int x, int y);

/* Pinta la pantalla completa. */
void rk_qr_draw(rk_fb_t *fb, const rk_qr_t *q, rk_qr_estado_t estado, uint32_t t_ms);

/* Tamaño de módulo, en pixeles, que usa rk_qr_draw en ese panel. Expuesto
 * para los tests: por debajo de 2 un teléfono no lo lee. */
int rk_qr_escala(const rk_qr_t *q, int w, int h);

#define RK_QR_FONDO    RK_RGB( 22,  24,  30)
#define RK_QR_TARJETA  RK_RGB(255, 255, 255)
#define RK_QR_TINTA    RK_RGB( 16,  18,  24)

#endif /* ROOTKIT_QR_H */

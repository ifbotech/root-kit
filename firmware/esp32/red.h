/* red.h — wifi de la casa y el pedido a la nube, sin congelar la cara.
 *
 * Un pedido HTTPS tarda entre medio y dos segundos, casi todo esperando al
 * apretón de manos TLS. Si eso corriera en el mismo hilo que dibuja, la cara
 * se congelaría cada vez que la maceta habla con la nube. Así que el pedido
 * corre en una tarea aparte: el hilo principal deja el cuerpo, sigue
 * animando, y levanta la respuesta cuando está.
 */
#ifndef ROOTKIT_ESP32_RED_H
#define ROOTKIT_ESP32_RED_H

#include <Arduino.h>

void red_iniciar(void);

/* Wifi de la casa. No bloquea: conectar() arranca y estado() informa. */
typedef enum { RK_WIFI_APAGADO = 0, RK_WIFI_CONECTANDO, RK_WIFI_CONECTADO, RK_WIFI_FALLO } rk_wifi_t;
void      red_wifi_conectar(const char *ssid, const char *clave);
void      red_wifi_olvidar(void);
rk_wifi_t red_wifi_estado(void);
int16_t   red_rssi(void);

/* Encola un pedido. Devuelve false si ya hay uno en curso. */
bool red_pedir(const char *url, const char *token, const char *cuerpo, size_t len);

/* ¿Terminó el pedido? Si sí, deja el código HTTP (negativo si no llegó) y el
 * cuerpo de la respuesta, y vuelve a quedar libre. */
bool red_respuesta(int *codigo, char *cuerpo, size_t cap);

bool red_ocupada(void);

#endif

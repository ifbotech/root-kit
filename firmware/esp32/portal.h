/* portal.h — la red ROOTKIT-XXXX y su página para elegir el wifi.
 *
 * POR QUÉ UN PORTAL CAUTIVO Y NO BLUETOOTH
 *
 * La app es web. Web Bluetooth no existe en iPhone, y una app nativa sólo
 * para pasar la clave del wifi es exactamente la fricción que el QR evita.
 * Tampoco la app puede hablarle a la maceta por wifi directo: una página
 * HTTPS no puede pedirle nada a http://192.168.4.1.
 *
 * El portal cautivo anda en cualquier teléfono sin instalar nada: te
 * conectás a ROOTKIT-XXXX, el sistema abre solo la página (porque todo
 * pedido DNS contesta con la IP del aparato), elegís tu red, escribís la
 * clave. La app guía cada paso con capturas.
 *
 * La página es HTML plano embebido, sin recursos externos: el teléfono no
 * tiene internet mientras está conectado a la maceta.
 */
#ifndef ROOTKIT_ESP32_PORTAL_H
#define ROOTKIT_ESP32_PORTAL_H

#include <Arduino.h>

typedef struct {
    bool recibido;          /* llegó un formulario nuevo                 */
    char ssid[33];
    char clave[65];
    char nube[96];          /* vacío = no se tocó                        */
} rk_portal_datos_t;

void portal_iniciar(const char *ssid_ap, const char *codigo);
void portal_detener(void);
bool portal_activo(void);

/* Atiende DNS y HTTP. Devuelve true si llegó una red nueva, en `datos`. */
bool portal_atender(rk_portal_datos_t *datos);

/* Lo que la página muestra mientras el aparato prueba la red. */
typedef enum { RK_PORTAL_ESPERANDO = 0, RK_PORTAL_PROBANDO, RK_PORTAL_OK, RK_PORTAL_FALLO } rk_portal_estado_t;
void portal_informar(rk_portal_estado_t e);

#endif

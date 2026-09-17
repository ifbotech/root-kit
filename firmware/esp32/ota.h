/* ota.h — bajar, verificar e instalar una versión nueva, sin congelar la cara.
 *
 * Lo que decide (si conviene, cuántas veces, qué hacer al arrancar) está en
 * core/ota.c y se prueba en el escritorio. Acá está lo que toca hardware:
 *
 *   1. baja el binario por HTTPS con el token del aparato, en una tarea
 *      aparte (el bucle principal sigue dibujando);
 *   2. lo va escribiendo en la partición inactiva mientras calcula su SHA-256;
 *   3. compara el hash con el del manifiesto y verifica la FIRMA ECDSA P-256
 *      con la clave pública de ota_clave.h (mbedTLS);
 *   4. sólo si las dos cosas dan bien cierra la partición y queda LISTA: el
 *      bucle principal guarda lo que haga falta y reinicia.
 *
 * Al arrancar la versión nueva, el gestor de arranque la deja "pendiente de
 * verificar": ota_confirmar() la da por buena cuando la nube contestó, y
 * ota_volver_atras() la descarta si pasó el plazo sin lograrlo.
 */
#ifndef ROOTKIT_ESP32_OTA_H
#define ROOTKIT_ESP32_OTA_H

#include <Arduino.h>
extern "C" {
#include "../core/ota.h"
}

typedef enum {
    RK_OTA_OCIOSA = 0,
    RK_OTA_BAJANDO,
    RK_OTA_LISTA,        /* instalada: falta reiniciar */
    RK_OTA_FALLO
} rk_ota_fase_t;

void          ota_iniciar_tarea(void);
/* Arranca la descarga. false si ya hay una en curso. */
bool          ota_empezar(const rk_ota_manifiesto_t *m, const char *token);
rk_ota_fase_t ota_fase(void);
uint8_t       ota_porcentaje(void);
const char   *ota_motivo(void);          /* por qué falló */
bool          ota_activa(void);
void          ota_olvidar_fallo(void);   /* FALLO -> OCIOSA, ya se contó */

/* ¿Esta imagen arrancó recién instalada y todavía no está confirmada? */
bool ota_pendiente_de_verificar(void);
void ota_confirmar(void);
void ota_volver_atras(void);             /* no vuelve: reinicia */

#endif

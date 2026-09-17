/* fabrica.h — escucha el puerto serie por si habla la estación de fábrica.
 *
 * El protocolo (una línea `FABRICA {...}` y una respuesta JSON) está en
 * core/fabrica.h y se prueba en el escritorio. Acá sólo se juntan los bytes
 * del puerto serie en líneas y se graba la NVS.
 */
#ifndef ROOTKIT_ESP32_FABRICA_H
#define ROOTKIT_ESP32_FABRICA_H

#include "almacen.h"

/* Llamar en cada vuelta del bucle. Devuelve true si cambió la identidad
 * (secreto, persona o lote): hay que recalcular token, código y QR. */
bool fabrica_atender(rk_almacen_t *a, bool vinculado, const char *id, const char *codigo, const char *fw);

#endif

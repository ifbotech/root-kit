/* fabrica.h — la estación de fábrica habla con el aparato por el puerto serie.
 *
 * Cada ROOTKIT sale de la caja con dos cosas grabadas: su SECRETO (de donde
 * salen el token y el código del QR) y su Rooti (la figura). Las graba la
 * estación de fábrica (tools/fabrica.py) mandando una línea por el puerto
 * serie del aparato recién flasheado:
 *
 *   FABRICA {"secreto":"<32 hex>","persona":"brote","lote":"L2609"}
 *
 * y el aparato contesta una línea JSON con su id (la MAC) y el código, que
 * la estación registra en la nube y imprime en la etiqueta. `FABRICA?`
 * sólo pregunta, sin cambiar nada (para reimprimir una etiqueta).
 *
 * Sólo se acepta con el aparato SIN VINCULAR: una maceta que ya es de
 * alguien no cambia de identidad por el cable. Quien tiene el cable y la
 * placa en la mano tiene todo de todas formas; esta regla evita el
 * accidente, no al atacante.
 */
#ifndef ROOTKIT_FABRICA_H
#define ROOTKIT_FABRICA_H

#include <stdbool.h>
#include <stddef.h>
#include "codigo.h"

#define RK_LOTE_LEN 12

typedef struct {
    bool    consulta;                /* FABRICA? : sólo preguntar           */
    uint8_t secreto[RK_SECRETO_LEN];
    char    persona[16];
    char    lote[RK_LOTE_LEN];
} rk_fabrica_orden_t;

/* Interpreta una línea del puerto serie. Devuelve true si es una orden de
 * fábrica válida (con persona conocida y secreto válido, o una consulta). */
bool rk_fabrica_parsear(const char *linea, rk_fabrica_orden_t *o);

/* ¿Empieza como una orden de fábrica? (para no intentar parsear el log). */
bool rk_fabrica_es_orden(const char *linea);

typedef struct {
    const char *id;
    const char *persona;
    const char *lote;
    const char *codigo;
    const char *fw;
    bool        vinculado;
} rk_fabrica_estado_t;

/* La respuesta que se manda por serie (sin salto de línea). */
size_t rk_fabrica_respuesta(char *buf, size_t cap, const rk_fabrica_estado_t *e);
size_t rk_fabrica_error(char *buf, size_t cap, const char *motivo);

#endif /* ROOTKIT_FABRICA_H */

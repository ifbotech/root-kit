/* codigo.h — la identidad del aparato: id, código de vinculación, token.
 *
 * TRES VALORES, UN SECRETO
 *
 * Cada ROOTKIT sale de fábrica con 16 bytes aleatorios grabados en NVS. De
 * ese secreto salen:
 *
 *   id       12 hex, derivado de la MAC. Público: identifica, no autoriza.
 *   código   8 caracteres, el que muestra el QR. Prueba que tenés la maceta
 *            adelante: sólo se ve en la pantalla, y sólo mientras está sin
 *            vincular.
 *   token    64 hex, con el que el aparato firma cada pedido a la nube.
 *
 * POR QUÉ EL CÓDIGO CAMBIA CON CADA VINCULACIÓN
 *
 * El código depende de una ÉPOCA que sube cada vez que el aparato se
 * desvincula. Si fuera fijo, el dueño anterior de una maceta regalada podría
 * volver a vincularla con una foto vieja del QR. Con la época, el QR viejo
 * deja de valer en el mismo momento en que se desvincula.
 *
 * EL ALFABETO
 *
 * Base32 de Crockford: dígitos y mayúsculas sin I, L, O ni U. Dos razones:
 * se puede tipear sin confundir 0 con O ni 1 con I (el código también se
 * escribe a mano, cuando la cámara no coopera), y todos sus caracteres caben
 * en el modo alfanumérico del QR, que ocupa bastante menos que el binario:
 * en una pantalla de 128 pixeles eso es la diferencia entre módulos de 3 y
 * de 2 pixeles.
 */
#ifndef ROOTKIT_CODIGO_H
#define ROOTKIT_CODIGO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RK_SECRETO_LEN   16u
#define RK_CODIGO_LEN     8u   /* sin contar el terminador */
#define RK_ID_LEN        12u
#define RK_TOKEN_LEN     64u

/* "A1B2C3D4E5F6" a partir de la MAC. `out` tiene lugar para 13. */
void rk_id_desde_mac(const uint8_t mac[6], char out[RK_ID_LEN + 1]);

/* Código de vinculación de la época. `out` tiene lugar para 9. */
void rk_codigo_vinculo(const uint8_t secreto[RK_SECRETO_LEN], uint32_t epoca,
                       char out[RK_CODIGO_LEN + 1]);

/* Token de la API, en hex. `out` tiene lugar para 65. */
void rk_token_api(const uint8_t secreto[RK_SECRETO_LEN], char out[RK_TOKEN_LEN + 1]);

/* Normaliza lo que tipeó una persona: mayúsculas, sin guiones ni espacios,
 * O->0, I y L->1, U->V. Devuelve false si no quedan exactamente 8
 * caracteres válidos. Es la misma regla que aplica el servidor. */
bool rk_codigo_normalizar(const char *entrada, char out[RK_CODIGO_LEN + 1]);

/* Nombre de la red del portal: "ROOTKIT-" y los cuatro primeros caracteres
 * del código. La app lo deduce del código sin preguntarle nada a nadie. */
void rk_codigo_ssid(const char *codigo, char *out, size_t n);

/* La URL del QR: base + "/V/" + código, toda en mayúsculas para que entre en
 * el modo alfanumérico. Devuelve false si no cabe o si la base tiene
 * caracteres que ese modo no admite. Las rutas se leen igual en mayúscula:
 * el servidor atiende /V/ y /v/. */
bool rk_codigo_url(const char *base, const char *codigo, char *out, size_t n);

/* Un secreto de fábrica válido no es todo ceros ni todo 0xFF (NVS borrado). */
bool rk_secreto_valido(const uint8_t secreto[RK_SECRETO_LEN]);

#endif /* ROOTKIT_CODIGO_H */

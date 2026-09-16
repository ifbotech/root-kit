/* sha256.h — SHA-256 y HMAC-SHA256, portables.
 *
 * Para qué los necesita un macetero: el código de vinculación que muestra el
 * QR y el token con el que el aparato se presenta ante la nube salen de un
 * secreto grabado en fábrica. Tienen que ser imposibles de adivinar a partir
 * del número de serie (si no, cualquiera vincula la maceta de otro) y tienen
 * que calcularse igual en la placa, en el simulador y en los tests.
 *
 * En el ESP32 existe mbedTLS, pero este archivo mantiene el núcleo portable
 * y testeable en el escritorio con los vectores del estándar. Son 150 líneas
 * y se ejecutan un puñado de veces por arranque: no hay nada que optimizar.
 */
#ifndef ROOTKIT_SHA256_H
#define ROOTKIT_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define RK_SHA256_LEN 32u

typedef struct {
    uint32_t h[8];
    uint64_t bits;
    uint8_t  buf[64];
    size_t   n;
} rk_sha256_t;

void rk_sha256_init(rk_sha256_t *s);
void rk_sha256_update(rk_sha256_t *s, const uint8_t *data, size_t len);
void rk_sha256_final(rk_sha256_t *s, uint8_t out[RK_SHA256_LEN]);

void rk_sha256(const uint8_t *data, size_t len, uint8_t out[RK_SHA256_LEN]);
void rk_hmac_sha256(const uint8_t *key, size_t key_len,
                    const uint8_t *msg, size_t msg_len,
                    uint8_t out[RK_SHA256_LEN]);

#endif /* ROOTKIT_SHA256_H */

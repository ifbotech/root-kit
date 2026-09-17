/* ota.h — actualizarse por aire sin que nadie tenga que venir con un cable.
 *
 * CÓMO LLEGA UNA VERSIÓN NUEVA
 *
 * La nube conoce la versión de cada aparato (va en cada sync) y su canal
 * (estable o beta). Cuando hay una más nueva para ese canal y esa placa, la
 * respuesta del sync trae un manifiesto:
 *
 *   "firmware": { "version": "0.6.0", "url": "https://.../api/d/firmware/...",
 *                 "sha256": "<64 hex>", "firma": "<base64>", "tamano": 1234567 }
 *
 * El aparato decide si lo aplica (rk_ota_decidir), lo baja en la tarea de
 * red mientras la cara sigue animándose, calcula el SHA-256 de lo que bajó y
 * lo compara con el del manifiesto, verifica la FIRMA (ECDSA P-256 sobre ese
 * hash, con la clave pública que viene en esp32/ota_clave.h) y recién ahí
 * escribe la partición inactiva y reinicia. Un binario que no firmó quien
 * tiene la clave privada no se instala aunque venga por HTTPS.
 *
 * SI LA VERSIÓN NUEVA NO ANDA
 *
 * El gestor de arranque del ESP32 deja la versión nueva en "pendiente de
 * verificar". Si logra hablar con la nube en RK_OTA_VERIFICAR_MS, se confirma;
 * si no, vuelve sola a la anterior. Y cada versión se intenta como mucho
 * RK_OTA_INTENTOS_MAX veces: un binario que siempre falla no deja al aparato
 * bajando y reiniciando para siempre.
 *
 * Todo lo que decide está acá, en C99 puro, y se prueba en el escritorio. Lo
 * que toca la red, la flash y mbedTLS está en esp32/ota.cpp.
 */
#ifndef ROOTKIT_OTA_H
#define ROOTKIT_OTA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RK_OTA_VERSION_LEN   16
#define RK_OTA_URL_LEN       160
#define RK_OTA_FIRMA_MAX     80          /* DER de ECDSA P-256: hasta 72 bytes */
#define RK_OTA_TAMANO_MAX    1900000u    /* lo que entra en una partición      */
#define RK_OTA_INTENTOS_MAX  3
#define RK_OTA_BAT_MIN_MV    3700        /* a batería, sólo con carga de sobra */
#define RK_OTA_VERIFICAR_MS  (10u * 60u * 1000u)

typedef struct {
    char     version[RK_OTA_VERSION_LEN];
    char     url[RK_OTA_URL_LEN];
    uint8_t  sha256[32];
    uint8_t  firma[RK_OTA_FIRMA_MAX];
    uint8_t  firma_len;
    uint32_t tamano;
} rk_ota_manifiesto_t;

/* Lo que sobrevive al reinicio (NVS). */
typedef struct {
    char    version[RK_OTA_VERSION_LEN];  /* la última que se intentó          */
    uint8_t intentos;                     /* cuántas veces esa misma versión   */
    bool    verificar;                    /* arrancó nueva y no habló con la nube */
} rk_ota_nvs_t;

typedef enum {
    RK_OTA_NO_HAY = 0,       /* la nube no ofreció nada                  */
    RK_OTA_MISMA_VERSION,    /* ya corre esa                             */
    RK_OTA_INVALIDA,         /* manifiesto incompleto o absurdo          */
    RK_OTA_AGOTADA,          /* esa versión ya falló RK_OTA_INTENTOS_MAX */
    RK_OTA_SIN_BATERIA,      /* a batería y baja: mejor otro día         */
    RK_OTA_ADELANTE
} rk_ota_decision_t;

/* "0.5.0" -> válida. Tres números separados por puntos, opcionalmente
 * seguidos de un sufijo con guion ("0.6.0-beta.2"). */
bool rk_version_valida(const char *v);
/* Compara dos versiones válidas: <0, 0, >0. El sufijo no cuenta. */
int  rk_version_cmp(const char *a, const char *b);

/* Exactamente 2*n caracteres hexadecimales. */
bool   rk_hex_decodificar(const char *hex, uint8_t *out, size_t n);
/* Devuelve cuántos bytes salieron; 0 si el texto no es base64 o no entra. */
size_t rk_base64_decodificar(const char *b64, uint8_t *out, size_t cap);

/* Lee el objeto "firmware" de la respuesta del sync. false si no está o no
 * sirve (y entonces el manifiesto queda en cero). */
bool rk_ota_manifiesto_parsear(const char *json, rk_ota_manifiesto_t *m);

rk_ota_decision_t rk_ota_decidir(const rk_ota_nvs_t *nvs, const rk_ota_manifiesto_t *m,
                                 const char *version_actual, bool usb, uint16_t bat_mv);
const char *rk_ota_decision_nombre(rk_ota_decision_t d);

/* Antes de bajar: anota la versión y suma un intento. */
void rk_ota_marcar_intento(rk_ota_nvs_t *nvs, const char *version);
/* Al arrancar. Devuelve true si la versión que corre es la que se acaba de
 * instalar y todavía hay que confirmarla con la nube. Si corre otra (el
 * gestor de arranque ya volvió atrás), limpia la bandera. */
bool rk_ota_arranque(rk_ota_nvs_t *nvs, const char *version_actual);
/* La nube contestó bien con la versión nueva: queda confirmada. */
void rk_ota_confirmar(rk_ota_nvs_t *nvs);

#endif /* ROOTKIT_OTA_H */

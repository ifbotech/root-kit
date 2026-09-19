#include "ota.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/pk.h>
#include "certificados.h"
#include "ota_clave.h"
#include "placa.h"

extern "C" {
#include "../net/nube.h"
#include "../core/sha256.h"
}

#define OTA_PILA          12288      /* TLS + verificación ECDSA            */
#define OTA_TROZO          1024
#define OTA_SILENCIO_MS   15000      /* sin un byte en este tiempo: se corta */

static TaskHandle_t        g_tarea;
static rk_ota_manifiesto_t g_m;
static char                g_token[80];
static volatile rk_ota_fase_t g_fase = RK_OTA_OCIOSA;
static volatile uint8_t    g_pct;
static char                g_motivo[32];

static void fallo(const char *motivo)
{
    strncpy(g_motivo, motivo, sizeof g_motivo - 1);
    g_motivo[sizeof g_motivo - 1] = '\0';
    Update.abort();
    g_fase = RK_OTA_FALLO;
    Serial.printf("[ota] fallo: %s\n", g_motivo);
}

/* ECDSA P-256 sobre el SHA-256 del binario, con la pública compilada. */
static bool firma_valida(const uint8_t hash[32], const uint8_t *firma, size_t n)
{
    mbedtls_pk_context pk;
    int rc;

    mbedtls_pk_init(&pk);
    rc = mbedtls_pk_parse_public_key(&pk, (const unsigned char *)RK_OTA_CLAVE_PUBLICA,
                                     strlen(RK_OTA_CLAVE_PUBLICA) + 1);
    if (rc == 0) {
        rc = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 32, firma, n);
    }
    mbedtls_pk_free(&pk);
    return rc == 0;
}

static void bajar(void)
{
    HTTPClient http;
    WiFiClient plano;
    WiFiClientSecure seguro;
    rk_sha256_t sha;
    uint8_t hash[32];
    static uint8_t trozo[OTA_TROZO];
    uint32_t leidos = 0u, ultimo_byte;
    bool ok;

    g_pct = 0u;
    http.setTimeout(15000);
    http.setReuse(false);
    if (strncmp(g_m.url, "https://", 8) == 0) {
#if defined(RK_NUBE_INSEGURO) && RK_NUBE_INSEGURO
        seguro.setInsecure();               /* sólo desarrollo: ver red.cpp */
#elif defined(RK_NUBE_CA)
        seguro.setCACert(RK_NUBE_CA);
#else
        seguro.setCACert(RK_CA_RAICES);
#endif
        ok = http.begin(seguro, g_m.url);
    } else {
        ok = http.begin(plano, g_m.url);
    }
    if (!ok) {
        fallo("url");
        return;
    }
    http.addHeader("Authorization", String("Bearer ") + g_token);
    int codigo = http.GET();
    if (codigo != 200) {
        http.end();
        fallo(codigo == 401 || codigo == 403 ? "no autorizado" : "descarga");
        return;
    }
    if (http.getSize() != (int)g_m.tamano) {
        http.end();
        fallo("tamano");
        return;
    }
    if (!Update.begin(g_m.tamano, U_FLASH)) {
        http.end();
        fallo("sin lugar");
        return;
    }

    rk_sha256_init(&sha);
    WiFiClient *flujo = http.getStreamPtr();
    ultimo_byte = millis();
    while (leidos < g_m.tamano) {
        size_t hay = flujo->available();
        if (hay > 0u) {
            size_t pedir = hay < sizeof trozo ? hay : sizeof trozo;
            if (pedir > g_m.tamano - leidos) {
                pedir = g_m.tamano - leidos;
            }
            int n = flujo->readBytes(trozo, pedir);
            if (n <= 0) {
                break;
            }
            if (Update.write(trozo, (size_t)n) != (size_t)n) {
                http.end();
                fallo("flash");
                return;
            }
            rk_sha256_update(&sha, trozo, (size_t)n);
            leidos += (uint32_t)n;
            g_pct = (uint8_t)((uint64_t)leidos * 100u / g_m.tamano);
            ultimo_byte = millis();
        } else {
            if (!http.connected() || millis() - ultimo_byte > OTA_SILENCIO_MS) {
                break;
            }
            delay(2);
        }
    }
    http.end();
    if (leidos != g_m.tamano) {
        fallo("cortada");
        return;
    }

    rk_sha256_final(&sha, hash);
    if (memcmp(hash, g_m.sha256, sizeof hash) != 0) {
        fallo("hash");
        return;
    }
    if (!firma_valida(hash, g_m.firma, g_m.firma_len)) {
        fallo("firma");
        return;
    }
    if (!Update.end(true) || !Update.isFinished()) {
        fallo("cierre");
        return;
    }
    g_pct = 100u;
    g_fase = RK_OTA_LISTA;
    Serial.printf("[ota] %s instalada: reiniciando\n", g_m.version);
}

static void tarea_ota(void *)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        bajar();
    }
}

void ota_iniciar_tarea(void)
{
    xTaskCreate(tarea_ota, "ota", OTA_PILA, nullptr, 1, &g_tarea);
}

bool ota_empezar(const rk_ota_manifiesto_t *m, const char *token)
{
    if (m == NULL || token == NULL || g_tarea == NULL || g_fase == RK_OTA_BAJANDO || g_fase == RK_OTA_LISTA) {
        return false;
    }
    /* La URL del binario la manda el servidor: igual que en el sync, el token
     * no sale por HTTP plano. */
    if (!rk_nube_url_aceptable(m->url, RK_ES_BANCO)) {
        Serial.println("[ota] la URL del firmware no es https: no la bajo");
        return false;
    }
    g_m = *m;
    strncpy(g_token, token, sizeof g_token - 1);
    g_token[sizeof g_token - 1] = '\0';
    g_motivo[0] = '\0';
    g_fase = RK_OTA_BAJANDO;
    Serial.printf("[ota] bajando %s (%lu bytes)\n", g_m.version, (unsigned long)g_m.tamano);
    xTaskNotifyGive(g_tarea);
    return true;
}

rk_ota_fase_t ota_fase(void)       { return g_fase; }
uint8_t       ota_porcentaje(void) { return g_pct; }
const char   *ota_motivo(void)     { return g_motivo; }
bool          ota_activa(void)     { return g_fase == RK_OTA_BAJANDO || g_fase == RK_OTA_LISTA; }

void ota_olvidar_fallo(void)
{
    if (g_fase == RK_OTA_FALLO) {
        g_fase = RK_OTA_OCIOSA;
    }
}

bool ota_pendiente_de_verificar(void)
{
    esp_ota_img_states_t estado;
    const esp_partition_t *corre = esp_ota_get_running_partition();
    return corre != NULL && esp_ota_get_state_partition(corre, &estado) == ESP_OK &&
           estado == ESP_OTA_IMG_PENDING_VERIFY;
}

void ota_confirmar(void)
{
    esp_ota_mark_app_valid_cancel_rollback();
}

void ota_volver_atras(void)
{
    Serial.println("[ota] la version nueva no logro hablar con la nube: vuelvo a la anterior");
    esp_ota_mark_app_invalid_rollback_and_reboot();
    /* Si no había a dónde volver, sigue con ésta. */
}

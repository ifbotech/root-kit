#include "red.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include "placa.h"

extern "C" {
#include "../net/nube.h"
}

#define CUERPO_MAX     4096
#define RESPUESTA_MAX  2048
#define WIFI_PLAZO_MS  15000

/* ------------------------------------------------------------------ wifi -- */
static rk_wifi_t g_wifi = RK_WIFI_APAGADO;
static uint32_t  g_wifi_desde;

void red_wifi_conectar(const char *ssid, const char *clave)
{
    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_STA);
    }
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, clave);
    g_wifi = RK_WIFI_CONECTANDO;
    g_wifi_desde = millis();
}

void red_wifi_olvidar(void)
{
    WiFi.disconnect(true, true);
    g_wifi = RK_WIFI_APAGADO;
}

rk_wifi_t red_wifi_estado(void)
{
    wl_status_t s = WiFi.status();

    if (s == WL_CONNECTED) {
        g_wifi = RK_WIFI_CONECTADO;
    } else if (g_wifi == RK_WIFI_CONECTADO) {
        /* Se cayó: el reintento lo hace el propio stack (autoReconnect). */
        g_wifi = RK_WIFI_CONECTANDO;
        g_wifi_desde = millis();
    } else if (g_wifi == RK_WIFI_CONECTANDO &&
               (s == WL_CONNECT_FAILED || s == WL_NO_SSID_AVAIL ||
                millis() - g_wifi_desde > WIFI_PLAZO_MS)) {
        g_wifi = RK_WIFI_FALLO;
    }
    return g_wifi;
}

int16_t red_rssi(void)
{
    return WiFi.status() == WL_CONNECTED ? (int16_t)WiFi.RSSI() : 0;
}

/* ------------------------------------------------------------------ nube -- */
static TaskHandle_t      g_tarea;
static SemaphoreHandle_t g_mutex;
static volatile bool     g_en_curso = false;
static volatile bool     g_lista = false;
static char g_url[160];
static char g_token[80];
static char g_cuerpo[CUERPO_MAX];
static size_t g_len;
static char g_resp[RESPUESTA_MAX];
static int  g_codigo;

static void tarea_nube(void *)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int codigo = -1;
        String cuerpo;
        {
            HTTPClient http;
            WiFiClient plano;
            WiFiClientSecure seguro;
            bool ok;

            http.setTimeout(8000);
            http.setReuse(false);
            if (strncmp(g_url, "https://", 8) == 0) {
#if defined(RK_NUBE_INSEGURO) && RK_NUBE_INSEGURO
                /* SÓLO DESARROLLO: un túnel o un servidor con certificado
                 * propio. Cifra pero no autentica: cualquiera en el camino
                 * podría hacerse pasar por la nube. Nunca en una placa que
                 * sale del banco. */
                seguro.setInsecure();
#elif defined(RK_NUBE_CA)
                /* Una CA propia, para un servidor que no use las públicas. */
                seguro.setCACert(RK_NUBE_CA);
#else
                /* Lo normal: sólo las raíces de Let's Encrypt y ZeroSSL, las
                 * que usa el Caddy del servidor (esp32/certificados.h). */
                seguro.setCACert(RK_CA_RAICES);
#endif
                ok = http.begin(seguro, g_url);
            } else {
                ok = http.begin(plano, g_url);
            }
            if (ok) {
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Authorization", String("Bearer ") + g_token);
                codigo = http.POST((uint8_t *)g_cuerpo, g_len);
                if (codigo > 0) {
                    cuerpo = http.getString();
                }
                http.end();
            }
        }

        xSemaphoreTake(g_mutex, portMAX_DELAY);
        g_codigo = codigo;
        strncpy(g_resp, cuerpo.c_str(), sizeof g_resp - 1);
        g_resp[sizeof g_resp - 1] = '\0';
        g_lista = true;
        g_en_curso = false;
        xSemaphoreGive(g_mutex);
    }
}

void red_iniciar(void)
{
    WiFi.persistent(false);             /* la clave vive en nuestro NVS */
    WiFi.mode(WIFI_STA);
    g_mutex = xSemaphoreCreateMutex();
    /* 8 KB de pila: TLS con mbedTLS necesita más que las 4 KB por defecto. */
    xTaskCreate(tarea_nube, "nube", 8192, nullptr, 1, &g_tarea);
}

bool red_pedir(const char *url, const char *token, const char *cuerpo, size_t len)
{
    if (g_en_curso || g_lista || len >= sizeof g_cuerpo) {
        return false;
    }
    /* El token no sale por HTTP plano ni a una URL rara: con él cualquiera
     * se hace pasar por esta maceta (net/nube.h). */
    if (!rk_nube_url_aceptable(url, RK_ES_BANCO)) {
        Serial.printf("[red] no mando el token a %s: tiene que ser https\n", url);
        return false;
    }
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    strncpy(g_url, url, sizeof g_url - 1);
    strncpy(g_token, token, sizeof g_token - 1);
    memcpy(g_cuerpo, cuerpo, len);
    g_len = len;
    g_en_curso = true;
    xSemaphoreGive(g_mutex);
    xTaskNotifyGive(g_tarea);
    return true;
}

bool red_respuesta(int *codigo, char *cuerpo, size_t cap)
{
    bool hay = false;
    if (!g_lista) {
        return false;
    }
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    if (g_lista) {
        *codigo = g_codigo;
        strncpy(cuerpo, g_resp, cap - 1);
        cuerpo[cap - 1] = '\0';
        g_lista = false;
        hay = true;
    }
    xSemaphoreGive(g_mutex);
    return hay;
}

bool red_ocupada(void)
{
    return g_en_curso || g_lista;
}

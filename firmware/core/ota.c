#include "ota.h"
#include "../net/json.h"
#include <string.h>

/* ------------------------------------------------------------ versiones -- */
static bool numero(const char **p, long *out)
{
    long v = 0;
    int n = 0;
    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (**p - '0');
        (*p)++;
        if (++n > 5) {
            return false;
        }
    }
    *out = v;
    return n > 0;
}

static bool partes(const char *v, long out[3])
{
    const char *p = v;
    int i;
    if (v == NULL) {
        return false;
    }
    for (i = 0; i < 3; i++) {
        if (!numero(&p, &out[i])) {
            return false;
        }
        if (i < 2) {
            if (*p != '.') {
                return false;
            }
            p++;
        }
    }
    if (*p == '-') {
        p++;
        if (*p == '\0') {
            return false;
        }
        while (*p != '\0') {
            char c = *p;
            bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                      (c >= 'A' && c <= 'Z') || c == '.' || c == '-';
            if (!ok) {
                return false;
            }
            p++;
        }
    }
    return *p == '\0' && strlen(v) < RK_OTA_VERSION_LEN;
}

bool rk_version_valida(const char *v)
{
    long n[3];
    return partes(v, n);
}

int rk_version_cmp(const char *a, const char *b)
{
    long x[3], y[3];
    int i;
    if (!partes(a, x)) {
        x[0] = x[1] = x[2] = 0;
    }
    if (!partes(b, y)) {
        y[0] = y[1] = y[2] = 0;
    }
    for (i = 0; i < 3; i++) {
        if (x[i] != y[i]) {
            return x[i] < y[i] ? -1 : 1;
        }
    }
    return 0;
}

/* --------------------------------------------------------- codificaciones -- */
static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool rk_hex_decodificar(const char *hex, uint8_t *out, size_t n)
{
    size_t i;
    if (hex == NULL || out == NULL) {
        return false;
    }
    for (i = 0; i < n; i++) {
        int hi = hexval(hex[2 * i]);
        int lo = hi < 0 ? -1 : hexval(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return hex[2 * n] == '\0';
}

static int b64val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t rk_base64_decodificar(const char *b64, uint8_t *out, size_t cap)
{
    size_t n = 0;
    uint32_t acc = 0;
    int bits = 0;
    const char *p;

    if (b64 == NULL || out == NULL) {
        return 0;
    }
    for (p = b64; *p != '\0' && *p != '='; p++) {
        int v = b64val(*p);
        if (v < 0) {
            return 0;
        }
        acc = (acc << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n >= cap) {
                return 0;
            }
            out[n++] = (uint8_t)((acc >> bits) & 0xFFu);
        }
    }
    /* Relleno: como mucho dos '=' y nada después. */
    {
        int iguales = 0;
        while (*p == '=') {
            iguales++;
            p++;
        }
        if (iguales > 2 || *p != '\0') {
            return 0;
        }
    }
    return n;
}

/* ------------------------------------------------------------ manifiesto -- */
bool rk_ota_manifiesto_parsear(const char *json, rk_ota_manifiesto_t *m)
{
    char hex[65];
    char b64[128];
    long v;
    size_t n;

    if (m == NULL) {
        return false;
    }
    memset(m, 0, sizeof *m);
    if (json == NULL || !rk_json_hay(json, "firmware")) {
        return false;
    }
    if (!rk_json_texto(json, "firmware.version", m->version, sizeof m->version) ||
        !rk_version_valida(m->version)) {
        goto mal;
    }
    if (!rk_json_texto(json, "firmware.url", m->url, sizeof m->url) ||
        (strncmp(m->url, "https://", 8) != 0 && strncmp(m->url, "http://", 7) != 0)) {
        goto mal;
    }
    if (!rk_json_texto(json, "firmware.sha256", hex, sizeof hex) ||
        !rk_hex_decodificar(hex, m->sha256, 32)) {
        goto mal;
    }
    if (!rk_json_texto(json, "firmware.firma", b64, sizeof b64)) {
        goto mal;
    }
    n = rk_base64_decodificar(b64, m->firma, sizeof m->firma);
    /* Una firma DER de P-256 tiene entre 70 y 72 bytes; se acepta un margen
     * por si el codificador la escribe corta, pero nunca algo diminuto. */
    if (n < 64 || n > RK_OTA_FIRMA_MAX) {
        goto mal;
    }
    m->firma_len = (uint8_t)n;
    if (!rk_json_entero(json, "firmware.tamano", &v) || v <= 0 || v > (long)RK_OTA_TAMANO_MAX) {
        goto mal;
    }
    m->tamano = (uint32_t)v;
    return true;
mal:
    memset(m, 0, sizeof *m);
    return false;
}

/* -------------------------------------------------------------- decidir -- */
rk_ota_decision_t rk_ota_decidir(const rk_ota_nvs_t *nvs, const rk_ota_manifiesto_t *m,
                                 const char *version_actual, bool usb, uint16_t bat_mv)
{
    if (m == NULL || m->version[0] == '\0') {
        return RK_OTA_NO_HAY;
    }
    if (!rk_version_valida(m->version) || m->tamano == 0u || m->firma_len == 0u || m->url[0] == '\0') {
        return RK_OTA_INVALIDA;
    }
    if (version_actual != NULL && strcmp(m->version, version_actual) == 0) {
        return RK_OTA_MISMA_VERSION;
    }
    if (nvs != NULL && strcmp(nvs->version, m->version) == 0 && nvs->intentos >= RK_OTA_INTENTOS_MAX) {
        return RK_OTA_AGOTADA;
    }
    if (!usb && bat_mv > 0u && bat_mv < RK_OTA_BAT_MIN_MV) {
        return RK_OTA_SIN_BATERIA;
    }
    return RK_OTA_ADELANTE;
}

const char *rk_ota_decision_nombre(rk_ota_decision_t d)
{
    switch (d) {
    case RK_OTA_NO_HAY:        return "no hay";
    case RK_OTA_MISMA_VERSION: return "misma version";
    case RK_OTA_INVALIDA:      return "manifiesto invalido";
    case RK_OTA_AGOTADA:       return "agotada";
    case RK_OTA_SIN_BATERIA:   return "sin bateria";
    case RK_OTA_ADELANTE:      return "adelante";
    default:                   return "?";
    }
}

void rk_ota_marcar_intento(rk_ota_nvs_t *nvs, const char *version)
{
    if (nvs == NULL || version == NULL) {
        return;
    }
    if (strcmp(nvs->version, version) != 0) {
        memset(nvs, 0, sizeof *nvs);
        strncpy(nvs->version, version, sizeof nvs->version - 1u);
    }
    if (nvs->intentos < 255u) {
        nvs->intentos++;
    }
    nvs->verificar = false;
}

bool rk_ota_arranque(rk_ota_nvs_t *nvs, const char *version_actual)
{
    if (nvs == NULL || !nvs->verificar) {
        return false;
    }
    if (version_actual != NULL && strcmp(nvs->version, version_actual) == 0) {
        return true;
    }
    /* Corre otra versión: el gestor de arranque ya volvió atrás. La cuenta
     * de intentos queda, para no volver a bajar la misma. */
    nvs->verificar = false;
    return false;
}

void rk_ota_confirmar(rk_ota_nvs_t *nvs)
{
    if (nvs == NULL) {
        return;
    }
    nvs->verificar = false;
    nvs->intentos = 0u;
}

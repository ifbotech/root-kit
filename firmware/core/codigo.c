#include "codigo.h"
#include "sha256.h"
#include <string.h>

static const char CROCKFORD[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
static const char HEX[] = "0123456789ABCDEF";

void rk_id_desde_mac(const uint8_t mac[6], char out[RK_ID_LEN + 1])
{
    int i;
    for (i = 0; i < 6; i++) {
        out[i * 2]     = HEX[(mac[i] >> 4) & 0xF];
        out[i * 2 + 1] = HEX[mac[i] & 0xF];
    }
    out[RK_ID_LEN] = '\0';
}

void rk_codigo_vinculo(const uint8_t secreto[RK_SECRETO_LEN], uint32_t epoca,
                       char out[RK_CODIGO_LEN + 1])
{
    static const char PREFIJO[] = "rootkit-vinculo:";
    uint8_t msg[sizeof PREFIJO - 1 + 4];
    uint8_t mac[RK_SHA256_LEN];
    uint64_t bits = 0u;
    int i;

    memcpy(msg, PREFIJO, sizeof PREFIJO - 1);
    msg[sizeof PREFIJO - 1]     = (uint8_t)epoca;
    msg[sizeof PREFIJO - 1 + 1] = (uint8_t)(epoca >> 8);
    msg[sizeof PREFIJO - 1 + 2] = (uint8_t)(epoca >> 16);
    msg[sizeof PREFIJO - 1 + 3] = (uint8_t)(epoca >> 24);
    rk_hmac_sha256(secreto, RK_SECRETO_LEN, msg, sizeof msg, mac);

    /* 40 bits -> 8 símbolos de 5 bits. Un billón de códigos posibles: nadie
     * adivina uno en la ventana en que una maceta está sin vincular. */
    for (i = 0; i < 5; i++) {
        bits = (bits << 8) | mac[i];
    }
    for (i = 0; i < (int)RK_CODIGO_LEN; i++) {
        out[i] = CROCKFORD[(bits >> (35 - 5 * i)) & 0x1Fu];
    }
    out[RK_CODIGO_LEN] = '\0';
}

void rk_token_api(const uint8_t secreto[RK_SECRETO_LEN], char out[RK_TOKEN_LEN + 1])
{
    static const char MSG[] = "rootkit-api";
    uint8_t mac[RK_SHA256_LEN];
    int i;

    rk_hmac_sha256(secreto, RK_SECRETO_LEN, (const uint8_t *)MSG, sizeof MSG - 1, mac);
    for (i = 0; i < (int)RK_SHA256_LEN; i++) {
        out[i * 2]     = (char)(HEX[(mac[i] >> 4) & 0xF] | 0x20); /* minúsculas */
        out[i * 2 + 1] = (char)(HEX[mac[i] & 0xF] | 0x20);
    }
    out[RK_TOKEN_LEN] = '\0';
}

bool rk_codigo_normalizar(const char *entrada, char out[RK_CODIGO_LEN + 1])
{
    size_t n = 0u;

    if (entrada == NULL) {
        return false;
    }
    for (; *entrada != '\0'; entrada++) {
        char c = *entrada;
        if (c == '-' || c == ' ' || c == '_' || c == '.') {
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c == 'O') { c = '0'; }
        if (c == 'I' || c == 'L') { c = '1'; }
        if (c == 'U') { c = 'V'; }
        if (strchr(CROCKFORD, c) == NULL || c == '\0') {
            return false;
        }
        if (n >= RK_CODIGO_LEN) {
            return false;
        }
        out[n++] = c;
    }
    out[n] = '\0';
    return n == RK_CODIGO_LEN;
}

void rk_codigo_ssid(const char *codigo, char *out, size_t n)
{
    static const char PRE[] = "ROOTKIT-";
    size_t i, k = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    for (i = 0; PRE[i] != '\0' && k + 1u < n; i++) {
        out[k++] = PRE[i];
    }
    for (i = 0; codigo != NULL && i < 4u && codigo[i] != '\0' && k + 1u < n; i++) {
        out[k++] = codigo[i];
    }
    out[k] = '\0';
}

/* El modo alfanumérico del QR admite 0-9, A-Z, espacio y $%*+-./: */
static bool alfanumerico(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
           c == ' ' || c == '$' || c == '%' || c == '*' || c == '+' ||
           c == '-' || c == '.' || c == '/' || c == ':';
}

bool rk_codigo_url(const char *base, const char *codigo, char *out, size_t n)
{
    size_t k = 0u, i;
    size_t lb;

    if (base == NULL || codigo == NULL || out == NULL) {
        return false;
    }
    lb = strlen(base);
    while (lb > 0u && base[lb - 1u] == '/') {
        lb--;                              /* "http://x/" y "http://x" */
    }
    if (lb + 3u + strlen(codigo) + 1u > n) {
        return false;
    }
    for (i = 0; i < lb; i++) {
        char c = base[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (!alfanumerico(c)) {
            return false;
        }
        out[k++] = c;
    }
    out[k++] = '/';
    out[k++] = 'V';
    out[k++] = '/';
    for (i = 0; codigo[i] != '\0'; i++) {
        if (!alfanumerico(codigo[i])) {
            return false;
        }
        out[k++] = codigo[i];
    }
    out[k] = '\0';
    return true;
}

bool rk_secreto_valido(const uint8_t secreto[RK_SECRETO_LEN])
{
    size_t i;
    bool ceros = true, unos = true;

    if (secreto == NULL) {
        return false;
    }
    for (i = 0; i < RK_SECRETO_LEN; i++) {
        if (secreto[i] != 0x00u) { ceros = false; }
        if (secreto[i] != 0xFFu) { unos = false; }
    }
    return !ceros && !unos;
}

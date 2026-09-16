#include "json.h"
#include <string.h>

/* ---------------------------------------------------------- escritura ---- */
static void poner(rk_jw_t *w, const char *s, size_t n)
{
    if (w->error) {
        return;
    }
    if (w->len + n + 1u > w->cap) {
        w->error = true;
        return;
    }
    memcpy(w->buf + w->len, s, n);
    w->len += n;
    w->buf[w->len] = '\0';
}

static void poner_c(rk_jw_t *w, char c)
{
    poner(w, &c, 1u);
}

/* Coma antes de un elemento que no es el primero de su nivel. */
static void separar(rk_jw_t *w)
{
    if (w->tras_clave) {
        w->tras_clave = false;
        return;
    }
    if (w->nivel > 0) {
        if (!w->vacio[w->nivel - 1]) {
            poner_c(w, ',');
        }
        w->vacio[w->nivel - 1] = false;
    }
}

void rk_jw_iniciar(rk_jw_t *w, char *buf, size_t cap)
{
    memset(w, 0, sizeof *w);
    w->buf = buf;
    w->cap = cap;
    if (buf == NULL || cap == 0u) {
        w->error = true;
        return;
    }
    buf[0] = '\0';
}

static void abrir(rk_jw_t *w, char c)
{
    separar(w);
    poner_c(w, c);
    if (w->nivel >= RK_JW_PROFUNDIDAD) {
        w->error = true;
        return;
    }
    w->vacio[w->nivel++] = true;
}

static void cerrar(rk_jw_t *w, char c)
{
    if (w->nivel <= 0) {
        w->error = true;
        return;
    }
    w->nivel--;
    poner_c(w, c);
}

void rk_jw_obj(rk_jw_t *w)     { abrir(w, '{'); }
void rk_jw_fin_obj(rk_jw_t *w) { cerrar(w, '}'); }
void rk_jw_arr(rk_jw_t *w)     { abrir(w, '['); }
void rk_jw_fin_arr(rk_jw_t *w) { cerrar(w, ']'); }

static void cadena(rk_jw_t *w, const char *s)
{
    static const char HEX[] = "0123456789abcdef";
    poner_c(w, '"');
    for (; s != NULL && *s != '\0'; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            poner_c(w, '\\');
            poner_c(w, (char)c);
        } else if (c == '\n') {
            poner(w, "\\n", 2u);
        } else if (c < 0x20u) {
            char u[6] = { '\\', 'u', '0', '0', HEX[c >> 4], HEX[c & 0xF] };
            poner(w, u, 6u);
        } else {
            poner_c(w, (char)c);
        }
    }
    poner_c(w, '"');
}

void rk_jw_clave(rk_jw_t *w, const char *k)
{
    separar(w);
    cadena(w, k);
    poner_c(w, ':');
    w->tras_clave = true;
}

void rk_jw_texto(rk_jw_t *w, const char *s)
{
    separar(w);
    cadena(w, s);
}

void rk_jw_entero(rk_jw_t *w, long v)
{
    char tmp[24];
    int n = 0;
    unsigned long u;
    char out[24];
    int i;

    separar(w);
    if (v < 0) {
        out[n++] = '-';
        u = (unsigned long)(-(v + 1)) + 1u;
    } else {
        u = (unsigned long)v;
    }
    i = 0;
    do {
        tmp[i++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u > 0u && i < (int)sizeof tmp);
    while (i > 0) {
        out[n++] = tmp[--i];
    }
    poner(w, out, (size_t)n);
}

void rk_jw_bool(rk_jw_t *w, bool v)
{
    separar(w);
    poner(w, v ? "true" : "false", v ? 4u : 5u);
}

void rk_jw_nulo(rk_jw_t *w)
{
    separar(w);
    poner(w, "null", 4u);
}

bool rk_jw_terminar(rk_jw_t *w)
{
    return w != NULL && !w->error && w->nivel == 0 && !w->tras_clave;
}

/* ------------------------------------------------------------ lectura ---- */
#define PROFUNDIDAD_MAX 16

static const char *blancos(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    return p;
}

/* Saltea una cadena que empieza en la comilla. NULL si no termina. */
static const char *saltar_cadena(const char *p)
{
    if (*p != '"') {
        return NULL;
    }
    p++;
    while (*p != '\0') {
        if (*p == '\\') {
            if (p[1] == '\0') {
                return NULL;
            }
            p += 2;
            continue;
        }
        if (*p == '"') {
            return p + 1;
        }
        p++;
    }
    return NULL;
}

static const char *saltar_valor(const char *p, int prof)
{
    p = blancos(p);
    if (prof > PROFUNDIDAD_MAX) {
        return NULL;
    }
    if (*p == '"') {
        return saltar_cadena(p);
    }
    if (*p == '{' || *p == '[') {
        char fin = (*p == '{') ? '}' : ']';
        bool objeto = (*p == '{');
        p = blancos(p + 1);
        if (*p == fin) {
            return p + 1;
        }
        for (;;) {
            if (objeto) {
                p = saltar_cadena(blancos(p));
                if (p == NULL) {
                    return NULL;
                }
                p = blancos(p);
                if (*p != ':') {
                    return NULL;
                }
                p++;
            }
            p = saltar_valor(p, prof + 1);
            if (p == NULL) {
                return NULL;
            }
            p = blancos(p);
            if (*p == ',') {
                p++;
                continue;
            }
            if (*p == fin) {
                return p + 1;
            }
            return NULL;
        }
    }
    /* número, true, false, null: hasta el próximo delimitador */
    if (*p == '\0' || *p == ',' || *p == '}' || *p == ']') {
        return NULL;
    }
    while (*p != '\0' && *p != ',' && *p != '}' && *p != ']' &&
           *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') {
        p++;
    }
    return p;
}

/* ¿La clave que empieza en `p` (comilla incluida) es igual a k[0..n)? */
static bool clave_igual(const char *p, const char *k, size_t n)
{
    size_t i;
    if (*p != '"') {
        return false;
    }
    p++;
    for (i = 0; i < n; i++) {
        if (p[i] == '\0' || p[i] == '\\' || p[i] != k[i]) {
            return false;
        }
    }
    return p[n] == '"';
}

/* Devuelve el comienzo del valor de la ruta, o NULL. */
static const char *buscar(const char *json, const char *ruta)
{
    const char *p;

    if (json == NULL || ruta == NULL) {
        return NULL;
    }
    p = blancos(json);
    while (*ruta != '\0') {
        const char *punto = strchr(ruta, '.');
        size_t n = punto ? (size_t)(punto - ruta) : strlen(ruta);
        bool hallado = false;

        if (*p != '{') {
            return NULL;
        }
        p = blancos(p + 1);
        if (*p == '}') {
            return NULL;
        }
        for (;;) {
            const char *clave = blancos(p);
            const char *tras = saltar_cadena(clave);
            if (tras == NULL) {
                return NULL;
            }
            tras = blancos(tras);
            if (*tras != ':') {
                return NULL;
            }
            tras = blancos(tras + 1);
            if (clave_igual(clave, ruta, n)) {
                p = tras;
                hallado = true;
                break;
            }
            p = saltar_valor(tras, 1);
            if (p == NULL) {
                return NULL;
            }
            p = blancos(p);
            if (*p == ',') {
                p++;
                continue;
            }
            return NULL;
        }
        if (!hallado) {
            return NULL;
        }
        ruta += n;
        if (*ruta == '.') {
            ruta++;
        }
    }
    return p;
}

bool rk_json_hay(const char *json, const char *ruta)
{
    const char *p = buscar(json, ruta);
    return p != NULL && strncmp(p, "null", 4) != 0;
}

bool rk_json_entero(const char *json, const char *ruta, long *out)
{
    const char *p = buscar(json, ruta);
    long v = 0;
    bool neg = false, algun = false;

    if (p == NULL) {
        return false;
    }
    if (*p == '-') {
        neg = true;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        if (v > 214748364L) {
            return false;               /* no entra en 32 bits: no es nuestro */
        }
        v = v * 10 + (*p - '0');
        p++;
        algun = true;
    }
    if (!algun) {
        return false;
    }
    if (*p == '.') {                    /* se trunca la parte decimal */
        p++;
        while (*p >= '0' && *p <= '9') {
            p++;
        }
    }
    if (out != NULL) {
        *out = neg ? -v : v;
    }
    return true;
}

bool rk_json_bool(const char *json, const char *ruta, bool *out)
{
    const char *p = buscar(json, ruta);
    if (p == NULL) {
        return false;
    }
    if (strncmp(p, "true", 4) == 0) {
        if (out != NULL) { *out = true; }
        return true;
    }
    if (strncmp(p, "false", 5) == 0) {
        if (out != NULL) { *out = false; }
        return true;
    }
    return false;
}

bool rk_json_texto(const char *json, const char *ruta, char *out, size_t n)
{
    const char *p = buscar(json, ruta);
    size_t k = 0u;

    if (p == NULL || *p != '"' || out == NULL || n == 0u) {
        return false;
    }
    if (saltar_cadena(p) == NULL) {
        return false;
    }
    p++;
    while (*p != '"' && *p != '\0') {
        char c = *p;
        if (c == '\\') {
            p++;
            switch (*p) {
            case 'n':  c = '\n'; break;
            case 't':  c = '\t'; break;
            case 'u':
                /* Fuera de ASCII no hace falta en el aparato: se reemplaza. */
                c = '?';
                if (p[1] && p[2] && p[3] && p[4]) {
                    p += 4;
                }
                break;
            default:   c = *p; break;
            }
        }
        if (k + 1u < n) {
            out[k++] = c;
        }
        p++;
    }
    out[k] = '\0';
    return true;
}

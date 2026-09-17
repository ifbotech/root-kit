#include "fabrica.h"
#include "ota.h"
#include "persona.h"
#include "../net/json.h"
#include <string.h>

#define PREFIJO "FABRICA"

bool rk_fabrica_es_orden(const char *linea)
{
    return linea != NULL && strncmp(linea, PREFIJO, sizeof PREFIJO - 1u) == 0;
}

bool rk_fabrica_parsear(const char *linea, rk_fabrica_orden_t *o)
{
    const char *p;
    char hex[40];
    size_t i;

    if (o == NULL) {
        return false;
    }
    memset(o, 0, sizeof *o);
    if (!rk_fabrica_es_orden(linea)) {
        return false;
    }
    p = linea + sizeof PREFIJO - 1u;
    if (*p == '?') {
        o->consulta = true;
        return true;
    }
    while (*p == ' ') {
        p++;
    }
    if (*p != '{') {
        return false;
    }
    if (!rk_json_texto(p, "secreto", hex, sizeof hex) ||
        !rk_hex_decodificar(hex, o->secreto, RK_SECRETO_LEN) ||
        !rk_secreto_valido(o->secreto)) {
        goto mal;
    }
    if (!rk_json_texto(p, "persona", o->persona, sizeof o->persona) ||
        rk_persona_find(o->persona) == NULL) {
        goto mal;
    }
    rk_json_texto(p, "lote", o->lote, sizeof o->lote);
    for (i = 0; o->lote[i] != '\0'; i++) {
        char c = o->lote[i];
        bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-';
        if (!ok) {
            goto mal;
        }
    }
    return true;
mal:
    memset(o, 0, sizeof *o);
    return false;
}

size_t rk_fabrica_respuesta(char *buf, size_t cap, const rk_fabrica_estado_t *e)
{
    rk_jw_t w;
    if (buf == NULL || e == NULL) {
        return 0u;
    }
    rk_jw_iniciar(&w, buf, cap);
    rk_jw_obj(&w);
    rk_jw_clave(&w, "fabrica");   rk_jw_bool(&w, true);
    rk_jw_clave(&w, "id");        rk_jw_texto(&w, e->id ? e->id : "");
    rk_jw_clave(&w, "persona");   rk_jw_texto(&w, e->persona ? e->persona : "");
    rk_jw_clave(&w, "lote");      rk_jw_texto(&w, e->lote ? e->lote : "");
    rk_jw_clave(&w, "codigo");    rk_jw_texto(&w, e->codigo ? e->codigo : "");
    rk_jw_clave(&w, "fw");        rk_jw_texto(&w, e->fw ? e->fw : "");
    rk_jw_clave(&w, "vinculado"); rk_jw_bool(&w, e->vinculado);
    rk_jw_fin_obj(&w);
    return rk_jw_terminar(&w) ? w.len : 0u;
}

size_t rk_fabrica_error(char *buf, size_t cap, const char *motivo)
{
    rk_jw_t w;
    if (buf == NULL) {
        return 0u;
    }
    rk_jw_iniciar(&w, buf, cap);
    rk_jw_obj(&w);
    rk_jw_clave(&w, "fabrica"); rk_jw_bool(&w, false);
    rk_jw_clave(&w, "error");   rk_jw_texto(&w, motivo ? motivo : "");
    rk_jw_fin_obj(&w);
    return rk_jw_terminar(&w) ? w.len : 0u;
}

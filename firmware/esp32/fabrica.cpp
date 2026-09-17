#include "fabrica.h"

extern "C" {
#include "../core/fabrica.h"
}

static char   g_linea[200];
static size_t g_n;

static void responder(const rk_almacen_t *a, bool vinculado, const char *id, const char *codigo, const char *fw)
{
    char buf[220];
    rk_fabrica_estado_t e = { id, a->persona, a->lote, codigo, fw, vinculado };
    if (rk_fabrica_respuesta(buf, sizeof buf, &e) > 0u) {
        Serial.println(buf);
    }
}

bool fabrica_atender(rk_almacen_t *a, bool vinculado, const char *id, const char *codigo, const char *fw)
{
    bool cambio = false;

    while (Serial.available() > 0) {
        int c = Serial.read();
        if (c == '\r') {
            continue;
        }
        if (c != '\n') {
            if (g_n < sizeof g_linea - 1u) {
                g_linea[g_n++] = (char)c;
            } else {
                g_n = 0u;               /* una línea demasiado larga se descarta */
            }
            continue;
        }
        g_linea[g_n] = '\0';
        g_n = 0u;
        if (!rk_fabrica_es_orden(g_linea)) {
            continue;
        }

        rk_fabrica_orden_t o;
        char err[64];
        if (!rk_fabrica_parsear(g_linea, &o)) {
            rk_fabrica_error(err, sizeof err, "orden invalida");
            Serial.println(err);
        } else if (o.consulta) {
            responder(a, vinculado, id, codigo, fw);
        } else if (vinculado) {
            /* Una maceta que ya es de alguien no cambia de identidad. */
            rk_fabrica_error(err, sizeof err, "vinculado");
            Serial.println(err);
        } else {
            almacen_grabar_fabrica(a, o.secreto, o.persona, o.lote);
            cambio = true;
            /* La respuesta sale después de recalcular el código: la manda
             * quien llama, con otra consulta. Acá se avisa que quedó. */
            Serial.println("{\"fabrica\":true,\"grabado\":true}");
        }
    }
    return cambio;
}

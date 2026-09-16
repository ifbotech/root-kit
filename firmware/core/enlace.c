#include "enlace.h"
#include "../ui/despertar.h"
#include <stddef.h>
#include <string.h>

static void ir(rk_enlace_t *e, rk_enlace_estado_t s, uint32_t ahora_ms)
{
    if (e->estado != s) {
        e->estado = s;
        e->desde_ms = ahora_ms;
    }
}

void rk_enlace_iniciar(rk_enlace_t *e, const rk_enlace_nvs_t *nvs, uint32_t ahora_ms)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof *e);
    if (nvs != NULL) {
        e->nvs = *nvs;
    }
    /* Un aparato que ya tenía cara arranca con la cara: sin esperar al wifi
     * ni a la nube. Es la diferencia entre un objeto y un dispositivo que
     * bootea. */
    if (e->nvs.vinculado && e->nvs.revelado) {
        e->estado = RK_ENL_ACTIVO;
    } else if (e->nvs.vinculado) {
        e->estado = RK_ENL_ESPERA_COFRE;
    } else if (e->nvs.tiene_wifi) {
        e->estado = RK_ENL_CONECTANDO;
    } else {
        e->estado = RK_ENL_SIN_WIFI;
    }
    e->desde_ms = ahora_ms;
}

/* La nube dijo que ya no está vinculado. Código nuevo, cofre cerrado, pero
 * el wifi se conserva: volver a vincular es escanear y listo. */
static void desvincular(rk_enlace_t *e, uint32_t ahora_ms)
{
    e->nvs.epoca++;
    e->nvs.vinculado = false;
    e->nvs.revelado = false;
    e->sucio = true;
    ir(e, RK_ENL_SIN_VINCULO, ahora_ms);
}

static bool sin_vincular(rk_enlace_estado_t s)
{
    return s == RK_ENL_SIN_WIFI || s == RK_ENL_CONECTANDO || s == RK_ENL_SIN_VINCULO;
}

void rk_enlace_evento(rk_enlace_t *e, rk_evento_t ev,
                      const rk_enlace_nube_t *nube, uint32_t ahora_ms)
{
    if (e == NULL || e->estado >= RK_ENL_COUNT) {
        return;
    }

    switch (ev) {
    case RK_EV_BOTON_LARGO:
        e->nvs.epoca++;
        e->nvs.vinculado = false;
        e->nvs.revelado = false;
        e->nvs.tiene_wifi = false;
        e->borrar_wifi = true;
        e->sucio = true;
        e->fallos_wifi = 0u;
        e->fallos_nube = 0u;
        ir(e, RK_ENL_SIN_WIFI, ahora_ms);
        break;

    case RK_EV_WIFI_GUARDADO:
        if (!e->nvs.tiene_wifi) {
            e->nvs.tiene_wifi = true;
            e->sucio = true;
        }
        e->fallos_wifi = 0u;
        if (e->estado == RK_ENL_SIN_WIFI) {
            ir(e, RK_ENL_CONECTANDO, ahora_ms);
        }
        break;

    case RK_EV_WIFI_OK:
        e->fallos_wifi = 0u;
        if (e->estado == RK_ENL_SIN_WIFI || e->estado == RK_ENL_CONECTANDO) {
            ir(e, RK_ENL_SIN_VINCULO, ahora_ms);
        }
        break;

    case RK_EV_WIFI_FALLO:
        if (e->fallos_wifi < 255u) {
            e->fallos_wifi++;
        }
        if (e->estado == RK_ENL_SIN_VINCULO) {
            ir(e, RK_ENL_CONECTANDO, ahora_ms);
        }
        if (e->estado == RK_ENL_CONECTANDO && e->fallos_wifi >= RK_ENL_FALLOS_PORTAL) {
            /* La red guardada no anda. Se vuelve a ofrecer el portal sin
             * olvidarla: si era el router reiniciándose, se reconecta sola. */
            ir(e, RK_ENL_SIN_WIFI, ahora_ms);
        }
        break;

    case RK_EV_NUBE_OK:
        e->fallos_nube = 0u;
        if (nube == NULL) {
            break;
        }
        if (sin_vincular(e->estado)) {
            if (nube->vinculado) {
                e->nvs.vinculado = true;
                e->sucio = true;
                if (nube->revelado) {
                    e->nvs.revelado = true;
                    ir(e, RK_ENL_DESPERTANDO, ahora_ms);
                } else {
                    ir(e, RK_ENL_ESPERA_COFRE, ahora_ms);
                }
            } else if (e->estado != RK_ENL_SIN_VINCULO) {
                /* Si contestó la nube, hay red, aunque no hubiera llegado el
                 * evento del wifi. */
                ir(e, RK_ENL_SIN_VINCULO, ahora_ms);
            }
            break;
        }
        if (!nube->vinculado) {
            desvincular(e, ahora_ms);
            break;
        }
        if (e->estado == RK_ENL_ESPERA_COFRE && nube->revelado) {
            e->nvs.revelado = true;
            e->sucio = true;
            ir(e, RK_ENL_DESPERTANDO, ahora_ms);
        } else if (e->estado == RK_ENL_ACTIVO && !nube->revelado) {
            /* El cofre se volvió a cerrar desde la app (soporte, o un
             * aparato reasignado). La nube manda. */
            e->nvs.revelado = false;
            e->sucio = true;
            ir(e, RK_ENL_ESPERA_COFRE, ahora_ms);
        }
        break;

    case RK_EV_NUBE_FALLO:
        if (e->fallos_nube < 255u) {
            e->fallos_nube++;
        }
        break;

    case RK_EV_TICK:
    default:
        if (e->estado == RK_ENL_DESPERTANDO &&
            ahora_ms - e->desde_ms >= RK_DESP_FIN_MS) {
            ir(e, RK_ENL_ACTIVO, ahora_ms);
        }
        break;
    }
}

rk_pantalla_t rk_enlace_pantalla(const rk_enlace_t *e)
{
    if (e == NULL) {
        return RK_PANT_QR;
    }
    switch (e->estado) {
    case RK_ENL_ESPERA_COFRE: return RK_PANT_DORMIDA;
    case RK_ENL_DESPERTANDO:  return RK_PANT_DESPERTAR;
    case RK_ENL_ACTIVO:       return RK_PANT_CARA;
    default:                  return RK_PANT_QR;
    }
}

bool rk_enlace_portal(const rk_enlace_t *e)
{
    return e != NULL && e->estado == RK_ENL_SIN_WIFI;
}

bool rk_enlace_quiere_wifi(const rk_enlace_t *e)
{
    return e != NULL && e->nvs.tiene_wifi;
}

bool rk_enlace_manda_codigo(const rk_enlace_t *e)
{
    return e != NULL && sin_vincular(e->estado);
}

uint32_t rk_enlace_consulta_ms(const rk_enlace_t *e, bool usb, uint32_t ahora_ms)
{
    uint32_t base;
    uint8_t k;

    if (e == NULL) {
        return 0u;
    }
    if (e->estado != RK_ENL_SIN_VINCULO && e->estado != RK_ENL_ESPERA_COFRE) {
        return 0u;
    }
    /* Alguien está con el teléfono en la mano esperando que la maceta
     * reaccione: consulta seguido. Si nadie apareció en veinte minutos y
     * está a batería, afloja. */
    base = (usb || ahora_ms - e->desde_ms < RK_ENL_VENTANA_RAPIDA_MS)
           ? RK_ENL_CONSULTA_RAPIDA_MS : RK_ENL_CONSULTA_LENTA_MS;
    for (k = 0u; k < e->fallos_nube && base < RK_ENL_CONSULTA_TECHO_MS; k++) {
        base *= 2u;
    }
    return base > RK_ENL_CONSULTA_TECHO_MS ? RK_ENL_CONSULTA_TECHO_MS : base;
}

uint32_t rk_enlace_en_estado_ms(const rk_enlace_t *e, uint32_t ahora_ms)
{
    return (e == NULL) ? 0u : ahora_ms - e->desde_ms;
}

const char *rk_enlace_nombre(rk_enlace_estado_t s)
{
    switch (s) {
    case RK_ENL_SIN_WIFI:     return "SIN_WIFI";
    case RK_ENL_CONECTANDO:   return "CONECTANDO";
    case RK_ENL_SIN_VINCULO:  return "SIN_VINCULO";
    case RK_ENL_ESPERA_COFRE: return "ESPERA_COFRE";
    case RK_ENL_DESPERTANDO:  return "DESPERTANDO";
    case RK_ENL_ACTIVO:       return "ACTIVO";
    default:                  return "?";
    }
}

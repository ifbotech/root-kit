/* enlace.h — del primer encendido a la cara: la máquina de estados.
 *
 * EL FLUJO QUE ESTE ARCHIVO IMPLEMENTA
 *
 *   1. Se enciende por primera vez: muestra un QR. A la vez levanta una red
 *      propia, ROOTKIT-XXXX, con un portal cautivo.
 *   2. El QR abre la app en el teléfono. La app pide instalarse y activar
 *      notificaciones, y guía para conectar la maceta al wifi de la casa
 *      (a través del portal).
 *   3. Con wifi, el aparato se reporta a la nube con su código. La app, que
 *      tiene el mismo código porque lo leyó del QR, lo reclama: quedan
 *      vinculados. El QR desaparece y la maceta duerme.
 *   4. En la app se abre el cofre y se ve qué personaje tocó. En la siguiente
 *      consulta la nube lo informa y la maceta DESPIERTA: los ojos se abren.
 *   5. De ahí en adelante, la cara.
 *
 * Desvincular (desde la app, o manteniendo apretado el botón 10 s) vuelve al
 * QR con un código nuevo. Mantener apretado además olvida el wifi.
 *
 * POR QUÉ ES UNA FUNCIÓN PURA
 *
 * El flujo cruza wifi, nube, NVS, pantalla y botón, y cada uno falla a su
 * manera. Acá no hay nada de eso: entran eventos, sale un estado. La placa
 * traduce ese estado a acciones, y los tests recorren todos los caminos
 * —incluidos los feos, como perder la red a mitad de vinculación— sin tocar
 * un ESP32.
 *
 * LA REGLA DE LA PANTALLA
 *
 * Una vez vinculado, NADA de la red cambia lo que se ve: ni perder el wifi
 * ni que la nube no conteste. La cara sigue evaluando la planta con lo
 * último que sabe. Sólo la nube puede devolverlo al QR, diciendo que ya no
 * está vinculado; un corte no es una desvinculación.
 */
#ifndef ROOTKIT_ENLACE_H
#define ROOTKIT_ENLACE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RK_ENL_SIN_WIFI = 0,   /* sin red que funcione: QR + portal           */
    RK_ENL_CONECTANDO,     /* con red guardada, todavía sin respuesta     */
    RK_ENL_SIN_VINCULO,    /* en línea, esperando que alguien lo reclame  */
    RK_ENL_ESPERA_COFRE,   /* vinculado, el cofre sigue cerrado           */
    RK_ENL_DESPERTANDO,    /* el cofre se abrió: los ojos se abren        */
    RK_ENL_ACTIVO,         /* la cara, para siempre                       */
    RK_ENL_COUNT
} rk_enlace_estado_t;

typedef enum {
    RK_PANT_QR = 0,
    RK_PANT_DORMIDA,
    RK_PANT_DESPERTAR,
    RK_PANT_CARA
} rk_pantalla_t;

typedef enum {
    RK_EV_TICK = 0,
    RK_EV_WIFI_GUARDADO,   /* el portal recibió una red                   */
    RK_EV_WIFI_OK,
    RK_EV_WIFI_FALLO,
    RK_EV_NUBE_OK,         /* con la respuesta resumida                   */
    RK_EV_NUBE_FALLO,
    RK_EV_BOTON_LARGO      /* borrón y cuenta nueva: vínculo y wifi       */
} rk_evento_t;

/* Lo que sobrevive a un reinicio. Vive en NVS. */
typedef struct {
    uint32_t epoca;        /* sube con cada desvinculación                */
    bool     tiene_wifi;
    bool     vinculado;
    bool     revelado;
} rk_enlace_nvs_t;

/* Lo único de la respuesta de la nube que le importa a esta máquina. */
typedef struct {
    bool vinculado;
    bool revelado;
} rk_enlace_nube_t;

typedef struct {
    rk_enlace_estado_t estado;
    rk_enlace_nvs_t    nvs;
    uint32_t desde_ms;     /* cuándo se entró al estado actual            */
    uint8_t  fallos_wifi;
    uint8_t  fallos_nube;
    bool     sucio;        /* nvs cambió: la placa tiene que guardarlo    */
    bool     borrar_wifi;  /* orden de una vez: olvidar la red guardada   */
} rk_enlace_t;

/* Intentos de conexión fallidos antes de volver a levantar el portal. Tres
 * es lo que tarda un router en reiniciarse sin que el aparato pida ayuda,
 * y lo suficientemente poco para que una contraseña mal tipeada se corrija
 * en el mismo minuto. */
#define RK_ENL_FALLOS_PORTAL   3u

/* Cadencia de consulta mientras se espera a la app. */
#define RK_ENL_CONSULTA_RAPIDA_MS   3000u
#define RK_ENL_CONSULTA_LENTA_MS   30000u
#define RK_ENL_VENTANA_RAPIDA_MS  (20u * 60u * 1000u)
#define RK_ENL_CONSULTA_TECHO_MS   60000u

void rk_enlace_iniciar(rk_enlace_t *e, const rk_enlace_nvs_t *nvs, uint32_t ahora_ms);

/* `nube` sólo se lee con RK_EV_NUBE_OK. */
void rk_enlace_evento(rk_enlace_t *e, rk_evento_t ev,
                      const rk_enlace_nube_t *nube, uint32_t ahora_ms);

rk_pantalla_t rk_enlace_pantalla(const rk_enlace_t *e);

/* ¿Tiene que estar levantado el portal (la red ROOTKIT-XXXX)? */
bool rk_enlace_portal(const rk_enlace_t *e);

/* ¿Tiene que intentar conectarse a la red guardada? */
bool rk_enlace_quiere_wifi(const rk_enlace_t *e);

/* ¿El código de vinculación viaja en la consulta? Sólo mientras no está
 * vinculado: una vez reclamado, el código deja de servir. */
bool rk_enlace_manda_codigo(const rk_enlace_t *e);

/* Cada cuánto consultar la nube, en ms. 0 significa "no por el enlace": en
 * ACTIVO la cadencia la decide el muestreador, y sin red no hay a quién
 * preguntar. */
uint32_t rk_enlace_consulta_ms(const rk_enlace_t *e, bool usb, uint32_t ahora_ms);

/* Milisegundos dentro del estado actual. Es el reloj del despertar. */
uint32_t rk_enlace_en_estado_ms(const rk_enlace_t *e, uint32_t ahora_ms);

const char *rk_enlace_nombre(rk_enlace_estado_t s);

#endif /* ROOTKIT_ENLACE_H */

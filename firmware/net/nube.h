/* nube.h — la única conversación del aparato con el mundo.
 *
 * UN SOLO PEDIDO
 *
 *   POST {base}/api/d/sync
 *   Authorization: Bearer <token>
 *
 * El aparato manda quién es, en qué estado está y las lecturas que tiene
 * pendientes. La nube contesta todo lo que el aparato necesita saber: si
 * está vinculado, si el cofre se abrió, qué personaje es, cómo se llama la
 * planta, los umbrales de la especie y cada cuánto volver a preguntar.
 *
 * Un pedido y no cinco endpoints porque cada conexión TLS cuesta casi un
 * segundo de radio: juntar todo en un ida y vuelta es la mayor optimización
 * de batería que hay a nivel protocolo. El detalle de campos está en
 * docs/nube.md y es el contrato con el servidor de root-lab.
 *
 * POR QUÉ EL APARATO EMPUJA Y NO LA APP TIRA
 *
 * La app es una página HTTPS en el teléfono: el navegador no la deja pedir
 * nada a una IP privada de la casa, y además el teléfono casi nunca está en
 * la misma red que la maceta. Y las notificaciones push necesitan un
 * servidor que las mande aunque la app esté cerrada. Así que el aparato le
 * cuenta todo a la nube, y la app y las notificaciones salen de ahí.
 */
#ifndef ROOTKIT_NUBE_H
#define ROOTKIT_NUBE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../core/species.h"
#include "../core/vinculo.h"
#include "../nodo/historial.h"
#include "../nodo/soil.h"

#define RK_NUBE_RUTA_SYNC "/api/d/sync"

/* Lo que el aparato cuenta de sí mismo. */
typedef struct {
    const char *id;
    const char *fw;          /* "0.5.0"                                   */
    const char *placa;       /* "c3-supermini", "esp32-devkit"            */
    const char *pantalla;    /* "st7735-128", "ili9341-240x320"           */
    const char *persona;     /* id grabado en fábrica, o "" si no tiene   */
    const char *codigo;      /* NULL una vez vinculado                    */
    const char *estado;      /* rk_enlace_nombre()                        */
    uint32_t    epoca;
    uint32_t    reloj_s;     /* reloj monótono ahora                      */
    int16_t     rssi;
    bool        usb;
    uint16_t    bat_mv;
    uint32_t    arranques;
} rk_nube_yo_t;

/* Arma el cuerpo con hasta `max_lecturas` de las más viejas del historial.
 * Devuelve la longitud (0 si no entra) y cuántas lecturas incluyó en
 * `*incluidas`: sólo esas se descartan cuando la nube confirma. */
size_t rk_nube_armar_sync(char *buf, size_t cap, const rk_nube_yo_t *yo,
                          const rk_historial_t *h, uint16_t max_lecturas,
                          uint16_t *incluidas);

/* La especie tal como la guarda el aparato: con lugar propio para los
 * textos, así se puede copiar a NVS y volver. */
typedef struct {
    char         id[24];
    char         nombre[32];
    rk_species_t sp;         /* sus punteros se arreglan con rk_especie_ver */
} rk_especie_guardada_t;

/* Devuelve la especie lista para core/mood.c, con los punteros apuntando a
 * los textos propios. Llamarla después de copiar o cargar desde NVS. */
const rk_species_t *rk_especie_ver(rk_especie_guardada_t *e);

typedef struct {
    bool     ok;
    bool     vinculado;
    bool     revelado;
    char     persona[16];
    char     nombre[24];
    bool     hay_especie;
    rk_especie_guardada_t especie;
    uint32_t intervalo_s;    /* 0 = no dijo: se usa el propio             */
    uint16_t aceptadas;      /* lecturas que la nube guardó               */
    uint32_t hora;           /* unix, 0 = no dijo                         */
    bool     hay_calibracion;
    rk_soil_cal_t cal;
    uint8_t  brillo;         /* 0-100; 0 = no dijo                        */
    /* A batería la pantalla se apaga a los 20 s sin tocarla. El usuario
     * puede pedir desde la app que quede siempre encendida (y lo paga en
     * autonomía: la app se lo dice con números). */
    bool     pantalla_siempre;
    /* Los días sanos los cuenta la nube, que ve el día entero aunque el
     * aparato duerma o se quede sin luz. El aparato los usa para los
     * adornos de la cara y sólo cuenta por su cuenta si nunca se los
     * mandaron. */
    bool     hay_vinculo;
    rk_bond_t vinculo;
} rk_nube_resp_t;

/* Interpreta la respuesta. Devuelve false si no es JSON de la nube (sin
 * "ok": true): un portal de hotel o un proxy que contesta HTML no pueden
 * desvincular una maceta por accidente. */
bool rk_nube_parsear(const char *json, rk_nube_resp_t *r);

#endif /* ROOTKIT_NUBE_H */

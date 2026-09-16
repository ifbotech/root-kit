/* json.h — lo justo de JSON para hablar con la nube.
 *
 * POR QUÉ JSON Y NO EL PROTOCOLO BINARIO DE ANTES
 *
 * El protocolo binario existía para ESP-NOW, donde cada byte de trama
 * costaba aire. Ahora cada aparato habla HTTPS con la nube, y ahí el costo
 * energético lo pone el apretón de manos TLS (unos cuantos KB y la mayor
 * parte del segundo que la radio está prendida), no si el cuerpo mide 200 o
 * 400 bytes. JSON se lee en los logs, se prueba con curl y el servidor lo
 * entiende sin decodificador: la cuenta cambió y la decisión con ella.
 *
 * POR QUÉ PROPIO Y NO ArduinoJson
 *
 * El núcleo del firmware se compila y se prueba en el escritorio, sin
 * Arduino. Escribir es trivial; leer se limita a buscar valores por ruta
 * ("especie.suelo_min") sin construir un árbol ni pedir memoria: recorre el
 * texto, y un JSON malformado o hostil devuelve "no está" en vez de leer
 * fuera del buffer.
 */
#ifndef ROOTKIT_JSON_H
#define ROOTKIT_JSON_H

#include <stdbool.h>
#include <stddef.h>

/* ---------------------------------------------------------- escritura ---- */
#define RK_JW_PROFUNDIDAD 8

typedef struct {
    char   *buf;
    size_t  cap;
    size_t  len;
    bool    error;              /* se quedó sin lugar o se anidó de más  */
    int     nivel;
    bool    vacio[RK_JW_PROFUNDIDAD];
    bool    tras_clave;
} rk_jw_t;

void rk_jw_iniciar(rk_jw_t *w, char *buf, size_t cap);
void rk_jw_obj(rk_jw_t *w);
void rk_jw_fin_obj(rk_jw_t *w);
void rk_jw_arr(rk_jw_t *w);
void rk_jw_fin_arr(rk_jw_t *w);
void rk_jw_clave(rk_jw_t *w, const char *k);
void rk_jw_texto(rk_jw_t *w, const char *s);
void rk_jw_entero(rk_jw_t *w, long v);
void rk_jw_bool(rk_jw_t *w, bool v);
void rk_jw_nulo(rk_jw_t *w);
/* Devuelve true si todo entró y quedó balanceado. */
bool rk_jw_terminar(rk_jw_t *w);

/* ------------------------------------------------------------ lectura ---- */
/* `ruta` son claves separadas por puntos. Sólo objetos: la nube no manda
 * listas que el aparato tenga que leer. */
bool rk_json_entero(const char *json, const char *ruta, long *out);
bool rk_json_bool(const char *json, const char *ruta, bool *out);
/* Copia el texto (sin comillas, con los escapes simples resueltos) y
 * trunca a `n - 1`. */
bool rk_json_texto(const char *json, const char *ruta, char *out, size_t n);
/* ¿Existe la ruta y no es null? */
bool rk_json_hay(const char *json, const char *ruta);

#endif /* ROOTKIT_JSON_H */

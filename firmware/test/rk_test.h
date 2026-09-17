/* rk_test.h — arnés de pruebas mínimo.
 *
 * Sin dependencias: corre en el escritorio, en CI y —si alguna vez hace
 * falta— sobre el propio ESP32. Cada archivo de pruebas expone una función
 * suite_*() y test/main.c las corre todas.
 *
 * La salida está pensada para leerse de un vistazo: sólo se imprime el
 * detalle de lo que falla, y al final un resumen por suite.
 */
#ifndef RK_TEST_H
#define RK_TEST_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern int         rk_t_run;
extern int         rk_t_failed;
extern int         rk_t_suite_run;
extern int         rk_t_suite_failed;
extern const char *rk_t_suite;

void rk_t_begin(const char *name);
void rk_t_end(void);
void rk_t_fail(const char *label, const char *detail);
void rk_t_pass(void);
int  rk_t_report(void);

#define RK_SUITE(name)  rk_t_begin(name)
#define RK_SUITE_END()  rk_t_end()

#define CHECK_TRUE(label, cond)                                            \
    do {                                                                   \
        if (cond) { rk_t_pass(); }                                         \
        else      { rk_t_fail((label), "la condicion es falsa"); }         \
    } while (0)

#define CHECK_INT(label, expected, actual)                                 \
    do {                                                                   \
        long _e = (long)(expected), _a = (long)(actual);                   \
        if (_e == _a) { rk_t_pass(); }                                     \
        else {                                                             \
            char _b[128];                                                  \
            snprintf(_b, sizeof _b, "esperaba %ld, obtuvo %ld", _e, _a);   \
            rk_t_fail((label), _b);                                        \
        }                                                                  \
    } while (0)

/* Igualdad con tolerancia: para todo lo que pasa por aritmética entera con
 * redondeo, donde exigir el valor exacto vuelve el test frágil sin agregar
 * garantía. */
#define CHECK_NEAR(label, expected, actual, tol)                           \
    do {                                                                   \
        long _e = (long)(expected), _a = (long)(actual), _t = (long)(tol); \
        long _d = _a > _e ? _a - _e : _e - _a;                             \
        if (_d <= _t) { rk_t_pass(); }                                     \
        else {                                                             \
            char _b[128];                                                  \
            snprintf(_b, sizeof _b, "esperaba %ld +-%ld, obtuvo %ld",      \
                     _e, _t, _a);                                          \
            rk_t_fail((label), _b);                                        \
        }                                                                  \
    } while (0)

#define CHECK_STR(label, expected, actual)                                 \
    do {                                                                   \
        const char *_e = (expected), *_a = (actual);                       \
        if (_a != NULL && strcmp(_e, _a) == 0) { rk_t_pass(); }            \
        else {                                                             \
            char _b[640];                                                  \
            snprintf(_b, sizeof _b, "esperaba \"%s\", obtuvo \"%s\"",      \
                     _e, _a ? _a : "(null)");                              \
            rk_t_fail((label), _b);                                        \
        }                                                                  \
    } while (0)

#define CHECK_HEX(label, expected, actual)                                 \
    do {                                                                   \
        unsigned long _e = (unsigned long)(expected);                      \
        unsigned long _a = (unsigned long)(actual);                        \
        if (_e == _a) { rk_t_pass(); }                                     \
        else {                                                             \
            char _b[128];                                                  \
            snprintf(_b, sizeof _b, "esperaba 0x%lX, obtuvo 0x%lX",_e,_a); \
            rk_t_fail((label), _b);                                        \
        }                                                                  \
    } while (0)

void suite_mood(void);
void suite_nodo(void);
void suite_gfx(void);
void suite_render(void);
void suite_persona(void);
void suite_sensores(void);
void suite_qr(void);
void suite_enlace(void);
void suite_red(void);
void suite_ota(void);

#endif /* RK_TEST_H */

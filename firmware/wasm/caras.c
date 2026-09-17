/* caras.c — el renderer del ROOTKIT compilado a WebAssembly.
 *
 * La app muestra la cara de cada maceta con EXACTAMENTE el mismo código que
 * la dibuja en la pantalla del aparato: art/face.c, gfx/aa.c y la tabla de
 * core/persona.c, compilados para el navegador. No hay una segunda versión
 * de las caras en CSS o SVG que se pueda desincronizar: si la artista cambia
 * una fila de la tabla, cambia en la maceta y en el teléfono a la vez.
 *
 * Se compila sin libc (ver wasm/include/string.h y las cuatro funciones de
 * abajo) y pesa unas decenas de KB. Lo usa root-lab/public/lib/caras.mjs.
 *
 *   make wasm   ->  build/rootkit_caras.wasm
 */
#include <stddef.h>
#include <stdint.h>
#include "../gfx/fb.h"
#include "../art/face.h"
#include "../core/persona.h"
#include "../ui/cara.h"
#include "../ui/despertar.h"

#define EXPORTA(n) __attribute__((export_name(n)))

#define LADO_MAX 512

static rk_color_t g_565[LADO_MAX * LADO_MAX];
static uint8_t    g_rgba[LADO_MAX * LADO_MAX * 4];
static rk_fb_t    g_fb;

/* ---------------------------------------------------------- libc mínima -- */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) { *p++ = (unsigned char)c; }
    return s;
}

void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    while (n--) { *a++ = *b++; }
    return d;
}

int strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

size_t strlen(const char *s)
{
    size_t n = 0u;
    while (s[n] != '\0') { n++; }
    return n;
}

char *strncpy(char *d, const char *s, size_t n)
{
    size_t i = 0u;
    for (; i < n && s[i] != '\0'; i++) { d[i] = s[i]; }
    for (; i < n; i++) { d[i] = '\0'; }
    return d;
}

void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    if (a < b) {
        while (n--) { *a++ = *b++; }
    } else {
        while (n--) { a[n] = b[n]; }
    }
    return d;
}

char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) { return (char *)s; }
        if (*s == '\0') { return NULL; }
    }
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    for (; n > 0u; n--, x++, y++) {
        if (*x != *y) { return (int)*x - (int)*y; }
    }
    return 0;
}

long labs(long v)
{
    return v < 0 ? -v : v;
}

int abs(int v)
{
    return v < 0 ? -v : v;
}

/* ------------------------------------------------------------ exportado -- */
static void a_rgba(void)
{
    int i, n = g_fb.w * g_fb.h;
    for (i = 0; i < n; i++) {
        rk_color_t c = g_565[i];
        g_rgba[i * 4]     = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        g_rgba[i * 4 + 1] = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
        g_rgba[i * 4 + 2] = (uint8_t)((c & 0x1F) * 255 / 31);
        g_rgba[i * 4 + 3] = 255u;
    }
}

/* Prepara un lienzo de w x h. Devuelve 0 si el tamaño no entra. */
EXPORTA("lienzo")
int rk_wasm_lienzo(int w, int h)
{
    if (w < 8 || h < 8 || w > LADO_MAX || h > LADO_MAX) {
        return 0;
    }
    rk_fb_init(&g_fb, g_565, w, h);
    return 1;
}

EXPORTA("rgba")
uint8_t *rk_wasm_rgba(void)
{
    return g_rgba;
}

EXPORTA("personas")
int rk_wasm_personas(void)
{
    return rk_persona_count;
}

/* Id del modelo `i` como puntero a texto en la memoria del módulo. */
EXPORTA("persona_id")
const char *rk_wasm_persona_id(int i)
{
    const rk_persona_t *p = rk_persona_at(i);
    return p != NULL ? p->id : "";
}

/* Un color de una piel, en RGB565: campo 0 fondo, 1 ojos, 2 piel, 3 rubor. */
EXPORTA("piel_color")
int rk_wasm_piel_color(int i, int rareza, int campo)
{
    const rk_piel_t *pl = rk_persona_piel(rk_persona_at(i), rareza);
    if (pl == NULL) {
        return 0;
    }
    switch (campo) {
    case 1:  return (int)pl->ojos;
    case 2:  return (int)pl->piel;
    case 3:  return (int)pl->rubor;
    default: return (int)pl->fondo;
    }
}

EXPORTA("animos")
int rk_wasm_animos(void)
{
    return RK_MOOD_COUNT;
}

EXPORTA("animo_id")
const char *rk_wasm_animo_id(int m)
{
    return rk_mood_name((rk_mood_t)m);
}

static uint8_t rareza_de(int r)
{
    return (uint8_t)(r < 0 || r >= (int)RK_RAREZA_COUNT ? 0 : r);
}

/* La cara de un Rooti con una piel, en un ánimo, con los adornos de una
 * etapa. */
EXPORTA("cara")
void rk_wasm_cara(int persona, int rareza, int mood, int etapa, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_face_draw(&g_fb, rk_persona_at(persona), rareza_de(rareza), (rk_mood_t)mood,
                 RK_SEV_OK, rk_face_adornos_etapa(etapa), t_ms);
    a_rgba();
}

/* Antes del cofre: el Rooti dormido, en grises. */
EXPORTA("dormida")
void rk_wasm_dormida(int persona, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_cara_dormida(&g_fb, rk_persona_at(persona), t_ms);
    a_rgba();
}

EXPORTA("despertar")
void rk_wasm_despertar(int persona, int rareza, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_despertar_draw(&g_fb, rk_persona_at(persona), rareza_de(rareza), t_ms);
    a_rgba();
}

EXPORTA("despertar_ms")
int rk_wasm_despertar_ms(void)
{
    return (int)RK_DESP_FIN_MS;
}

/* =========================================================================
 * EL APARATO ENTERO, PARA EL EMULADOR
 *
 * root-lab trae un emulador de ROOTKIT en el navegador para recorrer el
 * flujo completo sin placa: QR, vínculo, cofre, despertar y caras. No
 * reimplementa nada: usa la misma máquina de estados (core/enlace.c), el
 * mismo código de vinculación (core/codigo.c), la misma pantalla del QR
 * (ui/qr.c) y la misma evaluación del ánimo (core/mood.c) que la placa.
 * ========================================================================= */
#include "../core/codigo.h"
#include "../core/enlace.h"
#include "../core/mood.h"
#include "../ui/qr.h"
#include "../nodo/soil.h"

static char     g_entrada[256];
static char     g_salida[128];
static uint8_t  g_secreto[RK_SECRETO_LEN];
static rk_qr_t  g_qr;
static rk_enlace_t g_enl;
static rk_species_t g_sp;
static rk_mood_state_t g_mst;
static rk_verdict_t g_ver;

/* Buffer donde JS escribe texto para pasárselo al módulo. */
EXPORTA("entrada")
char *rk_wasm_entrada(void) { return g_entrada; }

EXPORTA("secreto")
uint8_t *rk_wasm_secreto(void) { return g_secreto; }

EXPORTA("codigo")
const char *rk_wasm_codigo(uint32_t epoca)
{
    rk_codigo_vinculo(g_secreto, epoca, g_salida);
    return g_salida;
}

EXPORTA("token")
const char *rk_wasm_token(void)
{
    static char tok[RK_TOKEN_LEN + 1];
    rk_token_api(g_secreto, tok);
    return tok;
}

/* `entrada` trae la URL base; devuelve la URL del QR o "" si no entra. */
EXPORTA("qr_preparar")
const char *rk_wasm_qr_preparar(uint32_t epoca)
{
    static char url[160];
    char codigo[RK_CODIGO_LEN + 1];
    rk_codigo_vinculo(g_secreto, epoca, codigo);
    if (!rk_codigo_url(g_entrada, codigo, url, sizeof url)) {
        url[0] = '\0';
    }
    rk_qr_preparar(&g_qr, url[0] ? url : NULL, codigo);
    return url;
}

EXPORTA("qr")
void rk_wasm_qr(int estado, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_qr_draw(&g_fb, &g_qr, (rk_qr_estado_t)estado, t_ms);
    a_rgba();
}

EXPORTA("enlace_iniciar")
void rk_wasm_enlace_iniciar(uint32_t epoca, int wifi, int vinculado, int revelado, uint32_t ahora)
{
    rk_enlace_nvs_t nvs;
    nvs.epoca = epoca;
    nvs.tiene_wifi = wifi != 0;
    nvs.vinculado = vinculado != 0;
    nvs.revelado = revelado != 0;
    rk_enlace_iniciar(&g_enl, &nvs, ahora);
}

EXPORTA("enlace_evento")
void rk_wasm_enlace_evento(int ev, int vinculado, int revelado, uint32_t ahora)
{
    rk_enlace_nube_t n;
    n.vinculado = vinculado != 0;
    n.revelado = revelado != 0;
    rk_enlace_evento(&g_enl, (rk_evento_t)ev, &n, ahora);
}

EXPORTA("enlace_estado")      int rk_wasm_enlace_estado(void) { return (int)g_enl.estado; }
EXPORTA("enlace_pantalla")    int rk_wasm_enlace_pantalla(void) { return (int)rk_enlace_pantalla(&g_enl); }
EXPORTA("enlace_portal")      int rk_wasm_enlace_portal(void) { return rk_enlace_portal(&g_enl); }
EXPORTA("enlace_codigo")      int rk_wasm_enlace_codigo(void) { return rk_enlace_manda_codigo(&g_enl); }
EXPORTA("enlace_epoca")       uint32_t rk_wasm_enlace_epoca(void) { return g_enl.nvs.epoca; }
EXPORTA("enlace_wifi")        int rk_wasm_enlace_wifi(void) { return g_enl.nvs.tiene_wifi; }
EXPORTA("enlace_vinculado")   int rk_wasm_enlace_vinculado(void) { return g_enl.nvs.vinculado; }
EXPORTA("enlace_revelado")    int rk_wasm_enlace_revelado(void) { return g_enl.nvs.revelado; }
EXPORTA("enlace_nombre")      const char *rk_wasm_enlace_nombre(void) { return rk_enlace_nombre(g_enl.estado); }

EXPORTA("enlace_ms")
uint32_t rk_wasm_enlace_ms(uint32_t ahora) { return rk_enlace_en_estado_ms(&g_enl, ahora); }

EXPORTA("enlace_consulta_ms")
uint32_t rk_wasm_enlace_consulta_ms(int usb, uint32_t ahora)
{
    return rk_enlace_consulta_ms(&g_enl, usb != 0, ahora);
}

EXPORTA("especie")
void rk_wasm_especie(int smin, int smax, int tmin, int tmax, int hr, int lmin, int lmax)
{
    g_sp.id = "nube";
    g_sp.nombre = "nube";
    g_sp.soil_min = (uint8_t)smin;
    g_sp.soil_max = (uint8_t)smax;
    g_sp.temp_min_dc = (int16_t)tmin;
    g_sp.temp_max_dc = (int16_t)tmax;
    g_sp.rh_min = (uint8_t)hr;
    g_sp.lux_min = (uint32_t)lmin;
    g_sp.lux_max = (uint32_t)lmax;
    rk_mood_state_init(&g_mst);
}

/* Evalúa una lectura con la especie cargada. Devuelve el rk_mood_t. */
EXPORTA("animo")
int rk_wasm_animo(int suelo, int temp_dc, int hr, int lux, int fallas)
{
    rk_telemetry_t t;
    memset(&t, 0, sizeof t);
    t.valid = true;
    t.soil_pct = (uint8_t)suelo;
    t.temp_dc = (int16_t)temp_dc;
    t.rh_pct = (uint8_t)hr;
    t.lux = (uint32_t)lux;
    t.fallas = (uint8_t)fallas;
    g_ver = rk_mood_eval(&g_mst, &g_sp, &t);
    return (int)g_ver.mood;
}

EXPORTA("severidad")
int rk_wasm_severidad(void) { return (int)g_ver.severity; }

EXPORTA("persona_indice")
int rk_wasm_persona_indice(void)
{
    /* `entrada` trae el id. */
    return rk_persona_index(rk_persona_find(g_entrada));
}

/* Un punto de la transición entre dos ánimos (ver ui/cara.h). `pct` ya viene
 * con su curva: usar cara_anim_pct. */
EXPORTA("cara_mezcla")
void rk_wasm_cara_mezcla(int persona, int rareza, int desde, int hacia, int pct, int etapa,
                         int cierre, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_face_draw_mezcla(&g_fb, rk_persona_at(persona), rareza_de(rareza),
                        (rk_mood_t)desde, (rk_mood_t)hacia,
                        (uint8_t)(pct < 0 ? 0 : pct > 100 ? 100 : pct), RK_SEV_OK,
                        rk_face_adornos_etapa(etapa),
                        (uint8_t)(cierre < 0 ? 0 : cierre > 100 ? 100 : cierre), t_ms);
    a_rgba();
}

/* Cuánto de la transición pasó a los `pasado_ms`, con la curva suave. */
EXPORTA("cara_anim_pct")
int rk_wasm_cara_anim_pct(uint32_t pasado_ms)
{
    rk_cara_anim_t a;
    a.desde = (uint8_t)RK_MOOD_HAPPY;
    a.hacia = (uint8_t)RK_MOOD_THIRSTY;
    a.t0_ms = 0u;
    a.iniciada = true;
    return (int)rk_cara_anim_pct(&a, pasado_ms);
}

EXPORTA("transicion_ms")
int rk_wasm_transicion_ms(void)
{
    return (int)RK_CARA_TRANSICION_MS;
}

/* El detector de riego, para que el emulador marque un escurrimiento igual
 * que la placa. */
static rk_riego_t g_riego;

EXPORTA("riego_paso")
int rk_wasm_riego_paso(int soil_pct, uint32_t t_s)
{
    return (int)rk_riego_paso(&g_riego, (uint8_t)(soil_pct < 0 ? 0 : soil_pct), t_s);
}

EXPORTA("riego_escurre")
int rk_wasm_riego_escurre(uint32_t t_s)
{
    return rk_riego_escurriendo(&g_riego, t_s) ? 1 : 0;
}

/* La cara mientras la acarician en el teléfono: `mimo_pct` 0..100 sobre la
 * cara del ánimo (ver rk_face_draw_mimo). Nunca llega a la maceta. */
EXPORTA("cara_mimo")
void rk_wasm_cara_mimo(int persona, int rareza, int mood, int etapa, int mimo_pct, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_face_draw_mimo(&g_fb, rk_persona_at(persona), rareza_de(rareza), (rk_mood_t)mood,
                      (uint8_t)(mimo_pct < 0 ? 0 : mimo_pct > 100 ? 100 : mimo_pct),
                      RK_SEV_OK, rk_face_adornos_etapa(etapa), t_ms);
    a_rgba();
}

/* La cara mirando a un vecino, con o sin preocupación (el invernadero de
 * la app; ver rk_face_draw_mirada). */
EXPORTA("cara_mirada")
void rk_wasm_cara_mirada(int persona, int rareza, int mood, int etapa, int mira_x, int mira_y,
                         int preocupado, uint32_t t_ms)
{
    rk_face_mirada_t m;
    if (g_fb.px == NULL) {
        return;
    }
    m.mira_x = mira_x;
    m.mira_y = mira_y;
    m.preocupado = (uint8_t)(preocupado < 0 ? 0 : preocupado > 100 ? 100 : preocupado);
    rk_face_draw_mirada(&g_fb, rk_persona_at(persona), rareza_de(rareza), (rk_mood_t)mood, RK_SEV_OK,
                        rk_face_adornos_etapa(etapa), &m, t_ms);
    a_rgba();
}

/* La cara con los párpados forzados: lo que muestra la maceta mientras se
 * mantiene apretado el botón (los ojos se van cerrando antes de reiniciar). */
EXPORTA("cara_cierre")
void rk_wasm_cara_cierre(int persona, int rareza, int mood, int etapa, int cierre, uint32_t t_ms)
{
    if (g_fb.px == NULL) {
        return;
    }
    rk_face_draw_cierre(&g_fb, rk_persona_at(persona), rareza_de(rareza), (rk_mood_t)mood, RK_SEV_OK,
                        rk_face_adornos_etapa(etapa), (uint8_t)(cierre < 0 ? 0 : cierre > 100 ? 100 : cierre), t_ms);
    a_rgba();
}

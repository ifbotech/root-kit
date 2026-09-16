/* main.cpp — el ROOTKIT: une el núcleo portable con el hardware.
 *
 * Todo lo que decide (qué pantalla mostrar, qué cara poner, cuándo hablar
 * con la nube, qué hacer con la respuesta) está en el núcleo C, probado en el
 * escritorio. Este archivo sólo traduce: lee pines y bytes, se los pasa al
 * núcleo, y hace lo que el núcleo pide.
 *
 * El bucle no bloquea nunca. Cada vuelta:
 *
 *   botón  ->  portal  ->  wifi  ->  medir  ->  nube  ->  enlace  ->
 *   guardar  ->  energía  ->  dibujar
 *
 * Ver docs/firmware.md para el mapa completo y cómo compilar y flashear.
 */
#include <Arduino.h>
#include <esp_mac.h>

#include "placa.h"
#include "pantalla.h"
#include "sensores_hw.h"
#include "almacen.h"
#include "portal.h"
#include "red.h"
#include "energia.h"

extern "C" {
#include "../core/codigo.h"
#include "../core/enlace.h"
#include "../core/mood.h"
#include "../core/node.h"
#include "../core/persona.h"
#include "../art/face.h"
#include "../ui/cara.h"
#include "../nodo/soil.h"
#include "../ui/qr.h"
#include "../ui/despertar.h"
#include "../nodo/sampler.h"
#include "../nodo/power.h"
#include "../net/nube.h"
}

#ifndef RK_FW_VERSION
#define RK_FW_VERSION "0.5.0"
#endif
#ifndef RK_NUBE_URL
#define RK_NUBE_URL "https://ifbotech.com/rootkit"
#endif
/* A dónde lleva el QR. Normalmente es la misma nube; en desarrollo puede ser
 * un túnel HTTPS para que el teléfono pueda instalar la app y recibir
 * notificaciones mientras la placa habla con la PC por la red local. */
#ifndef RK_APP_URL
#define RK_APP_URL ""
#endif

/* Cuánto aguanta la pantalla prendida sin que la toquen, a batería. */
#define PANTALLA_OCIOSA_MS     20000u
/* Mantener apretado para borrar vínculo y wifi. */
#define BOTON_LARGO_MS         10000u
/* A partir de acá los ojos empiezan a cerrarse: el aviso sin palabras de
 * que, si seguís apretando, la maceta se olvida de todo. */
#define BOTON_AVISO_MS          2000u
#define WIFI_REINTENTO_MS      10000u
#define LECTURAS_POR_PEDIDO       20u

static rk_almacen_t   A;
static rk_enlace_t    E;
static rk_historial_t H;
static rk_sampler_t   S;
static rk_node_t      N;
static rk_qr_t        Q;
static rk_nube_resp_t R;

static char g_id[RK_ID_LEN + 1];
static char g_token[RK_TOKEN_LEN + 1];
static char g_codigo[RK_CODIGO_LEN + 1];
static char g_ssid_ap[20];
static char g_url_sync[140];
static uint32_t g_epoca_qr = 0xFFFFFFFFu;

static char g_cuerpo[4096];
static char g_respuesta[2048];
static uint16_t g_incluidas;

static uint32_t g_t_medir;
static uint32_t g_t_consulta;
static uint32_t g_t_actividad;
static uint32_t g_t_cuadro;
static uint32_t g_t_boton;
static uint32_t g_t_wifi_ok;
static uint32_t g_t_wifi_reintento;
static bool     g_largo_disparado;
static bool     g_transmitir = true;
static uint32_t g_intervalo_nube_s = 900u;
static rk_wifi_t g_wifi_prev = RK_WIFI_APAGADO;
static rk_enlace_estado_t g_estado_prev = RK_ENL_COUNT;

RTC_DATA_ATTR static uint32_t g_dia_desde_s;
RTC_DATA_ATTR static bool     g_dia_urgente;
/* El detector de riego mira varias lecturas seguidas: vive en la memoria
 * RTC para sobrevivir al deep sleep entre una y otra. */
RTC_DATA_ATTR static rk_riego_t g_riego;
static rk_cara_anim_t g_anim;

/* --------------------------------------------------------- identidad ---- */
static void refrescar_codigo(void)
{
    char url_qr[128];
    bool ok;

    if (g_epoca_qr == E.nvs.epoca) {
        return;
    }
    g_epoca_qr = E.nvs.epoca;
    rk_codigo_vinculo(A.secreto, E.nvs.epoca, g_codigo);
    rk_codigo_ssid(g_codigo, g_ssid_ap, sizeof g_ssid_ap);
    ok = rk_codigo_url(A.app[0] != 0 ? A.app : A.nube, g_codigo, url_qr, sizeof url_qr);
    rk_qr_preparar(&Q, ok ? url_qr : NULL, g_codigo);
    snprintf(g_url_sync, sizeof g_url_sync, "%s%s", A.nube, RK_NUBE_RUTA_SYNC);
    Serial.printf("[enlace] codigo %s  red %s  qr %s\n", g_codigo, g_ssid_ap, ok ? url_qr : "(sin QR)");
}

static void aplicar_persona(void)
{
    const rk_persona_t *p = rk_persona_find(A.persona);
    N.persona = p != NULL ? p : rk_persona_at(0);
}

static void aplicar_especie(void)
{
    N.sp = A.hay_especie ? rk_especie_ver(&A.especie) : NULL;
    rk_mood_state_init(&N.mst);
}

/* ------------------------------------------------------------- medir ---- */
static void medir(uint32_t ahora)
{
    rk_crudos_t crudos;
    uint32_t reloj = energia_reloj_s();

    sensores_leer(&crudos);
    rk_sensores_telemetria(&crudos, &A.cal, &N.tel);

    /* ¿El último riego empapó o se escurrió por los costados? */
    if (!(N.tel.fallas & RK_FALLA_SUELO)) {
        rk_riego_evento_t ev = rk_riego_paso(&g_riego, N.tel.soil_pct, reloj);
        if (ev == RK_RIEGO_ESCURRIO) {
            Serial.println("[riego] el agua se escurrio sin empapar");
        } else if (ev == RK_RIEGO_EMPAPO) {
            Serial.println("[riego] riego de verdad");
        }
    }
    if (rk_riego_escurriendo(&g_riego, reloj)) {
        N.tel.fallas |= RK_FALLA_ESCURRE;
    }

    if (N.sp != NULL) {
        N.verdict = rk_mood_eval(&N.mst, N.sp, &N.tel);
    } else {
        /* Sin especie todavía (la foto no se sacó): contento de conocerte. */
        N.verdict.mood = RK_MOOD_HAPPY;
        N.verdict.severity = RK_SEV_OK;
        N.verdict.reason = rk_mood_reason(RK_MOOD_HAPPY);
    }

    rk_registro_t reg = rk_registro_desde(&N.tel, reloj, (uint8_t)N.verdict.mood,
                                          (uint8_t)N.verdict.severity);
    rk_historial_agregar(&H, &reg);
    almacen_historial_guardar(&H);

    rk_sampler_decision_t d = rk_sampler_step(&S, &N.tel, reloj, N.sp);
    if (d.transmit) {
        g_transmitir = true;
    }
    g_t_medir = ahora + (uint32_t)d.sleep_s * 1000u;

    /* El día del vínculo: 24 h de reloj, sano si no hubo nada urgente. */
    if (N.verdict.severity == RK_SEV_URGENT) {
        g_dia_urgente = true;
    }
    if (E.estado == RK_ENL_ACTIVO && reloj - g_dia_desde_s >= 86400u) {
        rk_bond_dia(&A.vinculo, !g_dia_urgente);
        N.bond = A.vinculo;
        almacen_guardar_vinculo(&A);
        g_dia_desde_s = reloj;
        g_dia_urgente = false;
    }

    Serial.printf("[medir] suelo %u%% aire %d.%d C %u%% luz %lu bat %u usb %d fallas %x -> %s\n",
                  N.tel.soil_pct, N.tel.temp_dc / 10, abs(N.tel.temp_dc % 10), N.tel.rh_pct,
                  (unsigned long)N.tel.lux, N.tel.batt_mv, N.tel.usb, N.tel.fallas,
                  rk_mood_name(N.verdict.mood));
}

/* -------------------------------------------------------------- nube ---- */
static void sincronizar(uint32_t ahora)
{
    uint32_t cada_ms;
    rk_nube_yo_t yo;
    size_t n;

    if (red_ocupada() || red_wifi_estado() != RK_WIFI_CONECTADO) {
        return;
    }
    cada_ms = rk_enlace_consulta_ms(&E, N.tel.usb, ahora);
    if (cada_ms > 0u) {
        if (ahora < g_t_consulta) {
            return;
        }
    } else if (E.estado == RK_ENL_ACTIVO) {
        if (!g_transmitir && ahora < g_t_consulta) {
            return;
        }
    } else {
        return;
    }

    memset(&yo, 0, sizeof yo);
    yo.id = g_id;
    yo.fw = RK_FW_VERSION;
    yo.placa = RK_PLACA_NOMBRE;
    yo.pantalla = RK_PANTALLA_NOMBRE;
    yo.persona = A.persona;
    yo.codigo = rk_enlace_manda_codigo(&E) ? g_codigo : NULL;
    yo.estado = rk_enlace_nombre(E.estado);
    yo.epoca = E.nvs.epoca;
    yo.reloj_s = energia_reloj_s();
    yo.rssi = red_rssi();
    yo.usb = N.tel.usb;
    yo.bat_mv = N.tel.batt_mv;
    yo.arranques = A.arranques;

    n = rk_nube_armar_sync(g_cuerpo, sizeof g_cuerpo, &yo, &H, LECTURAS_POR_PEDIDO, &g_incluidas);
    if (n > 0u && red_pedir(g_url_sync, g_token, g_cuerpo, n)) {
        /* Si la respuesta no llega, se vuelve a intentar en la cadencia. */
        g_t_consulta = ahora + (cada_ms > 0u ? cada_ms : g_intervalo_nube_s * 1000u);
    }
}

static void procesar_respuesta(uint32_t ahora)
{
    int codigo;
    bool cambio = false;

    if (!red_respuesta(&codigo, g_respuesta, sizeof g_respuesta)) {
        return;
    }
    if (codigo != 200 || !rk_nube_parsear(g_respuesta, &R)) {
        Serial.printf("[nube] fallo %d\n", codigo);
        rk_enlace_evento(&E, RK_EV_NUBE_FALLO, NULL, ahora);
        return;
    }

    rk_enlace_nube_t resumen = { R.vinculado, R.revelado };
    rk_enlace_evento(&E, RK_EV_NUBE_OK, &resumen, ahora);

    if (R.vinculado) {
        if (R.persona[0] != '\0' && strcmp(R.persona, A.persona) != 0) {
            strncpy(A.persona, R.persona, sizeof A.persona - 1);
            aplicar_persona();
            cambio = true;
        }
        if (strcmp(R.nombre, A.nombre) != 0) {
            strncpy(A.nombre, R.nombre, sizeof A.nombre - 1);
            cambio = true;
        }
        if (R.hay_especie && (!A.hay_especie || memcmp(&R.especie.sp.soil_min, &A.especie.sp.soil_min,
                sizeof(rk_species_t) - offsetof(rk_species_t, soil_min)) != 0 ||
                strcmp(R.especie.id, A.especie.id) != 0)) {
            A.especie = R.especie;
            A.hay_especie = true;
            aplicar_especie();
            cambio = true;
        }
        if (R.hay_calibracion && memcmp(&R.cal, &A.cal, sizeof A.cal) != 0) {
            A.cal = R.cal;
            cambio = true;
        }
        if (R.brillo > 0 && R.brillo != A.brillo) {
            A.brillo = R.brillo;
            cambio = true;
        }
        if (R.pantalla_siempre != A.pantalla_siempre) {
            A.pantalla_siempre = R.pantalla_siempre;
            cambio = true;
        }
        if (R.hay_vinculo && memcmp(&R.vinculo, &A.vinculo, sizeof A.vinculo) != 0) {
            A.vinculo = R.vinculo;
            N.bond = A.vinculo;
            almacen_guardar_vinculo(&A);
        }
        if (cambio) {
            almacen_guardar_config(&A);
        }
    }
    if (R.intervalo_s > 0u) {
        g_intervalo_nube_s = R.intervalo_s;
    }
    rk_historial_descartar(&H, R.aceptadas < g_incluidas ? R.aceptadas : g_incluidas);
    almacen_historial_guardar(&H);
    g_transmitir = H.cuenta > 0u;          /* quedan pendientes: otra tanda */
    if (g_transmitir) {
        g_t_consulta = ahora + 2000u;
    }
}

/* ------------------------------------------------------ wifi y portal ---- */
static void atender_wifi(uint32_t ahora)
{
    if (!rk_enlace_quiere_wifi(&E)) {
        return;
    }
    rk_wifi_t w = red_wifi_estado();
    if (w == RK_WIFI_APAGADO || (w == RK_WIFI_FALLO && ahora >= g_t_wifi_reintento)) {
        red_wifi_conectar(A.ssid, A.clave);
        w = RK_WIFI_CONECTANDO;
    }
    if (w != g_wifi_prev) {
        if (w == RK_WIFI_CONECTADO) {
            rk_enlace_evento(&E, RK_EV_WIFI_OK, NULL, ahora);
            portal_informar(RK_PORTAL_OK);
            g_t_wifi_ok = ahora;
            g_t_consulta = ahora;           /* presentarse ya */
            Serial.printf("[wifi] conectado a %s\n", A.ssid);
        } else if (w == RK_WIFI_FALLO) {
            rk_enlace_evento(&E, RK_EV_WIFI_FALLO, NULL, ahora);
            portal_informar(RK_PORTAL_FALLO);
            g_t_wifi_reintento = ahora + WIFI_REINTENTO_MS;
            Serial.printf("[wifi] no pude conectar a %s\n", A.ssid);
        }
        g_wifi_prev = w;
    }
}

static void atender_portal(uint32_t ahora)
{
    rk_portal_datos_t datos;
    bool quiere = rk_enlace_portal(&E);

    if (quiere && !portal_activo()) {
        portal_iniciar(g_ssid_ap, g_codigo);
        Serial.printf("[portal] red %s\n", g_ssid_ap);
    }
    /* Se apaga unos segundos después de conectar, para que la página llegue
     * a mostrar "Listo". Mientras prueba la red, sigue arriba. */
    if (!quiere && portal_activo() && E.estado != RK_ENL_CONECTANDO &&
        ahora - g_t_wifi_ok > 8000u) {
        portal_detener();
    }
    if (portal_atender(&datos)) {
        strncpy(A.ssid, datos.ssid, sizeof A.ssid - 1);
        strncpy(A.clave, datos.clave, sizeof A.clave - 1);
        almacen_guardar_wifi(&A);
        if (datos.nube[0] != '\0') {
            strncpy(A.nube, datos.nube, sizeof A.nube - 1);
            almacen_guardar_nube(&A);
            g_epoca_qr = 0xFFFFFFFFu;
            refrescar_codigo();
        }
        rk_enlace_evento(&E, RK_EV_WIFI_GUARDADO, NULL, ahora);
        red_wifi_conectar(A.ssid, A.clave);
        g_wifi_prev = RK_WIFI_CONECTANDO;
    }
}

/* ------------------------------------------------------------- botón ---- */
static uint32_t boton_apretado_ms(uint32_t ahora)
{
    bool apretado = digitalRead(RK_PIN_BOTON) == LOW || digitalRead(RK_PIN_TOQUE) == HIGH;

    if (!apretado) {
        g_t_boton = 0u;
        g_largo_disparado = false;
        return 0u;
    }
    if (g_t_boton == 0u) {
        g_t_boton = ahora;
    }
    g_t_actividad = ahora;
    if (!g_largo_disparado && ahora - g_t_boton >= BOTON_LARGO_MS) {
        g_largo_disparado = true;
        Serial.println("[boton] borron y cuenta nueva");
        rk_enlace_evento(&E, RK_EV_BOTON_LARGO, NULL, ahora);
    }
    return ahora - g_t_boton;
}

/* ----------------------------------------------------------- guardar ---- */
static void persistir(void)
{
    if (E.borrar_wifi) {
        E.borrar_wifi = false;
        portal_detener();
        red_wifi_olvidar();
        almacen_borrar_wifi(&A);
        g_wifi_prev = RK_WIFI_APAGADO;
    }
    if (E.sucio) {
        E.sucio = false;
        A.enlace = E.nvs;
        almacen_guardar_enlace(&A);
        /* Desvinculado: la planta y el nombre eran del dueño anterior. La
         * persona no, que es la carcasa. */
        if (!E.nvs.vinculado && (A.hay_especie || A.nombre[0] != '\0')) {
            A.hay_especie = false;
            A.nombre[0] = '\0';
            rk_bond_init(&A.vinculo);
            N.bond = A.vinculo;
            aplicar_especie();
            almacen_guardar_config(&A);
            almacen_guardar_vinculo(&A);
        }
    }
    refrescar_codigo();
}

/* ----------------------------------------------------------- energía ---- */
static void administrar_energia(uint32_t ahora)
{
    bool usb = N.tel.usb;
    bool config = E.estado != RK_ENL_ACTIVO;
    bool siempre = usb || A.pantalla_siempre || config;

    if (siempre || ahora - g_t_actividad < PANTALLA_OCIOSA_MS) {
        pantalla_brillo(A.brillo);
        return;
    }
    pantalla_brillo(0);

    /* A batería, con cara, pantalla apagada y nada en vuelo: a dormir hasta
     * la próxima medición o hasta que la toquen. */
    if (!red_ocupada() && !g_transmitir && !portal_activo() && g_t_boton == 0u) {
        uint32_t falta = g_t_medir > ahora ? (g_t_medir - ahora) / 1000u : 1u;
        if (!usb && N.tel.batt_mv > 0u && rk_batt_is_critical(N.tel.batt_mv)) {
            falta = 3600u;              /* proteger la celda */
        }
        Serial.printf("[energia] durmiendo %lu s\n", (unsigned long)falta);
        almacen_historial_guardar(&H);
        energia_dormir(falta);
    }
}

/* ----------------------------------------------------------- dibujar ---- */
static void dibujar(uint32_t ahora, uint32_t apretado_ms)
{
    rk_fb_t *fb = pantalla_fb();
    uint32_t cuadro_ms = N.tel.usb ? 33u : 66u;

    if (pantalla_brillo_actual() == 0u || ahora - g_t_cuadro < cuadro_ms) {
        return;
    }
    g_t_cuadro = ahora;

    switch (rk_enlace_pantalla(&E)) {
    case RK_PANT_QR: {
        rk_qr_estado_t q = E.estado == RK_ENL_SIN_WIFI ? RK_QR_PORTAL
                         : E.estado == RK_ENL_CONECTANDO ? RK_QR_CONECTANDO : RK_QR_EN_LINEA;
        rk_qr_draw(fb, &Q, q, ahora);
        break;
    }
    case RK_PANT_DORMIDA:
        rk_cara_dormida(fb, ahora);
        break;
    case RK_PANT_DESPERTAR:
        rk_despertar_draw(fb, N.persona, rk_enlace_en_estado_ms(&E, ahora));
        break;
    case RK_PANT_CARA:
    default: {
        /* Con transición entre ánimos; apretando el botón los párpados van
         * bajando hasta el reinicio. */
        uint8_t cierre = 0u;
        if (apretado_ms > BOTON_AVISO_MS) {
            uint32_t k = (apretado_ms - BOTON_AVISO_MS) * 100u / (BOTON_LARGO_MS - BOTON_AVISO_MS);
            cierre = (uint8_t)(k > 100u ? 100u : k);
        }
        rk_cara_draw_anim(fb, &N, &g_anim, cierre, ahora);
        break;
    }
    }
    pantalla_presentar(fb->px[0]);
}

/* ------------------------------------------------------ setup y loop ---- */
void setup(void)
{
    uint8_t mac[6];

    Serial.begin(115200);
    energia_iniciar();
    pinMode(RK_PIN_BOTON, INPUT_PULLUP);
    pinMode(RK_PIN_TOQUE, INPUT);

    if (!almacen_cargar(&A, RK_NUBE_URL, RK_APP_URL)) {
        Serial.println("[almacen] NVS no responde");
    }

    /* El framebuffer se pide antes que el wifi: después la memoria contigua
     * escasea. En un despertar por timer la pantalla no se prende. */
    if (!pantalla_iniciar(energia_causa() == RK_DESPERTAR_TIMER ? 0 : A.brillo)) {
        Serial.println("[pantalla] sin memoria para el framebuffer");
    }
    sensores_iniciar();

    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    rk_id_desde_mac(mac, g_id);
    rk_token_api(A.secreto, g_token);

    memset(&N, 0, sizeof N);
    rk_enlace_iniciar(&E, &A.enlace, millis());
    aplicar_persona();
    aplicar_especie();
    N.bond = A.vinculo;
    strncpy(N.nombre, A.nombre, sizeof N.nombre - 1);
    rk_sampler_init(&S, NULL);

    if (almacen_historial_cargar(&H) && H.cuenta > 0u) {
        energia_reloj_minimo(rk_historial_ver(&H, (uint16_t)(H.cuenta - 1u))->reloj_s + 1u);
    }
    refrescar_codigo();
    red_iniciar();

    g_t_medir = 0u;
    g_t_actividad = energia_causa() == RK_DESPERTAR_TIMER ? (uint32_t)(0u - PANTALLA_OCIOSA_MS) : millis();
    Serial.printf("\nROOTKIT %s  %s  %s  id %s  persona %s  estado %s\n", RK_FW_VERSION,
                  RK_PLACA_NOMBRE, RK_PANTALLA_NOMBRE, g_id, N.persona->id,
                  rk_enlace_nombre(E.estado));
}

void loop(void)
{
    uint32_t ahora = millis();
    uint32_t apretado = boton_apretado_ms(ahora);

    atender_portal(ahora);
    atender_wifi(ahora);
    if (ahora >= g_t_medir) {
        medir(ahora);
    }
    procesar_respuesta(ahora);
    sincronizar(ahora);
    rk_enlace_evento(&E, RK_EV_TICK, NULL, ahora);
    if (E.estado != g_estado_prev) {
        Serial.printf("[enlace] %s\n", rk_enlace_nombre(E.estado));
        g_estado_prev = E.estado;
        g_t_actividad = ahora;           /* un cambio de estado se muestra */
        pantalla_invalidar();
    }
    persistir();
    administrar_energia(ahora);
    dibujar(ahora, apretado);
    delay(2);
}

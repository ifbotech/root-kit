/* El camino del primer encendido a la cara: identidad y máquina de estados.
 *
 * Es el flujo que ve cada usuario exactamente una vez, y por eso el que
 * menos se prueba a mano: nadie desvincula y revincula su maceta veinte
 * veces con el router apagándose en el medio. Acá sí.
 */
#include <stddef.h>
#include "rk_test.h"
#include "../core/sha256.h"
#include "../core/codigo.h"
#include "../core/enlace.h"
#include "../ui/despertar.h"

static void hex(const uint8_t *b, size_t n, char *out)
{
    static const char H[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) {
        out[i * 2] = H[b[i] >> 4];
        out[i * 2 + 1] = H[b[i] & 0xF];
    }
    out[n * 2] = '\0';
}

static void test_sha256(void)
{
    uint8_t d[32];
    char s[65];
    uint8_t k1[20];
    size_t i;

    /* Vectores de FIPS 180-2. */
    rk_sha256((const uint8_t *)"", 0, d);
    hex(d, 32, s);
    CHECK_STR("sha256 de vacio",
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", s);
    rk_sha256((const uint8_t *)"abc", 3, d);
    hex(d, 32, s);
    CHECK_STR("sha256 de abc",
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", s);
    rk_sha256((const uint8_t *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56, d);
    hex(d, 32, s);
    CHECK_STR("sha256 de dos bloques",
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", s);

    /* RFC 4231, casos 1 y 2. */
    for (i = 0; i < sizeof k1; i++) {
        k1[i] = 0x0bu;
    }
    rk_hmac_sha256(k1, sizeof k1, (const uint8_t *)"Hi There", 8, d);
    hex(d, 32, s);
    CHECK_STR("hmac rfc4231 caso 1",
              "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", s);
    rk_hmac_sha256((const uint8_t *)"Jefe", 4,
                   (const uint8_t *)"what do ya want for nothing?", 28, d);
    hex(d, 32, s);
    CHECK_STR("hmac rfc4231 caso 2",
              "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", s);
}

static const uint8_t SECRETO[RK_SECRETO_LEN] = {
    0x3a, 0x91, 0x7c, 0x05, 0xee, 0x42, 0x18, 0xb6,
    0x9d, 0x60, 0x2f, 0xc3, 0x71, 0x0e, 0x84, 0x5b
};

static void test_codigo(void)
{
    char a[9], b[9], c[9], n[9], url[96], ssid[16], id[13], tok[65];
    uint8_t mac[6] = { 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6 };
    int i;
    bool alfabeto = true;

    rk_id_desde_mac(mac, id);
    CHECK_STR("el id es la MAC en hex", "A1B2C3D4E5F6", id);

    rk_codigo_vinculo(SECRETO, 0u, a);
    rk_codigo_vinculo(SECRETO, 0u, b);
    rk_codigo_vinculo(SECRETO, 1u, c);
    /* Vectores de referencia, calculados aparte con Python (hmac + hashlib).
     * root-lab/test/codigo.test.mjs verifica los mismos: si el servidor y la
     * placa derivan distinto, ningún QR vincula nada. */
    CHECK_STR("vector de referencia, epoca 0", "PTS0JHM6", a);
    CHECK_STR("vector de referencia, epoca 1", "A8XTJCFQ", c);
    rk_token_api(SECRETO, tok);
    CHECK_STR("token de referencia",
              "71859c23a4eb073e425391d23d46eede1760af53a9bec3b4b168724b2d8e6be3", tok);
    CHECK_INT("el codigo mide 8", 8, (long)strlen(a));
    CHECK_STR("el mismo secreto y la misma epoca dan el mismo codigo", a, b);
    CHECK_TRUE("otra epoca da otro codigo: el QR viejo deja de valer",
               strcmp(a, c) != 0);
    for (i = 0; i < 8; i++) {
        if (strchr("ILOU", a[i]) != NULL) {
            alfabeto = false;
        }
    }
    CHECK_TRUE("el codigo no usa I, L, O ni U", alfabeto);

    /* Lo que tipea una persona. */
    CHECK_TRUE("se normaliza con guion y minusculas",
               rk_codigo_normalizar("k7q2-m9xa", n));
    CHECK_STR("y queda en mayusculas sin guion", "K7Q2M9XA", n);
    CHECK_TRUE("O e I se leen como 0 y 1", rk_codigo_normalizar("O0II LLAA", n));
    CHECK_STR("O0II LLAA queda 001111AA", "001111AA", n);
    CHECK_TRUE("un codigo corto no vale", !rk_codigo_normalizar("K7Q2", n));
    CHECK_TRUE("uno largo tampoco", !rk_codigo_normalizar("K7Q2M9XAB", n));
    CHECK_TRUE("un caracter raro tampoco", !rk_codigo_normalizar("K7Q2M9X#", n));
    CHECK_TRUE("NULL tampoco", !rk_codigo_normalizar(NULL, n));
    rk_codigo_vinculo(SECRETO, 7u, a);
    CHECK_TRUE("todo codigo generado se normaliza a si mismo",
               rk_codigo_normalizar(a, n) && strcmp(a, n) == 0);

    rk_codigo_ssid("K7Q2M9XA", ssid, sizeof ssid);
    CHECK_STR("la red del portal sale del codigo", "ROOTKIT-K7Q2", ssid);
    rk_codigo_ssid("K7Q2M9XA", ssid, 6);
    CHECK_STR("y se trunca sin desbordar", "ROOTK", ssid);

    CHECK_TRUE("la URL se arma", rk_codigo_url("https://rootlab.app/", "K7Q2M9XA", url, sizeof url));
    CHECK_STR("en mayusculas y sin doble barra", "HTTPS://ROOTLAB.APP/V/K7Q2M9XA", url);
    CHECK_TRUE("con IP y puerto tambien",
               rk_codigo_url("http://192.168.0.20:8080", "K7Q2M9XA", url, sizeof url));
    CHECK_STR("http://192.168.0.20:8080", "HTTP://192.168.0.20:8080/V/K7Q2M9XA", url);
    CHECK_TRUE("una base con caracteres fuera del modo alfanumerico se rechaza",
               !rk_codigo_url("https://root_lab.app", "K7Q2M9XA", url, sizeof url));
    CHECK_TRUE("una URL que no entra se rechaza",
               !rk_codigo_url("https://rootlab.app", "K7Q2M9XA", url, 20));

    rk_token_api(SECRETO, tok);
    CHECK_INT("el token mide 64", 64, (long)strlen(tok));
    rk_codigo_vinculo(SECRETO, 0u, a);
    CHECK_TRUE("el token no contiene el codigo", strstr(tok, a) == NULL);

    CHECK_TRUE("un secreto real es valido", rk_secreto_valido(SECRETO));
    {
        uint8_t ceros[RK_SECRETO_LEN] = { 0 };
        uint8_t borrado[RK_SECRETO_LEN];
        memset(borrado, 0xFF, sizeof borrado);
        CHECK_TRUE("todo ceros no es un secreto", !rk_secreto_valido(ceros));
        CHECK_TRUE("NVS borrado no es un secreto", !rk_secreto_valido(borrado));
    }
}

static rk_enlace_nube_t nube(bool vinculado, bool revelado)
{
    rk_enlace_nube_t n;
    n.vinculado = vinculado;
    n.revelado = revelado;
    return n;
}

static void test_flujo_completo(void)
{
    rk_enlace_t e;
    rk_enlace_nvs_t guardado;
    rk_enlace_nube_t r;
    uint32_t t = 1000u;

    /* 1. Primer encendido: QR y portal. */
    rk_enlace_iniciar(&e, NULL, t);
    CHECK_STR("arranca sin wifi", "SIN_WIFI", rk_enlace_nombre(e.estado));
    CHECK_INT("muestra el QR", RK_PANT_QR, rk_enlace_pantalla(&e));
    CHECK_TRUE("levanta el portal", rk_enlace_portal(&e));
    CHECK_TRUE("no intenta conectarse a nada", !rk_enlace_quiere_wifi(&e));
    CHECK_TRUE("manda el codigo", rk_enlace_manda_codigo(&e));
    CHECK_INT("sin red no consulta", 0, rk_enlace_consulta_ms(&e, false, t));

    /* 2. El portal recibe la red de la casa. */
    t += 40000u;
    rk_enlace_evento(&e, RK_EV_WIFI_GUARDADO, NULL, t);
    CHECK_STR("pasa a conectando", "CONECTANDO", rk_enlace_nombre(e.estado));
    CHECK_TRUE("baja el portal", !rk_enlace_portal(&e));
    CHECK_TRUE("hay que guardar", e.sucio);
    CHECK_INT("sigue en el QR", RK_PANT_QR, rk_enlace_pantalla(&e));

    /* 3. Conecta. */
    rk_enlace_evento(&e, RK_EV_WIFI_OK, NULL, t);
    CHECK_STR("en linea sin vincular", "SIN_VINCULO", rk_enlace_nombre(e.estado));
    CHECK_INT("consulta seguido mientras espera a la app",
              RK_ENL_CONSULTA_RAPIDA_MS, rk_enlace_consulta_ms(&e, false, t));

    /* 4. La nube todavía no sabe de nadie. */
    r = nube(false, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t);
    CHECK_STR("sin reclamo sigue esperando", "SIN_VINCULO", rk_enlace_nombre(e.estado));

    /* 5. La app lo reclama. */
    e.sucio = false;
    r = nube(true, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t);
    CHECK_STR("vinculado espera el cofre", "ESPERA_COFRE", rk_enlace_nombre(e.estado));
    CHECK_INT("y duerme", RK_PANT_DORMIDA, rk_enlace_pantalla(&e));
    CHECK_TRUE("ya no manda el codigo", !rk_enlace_manda_codigo(&e));
    CHECK_TRUE("guarda el vinculo", e.sucio && e.nvs.vinculado);

    /* Perder la red mientras duerme no le cambia la cara. */
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    rk_enlace_evento(&e, RK_EV_NUBE_FALLO, NULL, t);
    CHECK_INT("un corte no despierta ni devuelve el QR", RK_PANT_DORMIDA,
              rk_enlace_pantalla(&e));

    /* 6. Se abre el cofre. */
    t += 5000u;
    r = nube(true, true);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t);
    CHECK_STR("el cofre abierto lo despierta", "DESPERTANDO", rk_enlace_nombre(e.estado));
    CHECK_INT("pantalla del despertar", RK_PANT_DESPERTAR, rk_enlace_pantalla(&e));
    rk_enlace_evento(&e, RK_EV_TICK, NULL, t + RK_DESP_FIN_MS - 1u);
    CHECK_STR("no se corta el despertar", "DESPERTANDO", rk_enlace_nombre(e.estado));
    CHECK_INT("el reloj del despertar corre desde que empezo",
              RK_DESP_FIN_MS - 1u, rk_enlace_en_estado_ms(&e, t + RK_DESP_FIN_MS - 1u));
    rk_enlace_evento(&e, RK_EV_TICK, NULL, t + RK_DESP_FIN_MS);
    CHECK_STR("y termina en la cara", "ACTIVO", rk_enlace_nombre(e.estado));
    CHECK_INT("la cara", RK_PANT_CARA, rk_enlace_pantalla(&e));
    CHECK_INT("en la cara la cadencia la pone el muestreador", 0,
              rk_enlace_consulta_ms(&e, false, t));

    /* 7. Se reinicia: vuelve directo a la cara, sin esperar red. */
    guardado = e.nvs;
    rk_enlace_iniciar(&e, &guardado, 0u);
    CHECK_STR("reiniciado arranca con la cara", "ACTIVO", rk_enlace_nombre(e.estado));

    /* 8. Se desvincula desde la app. */
    r = nube(false, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, 100u);
    CHECK_STR("desvinculado vuelve a esperar", "SIN_VINCULO", rk_enlace_nombre(e.estado));
    CHECK_INT("con el QR", RK_PANT_QR, rk_enlace_pantalla(&e));
    CHECK_INT("con codigo nuevo", (long)guardado.epoca + 1, (long)e.nvs.epoca);
    CHECK_TRUE("y conserva el wifi", e.nvs.tiene_wifi && !e.borrar_wifi);
    CHECK_TRUE("sin portal: ya hay red", !rk_enlace_portal(&e));
}

static void test_caminos_feos(void)
{
    rk_enlace_t e;
    rk_enlace_nvs_t nvs;
    rk_enlace_nube_t r;
    uint32_t t = 0u;

    /* Contraseña mal tipeada: al tercer fallo vuelve el portal. */
    rk_enlace_iniciar(&e, NULL, t);
    rk_enlace_evento(&e, RK_EV_WIFI_GUARDADO, NULL, t);
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    CHECK_TRUE("dos fallos no alcanzan para pedir ayuda", !rk_enlace_portal(&e));
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    CHECK_TRUE("al tercero vuelve el portal", rk_enlace_portal(&e));
    CHECK_TRUE("sin olvidar la red: puede ser el router reiniciando",
               rk_enlace_quiere_wifi(&e));
    rk_enlace_evento(&e, RK_EV_WIFI_OK, NULL, t);
    CHECK_STR("si el router vuelve, sigue solo", "SIN_VINCULO", rk_enlace_nombre(e.estado));

    /* Se cae la red esperando el reclamo. */
    rk_enlace_evento(&e, RK_EV_WIFI_FALLO, NULL, t);
    CHECK_STR("sin red vuelve a conectando", "CONECTANDO", rk_enlace_nombre(e.estado));

    /* La nube contesta antes de que llegue el evento de wifi. */
    r = nube(false, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t);
    CHECK_STR("si contesto la nube, hay red", "SIN_VINCULO", rk_enlace_nombre(e.estado));

    /* Vinculado y revelado en la misma respuesta (el usuario abrió el cofre
     * mientras la maceta estaba sin red). */
    r = nube(true, true);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t);
    CHECK_STR("salta directo al despertar", "DESPERTANDO", rk_enlace_nombre(e.estado));
    CHECK_TRUE("y lo guarda", e.nvs.vinculado && e.nvs.revelado);

    /* Desvincular a mitad del despertar. */
    r = nube(false, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, t + 10u);
    CHECK_STR("desvincular corta el despertar", "SIN_VINCULO", rk_enlace_nombre(e.estado));

    /* El cofre se vuelve a cerrar desde la app. */
    memset(&nvs, 0, sizeof nvs);
    nvs.tiene_wifi = true;
    nvs.vinculado = true;
    nvs.revelado = true;
    rk_enlace_iniciar(&e, &nvs, 0u);
    r = nube(true, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, 5u);
    CHECK_STR("un cofre cerrado desde la app vuelve a dormir", "ESPERA_COFRE",
              rk_enlace_nombre(e.estado));

    /* El botón largo borra todo. */
    rk_enlace_iniciar(&e, &nvs, 0u);
    e.sucio = false;
    rk_enlace_evento(&e, RK_EV_BOTON_LARGO, NULL, 9u);
    CHECK_STR("el boton largo vuelve al principio", "SIN_WIFI", rk_enlace_nombre(e.estado));
    CHECK_TRUE("olvida la red", !e.nvs.tiene_wifi && e.borrar_wifi);
    CHECK_TRUE("y el vinculo", !e.nvs.vinculado && !e.nvs.revelado);
    CHECK_INT("con codigo nuevo", 1, (long)e.nvs.epoca);
    CHECK_TRUE("y lo guarda", e.sucio);

    /* Arranque vinculado sin cofre abierto. */
    nvs.revelado = false;
    rk_enlace_iniciar(&e, &nvs, 0u);
    CHECK_INT("vinculado sin cofre arranca dormido", RK_PANT_DORMIDA, rk_enlace_pantalla(&e));

    /* Cadencia: rápida al principio, lenta a batería, techo con fallos. */
    CHECK_INT("recien vinculado consulta rapido", RK_ENL_CONSULTA_RAPIDA_MS,
              rk_enlace_consulta_ms(&e, false, 1000u));
    CHECK_INT("a los 25 minutos a bateria afloja", RK_ENL_CONSULTA_LENTA_MS,
              rk_enlace_consulta_ms(&e, false, 25u * 60u * 1000u));
    CHECK_INT("enchufado no afloja", RK_ENL_CONSULTA_RAPIDA_MS,
              rk_enlace_consulta_ms(&e, true, 25u * 60u * 1000u));
    rk_enlace_evento(&e, RK_EV_NUBE_FALLO, NULL, 0u);
    rk_enlace_evento(&e, RK_EV_NUBE_FALLO, NULL, 0u);
    CHECK_INT("dos fallos de nube cuadruplican la espera", RK_ENL_CONSULTA_RAPIDA_MS * 4u,
              rk_enlace_consulta_ms(&e, true, 0u));
    for (t = 0u; t < 20u; t++) {
        rk_enlace_evento(&e, RK_EV_NUBE_FALLO, NULL, 0u);
    }
    CHECK_INT("pero con techo", RK_ENL_CONSULTA_TECHO_MS, rk_enlace_consulta_ms(&e, true, 0u));
    r = nube(true, false);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, &r, 0u);
    CHECK_INT("una respuesta buena resetea la espera", RK_ENL_CONSULTA_RAPIDA_MS,
              rk_enlace_consulta_ms(&e, true, 0u));

    /* NULL por todos lados. */
    rk_enlace_iniciar(NULL, NULL, 0u);
    rk_enlace_evento(NULL, RK_EV_TICK, NULL, 0u);
    rk_enlace_evento(&e, RK_EV_NUBE_OK, NULL, 0u);
    CHECK_INT("sin enlace se muestra el QR", RK_PANT_QR, rk_enlace_pantalla(NULL));
    CHECK_TRUE("sin enlace no hay portal ni wifi",
               !rk_enlace_portal(NULL) && !rk_enlace_quiere_wifi(NULL));
    CHECK_STR("un estado invalido tiene nombre", "?", rk_enlace_nombre((rk_enlace_estado_t)99));
}

void suite_enlace(void)
{
    RK_SUITE("identidad y vinculo");
    test_sha256();
    test_codigo();
    test_flujo_completo();
    test_caminos_feos();
    RK_SUITE_END();
}

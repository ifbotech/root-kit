/* Actualizarse por aire y pasar por fábrica.
 *
 * Lo que protege esta suite: que un manifiesto raro nunca haga que la maceta
 * baje algo que no debe, que una versión que falla no la deje reiniciando
 * para siempre, y que la línea de fábrica no le cambie la identidad a un
 * aparato con cualquier cosa que llegue por el puerto serie.
 */
#include "rk_test.h"
#include "../core/ota.h"
#include "../core/fabrica.h"
#include "../net/nube.h"

#define FIRMA_B64 "MEUCIQADChEYHyYtNDtCSVBXXmVsc3qBiI+WnaSrsrnAx87V3AIgAQYLEBUaHyQpLjM4PUJHTFFWW2Blam90eX6DiI2Sl5w="
#define SHA_HEX   "189ca7f3ff5335190ea4ecedaaad8e9613c8165bf99d563a82b1033af59c0e37"

#define MANIFIESTO(version, url, sha, firma, tamano)                                   \
    "{\"ok\":true,\"firmware\":{\"version\":\"" version "\",\"url\":\"" url "\","     \
    "\"sha256\":\"" sha "\",\"firma\":\"" firma "\",\"tamano\":" tamano "}}"

static void test_versiones(void)
{
    CHECK_TRUE("0.5.0 es una version", rk_version_valida("0.5.0"));
    CHECK_TRUE("con sufijo tambien", rk_version_valida("0.6.0-beta.2"));
    CHECK_TRUE("dos numeros no", !rk_version_valida("0.5"));
    CHECK_TRUE("letras no", !rk_version_valida("v0.5.0"));
    CHECK_TRUE("vacia no", !rk_version_valida(""));
    CHECK_TRUE("NULL no", !rk_version_valida(NULL));
    CHECK_TRUE("un sufijo vacio no", !rk_version_valida("1.0.0-"));
    CHECK_TRUE("un sufijo con espacios no", !rk_version_valida("1.0.0-be ta"));
    CHECK_TRUE("una larguisima no entra", !rk_version_valida("1.0.0-abcdefghijklmnop"));
    CHECK_TRUE("numeros gigantes no", !rk_version_valida("1234567.0.0"));

    CHECK_TRUE("0.5.0 < 0.6.0", rk_version_cmp("0.5.0", "0.6.0") < 0);
    CHECK_TRUE("0.10.0 > 0.9.9 (no es texto)", rk_version_cmp("0.10.0", "0.9.9") > 0);
    CHECK_INT("iguales", 0, rk_version_cmp("1.2.3", "1.2.3"));
    CHECK_INT("el sufijo no cuenta", 0, rk_version_cmp("1.2.3-beta", "1.2.3"));
    CHECK_TRUE("una invalida vale cero", rk_version_cmp("basura", "0.0.1") < 0);
}

static void test_codificaciones(void)
{
    uint8_t out[40];

    CHECK_TRUE("hex de 32 bytes", rk_hex_decodificar(SHA_HEX, out, 32));
    CHECK_HEX("primer byte", 0x18, out[0]);
    CHECK_HEX("ultimo byte", 0x37, out[31]);
    CHECK_TRUE("mayusculas tambien", rk_hex_decodificar("AB0f", out, 2) && out[0] == 0xAB && out[1] == 0x0F);
    CHECK_TRUE("corto no", !rk_hex_decodificar("abc", out, 2));
    CHECK_TRUE("largo no", !rk_hex_decodificar("abcdef", out, 2));
    CHECK_TRUE("con basura no", !rk_hex_decodificar("zz00", out, 2));
    CHECK_TRUE("NULL no", !rk_hex_decodificar(NULL, out, 2));

    CHECK_INT("base64 con dos de relleno", 4, (long)rk_base64_decodificar("aG9sYQ==", out, sizeof out));
    CHECK_TRUE("dice hola", out[0] == 'h' && out[1] == 'o' && out[2] == 'l' && out[3] == 'a');
    CHECK_INT("con uno de relleno", 2, (long)rk_base64_decodificar("aG8=", out, sizeof out));
    CHECK_INT("sin relleno", 2, (long)rk_base64_decodificar("aG8", out, sizeof out));
    CHECK_INT("un caracter raro lo invalida", 0, (long)rk_base64_decodificar("aG9s*Q==", out, sizeof out));
    CHECK_INT("tres de relleno no", 0, (long)rk_base64_decodificar("aA===", out, sizeof out));
    CHECK_INT("texto despues del relleno no", 0, (long)rk_base64_decodificar("aA==aA", out, sizeof out));
    CHECK_INT("si no entra, cero", 0, (long)rk_base64_decodificar("aG9sYQ==", out, 3));
    {
        uint8_t justo[5] = { 0, 0, 0, 0, 0xEE };
        CHECK_INT("entra justo", 4, (long)rk_base64_decodificar("aG9sYQ==", justo, 4));
        CHECK_HEX("y no pisa el byte de al lado", 0xEE, justo[4]);
    }
}

static void test_manifiesto(void)
{
    rk_ota_manifiesto_t m;
    rk_nube_resp_t r;

    CHECK_TRUE("un manifiesto completo se entiende",
               rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://ifbotech.com/rootkit/api/d/firmware/0.6.0-c3.bin",
                                                    SHA_HEX, FIRMA_B64, "1048576"), &m));
    CHECK_STR("version", "0.6.0", m.version);
    CHECK_INT("tamano", 1048576, (long)m.tamano);
    CHECK_INT("la firma DER mide 71", 71, m.firma_len);
    CHECK_HEX("empieza como una secuencia DER", 0x30, m.firma[0]);
    CHECK_HEX("el hash quedo en bytes", 0x18, m.sha256[0]);

    CHECK_TRUE("y llega por el sync",
               rk_nube_parsear(MANIFIESTO("0.6.0", "https://x/f.bin", SHA_HEX, FIRMA_B64, "500000"), &r)
               && r.hay_firmware && r.firmware.tamano == 500000u);

    CHECK_TRUE("sin objeto firmware no hay nada", !rk_ota_manifiesto_parsear("{\"ok\":true}", &m) && m.version[0] == '\0');
    CHECK_TRUE("una version rara no", !rk_ota_manifiesto_parsear(MANIFIESTO("ultima", "https://x/f", SHA_HEX, FIRMA_B64, "10"), &m));
    CHECK_TRUE("una url que no es http no", !rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "ftp://x/f", SHA_HEX, FIRMA_B64, "10"), &m));
    CHECK_TRUE("un hash corto no", !rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://x/f", "abcd", FIRMA_B64, "10"), &m));
    CHECK_TRUE("una firma diminuta no", !rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://x/f", SHA_HEX, "aG9sYQ==", "10"), &m));
    CHECK_TRUE("sin tamano no", !rk_ota_manifiesto_parsear(
               "{\"firmware\":{\"version\":\"0.6.0\",\"url\":\"https://x/f\",\"sha256\":\"" SHA_HEX "\",\"firma\":\"" FIRMA_B64 "\"}}", &m));
    CHECK_TRUE("uno que no entra en la particion no",
               !rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://x/f", SHA_HEX, FIRMA_B64, "3000000"), &m));
    CHECK_TRUE("un tamano negativo no",
               !rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://x/f", SHA_HEX, FIRMA_B64, "-5"), &m));
    CHECK_TRUE("y tras un fallo el manifiesto queda en cero", m.url[0] == '\0' && m.firma_len == 0u && m.tamano == 0u);
    CHECK_TRUE("NULL no explota", !rk_ota_manifiesto_parsear(NULL, &m) && !rk_ota_manifiesto_parsear("{}", NULL));
    CHECK_TRUE("un manifiesto malo no impide entender el resto del sync",
               rk_nube_parsear("{\"ok\":true,\"vinculado\":true,\"firmware\":{\"version\":\"x\"}}", &r)
               && r.vinculado && !r.hay_firmware);
}

static void test_decidir(void)
{
    rk_ota_manifiesto_t m;
    rk_ota_nvs_t nvs;
    int i;

    memset(&nvs, 0, sizeof nvs);
    rk_ota_manifiesto_parsear(MANIFIESTO("0.6.0", "https://x/f.bin", SHA_HEX, FIRMA_B64, "900000"), &m);

    CHECK_INT("enchufado y con version nueva: adelante", RK_OTA_ADELANTE,
              rk_ota_decidir(&nvs, &m, "0.5.0", true, 0));
    CHECK_INT("a bateria con carga: adelante", RK_OTA_ADELANTE,
              rk_ota_decidir(&nvs, &m, "0.5.0", false, 3900));
    CHECK_INT("a bateria y baja: otro dia", RK_OTA_SIN_BATERIA,
              rk_ota_decidir(&nvs, &m, "0.5.0", false, 3500));
    CHECK_INT("sin lectura de bateria no se frena", RK_OTA_ADELANTE,
              rk_ota_decidir(&nvs, &m, "0.5.0", false, 0));
    CHECK_INT("ya corre esa", RK_OTA_MISMA_VERSION, rk_ota_decidir(&nvs, &m, "0.6.0", true, 0));
    CHECK_INT("una mas vieja tambien se instala: la nube manda (volver atras)", RK_OTA_ADELANTE,
              rk_ota_decidir(&nvs, &m, "0.7.0", true, 0));
    CHECK_INT("sin manifiesto", RK_OTA_NO_HAY, rk_ota_decidir(&nvs, NULL, "0.5.0", true, 0));
    {
        rk_ota_manifiesto_t vacio;
        memset(&vacio, 0, sizeof vacio);
        CHECK_INT("manifiesto en cero", RK_OTA_NO_HAY, rk_ota_decidir(&nvs, &vacio, "0.5.0", true, 0));
        vacio = m;
        vacio.firma_len = 0u;
        CHECK_INT("sin firma no se baja nada", RK_OTA_INVALIDA, rk_ota_decidir(&nvs, &vacio, "0.5.0", true, 0));
    }

    /* Tres intentos de la misma versión y basta. */
    for (i = 0; i < RK_OTA_INTENTOS_MAX; i++) {
        CHECK_INT("todavia se puede intentar", RK_OTA_ADELANTE, rk_ota_decidir(&nvs, &m, "0.5.0", true, 0));
        rk_ota_marcar_intento(&nvs, m.version);
    }
    CHECK_INT("conto los intentos", RK_OTA_INTENTOS_MAX, nvs.intentos);
    CHECK_INT("la cuarta vez ya no", RK_OTA_AGOTADA, rk_ota_decidir(&nvs, &m, "0.5.0", true, 0));
    {
        rk_ota_manifiesto_t otra = m;
        strcpy(otra.version, "0.6.1");
        CHECK_INT("pero una version distinta si", RK_OTA_ADELANTE, rk_ota_decidir(&nvs, &otra, "0.5.0", true, 0));
        rk_ota_marcar_intento(&nvs, otra.version);
        CHECK_INT("y la cuenta arranca de nuevo", 1, nvs.intentos);
        CHECK_STR("con su version", "0.6.1", nvs.version);
    }
    CHECK_STR("los motivos tienen nombre", "agotada", rk_ota_decision_nombre(RK_OTA_AGOTADA));
}

static void test_arranque(void)
{
    rk_ota_nvs_t nvs;

    memset(&nvs, 0, sizeof nvs);
    CHECK_TRUE("un arranque comun no verifica nada", !rk_ota_arranque(&nvs, "0.5.0"));

    rk_ota_marcar_intento(&nvs, "0.6.0");
    nvs.verificar = true;                       /* lo que se guarda antes de reiniciar */
    CHECK_TRUE("arranco la nueva: hay que confirmarla", rk_ota_arranque(&nvs, "0.6.0"));
    CHECK_TRUE("y sigue pendiente hasta hablar con la nube", nvs.verificar);
    rk_ota_confirmar(&nvs);
    CHECK_TRUE("la nube contesto: confirmada", !nvs.verificar && nvs.intentos == 0u);
    CHECK_TRUE("y ya no hay nada que verificar", !rk_ota_arranque(&nvs, "0.6.0"));

    rk_ota_marcar_intento(&nvs, "0.7.0");
    nvs.verificar = true;
    CHECK_TRUE("arranco la vieja: el gestor de arranque volvio atras", !rk_ota_arranque(&nvs, "0.6.0"));
    CHECK_TRUE("se limpia la bandera", !nvs.verificar);
    CHECK_INT("pero el intento queda contado", 1, nvs.intentos);
    CHECK_TRUE("NULL no explota", !rk_ota_arranque(NULL, "0.6.0"));
    rk_ota_confirmar(NULL);
    rk_ota_marcar_intento(NULL, "1.0.0");
}

static void test_cuerpo_con_ota(void)
{
    char buf[1024];
    rk_nube_yo_t yo;
    uint16_t n;

    memset(&yo, 0, sizeof yo);
    yo.id = "A1B2C3D4E5F6";
    yo.fw = "0.6.0";
    yo.placa = "c3-supermini";
    yo.pantalla = "st7735-128";
    yo.estado = "ACTIVO";
    CHECK_TRUE("sin ota ni lote el cuerpo no los nombra",
               rk_nube_armar_sync(buf, sizeof buf, &yo, NULL, 0, &n) > 0u &&
               strstr(buf, "\"ota\"") == NULL && strstr(buf, "\"lote\"") == NULL);
    yo.lote = "L2609";
    yo.ota_version = "0.6.0";
    yo.ota_estado = "ok";
    CHECK_TRUE("con ota y lote, van", rk_nube_armar_sync(buf, sizeof buf, &yo, NULL, 0, &n) > 0u);
    CHECK_TRUE("el lote", strstr(buf, "\"lote\":\"L2609\"") != NULL);
    CHECK_TRUE("y el estado de la actualizacion",
               strstr(buf, "\"ota\":{\"version\":\"0.6.0\",\"estado\":\"ok\"}") != NULL);
}

static void test_fabrica(void)
{
    rk_fabrica_orden_t o;
    char buf[256];
    rk_fabrica_estado_t e;

    CHECK_TRUE("una orden completa",
               rk_fabrica_parsear("FABRICA {\"secreto\":\"3a917c05ee4218b69d602fc3710e845b\",\"persona\":\"musgo\",\"lote\":\"L2609\"}", &o));
    CHECK_TRUE("no es consulta", !o.consulta);
    CHECK_HEX("el secreto en bytes", 0x3A, o.secreto[0]);
    CHECK_HEX("hasta el final", 0x5B, o.secreto[15]);
    CHECK_STR("la persona", "musgo", o.persona);
    CHECK_STR("el lote", "L2609", o.lote);

    CHECK_TRUE("sin lote tambien vale",
               rk_fabrica_parsear("FABRICA {\"secreto\":\"3a917c05ee4218b69d602fc3710e845b\",\"persona\":\"brote\"}", &o)
               && o.lote[0] == '\0');
    CHECK_TRUE("la consulta", rk_fabrica_parsear("FABRICA?", &o) && o.consulta);

    CHECK_TRUE("un Rooti que no existe no",
               !rk_fabrica_parsear("FABRICA {\"secreto\":\"3a917c05ee4218b69d602fc3710e845b\",\"persona\":\"kawaii\"}", &o));
    CHECK_TRUE("un secreto corto no",
               !rk_fabrica_parsear("FABRICA {\"secreto\":\"3a917c05\",\"persona\":\"brote\"}", &o));
    CHECK_TRUE("un secreto en ceros no",
               !rk_fabrica_parsear("FABRICA {\"secreto\":\"00000000000000000000000000000000\",\"persona\":\"brote\"}", &o));
    CHECK_TRUE("un lote con cosas raras no",
               !rk_fabrica_parsear("FABRICA {\"secreto\":\"3a917c05ee4218b69d602fc3710e845b\",\"persona\":\"brote\",\"lote\":\"L 1;\"}", &o));
    CHECK_TRUE("y tras un fallo la orden queda en cero", o.persona[0] == '\0' && o.secreto[0] == 0u);
    CHECK_TRUE("una linea del log no es una orden", !rk_fabrica_parsear("[medir] suelo 40%", &o));
    CHECK_TRUE("ni reconocerla como tal", !rk_fabrica_es_orden("[enlace] FABRICA"));
    CHECK_TRUE("sin JSON no", !rk_fabrica_parsear("FABRICA hola", &o));
    CHECK_TRUE("NULL no explota", !rk_fabrica_parsear(NULL, &o) && !rk_fabrica_parsear("FABRICA?", NULL));

    memset(&e, 0, sizeof e);
    e.id = "A1B2C3D4E5F6";
    e.persona = "musgo";
    e.lote = "L2609";
    e.codigo = "K7Q2M9XA";
    e.fw = "0.6.0";
    CHECK_TRUE("la respuesta se arma", rk_fabrica_respuesta(buf, sizeof buf, &e) > 0u);
    CHECK_STR("y es JSON de una linea",
              "{\"fabrica\":true,\"id\":\"A1B2C3D4E5F6\",\"persona\":\"musgo\",\"lote\":\"L2609\","
              "\"codigo\":\"K7Q2M9XA\",\"fw\":\"0.6.0\",\"vinculado\":false}", buf);
    CHECK_TRUE("el error tambien", rk_fabrica_error(buf, sizeof buf, "vinculado") > 0u);
    CHECK_STR("con su motivo", "{\"fabrica\":false,\"error\":\"vinculado\"}", buf);
    CHECK_INT("sin lugar devuelve cero", 0, (long)rk_fabrica_respuesta(buf, 10, &e));
}

void suite_ota(void)
{
    RK_SUITE("ota y fabrica");
    test_versiones();
    test_codificaciones();
    test_manifiesto();
    test_decidir();
    test_arranque();
    test_cuerpo_con_ota();
    test_fabrica();
    RK_SUITE_END();
}

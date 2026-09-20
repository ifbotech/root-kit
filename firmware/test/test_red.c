/* La conversación con la nube: JSON y el contrato de /api/d/sync.
 *
 * Lo que protege esta suite es que una respuesta rara nunca le haga daño a
 * la maceta. Un portal de hotel que contesta HTML, un proxy que corta el
 * cuerpo a la mitad o un servidor con un bug no pueden desvincularla, ni
 * cargarle umbrales absurdos, ni leer fuera del buffer.
 */
#include <stddef.h>
#include "rk_test.h"
#include "../net/json.h"
#include "../net/nube.h"
#include "../core/mood.h"
#include "../core/persona.h"

static void test_json_escritura(void)
{
    char buf[256];
    rk_jw_t w;

    rk_jw_iniciar(&w, buf, sizeof buf);
    rk_jw_obj(&w);
    rk_jw_clave(&w, "a");  rk_jw_entero(&w, 1);
    rk_jw_clave(&w, "b");  rk_jw_texto(&w, "dos \"comillas\"\n");
    rk_jw_clave(&w, "c");
    rk_jw_arr(&w);
    rk_jw_entero(&w, -40);
    rk_jw_bool(&w, true);
    rk_jw_nulo(&w);
    rk_jw_obj(&w);
    rk_jw_fin_obj(&w);
    rk_jw_fin_arr(&w);
    rk_jw_clave(&w, "d");  rk_jw_entero(&w, -2147483647L - 1L);
    rk_jw_fin_obj(&w);
    CHECK_TRUE("el documento quedo bien formado", rk_jw_terminar(&w));
    CHECK_STR("con comas, escapes y anidado",
              "{\"a\":1,\"b\":\"dos \\\"comillas\\\"\\n\",\"c\":[-40,true,null,{}],\"d\":-2147483648}",
              buf);

    /* Sin lugar: marca error y no escribe de más. */
    {
        char chico[12];
        chico[11] = 'Z';
        rk_jw_iniciar(&w, chico, 11);
        rk_jw_obj(&w);
        rk_jw_clave(&w, "una clave larga");
        rk_jw_entero(&w, 12345);
        rk_jw_fin_obj(&w);
        CHECK_TRUE("sin lugar avisa", !rk_jw_terminar(&w));
        CHECK_TRUE("y no pisa el byte de al lado", chico[11] == 'Z');
    }
    /* Desbalanceado. */
    rk_jw_iniciar(&w, buf, sizeof buf);
    rk_jw_obj(&w);
    CHECK_TRUE("un objeto sin cerrar no termina bien", !rk_jw_terminar(&w));
    rk_jw_iniciar(&w, buf, sizeof buf);
    rk_jw_fin_obj(&w);
    CHECK_TRUE("cerrar de mas tampoco", !rk_jw_terminar(&w));
}

static void test_json_lectura(void)
{
    static const char *J =
        " { \"ok\" : true, \"lista\": [1, {\"x\": \"}\"}, [2,3]],"
        " \"texto\": \"con \\\"escape\\\" y \\u00e1\","
        " \"n\": -42, \"dec\": 12.75, \"nada\": null,"
        " \"especie\": { \"id\": \"monstera\", \"suelo_min\": 25,"
        "   \"sub\": {\"hondo\": false} } }";
    long v;
    bool b;
    char s[64];

    CHECK_TRUE("lee un bool", rk_json_bool(J, "ok", &b) && b);
    CHECK_TRUE("lee un negativo", rk_json_entero(J, "n", &v) && v == -42);
    CHECK_TRUE("trunca decimales", rk_json_entero(J, "dec", &v) && v == 12);
    CHECK_TRUE("entra en objetos", rk_json_entero(J, "especie.suelo_min", &v) && v == 25);
    CHECK_TRUE("y en objetos dentro de objetos",
               rk_json_bool(J, "especie.sub.hondo", &b) && !b);
    CHECK_TRUE("lee texto", rk_json_texto(J, "especie.id", s, sizeof s));
    CHECK_STR("monstera", "monstera", s);
    CHECK_TRUE("con escapes", rk_json_texto(J, "texto", s, sizeof s));
    CHECK_STR("resuelve comillas y reemplaza unicode", "con \"escape\" y ?", s);
    CHECK_TRUE("trunca sin desbordar", rk_json_texto(J, "texto", s, 4));
    CHECK_STR("tres caracteres", "con", s);
    CHECK_TRUE("saltea listas con llaves adentro de cadenas",
               rk_json_entero(J, "n", &v));
    CHECK_TRUE("null no esta", !rk_json_hay(J, "nada"));
    CHECK_TRUE("una clave que no existe no esta", !rk_json_hay(J, "fantasma"));
    CHECK_TRUE("un prefijo de clave no es la clave", !rk_json_hay(J, "especie.suelo"));
    CHECK_TRUE("un texto no es un numero", !rk_json_entero(J, "especie.id", &v));
    CHECK_TRUE("un numero no es un bool", !rk_json_bool(J, "n", &b));
    CHECK_TRUE("no se entra en una lista", !rk_json_hay(J, "lista.x"));

    /* Basura y documentos cortados: nunca leen fuera ni se cuelgan. */
    CHECK_TRUE("html no es json", !rk_json_hay("<html><body>hotel</body></html>", "ok"));
    CHECK_TRUE("cortado a mitad de cadena", !rk_json_hay("{\"a\":\"sin fin", "b"));
    CHECK_TRUE("cortado a mitad de objeto", !rk_json_entero("{\"a\":{\"b\":1", "c", &v));
    CHECK_TRUE("clave sin dos puntos", !rk_json_hay("{\"a\" 1}", "a"));
    CHECK_TRUE("vacio", !rk_json_hay("", "a"));
    CHECK_TRUE("NULL", !rk_json_hay(NULL, "a"));
    {
        /* Anidamiento hostil: más profundo que el límite. */
        char hondo[256];
        int i, k = 0;
        k += sprintf(hondo + k, "{\"x\":");
        for (i = 0; i < 40; i++) { hondo[k++] = '['; }
        for (i = 0; i < 40; i++) { hondo[k++] = ']'; }
        k += sprintf(hondo + k, ",\"y\":1}");
        CHECK_TRUE("un anidamiento hostil se rechaza sin romper",
                   !rk_json_entero(hondo, "y", &v));
    }
}

static void test_armar_sync(void)
{
    rk_historial_t h;
    rk_telemetry_t t;
    rk_nube_yo_t yo;
    char buf[2048];
    uint16_t incl = 99u;
    size_t n;
    long v;
    int i;

    memset(&yo, 0, sizeof yo);
    yo.id = "A1B2C3D4E5F6";
    yo.fw = "0.5.0";
    yo.placa = "c3-supermini";
    yo.pantalla = "st7735-128";
    yo.persona = "kip";
    yo.codigo = "K7Q2M9XA";
    yo.estado = "SIN_VINCULO";
    yo.epoca = 3u;
    yo.reloj_s = 10000u;
    yo.rssi = -61;
    yo.bat_mv = 3920u;

    rk_historial_iniciar(&h);
    memset(&t, 0, sizeof t);
    t.valid = true;
    t.soil_pct = 38;
    t.temp_dc = 232;
    t.rh_pct = 55;
    t.lux = 4800;
    t.batt_mv = 3920;
    t.suelo_dc = RK_TEMP_NO_HAY;
    t.fallas = RK_FALLA_SONDA;
    for (i = 0; i < 5; i++) {
        rk_registro_t r = rk_registro_desde(&t, 10000u - 900u * (uint32_t)(4 - i),
                                            RK_MOOD_HAPPY, i == 4 ? RK_SEV_URGENT : RK_SEV_OK);
        rk_historial_agregar(&h, &r);
    }

    n = rk_nube_armar_sync(buf, sizeof buf, &yo, &h, 3u, &incl);
    CHECK_TRUE("arma el cuerpo", n > 0u && n == strlen(buf));
    CHECK_TRUE("sin escurrimiento no manda la bandera", strstr(buf, "escurre") == NULL);
    {
        rk_historial_t h2;
        rk_registro_t r2;
        char buf2[1024];
        rk_historial_iniciar(&h2);
        t.fallas = RK_FALLA_SONDA | RK_FALLA_ESCURRE;
        r2 = rk_registro_desde(&t, 10000u, RK_MOOD_THIRSTY, RK_SEV_URGENT);
        rk_historial_agregar(&h2, &r2);
        CHECK_TRUE("con escurrimiento arma igual", rk_nube_armar_sync(buf2, sizeof buf2, &yo, &h2, 1u, NULL) > 0u);
        CHECK_TRUE("y lo dice con nombre", strstr(buf2, "\"escurre\":true") != NULL);
        CHECK_TRUE("y en las fallas", strstr(buf2, "\"fallas\":24") != NULL);
        t.fallas = RK_FALLA_SONDA;
    }
    CHECK_INT("incluye hasta el maximo pedido", 3, incl);
    CHECK_TRUE("con el codigo mientras no esta vinculado",
               rk_json_hay(buf, "codigo"));
    CHECK_TRUE("con la epoca", rk_json_entero(buf, "epoca", &v) && v == 3);
    CHECK_TRUE("la primera lectura es la mas vieja",
               strstr(buf, "\"hace\":3600") != NULL);
    CHECK_TRUE("sin sonda no manda temperatura de tierra",
               strstr(buf, "tsuelo") == NULL);
    CHECK_TRUE("manda el animo por nombre", strstr(buf, "\"animo\":\"HAPPY\"") != NULL);
    CHECK_TRUE("y la severidad", strstr(buf, "\"sev\":\"OK\"") != NULL);

    yo.codigo = NULL;
    n = rk_nube_armar_sync(buf, sizeof buf, &yo, &h, 10u, &incl);
    CHECK_TRUE("la urgente viaja como URGENT", strstr(buf, "\"sev\":\"URGENT\"") != NULL);
    CHECK_TRUE("vinculado no manda el codigo", n > 0u && !rk_json_hay(buf, "codigo"));
    CHECK_INT("todas las pendientes si entran", 5, incl);

    n = rk_nube_armar_sync(buf, 120u, &yo, &h, 10u, &incl);
    CHECK_INT("si no entra devuelve 0", 0, (long)n);
    CHECK_INT("y no da nada por incluido", 0, incl);

    n = rk_nube_armar_sync(buf, sizeof buf, &yo, NULL, 10u, &incl);
    CHECK_TRUE("sin historial manda sin lecturas", n > 0u && strstr(buf, "\"lecturas\":[]"));
    CHECK_INT("sin yo no hay cuerpo", 0, (long)rk_nube_armar_sync(buf, sizeof buf, NULL, &h, 1u, &incl));
}

static void test_parsear(void)
{
    static const char *BUENA =
        "{\"ok\":true,\"vinculado\":true,\"revelado\":true,\"persona\":\"blink\","
        "\"rareza\":\"epico\",\"nombre\":\"Rulo\",\"especie\":{\"id\":\"monstera\",\"nombre\":\"Monstera deliciosa\","
        "\"suelo_min\":25,\"suelo_max\":60,\"temp_min\":180,\"temp_max\":300,"
        "\"hr_min\":50,\"lux_min\":1000,\"lux_max\":15000,\"dificultad\":45},"
        "\"intervalo_s\":900,\"aceptadas\":12,\"hora\":1758040000,"
        "\"calibracion\":{\"seco\":3100,\"mojado\":1300},\"brillo\":80,\"pantalla\":\"siempre\",\"vinculo\":{\"dias_sanos\":34,\"dias_vividos\":40,\"racha\":8,\"mejor_racha\":19}}";
    rk_nube_resp_t r;
    const rk_species_t *sp;

    CHECK_TRUE("una respuesta buena se entiende", rk_nube_parsear(BUENA, &r));
    CHECK_TRUE("vinculado y revelado", r.vinculado && r.revelado);
    CHECK_STR("persona", "blink", r.persona);
    CHECK_TRUE("con la piel que salio del cofre", r.hay_rareza && r.rareza == RK_RAREZA_EPICA);
    CHECK_STR("nombre", "Rulo", r.nombre);
    CHECK_TRUE("con especie", r.hay_especie);
    sp = &r.especie.sp;
    CHECK_STR("la especie apunta a su propio texto", "monstera", sp->id);
    CHECK_INT("suelo min", 25, sp->soil_min);
    CHECK_INT("temp max en decimas", 300, sp->temp_max_dc);
    CHECK_INT("lux max", 15000, (long)sp->lux_max);
    CHECK_INT("intervalo", 900, (long)r.intervalo_s);
    CHECK_INT("aceptadas", 12, r.aceptadas);
    CHECK_TRUE("calibracion", r.hay_calibracion && r.cal.dry_raw == 3100u);
    CHECK_INT("brillo", 80, r.brillo);
    CHECK_TRUE("pantalla siempre encendida", r.pantalla_siempre);
    CHECK_TRUE("con vinculo", r.hay_vinculo && r.vinculo.dias_sanos == 34u &&
               r.vinculo.racha == 8u && r.vinculo.mejor_racha == 19u);

    /* Copiar rompe los punteros; rk_especie_ver los arregla. */
    {
        rk_especie_guardada_t copia = r.especie;
        memset(&r, 0, sizeof r);
        sp = rk_especie_ver(&copia);
        CHECK_STR("despues de copiar la especie sigue legible", "Monstera deliciosa", sp->nombre);
    }

    CHECK_TRUE("ok false no se aplica", !rk_nube_parsear("{\"ok\":false,\"vinculado\":false}", &r));
    CHECK_TRUE("sin ok no se aplica: un portal cautivo no desvincula",
               !rk_nube_parsear("{\"vinculado\":false}", &r));
    CHECK_TRUE("html no se aplica", !rk_nube_parsear("<html>login del hotel</html>", &r));
    CHECK_TRUE("NULL no se aplica", !rk_nube_parsear(NULL, &r));

    CHECK_TRUE("una especie incoherente se descarta",
               rk_nube_parsear("{\"ok\":true,\"vinculado\":true,\"especie\":{\"suelo_min\":70,"
                               "\"suelo_max\":20,\"temp_min\":1,\"temp_max\":2,\"hr_min\":1,"
                               "\"lux_min\":1,\"lux_max\":2}}", &r) && !r.hay_especie);
    CHECK_TRUE("una especie a medias se descarta",
               rk_nube_parsear("{\"ok\":true,\"especie\":{\"suelo_min\":20}}", &r) && !r.hay_especie);
    CHECK_TRUE("un intervalo absurdo se acota",
               rk_nube_parsear("{\"ok\":true,\"intervalo_s\":1}", &r) && r.intervalo_s == 60u);
    CHECK_TRUE("una calibracion invertida no se aplica",
               rk_nube_parsear("{\"ok\":true,\"calibracion\":{\"seco\":1000,\"mojado\":3000}}", &r)
               && !r.hay_calibracion);
    CHECK_TRUE("sin campos, todo en falso",
               rk_nube_parsear("{\"ok\":true}", &r) && !r.vinculado && !r.revelado &&
               r.persona[0] == '\0' && !r.hay_rareza && r.intervalo_s == 0u);
    CHECK_TRUE("una rareza desconocida no cambia la piel",
               rk_nube_parsear("{\"ok\":true,\"revelado\":true,\"rareza\":\"legendaria\"}", &r)
               && !r.hay_rareza);
    CHECK_TRUE("sin firmware ni calibracion en curso, nada de eso",
               rk_nube_parsear("{\"ok\":true}", &r) && !r.hay_firmware && !r.calibrando);
    CHECK_TRUE("la app calibrando llega",
               rk_nube_parsear("{\"ok\":true,\"calibrando\":true}", &r) && r.calibrando);
    CHECK_TRUE("la rara llega como rara",
               rk_nube_parsear("{\"ok\":true,\"rareza\":\"raro\"}", &r)
               && r.hay_rareza && r.rareza == RK_RAREZA_RARA);
}

/* El token del aparato no sale por HTTP plano, ni a una URL que engaña. */
static void test_url_nube(void)
{
    CHECK_TRUE("la nube del producto",
               rk_nube_url_aceptable("https://ifbotech.com/rootkit", false));
    /* El transporte elige TLS mirando "https://" en minusculas: en mayusculas
     * iria por TCP plano al 443 con el token en claro. */
    CHECK_TRUE("el esquema en mayusculas NO",
               !rk_nube_url_aceptable("HTTPS://IFBOTECH.COM/ROOTKIT", false));
    CHECK_TRUE("el servidor en mayusculas si",
               rk_nube_url_aceptable("https://IFBOTECH.COM/ROOTKIT", false));
    CHECK_TRUE("con puerto",
               rk_nube_url_aceptable("https://nube.ejemplo:8443", false));

    CHECK_TRUE("HTTP plano no, en el producto",
               !rk_nube_url_aceptable("http://192.168.0.10:8080", false));
    CHECK_TRUE("HTTP plano si, en el banco",
               rk_nube_url_aceptable("http://192.168.0.10:8080", true));
    CHECK_TRUE("ni en el banco sin servidor",
               !rk_nube_url_aceptable("http://", true));

    CHECK_TRUE("con usuario no: https://ifbotech.com@otro.com va a otro.com",
               !rk_nube_url_aceptable("https://ifbotech.com@otro.com", false));
    CHECK_TRUE("sin servidor no", !rk_nube_url_aceptable("https:///rootkit", false));
    CHECK_TRUE("sin servidor, solo puerto, no", !rk_nube_url_aceptable("https://:443", false));
    CHECK_TRUE("con espacios no", !rk_nube_url_aceptable("https://ifbotech.com/root kit", false));
    CHECK_TRUE("con un salto de linea no", !rk_nube_url_aceptable("https://ifbotech.com\r\nX: y", false));
    CHECK_TRUE("con barra invertida no", !rk_nube_url_aceptable("https://ifbotech.com\\@otro", false));
    CHECK_TRUE("otro esquema no", !rk_nube_url_aceptable("ftp://ifbotech.com", true));
    CHECK_TRUE("vacia no", !rk_nube_url_aceptable("", true));
    CHECK_TRUE("NULL no", !rk_nube_url_aceptable(NULL, true));
    CHECK_TRUE("la URL entera del sync entra",
               rk_nube_url_aceptable("https://ifbotech.com/rootkit/api/d/sync", false));
    CHECK_TRUE("mas larga que los buffers del aparato no",
               !rk_nube_url_aceptable("https://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com", false));
}

void suite_red(void)
{
    RK_SUITE("nube");
    test_json_escritura();
    test_json_lectura();
    test_armar_sync();
    test_parsear();
    test_url_nube();
    RK_SUITE_END();
}

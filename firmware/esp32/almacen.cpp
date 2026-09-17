#include "almacen.h"
#include <Preferences.h>
#include <LittleFS.h>
#include <esp_random.h>

static Preferences g_nvs;
static bool g_fs = false;
static uint8_t g_hist_buf[RK_HIST_BYTES_MAX];

bool almacen_cargar(rk_almacen_t *a, const char *nube_por_defecto,
                   const char *app_por_defecto)
{
    memset(a, 0, sizeof *a);
    if (!g_nvs.begin("rootkit", false)) {
        return false;
    }

    if (g_nvs.getBytes("secreto", a->secreto, RK_SECRETO_LEN) != RK_SECRETO_LEN ||
        !rk_secreto_valido(a->secreto)) {
        /* Placa sin pasar por fábrica: se genera un secreto propio. Es tan
         * bueno como el de fábrica; lo único que falta es que la nube lo
         * conozca de antemano, y en desarrollo la nube acepta el primero que
         * se presenta (ver docs/nube.md, "confianza al primer uso"). */
        esp_fill_random(a->secreto, RK_SECRETO_LEN);
        g_nvs.putBytes("secreto", a->secreto, RK_SECRETO_LEN);
    }

    g_nvs.getString("persona", a->persona, sizeof a->persona);
    a->rareza = g_nvs.getUChar("rareza", 0);
    g_nvs.getBytes("enlace", &a->enlace, sizeof a->enlace);
    g_nvs.getString("ssid", a->ssid, sizeof a->ssid);
    g_nvs.getString("clave", a->clave, sizeof a->clave);
    if (g_nvs.getString("nube", a->nube, sizeof a->nube) == 0) {
        strncpy(a->nube, nube_por_defecto, sizeof a->nube - 1);
    }
    if (g_nvs.getString("app", a->app, sizeof a->app) == 0 && app_por_defecto != NULL) {
        strncpy(a->app, app_por_defecto, sizeof a->app - 1);
    }
    a->hay_especie = g_nvs.getBytes("especie", &a->especie, sizeof a->especie) == sizeof a->especie;
    if (a->hay_especie) {
        rk_especie_ver(&a->especie);
    }
    if (g_nvs.getBytes("cal", &a->cal, sizeof a->cal) != sizeof a->cal || !rk_soil_cal_valid(&a->cal)) {
        a->cal = RK_SOIL_CAL_DEFAULT;
    }
    if (g_nvs.getBytes("vinculo", &a->vinculo, sizeof a->vinculo) != sizeof a->vinculo) {
        rk_bond_init(&a->vinculo);
    }
    g_nvs.getString("nombre", a->nombre, sizeof a->nombre);
    a->brillo = g_nvs.getUChar("brillo", 80);
    a->pantalla_siempre = g_nvs.getBool("siempre", false);
    a->arranques = g_nvs.getUInt("arranques", 0) + 1;
    g_nvs.putUInt("arranques", a->arranques);

    /* La red guardada manda sobre la bandera: si alguien borró la clave a
     * mano, no hay wifi que intentar. */
    a->enlace.tiene_wifi = a->enlace.tiene_wifi && a->ssid[0] != '\0';

    g_fs = LittleFS.begin(true);
    return true;
}

void almacen_guardar_enlace(const rk_almacen_t *a)
{
    g_nvs.putBytes("enlace", &a->enlace, sizeof a->enlace);
}

void almacen_guardar_wifi(const rk_almacen_t *a)
{
    g_nvs.putString("ssid", a->ssid);
    g_nvs.putString("clave", a->clave);
}

void almacen_borrar_wifi(rk_almacen_t *a)
{
    a->ssid[0] = '\0';
    a->clave[0] = '\0';
    g_nvs.remove("ssid");
    g_nvs.remove("clave");
}

void almacen_guardar_nube(const rk_almacen_t *a)
{
    g_nvs.putString("nube", a->nube);
}

void almacen_guardar_config(const rk_almacen_t *a)
{
    g_nvs.putString("persona", a->persona);
    g_nvs.putUChar("rareza", a->rareza);
    if (a->hay_especie) {
        g_nvs.putBytes("especie", &a->especie, sizeof a->especie);
    } else {
        g_nvs.remove("especie");
    }
    g_nvs.putBytes("cal", &a->cal, sizeof a->cal);
    g_nvs.putString("nombre", a->nombre);
    g_nvs.putUChar("brillo", a->brillo);
    g_nvs.putBool("siempre", a->pantalla_siempre);
}

void almacen_guardar_vinculo(const rk_almacen_t *a)
{
    g_nvs.putBytes("vinculo", &a->vinculo, sizeof a->vinculo);
}

bool almacen_historial_cargar(rk_historial_t *h)
{
    rk_historial_iniciar(h);
    if (!g_fs || !LittleFS.exists("/historial.bin")) {
        return false;
    }
    File f = LittleFS.open("/historial.bin", "r");
    if (!f) {
        return false;
    }
    size_t n = f.read(g_hist_buf, sizeof g_hist_buf);
    f.close();
    return rk_historial_cargar(h, g_hist_buf, n);
}

void almacen_historial_guardar(const rk_historial_t *h)
{
    if (!g_fs) {
        return;
    }
    size_t n = rk_historial_serializar(h, g_hist_buf, sizeof g_hist_buf);
    if (n == 0) {
        return;
    }
    /* Escribir a un temporal y renombrar: un corte a mitad de escritura deja
     * el archivo viejo entero, no uno nuevo por la mitad. */
    File f = LittleFS.open("/historial.tmp", "w");
    if (!f) {
        return;
    }
    size_t escritos = f.write(g_hist_buf, n);
    f.close();
    if (escritos == n) {
        LittleFS.remove("/historial.bin");
        LittleFS.rename("/historial.tmp", "/historial.bin");
    }
}

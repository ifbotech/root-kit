#include "nube.h"
#include "json.h"
#include "../core/mood.h"
#include "../core/persona.h"
#include <string.h>

size_t rk_nube_armar_sync(char *buf, size_t cap, const rk_nube_yo_t *yo,
                          const rk_historial_t *h, uint16_t max_lecturas,
                          uint16_t *incluidas)
{
    rk_jw_t w;
    uint16_t i, n = 0u;

    if (incluidas != NULL) {
        *incluidas = 0u;
    }
    if (yo == NULL) {
        return 0u;
    }
    if (h != NULL) {
        n = h->cuenta < max_lecturas ? h->cuenta : max_lecturas;
    }

    rk_jw_iniciar(&w, buf, cap);
    rk_jw_obj(&w);
    rk_jw_clave(&w, "id");        rk_jw_texto(&w, yo->id);
    rk_jw_clave(&w, "fw");        rk_jw_texto(&w, yo->fw);
    rk_jw_clave(&w, "placa");     rk_jw_texto(&w, yo->placa);
    rk_jw_clave(&w, "pantalla");  rk_jw_texto(&w, yo->pantalla);
    rk_jw_clave(&w, "persona");   rk_jw_texto(&w, yo->persona ? yo->persona : "");
    rk_jw_clave(&w, "estado");    rk_jw_texto(&w, yo->estado ? yo->estado : "");
    rk_jw_clave(&w, "epoca");     rk_jw_entero(&w, (long)yo->epoca);
    if (yo->codigo != NULL) {
        rk_jw_clave(&w, "codigo"); rk_jw_texto(&w, yo->codigo);
    }
    rk_jw_clave(&w, "reloj");     rk_jw_entero(&w, (long)yo->reloj_s);
    rk_jw_clave(&w, "rssi");      rk_jw_entero(&w, yo->rssi);
    rk_jw_clave(&w, "usb");       rk_jw_bool(&w, yo->usb);
    rk_jw_clave(&w, "bat_mv");    rk_jw_entero(&w, yo->bat_mv);
    rk_jw_clave(&w, "arranques"); rk_jw_entero(&w, (long)yo->arranques);
    if (yo->lote != NULL && yo->lote[0] != '\0') {
        rk_jw_clave(&w, "lote"); rk_jw_texto(&w, yo->lote);
    }
    if (yo->ota_estado != NULL && yo->ota_estado[0] != '\0') {
        rk_jw_clave(&w, "ota");
        rk_jw_obj(&w);
        rk_jw_clave(&w, "version"); rk_jw_texto(&w, yo->ota_version ? yo->ota_version : "");
        rk_jw_clave(&w, "estado");  rk_jw_texto(&w, yo->ota_estado);
        rk_jw_fin_obj(&w);
    }

    rk_jw_clave(&w, "lecturas");
    rk_jw_arr(&w);
    for (i = 0; i < n; i++) {
        const rk_registro_t *r = rk_historial_ver(h, i);
        uint32_t hace = (yo->reloj_s >= r->reloj_s) ? yo->reloj_s - r->reloj_s : 0u;
        rk_jw_obj(&w);
        rk_jw_clave(&w, "hace");  rk_jw_entero(&w, (long)hace);
        if (r->suelo_pct != RK_HIST_SIN_DATO_U8) {
            rk_jw_clave(&w, "suelo"); rk_jw_entero(&w, r->suelo_pct);
        }
        rk_jw_clave(&w, "suelo_raw"); rk_jw_entero(&w, r->suelo_raw);
        if (r->temp_dc != RK_TEMP_NO_HAY) {
            rk_jw_clave(&w, "temp"); rk_jw_entero(&w, r->temp_dc);
        }
        if (r->hr_pct != RK_HIST_SIN_DATO_U8) {
            rk_jw_clave(&w, "hr"); rk_jw_entero(&w, r->hr_pct);
        }
        if (r->lux != RK_HIST_SIN_LUX) {
            rk_jw_clave(&w, "lux"); rk_jw_entero(&w, (long)r->lux);
        }
        if (r->suelo_dc != RK_TEMP_NO_HAY) {
            rk_jw_clave(&w, "tsuelo"); rk_jw_entero(&w, r->suelo_dc);
        }
        if (r->bat_mv > 0u) {
            rk_jw_clave(&w, "bat"); rk_jw_entero(&w, r->bat_mv);
        }
        rk_jw_clave(&w, "usb");    rk_jw_bool(&w, (r->banderas & 1u) != 0u);
        rk_jw_clave(&w, "animo");  rk_jw_texto(&w, rk_mood_name((rk_mood_t)r->animo));
        {
            static const char *SEV[4] = { "OK", "WATCH", "URGENT", "OK" };
            rk_jw_clave(&w, "sev"); rk_jw_texto(&w, SEV[rk_registro_severidad(r)]);
        }
        rk_jw_clave(&w, "fallas"); rk_jw_entero(&w, (long)rk_registro_fallas(r));
        if (r->banderas & 0x08u) {
            rk_jw_clave(&w, "escurre"); rk_jw_bool(&w, true);
        }
        rk_jw_fin_obj(&w);
    }
    rk_jw_fin_arr(&w);
    rk_jw_fin_obj(&w);

    if (!rk_jw_terminar(&w)) {
        return 0u;
    }
    if (incluidas != NULL) {
        *incluidas = n;
    }
    return w.len;
}

const rk_species_t *rk_especie_ver(rk_especie_guardada_t *e)
{
    if (e == NULL) {
        return NULL;
    }
    e->id[sizeof e->id - 1u] = '\0';
    e->nombre[sizeof e->nombre - 1u] = '\0';
    e->sp.id = e->id;
    e->sp.nombre = e->nombre;
    return &e->sp;
}

static long acotar(long v, long lo, long hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

bool rk_nube_parsear(const char *json, rk_nube_resp_t *r)
{
    long v;
    bool b;

    if (r == NULL) {
        return false;
    }
    memset(r, 0, sizeof *r);
    if (json == NULL || !rk_json_bool(json, "ok", &b) || !b) {
        return false;
    }
    r->ok = true;
    rk_json_bool(json, "vinculado", &r->vinculado);
    rk_json_bool(json, "revelado", &r->revelado);
    rk_json_texto(json, "persona", r->persona, sizeof r->persona);
    {
        /* Una rareza que el firmware no conoce se ignora: la maceta sigue
         * con la piel que tenía, en vez de inventar una. */
        char rar[12];
        if (rk_json_texto(json, "rareza", rar, sizeof rar)) {
            int k = rk_rareza_parse(rar);
            if (k >= 0) {
                r->rareza = (uint8_t)k;
                r->hay_rareza = true;
            }
        }
    }
    rk_json_texto(json, "nombre", r->nombre, sizeof r->nombre);

    if (rk_json_hay(json, "especie")) {
        rk_especie_guardada_t *e = &r->especie;
        long smin = -1, smax = -1, tmin = 0, tmax = 0, hr = 0, lmin = 0, lmax = 0;
        bool completa =
            rk_json_entero(json, "especie.suelo_min", &smin) &&
            rk_json_entero(json, "especie.suelo_max", &smax) &&
            rk_json_entero(json, "especie.temp_min", &tmin) &&
            rk_json_entero(json, "especie.temp_max", &tmax) &&
            rk_json_entero(json, "especie.hr_min", &hr) &&
            rk_json_entero(json, "especie.lux_min", &lmin) &&
            rk_json_entero(json, "especie.lux_max", &lmax);
        /* Una especie a medias o incoherente es peor que ninguna: la cara
         * reaccionaría a umbrales inventados. */
        if (completa && smin < smax && tmin < tmax && lmin < lmax) {
            rk_json_texto(json, "especie.id", e->id, sizeof e->id);
            rk_json_texto(json, "especie.nombre", e->nombre, sizeof e->nombre);
            e->sp.soil_min    = (uint8_t)acotar(smin, 0, 100);
            e->sp.soil_max    = (uint8_t)acotar(smax, 0, 100);
            e->sp.temp_min_dc = (int16_t)acotar(tmin, -400, 600);
            e->sp.temp_max_dc = (int16_t)acotar(tmax, -400, 600);
            e->sp.rh_min      = (uint8_t)acotar(hr, 0, 100);
            e->sp.lux_min     = (uint32_t)acotar(lmin, 0, 200000);
            e->sp.lux_max     = (uint32_t)acotar(lmax, 0, 200000);
            if (rk_json_entero(json, "especie.dificultad", &v)) {
                e->sp.dificultad = (uint8_t)acotar(v, 0, 100);
            }
            rk_especie_ver(e);
            r->hay_especie = true;
        }
    }

    if (rk_json_entero(json, "intervalo_s", &v)) {
        r->intervalo_s = (uint32_t)acotar(v, 60, 86400);
    }
    if (rk_json_entero(json, "aceptadas", &v)) {
        r->aceptadas = (uint16_t)acotar(v, 0, RK_HIST_CAP);
    }
    if (rk_json_entero(json, "hora", &v) && v > 0) {
        r->hora = (uint32_t)v;
    }
    if (rk_json_hay(json, "calibracion")) {
        long seco, mojado;
        if (rk_json_entero(json, "calibracion.seco", &seco) &&
            rk_json_entero(json, "calibracion.mojado", &mojado)) {
            r->cal.dry_raw = (uint16_t)acotar(seco, 0, 4095);
            r->cal.wet_raw = (uint16_t)acotar(mojado, 0, 4095);
            r->hay_calibracion = rk_soil_cal_valid(&r->cal);
        }
    }
    if (rk_json_entero(json, "brillo", &v)) {
        r->brillo = (uint8_t)acotar(v, 0, 100);
    }
    if (rk_json_hay(json, "vinculo")) {
        long sanos, vividos, racha, mejor;
        if (rk_json_entero(json, "vinculo.dias_sanos", &sanos) &&
            rk_json_entero(json, "vinculo.dias_vividos", &vividos)) {
            r->vinculo.dias_sanos = (uint16_t)acotar(sanos, 0, 65535);
            r->vinculo.dias_vividos = (uint16_t)acotar(vividos, 0, 65535);
            if (rk_json_entero(json, "vinculo.racha", &racha)) {
                r->vinculo.racha = (uint16_t)acotar(racha, 0, 65535);
            }
            if (rk_json_entero(json, "vinculo.mejor_racha", &mejor)) {
                r->vinculo.mejor_racha = (uint16_t)acotar(mejor, 0, 65535);
            }
            r->hay_vinculo = true;
        }
    }
    {
        char modo[12];
        if (rk_json_texto(json, "pantalla", modo, sizeof modo)) {
            r->pantalla_siempre = strcmp(modo, "siempre") == 0;
        }
    }
    rk_json_bool(json, "calibrando", &r->calibrando);
    /* Un manifiesto a medias o absurdo es como si no hubiera ninguno. */
    r->hay_firmware = rk_ota_manifiesto_parsear(json, &r->firmware);
    return true;
}

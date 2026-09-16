#include "portal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static WebServer *g_web = nullptr;
static DNSServer  g_dns;
static bool       g_activo = false;
static rk_portal_datos_t g_pendiente;
static rk_portal_estado_t g_estado = RK_PORTAL_ESPERANDO;
static char g_codigo[12];

static const char PAGINA[] PROGMEM = R"HTML(<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>ROOTKIT</title>
<style>
:root{--fondo:#16181e;--carta:#20232b;--texto:#eef0f4;--suave:#9aa0ad;--verde:#58cc02;--borde:#2e323c}
*{box-sizing:border-box}body{margin:0;background:var(--fondo);color:var(--texto);
font:17px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;padding:24px 18px 40px}
h1{font-size:26px;margin:4px 0 6px;letter-spacing:.5px}p{color:var(--suave);margin:0 0 18px}
.carta{background:var(--carta);border:2px solid var(--borde);border-radius:18px;padding:18px;margin-bottom:16px}
label{display:block;font-weight:700;margin:12px 0 6px}
input,select{width:100%;font:inherit;color:var(--texto);background:var(--fondo);border:2px solid var(--borde);
border-radius:12px;padding:13px 12px}input:focus,select:focus{outline:none;border-color:var(--verde)}
button{width:100%;margin-top:18px;font:inherit;font-weight:800;letter-spacing:.6px;color:#fff;background:var(--verde);
border:0;border-radius:14px;padding:15px;box-shadow:0 5px 0 #46a302}button:active{transform:translateY(3px);box-shadow:0 2px 0 #46a302}
.redes button{background:var(--fondo);box-shadow:none;border:2px solid var(--borde);margin:6px 0 0;text-align:left;font-weight:600;display:flex;justify-content:space-between}
.chip{font-size:13px;color:var(--suave)}details{margin-top:14px;color:var(--suave)}
#estado{font-weight:700;min-height:24px}.ok{color:var(--verde)}.mal{color:#ff6b6b}
.codigo{font:800 20px ui-monospace,Menlo,monospace;letter-spacing:2px}
</style></head><body>
<h1>Conectá tu ROOTKIT</h1>
<p>Elegí el wifi de tu casa. Es solo de 2,4 GHz: si tu router tiene dos redes, usá la que no dice 5G.</p>
<div class="carta redes" id="redes"><span class="chip">Buscando redes…</span></div>
<form class="carta" method="post" action="/guardar" id="form">
<label for="ssid">Red</label><input id="ssid" name="ssid" autocomplete="off" required maxlength="32">
<label for="clave">Clave</label><input id="clave" name="clave" type="password" maxlength="64">
<details><summary>Avanzado</summary><label for="nube">Servidor</label>
<input id="nube" name="nube" placeholder="se deja vacío" maxlength="95"></details>
<button type="submit">CONECTAR</button>
</form>
<div class="carta"><div id="estado"></div><p style="margin:8px 0 0">Código de esta maceta: <span class="codigo">%CODIGO%</span></p></div>
<script>
function redes(){fetch('/redes').then(r=>r.json()).then(l=>{const d=document.getElementById('redes');
if(!l.length){d.innerHTML='<span class="chip">Buscando redes…</span>';return setTimeout(redes,2500)}
d.innerHTML='';l.forEach(n=>{const b=document.createElement('button');b.type='button';
b.innerHTML='<span></span><span class="chip">'+(n.segura?'🔒 ':'')+n.senal+'</span>';b.firstChild.textContent=n.ssid;
b.onclick=()=>{document.getElementById('ssid').value=n.ssid;document.getElementById('clave').focus()};d.appendChild(b)})}).catch(()=>setTimeout(redes,2500))}
function estado(){fetch('/estado').then(r=>r.json()).then(e=>{const s=document.getElementById('estado');
s.className=e.estado=='ok'?'ok':e.estado=='fallo'?'mal':'';
s.textContent={esperando:'',probando:'Probando la red…',ok:'¡Listo! Ya podés volver a la app.',fallo:'No pude conectarme. Revisá la clave.'}[e.estado]||''}).catch(()=>{}).finally(()=>setTimeout(estado,1500))}
redes();estado();
</script></body></html>)HTML";

static void responder_pagina(void)
{
    String html = FPSTR(PAGINA);
    html.replace("%CODIGO%", g_codigo);
    g_web->sendHeader("Cache-Control", "no-store");
    g_web->send(200, "text/html; charset=utf-8", html);
}

/* Todo lo que los sistemas usan para detectar un portal cautivo termina en
 * la página: así el teléfono la abre solo al conectarse. */
static void redirigir(void)
{
    g_web->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    g_web->send(302, "text/plain", "");
}

static void responder_redes(void)
{
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);
        g_web->send(200, "application/json", "[]");
        return;
    }
    if (n == WIFI_SCAN_RUNNING) {
        g_web->send(200, "application/json", "[]");
        return;
    }
    String j = "[";
    int puestas = 0;
    for (int i = 0; i < n && puestas < 15; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) {
            continue;
        }
        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");
        int rssi = WiFi.RSSI(i);
        const char *senal = rssi > -60 ? "muy buena" : rssi > -70 ? "buena" : rssi > -80 ? "débil" : "muy débil";
        if (puestas++) {
            j += ",";
        }
        j += "{\"ssid\":\"" + ssid + "\",\"senal\":\"" + senal + "\",\"segura\":" +
             (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    j += "]";
    WiFi.scanDelete();
    WiFi.scanNetworks(true);            /* la próxima vez, fresca */
    g_web->send(200, "application/json", j);
}

static void responder_estado(void)
{
    static const char *NOMBRES[] = { "esperando", "probando", "ok", "fallo" };
    g_web->send(200, "application/json", String("{\"estado\":\"") + NOMBRES[g_estado] + "\"}");
}

static void responder_guardar(void)
{
    String ssid = g_web->arg("ssid");
    String clave = g_web->arg("clave");
    String nube = g_web->arg("nube");

    if (ssid.length() == 0 || ssid.length() > 32 || clave.length() > 64 || nube.length() > 95) {
        g_web->send(400, "text/plain; charset=utf-8", "Datos inválidos");
        return;
    }
    memset(&g_pendiente, 0, sizeof g_pendiente);
    strncpy(g_pendiente.ssid, ssid.c_str(), sizeof g_pendiente.ssid - 1);
    strncpy(g_pendiente.clave, clave.c_str(), sizeof g_pendiente.clave - 1);
    strncpy(g_pendiente.nube, nube.c_str(), sizeof g_pendiente.nube - 1);
    g_pendiente.recibido = true;
    g_estado = RK_PORTAL_PROBANDO;
    g_web->sendHeader("Location", "/", true);
    g_web->send(303, "text/plain", "");
}

void portal_iniciar(const char *ssid_ap, const char *codigo)
{
    if (g_activo) {
        return;
    }
    strncpy(g_codigo, codigo, sizeof g_codigo - 1);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid_ap);
    delay(100);
    g_dns.setErrorReplyCode(DNSReplyCode::NoError);
    g_dns.start(53, "*", WiFi.softAPIP());

    g_web = new WebServer(80);
    g_web->on("/", HTTP_GET, responder_pagina);
    g_web->on("/redes", HTTP_GET, responder_redes);
    g_web->on("/estado", HTTP_GET, responder_estado);
    g_web->on("/guardar", HTTP_POST, responder_guardar);
    g_web->on("/generate_204", redirigir);          /* Android */
    g_web->on("/hotspot-detect.html", redirigir);   /* Apple */
    g_web->on("/connecttest.txt", redirigir);       /* Windows */
    g_web->onNotFound(redirigir);
    g_web->begin();
    WiFi.scanNetworks(true);
    g_estado = RK_PORTAL_ESPERANDO;
    g_activo = true;
}

void portal_detener(void)
{
    if (!g_activo) {
        return;
    }
    g_web->stop();
    delete g_web;
    g_web = nullptr;
    g_dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    g_activo = false;
}

bool portal_activo(void)
{
    return g_activo;
}

bool portal_atender(rk_portal_datos_t *datos)
{
    if (!g_activo) {
        return false;
    }
    g_dns.processNextRequest();
    g_web->handleClient();
    if (g_pendiente.recibido) {
        *datos = g_pendiente;
        g_pendiente.recibido = false;
        return true;
    }
    return false;
}

void portal_informar(rk_portal_estado_t e)
{
    g_estado = e;
}

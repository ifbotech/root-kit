/* app.js — el Hub.
 *
 * Vanilla, sin build: la Terminal sirve estos archivos tal cual desde la SD,
 * así que cualquier paso de compilación sería un paso más que puede fallar a
 * 8.000 km del único que sabe arreglarlo.
 */
import {
  MOOD_ES, formatTemp, formatLux, formatEdad, battPct,
  ordenarPlantas, contarAlertas, posicionEnRango,
  validarAlta, interpretarIdentificacion,
} from './lib/model.mjs';

const $ = (sel) => document.querySelector(sel);
const REFRESCO_MS = 15000;

let especies = [];
let ultimoEstado = null;

/* ------------------------------------------------------------- red ------- */
async function api(ruta, opciones) {
  const r = await fetch(ruta, {
    headers: { 'content-type': 'application/json' },
    ...opciones,
  });
  if (r.status === 204) return null;
  const cuerpo = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(cuerpo.error || `error ${r.status}`);
  return cuerpo;
}

function avisar(texto, esError = false) {
  const el = $('#aviso');
  el.textContent = texto;
  el.classList.toggle('error', esError);
  el.hidden = false;
  clearTimeout(avisar._t);
  avisar._t = setTimeout(() => { el.hidden = true; }, 4000);
}

/* ---------------------------------------------------------- plantas ------ */
function tarjeta(p) {
  const li = document.createElement('li');
  li.className = `tarjeta sev-${(p.severity || 'OK').toLowerCase()}`;

  const esp = especies.find((e) => e.id === p.especie);
  const suelo = esp ? posicionEnRango(p.tel.soil_pct, esp.soil_min, esp.soil_max) : null;

  li.innerHTML = `
    <div class="cab">
      <strong>${escapar(p.nombre)}</strong>
      <span class="chip">${escapar(MOOD_ES[p.mood] || p.mood)}</span>
    </div>
    <p class="dice">«${escapar(p.reason || '')}»</p>
    <dl class="datos">
      <div><dt>Tierra</dt><dd>${p.tel.soil_pct}%</dd></div>
      <div><dt>Clima</dt><dd>${formatTemp(p.tel.temp_dc)}</dd></div>
      <div><dt>Luz</dt><dd>${formatLux(p.tel.lux)}</dd></div>
      <div><dt>Lectura</dt><dd>${formatEdad(p.tel.age_s)}</dd></div>
    </dl>
    ${suelo === null ? '' : `
      <div class="barra-rango" title="posición dentro del rango cómodo">
        <span style="left:${(suelo * 100).toFixed(1)}%"></span>
      </div>`}
    <p class="pie">
      ${p.spore
        ? `Spore ${escapar(p.spore.id.slice(-4))} · batería ${p.spore.batt_pct ?? battPct(p.tel.batt_mv)}%`
        : 'sin Spore asignado'}
    </p>`;
  return li;
}

function escapar(s) {
  return String(s ?? '').replace(/[&<>"']/g, (c) => (
    { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]
  ));
}

function pintarPlantas(estado) {
  const lista = $('#lista');
  const plantas = ordenarPlantas(estado.plants);
  lista.replaceChildren(...plantas.map(tarjeta));
  $('#vacio').hidden = plantas.length > 0;

  const alertas = contarAlertas(plantas);
  $('#resumen').textContent = plantas.length === 0
    ? 'sin plantas registradas'
    : alertas === 0
      ? `${plantas.length} plantas, todas bien`
      : `${alertas} de ${plantas.length} reclaman algo`;
  $('#resumen').classList.toggle('alerta', alertas > 0);
}

async function refrescar() {
  try {
    ultimoEstado = await api('/api/state');
    pintarPlantas(ultimoEstado);
  } catch (e) {
    $('#resumen').textContent = 'sin conexión con la Terminal';
    $('#resumen').classList.add('alerta');
  }
}

/* ------------------------------------------------------------- alta ------ */
function llenarEspecies() {
  const sel = $('#especie');
  sel.replaceChildren(
    new Option('— elegí una —', ''),
    ...especies.map((e) => new Option(e.nombre, e.id)),
  );
}

async function llenarSpores() {
  try {
    const libres = await api('/api/spores');
    const sel = $('#spore');
    sel.replaceChildren(
      new Option('— ninguno por ahora —', ''),
      ...libres.map((s) => new Option(`${s.id.slice(-4)} (visto ${formatEdad(s.visto_hace_s)})`, s.id)),
    );
  } catch { /* sin Spores libres no pasa nada */ }
}

async function identificar(archivo) {
  const b64 = await new Promise((res, rej) => {
    const fr = new FileReader();
    fr.onload = () => res(String(fr.result).split(',')[1] || '');
    fr.onerror = rej;
    fr.readAsDataURL(archivo);
  });

  $('#preview-img').src = URL.createObjectURL(archivo);
  $('#previsualizacion').hidden = false;

  const el = $('#ident');
  el.hidden = false;
  el.className = 'ident';
  el.textContent = 'Identificando…';

  try {
    const r = await api('/api/identify', {
      method: 'POST',
      body: JSON.stringify({ image_b64: b64, mime: archivo.type }),
    });
    const i = interpretarIdentificacion(r);
    el.textContent = i.mensaje;
    el.classList.add(i.estado);
    if (i.especie) {
      $('#especie').value = i.especie;
      if (!$('#nombre').value) {
        $('#nombre').value = (r.nombre || '').split(' ')[0].toUpperCase().slice(0, 17);
      }
    }
  } catch (e) {
    el.textContent = `No pude identificarla: ${e.message}. Elegí la especie a mano.`;
    el.classList.add('fallo');
  }
}

async function enviarAlta(ev) {
  ev.preventDefault();
  const datos = {
    nombre: $('#nombre').value,
    especie: $('#especie').value,
    spore_id: $('#spore').value || undefined,
  };

  const v = validarAlta(datos, especies.map((e) => e.id));
  const cajaErrores = $('#errores');
  cajaErrores.hidden = v.ok;
  cajaErrores.textContent = v.errores.join(' ');
  if (!v.ok) return;

  try {
    const creada = await api('/api/plants', { method: 'POST', body: JSON.stringify(datos) });
    avisar(creada.simbionte_nuevo
      ? `Listo. Se desbloqueó un simbionte nuevo para ${creada.nombre}.`
      : `Listo, ${creada.nombre} quedó registrada.`);
    $('#form-alta').reset();
    $('#previsualizacion').hidden = true;
    $('#ident').hidden = true;
    await Promise.all([refrescar(), llenarSpores(), pintarColeccion()]);
    mostrarVista('plantas');
  } catch (e) {
    avisar(`No pude registrarla: ${e.message}`, true);
  }
}

/* -------------------------------------------------------- coleccion ------ */
async function pintarColeccion() {
  try {
    const c = await api('/api/collection');
    $('#grilla').replaceChildren(...c.catalogo.map((s) => {
      const li = document.createElement('li');
      li.className = s.desbloqueado ? 'sim abierto' : 'sim cerrado';
      li.innerHTML = s.desbloqueado
        ? `<span class="sim-id">${escapar(s.nombre)}</span>
           <span class="sim-esp">${escapar(especies.find((e) => e.id === s.especie)?.nombre || s.especie)}</span>`
        : '<span class="sim-id">???</span><span class="sim-esp">sin descubrir</span>';
      return li;
    }));
  } catch { /* la colección es secundaria */ }
}

/* ------------------------------------------------------------ vistas ----- */
function mostrarVista(nombre) {
  document.querySelectorAll('.vista').forEach((v) => {
    v.hidden = v.id !== `vista-${nombre}`;
  });
  document.querySelectorAll('.tab').forEach((t) => {
    const activa = t.dataset.vista === nombre;
    t.classList.toggle('activa', activa);
    t.setAttribute('aria-selected', String(activa));
  });
}

/* -------------------------------------------------------------- init ----- */
async function init() {
  document.querySelectorAll('.tab').forEach((t) => {
    t.addEventListener('click', () => mostrarVista(t.dataset.vista));
  });
  $('#foto').addEventListener('change', (e) => {
    const f = e.target.files && e.target.files[0];
    if (f) identificar(f);
  });
  $('#form-alta').addEventListener('submit', enviarAlta);

  try {
    especies = await api('/api/species');
  } catch {
    especies = [];
  }
  llenarEspecies();
  await Promise.all([refrescar(), llenarSpores(), pintarColeccion()]);

  setInterval(refrescar, REFRESCO_MS);

  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('sw.js').catch(() => { /* sin offline, sigue andando */ });
  }
}

init();

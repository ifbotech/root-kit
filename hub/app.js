/* app.js — el armazón: estado, ruteo, instalación y refresco.
 *
 * Sin build: ES modules directos. Desplegar es copiar la carpeta, `make serve`
 * anda sin instalar nada y no hay un node_modules que se pudra. Ver
 * lib/ui.mjs para el razonamiento completo.
 *
 * CÓMO SE INSTALA
 *
 * La caja trae un QR que abre esta página. Desde ahí, "agregar a la pantalla
 * de inicio" la convierte en algo que abre a pantalla completa, sin barra de
 * navegador y con su propio ícono. No hay tienda de por medio: ni cuota
 * anual, ni revisión de actualizaciones, ni nadie mirando las mecánicas de
 * colección contra las políticas de cajas de botín.
 *
 * Android dispara `beforeinstallprompt` y se puede ofrecer un botón. iOS no
 * lo implementa y hay que explicarle al usuario dónde tocar, así que se
 * detecta y se muestra la instrucción en vez del botón.
 */
import { $, h, render } from './lib/ui.mjs';
import { tareasDelDia, contarEstados } from './lib/tareas.mjs';
import { actualizarRacha } from './lib/gamificacion.mjs';
import { vistaHoy } from './vistas/hoy.mjs';
import { vistaPlantas, vistaDetalle } from './vistas/plantas.mjs';
import { vistaEscaner, vistaDiagnostico } from './vistas/escaner.mjs';
import { vistaColeccion } from './vistas/coleccion.mjs';

const REFRESCO_MS = 30000;

/* Lo que vive en el teléfono y no en el servidor. Son estados de interfaz
   —"ya me ocupé de esto", "mi racha"— y no hechos sobre la planta. Los hechos
   sobre la planta los reporta el sensor. */
const LS = { hechas: 'rootkit:hechas', racha: 'rootkit:racha' };

const guardado = (k, porDefecto) => {
  try { return JSON.parse(localStorage.getItem(k)) ?? porDefecto; }
  catch { return porDefecto; }
};
const guardar = (k, v) => {
  try { localStorage.setItem(k, JSON.stringify(v)); } catch { /* modo privado */ }
};

const app = {
  estado: null,
  especies: [],
  modelos: [],
  coleccion: null,
  hechas: guardado(LS.hechas, {}),
  racha: guardado(LS.racha, { dias: 0, mejor: 0, ultimo: null }),
  vista: 'hoy',
  plantaId: null,
  instalable: null,
};

/* --------------------------------------------------------------- red --- */
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

/* ------------------------------------------------------------ estado --- */
async function cargarCatalogos() {
  const [esp, col] = await Promise.all([
    api('/api/species').catch(() => []),
    api('/api/collection').catch(() => null),
  ]);
  app.especies = esp || [];
  app.coleccion = col;
  app.modelos = col?.catalogo || [];
}

async function refrescar() {
  try {
    app.estado = await api('/api/state');
    $('#sinred').hidden = true;
    /* La racha se cierra una vez por día: si al mirar no hay urgencias, el
       día suma. Es la única cuenta que depende de haber observado, así que
       vive en el teléfono. */
    const hoy = new Date().toISOString().slice(0, 10);
    const c = contarEstados(app.estado.nodes);
    app.racha = actualizarRacha(app.racha, c.urgente > 0, hoy);
    guardar(LS.racha, app.racha);
  } catch {
    $('#sinred').hidden = false;
  }
  pintar();
}

/* Marcar una tarea como hecha la esconde mientras el sensor confirma. No se
   manda al servidor: no es un hecho sobre la planta, es una nota personal. */
function hacerTarea(t) {
  app.hechas = { ...app.hechas, [t.id]: Date.now() };
  guardar(LS.hechas, app.hechas);
  avisar(t.auto
    ? 'Anotado. Cuando el sensor lo confirme desaparece sola.'
    : 'Anotado.');
  pintar();
}

async function registrar(datos) {
  const creada = await api('/api/nodes', {
    method: 'POST', body: JSON.stringify(datos),
  });
  await Promise.all([cargarCatalogos(), refrescar()]);
  return creada;
}

async function declararCarcasa(modelo) {
  try {
    const r = await api('/api/collection', {
      method: 'POST', body: JSON.stringify({ modelo }),
    });
    avisar(r.nuevo ? `${r.nombre} se suma a tu colección.` : `Ya tenías ${r.nombre}.`);
    await cargarCatalogos();
    pintar();
  } catch (e) {
    avisar(`No pude sumarla: ${e.message}`, true);
  }
}

/* ------------------------------------------------------------- rutas --- */
function irA(vista, plantaId = null) {
  app.vista = vista;
  if (plantaId !== null) app.plantaId = plantaId;
  window.scrollTo(0, 0);
  pintar();
}

const ctxBase = () => ({
  estado: app.estado,
  especies: app.especies,
  modelos: app.modelos,
  coleccion: app.coleccion,
  hechas: app.hechas,
  racha: app.racha,
  plantaId: app.plantaId,
  api,
  avisar,
  irA,
  alHacer: hacerTarea,
  alAbrir: (id) => irA('detalle', id),
  alDiagnosticar: (id) => irA('diagnostico', id),
  alRegistrar: registrar,
  alDeclarar: declararCarcasa,
  volver: () => irA(app.vista === 'diagnostico' ? 'detalle' : 'plantas'),
});

const VISTAS = {
  hoy: vistaHoy,
  plantas: vistaPlantas,
  detalle: vistaDetalle,
  escaner: vistaEscaner,
  diagnostico: vistaDiagnostico,
  coleccion: vistaColeccion,
};

/* Qué pestaña se marca activa para cada vista. El detalle y el diagnóstico
   son hijos de "plantas", así que la pestaña se queda ahí. */
const PESTANA = {
  hoy: 'hoy', plantas: 'plantas', detalle: 'plantas', diagnostico: 'plantas',
  escaner: 'escaner', coleccion: 'coleccion',
};

function pintar() {
  const fn = VISTAS[app.vista] || vistaHoy;
  render($('#vista'), fn(ctxBase()));

  const activa = PESTANA[app.vista];
  for (const b of document.querySelectorAll('.tab')) {
    const suya = b.dataset.vista === activa;
    b.classList.toggle('activa', suya);
    b.setAttribute('aria-selected', suya ? 'true' : 'false');
  }

  /* El globo de la pestaña Hoy: cuántas cosas hay pendientes. */
  const n = app.estado
    ? tareasDelDia(app.estado.nodes, app.especies, app.hechas).length : 0;
  const globo = $('#globo-hoy');
  globo.textContent = n > 9 ? '9+' : String(n);
  globo.hidden = n === 0;
}

/* -------------------------------------------------------- instalación --- */
function prepararInstalacion() {
  const barra = $('#instalar');
  const enPantallaCompleta = window.matchMedia('(display-mode: standalone)').matches
    || window.navigator.standalone === true;
  if (enPantallaCompleta) {
    barra.hidden = true;
    return;
  }

  window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    app.instalable = e;
    render(barra,
      h('span', {}, 'Agregala a tu pantalla de inicio y se abre como una app.'),
      h('button', {
        class: 'boton chico', type: 'button',
        onClick: () => {
          barra.hidden = true;
          app.instalable?.prompt();
          app.instalable = null;
        },
      }, 'Instalar'));
    barra.hidden = false;
  });

  /* iOS no implementa beforeinstallprompt, así que ahí hay que explicarlo. */
  if (/iphone|ipad|ipod/i.test(navigator.userAgent)) {
    render(barra,
      h('span', {}, 'Tocá Compartir y después “Agregar a inicio” para abrirla a pantalla completa.'),
      h('button', {
        class: 'boton chico', type: 'button',
        onClick: () => { barra.hidden = true; },
      }, 'Listo'));
    barra.hidden = false;
  }
}

/* -------------------------------------------------------------- inicio -- */
async function inicio() {
  for (const b of document.querySelectorAll('.tab')) {
    b.addEventListener('click', () => irA(b.dataset.vista));
  }

  prepararInstalacion();
  pintar();                       /* el armazón aparece antes que los datos */

  await cargarCatalogos();
  await refrescar();
  setInterval(refrescar, REFRESCO_MS);

  /* Al volver a la app después de un rato, los datos en pantalla son viejos.
     Refrescar al recuperar el foco evita que alguien riegue mirando una
     lectura de ayer. */
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden) refrescar();
  });

  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('sw.js').catch(() => { /* sin offline */ });
  }
}

inicio();

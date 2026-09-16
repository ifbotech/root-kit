/* dev-server.mjs — implementa el contrato de API.md en Node.
 *
 * Existe para desarrollar y testear el Hub sin el Prime. Es también la
 * especificación ejecutable: cuando se escriba el servidor HTTP en C sobre el
 * ESP32, este archivo dice exactamente qué tiene que responder.
 *
 *     node hub/dev-server.mjs [puerto]
 *
 * Sin dependencias: sólo el runtime de Node.
 */
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { fileURLToPath } from 'node:url';

const RAIZ = fileURLToPath(new URL('.', import.meta.url));
const PUERTO = Number(process.argv[2]) || 8080;

/* ------------------------------------------------------------ catalogo --- */
/* Generado desde firmware/core/species.c por tools/sync_catalog.py.
 * No editar a mano: el catalogo tiene una sola fuente de verdad. */
export const ESPECIES = [
  { id: 'sansevieria', nombre: 'Lengua de suegra', soil_min: 8, soil_max: 35,
    temp_min_dc: 150, temp_max_dc: 320, rh_min: 30, lux_min: 800, lux_max: 30000, dificultad: 10 },
  { id: 'pothos', nombre: 'Potus', soil_min: 20, soil_max: 55,
    temp_min_dc: 170, temp_max_dc: 300, rh_min: 40, lux_min: 500, lux_max: 12000, dificultad: 15 },
  { id: 'zamioculcas', nombre: 'Zamioculca', soil_min: 10, soil_max: 40,
    temp_min_dc: 160, temp_max_dc: 300, rh_min: 30, lux_min: 400, lux_max: 15000, dificultad: 18 },
  { id: 'cactus', nombre: 'Cactus / suculenta', soil_min: 5, soil_max: 25,
    temp_min_dc: 100, temp_max_dc: 380, rh_min: 20, lux_min: 5000, lux_max: 80000, dificultad: 25 },
  { id: 'aloe', nombre: 'Aloe vera', soil_min: 8, soil_max: 30,
    temp_min_dc: 130, temp_max_dc: 350, rh_min: 25, lux_min: 3000, lux_max: 50000, dificultad: 28 },
  { id: 'monstera', nombre: 'Monstera deliciosa', soil_min: 25, soil_max: 60,
    temp_min_dc: 180, temp_max_dc: 300, rh_min: 50, lux_min: 1000, lux_max: 15000, dificultad: 45 },
  { id: 'filodendro', nombre: 'Filodendro', soil_min: 25, soil_max: 58,
    temp_min_dc: 180, temp_max_dc: 300, rh_min: 50, lux_min: 900, lux_max: 14000, dificultad: 40 },
  { id: 'helecho', nombre: 'Helecho de Boston', soil_min: 45, soil_max: 80,
    temp_min_dc: 160, temp_max_dc: 260, rh_min: 70, lux_min: 600, lux_max: 8000, dificultad: 70 },
  { id: 'orquidea', nombre: 'Orquídea phalaenopsis', soil_min: 30, soil_max: 60,
    temp_min_dc: 180, temp_max_dc: 290, rh_min: 60, lux_min: 1200, lux_max: 10000, dificultad: 75 },
  { id: 'calathea', nombre: 'Calathea', soil_min: 40, soil_max: 70,
    temp_min_dc: 180, temp_max_dc: 280, rh_min: 70, lux_min: 800, lux_max: 9000, dificultad: 78 },
  { id: 'ficus-lyrata', nombre: 'Ficus lyrata', soil_min: 25, soil_max: 55,
    temp_min_dc: 180, temp_max_dc: 270, rh_min: 50, lux_min: 2000, lux_max: 20000, dificultad: 85 },
  { id: 'bonsai', nombre: 'Bonsái de olmo', soil_min: 30, soil_max: 60,
    temp_min_dc: 150, temp_max_dc: 270, rh_min: 55, lux_min: 3000, lux_max: 25000, dificultad: 92 },
];

/* Generado desde firmware/core/persona.c por tools/sync_catalog.py.
 *
 * Los modelos de carcasa: lo que trae la caja ciega. La rareza es del
 * MODELO y no de la planta, porque el azar ocurre al abrir la caja y no
 * dentro del software. El campo `idx` es la clave con la que la carcasa
 * viaja por radio hasta el aparato, así que su orden importa. */
export const MODELOS = [
  { idx: 0, id: 'cresta', nombre: 'Cresta', rareza: 'COMUN',
    carcasa: 'carcasas/cresta.stl', lema: 'No te va a agradecer. Igual regala.' },
  { idx: 1, id: 'kawaii', nombre: 'Kawaii', rareza: 'COMUN',
    carcasa: 'carcasas/kawaii.stl', lema: 'Te quiere aunque la olvides. Eso es peor.' },
  { idx: 2, id: 'visor', nombre: 'Visor', rareza: 'COMUN',
    carcasa: 'carcasas/visor.stl', lema: 'Registra. No opina.' },
  { idx: 3, id: 'ciclope', nombre: 'Ciclope', rareza: 'RARO',
    carcasa: 'carcasas/ciclope.stl', lema: 'Mira una sola cosa. La mira mucho.' },
  { idx: 4, id: 'hongo', nombre: 'Hongo', rareza: 'RARO',
    carcasa: 'carcasas/hongo.stl', lema: 'Duerme. Crece igual.' },
  { idx: 5, id: 'glitch', nombre: '?????', rareza: 'SECRETO',
    carcasa: 'carcasas/glitch.stl', lema: 'No estaba en la caja. Igual salio.' },
];

/* --------------------------------------------------------------- estado -- */
export function estadoInicial() {
  return {
    seq: 2,
    nodes: [
      {
        id: 'p1', nombre: 'MONSTERA', modelo: 'cresta', especie: 'monstera',
        link: 'VIVO',
        mood: 'THIRSTY', severity: 'URGENT', reason: 'tengo sed',
        tel: { soil_pct: 22, temp_dc: 236, rh_pct: 54, lux: 5200, batt_mv: 3810, age_s: 240 },
        nodo: { id: 'a4cf129b4011', batt_pct: 62, seq: 4211 },
        bond: { dias_vividos: 104, dias_sanos: 95, racha: 12, mejor_racha: 40 },
      },
      {
        id: 'p2', nombre: 'POTUS', modelo: 'kawaii', especie: 'pothos',
        link: 'VIVO',
        mood: 'HAPPY', severity: 'OK', reason: 'estoy perfecta',
        tel: { soil_pct: 44, temp_dc: 229, rh_pct: 48, lux: 3100, batt_mv: 3950, age_s: 95 },
        nodo: { id: 'a4cf129b4077', batt_pct: 81, seq: 980 },
        bond: { dias_vividos: 40, dias_sanos: 34, racha: 8, mejor_racha: 19 },
      },
      {
        id: 'p3', nombre: 'BONSAI', modelo: 'hongo', especie: 'bonsai',
        link: 'TIBIO',
        mood: 'DARK', severity: 'WATCH', reason: 'necesito mas luz',
        tel: { soil_pct: 38, temp_dc: 221, rh_pct: 58, lux: 380, batt_mv: 3620, age_s: 16200 },
        nodo: { id: 'a4cf129b40b2', batt_pct: 44, seq: 311 },
        bond: { dias_vividos: 214, dias_sanos: 200, racha: 31, mejor_racha: 66 },
      },
    ],
    nodosLibres: [{ id: 'a4cf129b40aa', rssi: -61, visto_hace_s: 12 }],
    /* Que carcasas ya tiene. Se agregan cuando el usuario declara
     * una al dar de alta una maceta: la coleccion es de objetos
     * fisicos, asi que la app registra lo que ya esta en la casa. */
    tengo: ['cresta', 'kawaii', 'hongo'],
  };
}

let estado = estadoInicial();
export const reset = () => { estado = estadoInicial(); };

/* ------------------------------------------------------------- ruteo ----- */
const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8', '.webmanifest': 'application/manifest+json',
  '.png': 'image/png', '.svg': 'image/svg+xml',
};

const json = (res, code, body) => {
  const s = JSON.stringify(body);
  res.writeHead(code, { 'content-type': 'application/json; charset=utf-8',
                        'content-length': Buffer.byteLength(s) });
  res.end(s);
};

const leerCuerpo = (req) => new Promise((resolve, reject) => {
  let b = '';
  req.on('data', (c) => {
    b += c;
    if (b.length > 4_000_000) reject(new Error('cuerpo demasiado grande'));
  });
  req.on('end', () => {
    try { resolve(b ? JSON.parse(b) : {}); } catch { reject(new Error('JSON invalido')); }
  });
  req.on('error', reject);
});

/** Toda la lógica de la API, separada del transporte para poder testearla. */
export async function manejarApi(metodo, ruta, cuerpo) {
  if (metodo === 'GET' && ruta === '/api/state') {
    return [200, { app: { fw: '0.6.0', aparatos: estado.nodes.length },
                   nodes: estado.nodes }];
  }

  if (metodo === 'GET' && ruta === '/api/species') {
    return [200, ESPECIES];
  }

  if (metodo === 'GET' && ruta === '/api/nodos') {
    return [200, estado.nodosLibres];
  }

  /* La coleccion es de OBJETOS FISICOS: que carcasas ya tenes en tu casa.
   * El modelo secreto no se lista hasta que aparece, que es exactamente lo
   * que hace que sea un secreto y no una casilla vacia con un signo de
   * pregunta que ya te contó cuantos te faltan. */
  if (metodo === 'GET' && ruta === '/api/collection') {
    const visibles = MODELOS.filter(
      (m) => m.rareza !== 'SECRETO' || estado.tengo.includes(m.id));
    return [200, {
      tengo: estado.tengo,
      total: MODELOS.filter((m) => m.rareza !== 'SECRETO').length,
      catalogo: visibles.map((m) => ({
        ...m, tengo: estado.tengo.includes(m.id),
      })),
    }];
  }

  /* Declarar una carcasa que te toco, sin tener que dar de alta una maceta.
   * Existe porque la caja se abre antes que la planta se registre, y el
   * momento de "me salio el secreto" no puede esperar a tener tierra. */
  if (metodo === 'POST' && ruta === '/api/collection') {
    const { modelo } = cuerpo || {};
    const m = MODELOS.find((x) => x.id === modelo);
    if (!m) return [400, { error: 'ese modelo no existe' }];
    const nuevo = !estado.tengo.includes(m.id);
    if (nuevo) estado.tengo.push(m.id);
    return [200, { ...m, tengo: true, nuevo }];
  }

  if (metodo === 'POST' && ruta === '/api/identify') {
    if (!cuerpo || !cuerpo.image_b64) return [400, { error: 'falta image_b64' }];
    /* En el Prime esto llama a una API de visión con la clave guardada
     * en NVS. Acá devolvemos algo estable para poder testear la interfaz. */
    const largo = String(cuerpo.image_b64).length;
    const elegida = ESPECIES[largo % ESPECIES.length];
    const confianza = largo % 7 === 0 ? 0.41 : 0.93;   /* a veces dudosa, a proposito */
    return [200, {
      especie: elegida.id, nombre: elegida.nombre, confianza,
      alternativas: ESPECIES.filter((e) => e.id !== elegida.id)
        .slice(0, 2).map((e) => ({ especie: e.id, confianza: 0.04 })),
    }];
  }

  if (metodo === 'POST' && ruta === '/api/nodes') {
    const { nombre, especie, nodo_id: nodoId, modelo } = cuerpo || {};
    if (!nombre || !especie) return [400, { error: 'faltan nombre o especie' }];
    if (!ESPECIES.some((e) => e.id === especie)) return [400, { error: 'especie desconocida' }];

    if (modelo && !MODELOS.some((m) => m.id === modelo)) {
      return [400, { error: 'ese modelo de carcasa no existe' }];
    }
    const nueva = {
      id: `p${++estado.seq}`, nombre: String(nombre).trim().slice(0, 17),
      especie,
      mood: 'UNKNOWN', severity: 'OK', reason: 'esperando la primera lectura',
      tel: { soil_pct: 0, temp_dc: 0, rh_pct: 0, lux: 0, batt_mv: 0, age_s: 999999 },
      modelo: modelo || null,
      link: nodoId ? 'VIVO' : 'NUNCA',
      bond: { dias_vividos: 0, dias_sanos: 0, racha: 0, mejor_racha: 0 },
      nodo: nodoId ? { id: nodoId, batt_pct: 100, seq: 0 } : null,
    };
    estado.nodes.push(nueva);
    if (nodoId) estado.nodosLibres = estado.nodosLibres.filter((s) => s.id !== nodoId);
    /* Si la carcasa declarada no estaba en la coleccion, se suma. No hay
     * azar: el azar ya ocurrio cuando el usuario abrio la caja. */
    const modeloNuevo = Boolean(modelo) && !estado.tengo.includes(modelo);
    if (modeloNuevo) estado.tengo.push(modelo);
    return [201, { ...nueva, modelo_nuevo: modeloNuevo }];
  }

  const mPlant = ruta.match(/^\/api\/nodes\/([A-Za-z0-9_-]+)$/);
  if (mPlant) {
    const p = estado.nodes.find((x) => x.id === mPlant[1]);
    if (!p) return [404, { error: 'no existe ese nodo' }];
    if (metodo === 'GET') return [200, p];
    if (metodo === 'PATCH') {
      if (cuerpo.nombre !== undefined) p.nombre = String(cuerpo.nombre).trim().slice(0, 17);
      if (cuerpo.especie !== undefined) {
        if (!ESPECIES.some((e) => e.id === cuerpo.especie)) {
          return [400, { error: 'especie desconocida' }];
        }
        p.especie = cuerpo.especie;
      }
      return [200, p];
    }
    if (metodo === 'DELETE') {
      estado.nodes = estado.nodes.filter((x) => x.id !== p.id);
      return [204, null];
    }
  }

  const mHist = ruta.match(/^\/api\/history\/([A-Za-z0-9_-]+)$/);
  if (metodo === 'GET' && mHist) {
    const p = estado.nodes.find((x) => x.id === mHist[1]);
    if (!p) return [404, { error: 'no existe ese nodo' }];
    const ahora = Math.floor(Date.now() / 1000);
    const puntos = Array.from({ length: 48 }, (_, i) => ({
      t: ahora - i * 1800,
      soil_pct: Math.max(0, p.tel.soil_pct + ((i * 7) % 19) - 4),
      temp_dc: p.tel.temp_dc + (((i * 13) % 40) - 20),
      rh_pct: p.tel.rh_pct + (((i * 5) % 12) - 6),
      lux: Math.max(0, Math.round(p.tel.lux * Math.max(0, Math.sin((i / 48) * Math.PI * 2)))),
    }));
    return [200, { id: p.id, puntos }];
  }

  return [404, { error: 'ruta desconocida' }];
}

/* -------------------------------------------------------------- estatico - */
async function servirEstatico(res, ruta) {
  const limpia = normalize(ruta === '/' ? '/index.html' : ruta).replace(/^(\.\.[/\\])+/, '');
  const archivo = join(RAIZ, limpia);
  if (!archivo.startsWith(RAIZ)) {          /* nunca salir del directorio */
    res.writeHead(403).end('prohibido');
    return;
  }
  try {
    const datos = await readFile(archivo);
    res.writeHead(200, { 'content-type': MIME[extname(archivo)] || 'application/octet-stream' });
    res.end(datos);
  } catch {
    res.writeHead(404, { 'content-type': 'text/plain; charset=utf-8' });
    res.end('no encontrado');
  }
}

export function crearServidor() {
  return createServer(async (req, res) => {
    const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
    if (url.pathname.startsWith('/api/')) {
      try {
        const cuerpo = ['POST', 'PATCH', 'PUT'].includes(req.method)
          ? await leerCuerpo(req) : null;
        const [code, body] = await manejarApi(req.method, url.pathname, cuerpo);
        if (code === 204) { res.writeHead(204).end(); return; }
        json(res, code, body);
      } catch (e) {
        json(res, 400, { error: e.message });
      }
      return;
    }
    await servirEstatico(res, url.pathname);
  });
}

if (process.argv[1] && process.argv[1].endsWith('dev-server.mjs')) {
  crearServidor().listen(PUERTO, () => {
    console.log(`Hub de desarrollo en http://localhost:${PUERTO}`);
    console.log('En el Prime real esto lo sirve el ESP32 en rootkit.local');
  });
}

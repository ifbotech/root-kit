/* dev-server.mjs — implementa el contrato de API.md en Node.
 *
 * Existe para desarrollar y testear el Hub sin la Terminal. Es también la
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
/* Mismos valores que firmware/core/species.c. */
export const ESPECIES = [
  { id: 'monstera', nombre: 'Monstera deliciosa', soil_min: 25, soil_max: 60,
    temp_min_dc: 180, temp_max_dc: 300, rh_min: 50, lux_min: 1000, lux_max: 15000 },
  { id: 'pothos', nombre: 'Potus', soil_min: 20, soil_max: 55,
    temp_min_dc: 170, temp_max_dc: 300, rh_min: 40, lux_min: 500, lux_max: 12000 },
  { id: 'sansevieria', nombre: 'Lengua de suegra', soil_min: 8, soil_max: 35,
    temp_min_dc: 150, temp_max_dc: 320, rh_min: 30, lux_min: 800, lux_max: 30000 },
  { id: 'ficus-lyrata', nombre: 'Ficus lyrata', soil_min: 25, soil_max: 55,
    temp_min_dc: 180, temp_max_dc: 270, rh_min: 50, lux_min: 2000, lux_max: 20000 },
  { id: 'cactus', nombre: 'Cactus / suculenta', soil_min: 5, soil_max: 25,
    temp_min_dc: 100, temp_max_dc: 380, rh_min: 20, lux_min: 5000, lux_max: 80000 },
];

/* Un simbionte por especie. El desbloqueo es determinista: registrás una
 * especie nueva y aparece el suyo. Nada aleatorio, nada con dinero. */
export const SIMBIONTES = [
  { id: 'tuga', nombre: 'Tuga.exe', especie: 'monstera' },
  { id: 'myco', nombre: 'Myco.zip', especie: 'pothos' },
  { id: 'sable', nombre: 'Sable.bin', especie: 'sansevieria' },
  { id: 'ficus', nombre: 'Lyra.dll', especie: 'ficus-lyrata' },
  { id: 'spine', nombre: 'Spine.sys', especie: 'cactus' },
];

/* --------------------------------------------------------------- estado -- */
export function estadoInicial() {
  return {
    seq: 2,
    plants: [
      {
        id: 'p1', nombre: 'MONSTERA', especie: 'monstera', simbionte: 'tuga',
        mood: 'THIRSTY', severity: 'URGENT', reason: 'tengo sed',
        tel: { soil_pct: 22, temp_dc: 236, rh_pct: 54, lux: 5200, batt_mv: 3810, age_s: 240 },
        spore: { id: 'a4cf129b4011', batt_pct: 62, seq: 4211 },
      },
      {
        id: 'p2', nombre: 'POTUS', especie: 'pothos', simbionte: 'myco',
        mood: 'HAPPY', severity: 'OK', reason: 'estoy perfecta',
        tel: { soil_pct: 44, temp_dc: 229, rh_pct: 48, lux: 3100, batt_mv: 3950, age_s: 95 },
        spore: { id: 'a4cf129b4077', batt_pct: 81, seq: 980 },
      },
    ],
    sporesLibres: [{ id: 'a4cf129b40aa', rssi: -61, visto_hace_s: 12 }],
    desbloqueados: ['tuga', 'myco'],
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
    return [200, { terminal: { fw: '0.4.0', uptime_s: 84213, wifi_rssi: -54 },
                   plants: estado.plants }];
  }

  if (metodo === 'GET' && ruta === '/api/species') {
    return [200, ESPECIES];
  }

  if (metodo === 'GET' && ruta === '/api/spores') {
    return [200, estado.sporesLibres];
  }

  if (metodo === 'GET' && ruta === '/api/collection') {
    return [200, {
      desbloqueados: estado.desbloqueados,
      total: SIMBIONTES.length,
      catalogo: SIMBIONTES.map((s) => ({ ...s, desbloqueado: estado.desbloqueados.includes(s.id) })),
    }];
  }

  if (metodo === 'POST' && ruta === '/api/identify') {
    if (!cuerpo || !cuerpo.image_b64) return [400, { error: 'falta image_b64' }];
    /* En la Terminal esto llama a una API de visión con la clave guardada
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

  if (metodo === 'POST' && ruta === '/api/plants') {
    const { nombre, especie, spore_id: sporeId } = cuerpo || {};
    if (!nombre || !especie) return [400, { error: 'faltan nombre o especie' }];
    if (!ESPECIES.some((e) => e.id === especie)) return [400, { error: 'especie desconocida' }];

    const sim = SIMBIONTES.find((s) => s.especie === especie) || SIMBIONTES[0];
    const nueva = {
      id: `p${++estado.seq}`, nombre: String(nombre).trim().slice(0, 17),
      especie, simbionte: sim.id,
      mood: 'UNKNOWN', severity: 'OK', reason: 'esperando la primera lectura',
      tel: { soil_pct: 0, temp_dc: 0, rh_pct: 0, lux: 0, batt_mv: 0, age_s: 999999 },
      spore: sporeId ? { id: sporeId, batt_pct: 100, seq: 0 } : null,
    };
    estado.plants.push(nueva);
    if (sporeId) estado.sporesLibres = estado.sporesLibres.filter((s) => s.id !== sporeId);
    /* Desbloqueo determinista al registrar una especie nueva. */
    const nuevoSimbionte = !estado.desbloqueados.includes(sim.id);
    if (nuevoSimbionte) estado.desbloqueados.push(sim.id);
    return [201, { ...nueva, simbionte_nuevo: nuevoSimbionte }];
  }

  const mPlant = ruta.match(/^\/api\/plants\/([A-Za-z0-9_-]+)$/);
  if (mPlant) {
    const p = estado.plants.find((x) => x.id === mPlant[1]);
    if (!p) return [404, { error: 'no existe esa planta' }];
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
      estado.plants = estado.plants.filter((x) => x.id !== p.id);
      return [204, null];
    }
  }

  const mHist = ruta.match(/^\/api\/history\/([A-Za-z0-9_-]+)$/);
  if (metodo === 'GET' && mHist) {
    const p = estado.plants.find((x) => x.id === mHist[1]);
    if (!p) return [404, { error: 'no existe esa planta' }];
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
    console.log('En la Terminal real esto lo sirve el ESP32 en rootkit.local');
  });
}

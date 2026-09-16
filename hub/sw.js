/* sw.js — el service worker.
 *
 * Hace dos cosas y ninguna más: que la app abra sin red, y que se actualice
 * sola cuando hay una versión nueva.
 *
 * LA REGLA QUE NO SE NEGOCIA: LOS DATOS NUNCA SE CACHEAN.
 *
 * Una lectura vieja mostrada como actual es peor que no mostrar nada. El
 * usuario decide si regar mirando ese número, y un "34%" de ayer lo hace
 * regar una planta que ya está mojada. Así que todo lo que cuelga de /api/
 * va directo a la red, y si no hay red la app lo dice en vez de inventar.
 *
 * El armazón —HTML, CSS, módulos, iconos— sí se cachea, porque eso no
 * caduca: es la misma interfaz muestre lo que muestre.
 */

/* Subir la versión invalida el caché entero. Cuando se toca cualquier archivo
   del armazón hay que subirla, o el teléfono sigue con la versión vieja. */
const CACHE = 'rootkit-v2';

const ARMAZON = [
  '.',
  'index.html',
  'app.js',
  'style.css',
  'manifest.webmanifest',
  'lib/model.mjs',
  'lib/ui.mjs',
  'lib/tareas.mjs',
  'lib/diagnostico.mjs',
  'lib/gamificacion.mjs',
  'vistas/hoy.mjs',
  'vistas/plantas.mjs',
  'vistas/escaner.mjs',
  'vistas/coleccion.mjs',
  'iconos/icono-192.png',
  'iconos/icono-512.png',
  'iconos/apple-touch-icon.png',
];

self.addEventListener('install', (e) => {
  e.waitUntil(
    caches.open(CACHE)
      /* addAll falla entero si un solo archivo falla, y eso dejaría la app
         sin service worker. Se piden de a uno y se toleran las bajas. */
      .then((c) => Promise.allSettled(ARMAZON.map((u) => c.add(u))))
      .then(() => self.skipWaiting()),
  );
});

self.addEventListener('activate', (e) => {
  e.waitUntil(
    caches.keys()
      .then((ks) => Promise.all(ks.filter((k) => k !== CACHE).map((k) => caches.delete(k))))
      .then(() => self.clients.claim()),
  );
});

self.addEventListener('fetch', (e) => {
  const url = new URL(e.request.url);

  /* Los datos, siempre de la red. Sin excepciones y sin caché de respaldo. */
  if (url.pathname.startsWith('/api/')) {
    return;
  }
  if (e.request.method !== 'GET') {
    return;
  }

  /* El armazón: primero el caché, y de fondo se busca una versión nueva para
     la próxima vez. Así la app abre instantánea y aun así se mantiene al día
     sin que el usuario tenga que hacer nada. */
  e.respondWith(
    caches.match(e.request).then((guardada) => {
      const red = fetch(e.request)
        .then((r) => {
          if (r && r.ok) {
            const copia = r.clone();
            caches.open(CACHE).then((c) => c.put(e.request, copia));
          }
          return r;
        })
        .catch(() => guardada);
      return guardada || red;
    }),
  );
});

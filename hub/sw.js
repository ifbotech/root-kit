/* Service worker: la app se instala y abre sin red, pero los datos NUNCA se
   cachean. Una lectura vieja mostrada como actual es peor que no mostrar
   nada: el usuario decide si regar mirando ese número. */
const CACHE = 'rootkit-v1';
const SHELL = ['.', 'index.html', 'app.js', 'style.css', 'lib/model.mjs', 'manifest.webmanifest'];

self.addEventListener('install', (e) => {
  e.waitUntil(caches.open(CACHE).then((c) => c.addAll(SHELL)).then(() => self.skipWaiting()));
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
  if (url.pathname.startsWith('/api/')) return;   /* los datos siempre de la red */
  e.respondWith(caches.match(e.request).then((r) => r || fetch(e.request)));
});

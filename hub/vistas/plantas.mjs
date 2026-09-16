/* plantas.mjs — la lista y el detalle de cada maceta.
 *
 * Acá viven los números. Es la pantalla a la que va alguien que ya sabe que
 * algo pasa y quiere ver por qué, así que no se ahorra información: los
 * cuatro sensores con su rango, la antigüedad de la lectura, la pila, el
 * vínculo y el modelo de carcasa.
 *
 * La diferencia con un tablero cualquiera es que **cada número viene con su
 * rango**. Un 34% no significa nada solo; un 34% sobre una barra que marca
 * "esta especie quiere entre 25 y 60" se lee sin pensar.
 */
import { h, render, icono, medidor, progreso } from '../lib/ui.mjs';
import {
  MOOD_ES, LINK_ES, ETAPA_ES, formatTemp, formatLux, formatEdad,
  ordenarNodos, etapaDe, progresoEtapa, bateriaDe,
} from '../lib/model.mjs';

const SEV_CLASE = { URGENT: 'urgente', WATCH: 'atencion', OK: 'bien' };

/* Qué medidor corresponde a cada magnitud, con su rango de especie. */
function medidores(n, esp) {
  const t = n.tel || {};
  const m = n.mood;
  const estado = (mala) => (mala ? SEV_CLASE[n.severity] || 'ok' : 'ok');

  return [
    medidor({
      etiqueta: 'Tierra', texto: `${t.soil_pct ?? '—'}%`,
      valor: t.soil_pct, min: 0, max: 100,
      lo: esp?.soil_min, hi: esp?.soil_max,
      estado: estado(m === 'THIRSTY' || m === 'DROWNING'),
    }),
    medidor({
      etiqueta: 'Temperatura', texto: formatTemp(t.temp_dc),
      valor: t.temp_dc, min: 0, max: 450,
      lo: esp?.temp_min_dc, hi: esp?.temp_max_dc,
      estado: estado(m === 'COLD' || m === 'HOT'),
    }),
    medidor({
      etiqueta: 'Luz', texto: formatLux(t.lux),
      /* La luz se comprime: de 0 a 40k lineal deja todo el interior
         aplastado contra el borde izquierdo y la barra no dice nada. */
      valor: Math.min(t.lux ?? 0, 40000) / 400, min: 0, max: 100,
      lo: (esp?.lux_min ?? 0) / 400,
      hi: Math.min(esp?.lux_max ?? 40000, 40000) / 400,
      estado: estado(m === 'DARK' || m === 'SCORCHED'),
    }),
    medidor({
      etiqueta: 'Humedad del aire', texto: `${t.rh_pct ?? '—'}%`,
      valor: t.rh_pct, min: 0, max: 100,
      lo: esp?.rh_min, hi: 100,
      estado: estado(m === 'PARCHED_AIR'),
    }),
  ];
}

function fila(n, esp, alAbrir) {
  const sanos = n.bond?.dias_sanos ?? 0;
  const bat = bateriaDe(n);

  return h('li', {
    class: `planta sev-${SEV_CLASE[n.severity] || 'bien'}`,
    tabindex: '0', role: 'button',
    onClick: () => alAbrir(n.id),
    onKeydown: (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); alAbrir(n.id); } },
  },
    h('div', { class: 'planta-cab' },
      h('h3', {}, n.nombre),
      h('span', { class: 'chip' }, MOOD_ES[n.mood] || n.mood)),
    h('p', { class: 'planta-especie' },
      esp?.nombre || 'sin identificar',
      n.modelo ? ' · ' : '', n.modelo || ''),
    h('div', { class: 'planta-mini' },
      h('span', {}, `${n.tel?.soil_pct ?? '—'}%`),
      h('span', {}, formatTemp(n.tel?.temp_dc)),
      h('span', {}, formatLux(n.tel?.lux)),
      bat !== null ? h('span', { class: bat < 15 ? 'bajo' : '' }, `${bat}%`) : null),
    h('div', { class: 'planta-pie' },
      h('span', { class: `enlace enlace-${(n.link || '').toLowerCase()}` },
        LINK_ES[n.link] || '—'),
      h('span', {}, formatEdad(n.tel?.age_s)),
      h('span', { class: 'etapa' }, ETAPA_ES[etapaDe(sanos)] || '')));
}

export function vistaPlantas(ctx) {
  const { estado, especies, alAbrir, irA } = ctx;
  const nodos = ordenarNodos(estado?.nodes || []);
  const porId = new Map((especies || []).map((e) => [e.id, e]));
  const cont = h('div', { class: 'vista' });

  render(cont,
    h('header', { class: 'vista-cab' },
      h('h2', {}, 'Mis plantas'),
      h('button', { class: 'boton chico', type: 'button',
                    onClick: () => irA('escaner') },
        icono('camara', 16), ' Agregar')),
    nodos.length === 0
      ? h('section', { class: 'panel vacio' },
          h('p', {}, 'No hay macetas todavía.'))
      : h('ul', { class: 'plantas' },
          nodos.map((n) => fila(n, porId.get(n.especie), alAbrir))));

  return cont;
}

/* ----------------------------------------------------------- detalle --- */
export function vistaDetalle(ctx) {
  const { estado, especies, plantaId, volver, alDiagnosticar } = ctx;
  const n = (estado?.nodes || []).find((x) => x.id === plantaId);
  const cont = h('div', { class: 'vista' });

  if (!n) {
    render(cont,
      h('section', { class: 'panel vacio' },
        h('p', {}, 'Esa maceta ya no está.'),
        h('button', { class: 'boton', type: 'button', onClick: volver }, 'Volver')));
    return cont;
  }

  const esp = (especies || []).find((e) => e.id === n.especie);
  const sanos = n.bond?.dias_sanos ?? 0;
  const etapa = etapaDe(sanos);
  const bat = bateriaDe(n);

  render(cont,
    h('header', { class: 'vista-cab' },
      h('button', { class: 'boton chico', type: 'button', onClick: volver }, '‹ Volver'),
      h('h2', {}, n.nombre)),

    h('section', { class: `panel estado-grande sev-${SEV_CLASE[n.severity] || 'bien'}` },
      h('p', { class: 'dice' }, `«${n.reason || ''}»`),
      h('p', { class: 'estado-sub' },
        esp?.nombre || 'sin identificar',
        ' · ', LINK_ES[n.link] || '—',
        ' · ', formatEdad(n.tel?.age_s))),

    /* El botón de diagnóstico va arriba de los números a propósito: si
       alguien entró acá es porque algo le llamó la atención, y la foto es la
       única herramienta que puede explicar lo que los números no. */
    h('section', { class: 'panel' },
      h('button', { class: 'boton primario ancho', type: 'button',
                    onClick: () => alDiagnosticar(n.id) },
        icono('lupa', 18), ' Diagnosticar con una foto'),
      h('p', { class: 'nota' },
        'Sirve sobre todo cuando los números están bien y la planta igual se ve mal: hongos, plagas o falta de nutrientes no mueven ningún sensor.')),

    h('section', { class: 'panel' },
      h('h3', { class: 'panel-tit' }, 'Lo que miden los sensores'),
      medidores(n, esp)),

    h('section', { class: 'panel' },
      h('h3', { class: 'panel-tit' }, 'Vínculo'),
      h('div', { class: 'vinculo-cab' },
        h('span', { class: 'etapa-grande' }, ETAPA_ES[etapa] || etapa),
        h('span', { class: 'vinculo-dias' }, `${sanos} días sanos`)),
      progreso(progresoEtapa(sanos)),
      h('dl', { class: 'datos' },
        h('div', {}, h('dt', {}, 'Racha'), h('dd', {}, `${n.bond?.racha ?? 0} días`)),
        h('div', {}, h('dt', {}, 'Mejor racha'), h('dd', {}, `${n.bond?.mejor_racha ?? 0} días`)),
        h('div', {}, h('dt', {}, 'Con vos hace'), h('dd', {}, `${n.bond?.dias_vividos ?? 0} días`)))),

    h('section', { class: 'panel' },
      h('h3', { class: 'panel-tit' }, 'El aparato'),
      h('dl', { class: 'datos' },
        h('div', {}, h('dt', {}, 'Carcasa'), h('dd', {}, n.modelo || 'sin declarar')),
        h('div', {}, h('dt', {}, 'Pila'), h('dd', {}, bat === null ? '—' : `${bat}%`)),
        h('div', {}, h('dt', {}, 'Identificador'),
          h('dd', { class: 'mono' }, n.nodo?.id?.slice(-6) || '—')))));

  return cont;
}

/* hoy.mjs — la primera pantalla: qué hay que hacer.
 *
 * El orden de arriba hacia abajo no es decorativo, es la jerarquía de lo que
 * alguien necesita al abrir la app:
 *
 *   1. las TAREAS, porque son lo único accionable
 *   2. los CONTADORES, porque contestan "¿está todo bien?" de un vistazo
 *   3. el NIVEL, porque es recompensa y la recompensa va después del trabajo
 *
 * Poner el nivel arriba lo convertiría en el objetivo. Abajo, es lo que ves
 * después de resolver las cosas, que es cuando significa algo.
 */
import { h, render, icono, progreso } from '../lib/ui.mjs';
import { tareasDelDia, resumenDeTareas, contarEstados, URGENCIA_ES } from '../lib/tareas.mjs';
import { xpTotal, nivelDe, saludo, evaluarLogros } from '../lib/gamificacion.mjs';

function tarjetaTarea(t, alHacer) {
  return h('li', { class: `tarea tarea-${t.urgencia}`, dataset: { id: t.id } },
    h('div', { class: 'tarea-icono' }, icono(t.icono, 22)),
    h('div', { class: 'tarea-cuerpo' },
      h('h3', {}, t.titulo),
      h('p', { class: 'tarea-detalle' }, t.detalle),
      h('span', { class: 'tarea-urgencia' }, URGENCIA_ES[t.urgencia])),
    h('button', {
      class: 'tarea-hecha',
      type: 'button',
      title: t.auto
        ? 'Marcala como hecha; el sensor lo va a confirmar solo'
        : 'Marcala como hecha',
      'aria-label': `Marcar como hecha: ${t.titulo}`,
      onClick: () => alHacer(t),
    }, icono('tilde', 20)));
}

function bloqueContadores(c) {
  const chip = (n, et, clase) => (n > 0
    ? h('div', { class: `contador ${clase}` },
        h('b', {}, String(n)), h('span', {}, et))
    : null);

  return h('div', { class: 'contadores' },
    h('div', { class: 'contador contador-total' },
      h('b', {}, String(c.total)),
      h('span', {}, c.total === 1 ? 'planta' : 'plantas')),
    chip(c.urgente, c.urgente === 1 ? 'urgente' : 'urgentes', 'contador-urgente'),
    chip(c.atencion, 'para mirar', 'contador-atencion'),
    chip(c.bien, c.bien === 1 ? 'cómoda' : 'cómodas', 'contador-bien'),
    chip(c.sinDatos, 'sin datos', 'contador-sindatos'));
}

function bloqueNivel(nivel, racha, logros) {
  const cumplidos = logros.filter((l) => l.cumplido);
  const ultimo = cumplidos[cumplidos.length - 1];

  return h('section', { class: 'panel panel-nivel' },
    h('div', { class: 'nivel-cab' },
      h('div', {},
        h('span', { class: 'nivel-num' }, `Nivel ${nivel.nivel}`),
        h('h3', {}, nivel.titulo)),
      racha.dias > 0
        ? h('div', { class: 'racha', title: `Mejor racha: ${racha.mejor} días` },
            icono('llama', 18), h('b', {}, String(racha.dias)),
            h('span', {}, racha.dias === 1 ? 'día' : 'días'))
        : null),
    progreso(nivel.progreso, 'progreso-nivel'),
    h('p', { class: 'nivel-pie' },
      nivel.siguiente
        ? `${nivel.faltan} XP para ${nivel.siguiente.titulo}`
        : 'Llegaste al último nivel'),
    ultimo
      ? h('p', { class: 'logro-ultimo' },
          icono('trofeo', 16), ` ${ultimo.nombre} — ${ultimo.detalle}`)
      : null,
    h('p', { class: 'nivel-nota' },
      'La XP sale de días sanos y de resolver problemas. Abrir la app no suma.'));
}

export function vistaHoy(ctx) {
  const { estado, especies, hechas, racha, coleccion, alHacer, irA } = ctx;
  const nodos = estado?.nodes || [];
  const tareas = tareasDelDia(nodos, especies, hechas);
  const resumen = resumenDeTareas(tareas);
  const cuenta = contarEstados(nodos);
  const nivel = nivelDe(xpTotal(nodos));
  const logros = evaluarLogros({ nodos, racha, coleccion });
  const hayUrgentes = tareas.some((t) => t.urgencia === 'urgente');

  const cont = h('div', { class: 'vista' });

  render(cont,
    h('header', { class: 'saludo' },
      h('p', { class: 'saludo-hora' }, saludo(new Date().getHours(), hayUrgentes)),
      h('h2', { class: `saludo-titulo tono-${resumen.tono}` }, resumen.titulo),
      resumen.detalle ? h('p', { class: 'saludo-detalle' }, resumen.detalle) : null),

    nodos.length === 0
      ? h('section', { class: 'panel vacio' },
          h('p', {}, 'Todavía no hay ninguna maceta conectada.'),
          h('button', { class: 'boton primario', type: 'button',
                        onClick: () => irA('escaner') },
            icono('camara', 18), ' Registrar la primera'))
      : bloqueContadores(cuenta),

    tareas.length > 0
      ? h('section', { class: 'panel panel-tareas' },
          h('ul', { class: 'tareas' },
            tareas.map((t) => tarjetaTarea(t, alHacer))))
      : nodos.length > 0
        ? h('section', { class: 'panel panel-libre' },
            icono('tilde', 28),
            h('p', {}, 'Nada pendiente. Tus plantas están cómodas.'))
        : null,

    nodos.length > 0 ? bloqueNivel(nivel, racha, logros) : null);

  return cont;
}

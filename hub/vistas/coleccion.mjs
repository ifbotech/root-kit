/* coleccion.mjs — las carcasas que ya tenés, y los logros.
 *
 * La colección es de OBJETOS FÍSICOS: modelos impresos que salieron en la
 * caja. Por eso la vista deja declarar uno sin registrar una planta — la caja
 * se abre antes de que haya tierra, y el momento de "me salió el secreto" no
 * puede esperar.
 *
 * El modelo secreto no se lista hasta que aparece. Mostrarlo en gris ya le
 * contaría al usuario que existe, y con eso deja de ser un secreto para ser
 * una casilla vacía que además le informa cuántas le faltan.
 */
import { h, render, icono } from '../lib/ui.mjs';
import { RAREZA_ES, ordenarColeccion, progresoColeccion } from '../lib/model.mjs';
import { evaluarLogros } from '../lib/gamificacion.mjs';

function ficha(m) {
  return h('li', { class: `modelo rar-${m.rareza.toLowerCase()} ${m.tengo ? 'tengo' : 'falta'}` },
    h('div', { class: 'modelo-marco' },
      m.tengo ? icono('caja', 30) : h('span', { class: 'interrogante' }, '?')),
    h('h3', {}, m.tengo ? m.nombre : '???'),
    h('span', { class: 'modelo-rareza' }, RAREZA_ES[m.rareza] || m.rareza),
    m.tengo && m.lema ? h('p', { class: 'modelo-lema' }, m.lema) : null);
}

export function vistaColeccion(ctx) {
  const { coleccion, estado, racha, alDeclarar } = ctx;
  const catalogo = coleccion?.catalogo || [];
  const p = progresoColeccion(catalogo, coleccion?.tengo || []);
  const logros = evaluarLogros({
    nodos: estado?.nodes || [], racha, coleccion: p,
  });
  const faltantes = catalogo.filter((m) => !m.tengo);
  const cont = h('div', { class: 'vista' });

  const selNueva = h('select', { id: 'declarar' },
    h('option', { value: '' }, '— elegí cuál —'),
    /* Se listan TODOS los modelos, tenga o no el usuario, porque acá está
       declarando lo que acaba de sacar de una caja. El secreto también: si
       le salió, tiene que poder decirlo. */
    catalogo.concat(p.secretos ? [] : []).map((m) => h('option', { value: m.id }, m.nombre)));

  render(cont,
    h('header', { class: 'vista-cab' }, h('h2', {}, 'Colección')),

    h('section', { class: 'panel panel-coleccion' },
      h('p', { class: 'coleccion-resumen' },
        p.completa
          ? `Los tenés todos${p.secretos ? ', secreto incluido' : ''}`
          : `${p.tengo} de ${p.total}`),
      h('ul', { class: 'modelos' }, ordenarColeccion(catalogo).map(ficha)),
      faltantes.length === 0 && !p.secretos
        ? h('p', { class: 'nota' },
            'Tenés los cinco de la caja. Dicen que hay uno más.')
        : null),

    h('section', { class: 'panel form' },
      h('h3', { class: 'panel-tit' }, '¿Abriste una caja?'),
      h('p', { class: 'nota' }, 'Decime cuál te salió y la sumo, sin necesidad de registrar una planta.'),
      selNueva,
      h('button', {
        class: 'boton ancho', type: 'button',
        onClick: () => { if (selNueva.value) alDeclarar(selNueva.value); },
      }, 'Sumar a la colección')),

    h('section', { class: 'panel' },
      h('h3', { class: 'panel-tit' }, 'Logros'),
      h('ul', { class: 'logros' },
        logros.map((l) => h('li', { class: l.cumplido ? 'logro hecho' : 'logro' },
          icono(l.cumplido ? 'trofeo' : 'hoja', 18),
          h('div', {},
            h('b', {}, l.nombre),
            h('span', {}, l.detalle)))))));

  return cont;
}

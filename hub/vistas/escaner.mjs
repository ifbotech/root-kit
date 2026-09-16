/* escaner.mjs — la cámara, en sus dos trabajos.
 *
 * IDENTIFICAR: sacarle una foto a una planta nueva para saber qué es, y de
 * ahí sacar los umbrales con los que se la va a juzgar el resto de su vida.
 * Por eso la confianza se muestra siempre y una identificación dudosa se
 * puede corregir antes de guardar.
 *
 * DIAGNOSTICAR: sacarle una foto a una planta que ya está registrada para
 * cruzar lo que se ve con lo que miden los sensores. Ver lib/diagnostico.mjs
 * para por qué eso vale la pena.
 *
 * CÓMO SE ABRE LA CÁMARA SIN APP NATIVA
 *
 * `<input type="file" accept="image/*" capture="environment">`. Es una línea
 * de HTML y abre la cámara trasera en Android y en iOS. No hace falta
 * getUserMedia, ni permisos persistentes, ni una pantalla de previsualización
 * propia: el sistema operativo ya tiene una y es mejor que la que haríamos.
 */
import { h, render, icono, progreso } from '../lib/ui.mjs';
import { validarAlta, interpretarIdentificacion } from '../lib/model.mjs';
import { diagnosticar, HALLAZGO_ES } from '../lib/diagnostico.mjs';

/** Lee un File como base64 sin el prefijo data:. */
export function leerBase64(archivo) {
  return new Promise((res, rej) => {
    const fr = new FileReader();
    fr.onerror = () => rej(new Error('no pude leer la foto'));
    fr.onload = () => res(String(fr.result).split(',')[1] || '');
    fr.readAsDataURL(archivo);
  });
}

function campoFoto(id, texto, alElegir) {
  return h('label', { class: 'foto-campo', for: id },
    icono('camara', 26),
    h('span', {}, texto),
    h('input', {
      type: 'file', id, accept: 'image/*', capture: 'environment',
      class: 'oculto',
      onChange: (e) => { if (e.target.files?.[0]) alElegir(e.target.files[0]); },
    }));
}

/* ------------------------------------------------------- identificar --- */
export function vistaEscaner(ctx) {
  const { especies, modelos, api, avisar, alRegistrar, irA } = ctx;
  const cont = h('div', { class: 'vista' });

  let sugerida = null;      /* lo que dijo la IA */
  let previewUrl = null;

  const zona = h('div', { class: 'escaner-zona' });
  const errores = h('p', { class: 'errores', hidden: true });

  const selEspecie = h('select', { id: 'especie' },
    h('option', { value: '' }, '— elegí una —'),
    (especies || []).map((e) => h('option', { value: e.id }, e.nombre)));

  const selModelo = h('select', { id: 'modelo' },
    h('option', { value: '' }, '— la cargo después —'),
    (modelos || []).map((m) => h('option', { value: m.id }, m.nombre)));

  const inNombre = h('input', {
    type: 'text', id: 'nombre', maxlength: '17', autocomplete: 'off',
    placeholder: 'Monstera del living',
  });

  async function identificar(archivo) {
    if (previewUrl) URL.revokeObjectURL(previewUrl);
    previewUrl = URL.createObjectURL(archivo);
    render(zona,
      h('img', { class: 'preview', src: previewUrl, alt: 'La foto que sacaste' }),
      h('p', { class: 'cargando' }, 'Reconociendo…'));

    try {
      const b64 = await leerBase64(archivo);
      const r = await api('/api/identify', {
        method: 'POST',
        body: JSON.stringify({ image_b64: b64, mime: archivo.type }),
      });
      sugerida = interpretarIdentificacion(r);
      if (sugerida.especie) {
        selEspecie.value = sugerida.especie;
      }
      render(zona,
        h('img', { class: 'preview', src: previewUrl, alt: 'La foto que sacaste' }),
        h('p', { class: `ident ident-${sugerida.estado}` }, sugerida.mensaje),
        /* Una identificación dudosa no se acepta sola: de la especie salen
           los umbrales, y un error acá hace que el aparato juzgue mal la
           planta durante meses sin que nadie sospeche. */
        sugerida.estado === 'dudoso'
          ? h('p', { class: 'nota' }, 'Revisá la especie abajo antes de guardar.')
          : null);
    } catch (e) {
      render(zona, h('p', { class: 'errores' }, `No pude reconocerla: ${e.message}`));
    }
  }

  async function guardar(ev) {
    ev.preventDefault();
    const datos = {
      nombre: inNombre.value,
      especie: selEspecie.value,
      modelo: selModelo.value || undefined,
    };
    const v = validarAlta(datos, (especies || []).map((e) => e.id));
    errores.hidden = v.ok;
    errores.textContent = v.errores.join(' ');
    if (!v.ok) return;

    try {
      const creada = await alRegistrar(datos);
      avisar(creada.modelo_nuevo
        ? `Listo. ${datos.modelo} se suma a tu colección.`
        : `Listo, ${creada.nombre} quedó registrada.`);
      irA('plantas');
    } catch (e) {
      avisar(`No pude registrarla: ${e.message}`, true);
    }
  }

  render(cont,
    h('header', { class: 'vista-cab' }, h('h2', {}, 'Registrar una planta')),
    h('section', { class: 'panel' },
      campoFoto('foto-ident', 'Sacale una foto y te digo qué es', identificar),
      zona),
    h('form', { class: 'panel form', onSubmit: guardar },
      h('label', { for: 'nombre' }, 'Cómo le decís'),
      inNombre,
      h('label', { for: 'especie' }, 'Especie'),
      selEspecie,
      h('label', { for: 'modelo' }, 'Carcasa que te tocó'),
      selModelo,
      errores,
      h('button', { class: 'boton primario ancho', type: 'submit' }, 'Guardar')));

  return cont;
}

/* ------------------------------------------------------- diagnosticar --- */
function tarjetaConclusion(c) {
  return h('li', { class: `conclusion grav-${c.gravedad} ${c.confirma ? '' : 'revela'}` },
    h('div', { class: 'conclusion-cab' },
      h('h4', {}, c.causa),
      h('span', { class: 'conclusion-visto' }, HALLAZGO_ES[c.hallazgo] || c.hallazgo)),
    h('p', {}, c.detalle),
    c.accion ? h('p', { class: 'conclusion-accion' }, icono('tilde', 14), ' ', c.accion) : null,
    !c.confirma
      ? h('p', { class: 'conclusion-nota' },
          'Ningún sensor puede detectar esto: sólo se ve mirando.')
      : null);
}

export function vistaDiagnostico(ctx) {
  const { estado, especies, plantaId, api, volver } = ctx;
  const n = (estado?.nodes || []).find((x) => x.id === plantaId);
  const cont = h('div', { class: 'vista' });

  if (!n) {
    render(cont, h('section', { class: 'panel vacio' },
      h('p', {}, 'Esa maceta ya no está.'),
      h('button', { class: 'boton', type: 'button', onClick: volver }, 'Volver')));
    return cont;
  }

  const esp = (especies || []).find((e) => e.id === n.especie);
  const zona = h('div', { class: 'escaner-zona' });
  let previewUrl = null;

  async function analizar(archivo) {
    if (previewUrl) URL.revokeObjectURL(previewUrl);
    previewUrl = URL.createObjectURL(archivo);
    render(zona,
      h('img', { class: 'preview', src: previewUrl, alt: 'La foto de tu planta' }),
      h('p', { class: 'cargando' }, 'Comparando la foto con los sensores…'));

    try {
      const b64 = await leerBase64(archivo);
      const r = await api('/api/diagnose', {
        method: 'POST',
        body: JSON.stringify({ image_b64: b64, planta: n.id, mime: archivo.type }),
      });
      const d = diagnosticar(r.hallazgos, n.tel, esp);

      render(zona,
        h('img', { class: 'preview', src: previewUrl, alt: 'La foto de tu planta' }),
        h('div', { class: `veredicto veredicto-${d.veredicto}` },
          h('h3', {}, d.titulo),
          h('p', {}, d.resumen)),
        d.conclusiones.length
          ? h('ul', { class: 'conclusiones' }, d.conclusiones.map(tarjetaConclusion))
          : null,
        h('p', { class: 'nota' },
          'Esto es una orientación, no un diagnóstico de laboratorio. Si algo no cierra, mirá las raíces.'));
    } catch (e) {
      render(zona, h('p', { class: 'errores' }, `No pude analizarla: ${e.message}`));
    }
  }

  render(cont,
    h('header', { class: 'vista-cab' },
      h('button', { class: 'boton chico', type: 'button', onClick: volver }, '‹ Volver'),
      h('h2', {}, `Diagnóstico de ${n.nombre}`)),

    h('section', { class: 'panel' },
      h('p', { class: 'nota' },
        `Ahora mismo: tierra ${n.tel?.soil_pct ?? '—'}%, ${esp?.nombre || 'especie sin identificar'}. `
        + 'Sacá una foto de la planta entera, con luz y sin contraluz.'),
      campoFoto('foto-diag', 'Sacar la foto', analizar),
      zona));

  return cont;
}

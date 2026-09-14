/* model.mjs — lógica pura del Hub.
 *
 * Deliberadamente NO incluye evaluación de estados de ánimo: eso lo calcula
 * la Terminal en firmware/core/mood.c y llega ya resuelto. Dos
 * implementaciones de la misma regla terminan divergiendo, y el día que
 * divergen el usuario ve una cara en la pantalla y otra distinta en el
 * teléfono.
 *
 * Acá va lo que sí es responsabilidad de la vista: formatear, validar
 * entradas antes de mandarlas, y decidir el orden en que se muestran las
 * plantas.
 */

/* Severidades, de más a menos urgente. El orden importa: es el que define
 * qué planta va primero en la lista. */
export const SEVERIDADES = ['URGENT', 'WATCH', 'OK'];

export const MOOD_ES = {
  UNKNOWN: 'sin datos',
  OFFLINE: 'sin señal',
  SLEEPING: 'durmiendo',
  HAPPY: 'bien',
  THIRSTY: 'con sed',
  DROWNING: 'encharcada',
  COLD: 'con frío',
  HOT: 'con calor',
  SCORCHED: 'demasiado sol',
  DARK: 'sin luz',
  PARCHED_AIR: 'aire seco',
};

/** Décimas de grado a texto. `236` → `"23,6 °C"`. */
export function formatTemp(dc) {
  if (dc === null || dc === undefined || Number.isNaN(dc)) return '—';
  const signo = dc < 0 ? '-' : '';
  const abs = Math.abs(dc);
  return `${signo}${Math.floor(abs / 10)},${abs % 10} °C`;
}

/** Iluminancia legible. Arriba de mil se abrevia, que es como la lee la gente. */
export function formatLux(lux) {
  if (lux === null || lux === undefined || Number.isNaN(lux)) return '—';
  if (lux >= 10000) return `${Math.round(lux / 1000)}k lux`;
  if (lux >= 1000) return `${(lux / 1000).toFixed(1).replace('.', ',')}k lux`;
  return `${lux} lux`;
}

/** Antigüedad de la última lectura, en palabras. */
export function formatEdad(s) {
  if (s === null || s === undefined || Number.isNaN(s)) return '—';
  if (s < 90) return 'recién';
  if (s < 5400) return `hace ${Math.round(s / 60)} min`;
  if (s < 172800) return `hace ${Math.round(s / 3600)} h`;
  return `hace ${Math.round(s / 86400)} días`;
}

/** Tensión de celda a porcentaje. Misma curva que firmware/spore/power.c. */
const CURVA_BATT = [
  [4200, 100], [4100, 92], [4000, 85], [3900, 76], [3800, 66], [3700, 55],
  [3600, 43], [3500, 30], [3400, 18], [3300, 9], [3200, 3], [3000, 0],
];

export function battPct(mv) {
  if (!Number.isFinite(mv)) return 0;
  if (mv >= CURVA_BATT[0][0]) return 100;
  const ultimo = CURVA_BATT[CURVA_BATT.length - 1];
  if (mv <= ultimo[0]) return 0;
  for (let i = 1; i < CURVA_BATT.length; i++) {
    const [mvHi, pctHi] = CURVA_BATT[i - 1];
    const [mvLo, pctLo] = CURVA_BATT[i];
    if (mv >= mvLo) {
      return Math.round(pctLo + ((mv - mvLo) * (pctHi - pctLo)) / (mvHi - mvLo));
    }
  }
  return 0;
}

/**
 * Orden de la lista: primero lo que reclama atención, y dentro de cada
 * severidad por nombre, para que la lista no baile entre recargas.
 */
export function ordenarPlantas(plants) {
  const rank = (p) => {
    const i = SEVERIDADES.indexOf(p.severity);
    return i < 0 ? SEVERIDADES.length : i;
  };
  return [...(plants || [])].sort(
    (a, b) => rank(a) - rank(b) || String(a.nombre).localeCompare(String(b.nombre)),
  );
}

/** Cuántas plantas necesitan algo. Es el número del encabezado. */
export function contarAlertas(plants) {
  return (plants || []).filter((p) => p.severity === 'URGENT' || p.severity === 'WATCH').length;
}

/**
 * Posición relativa de un valor dentro del rango cómodo de su especie, de 0 a
 * 1, para dibujar la barra. Devuelve null si falta el rango.
 */
export function posicionEnRango(valor, min, max) {
  if (![valor, min, max].every(Number.isFinite) || max <= min) return null;
  return Math.min(1, Math.max(0, (valor - min) / (max - min)));
}

const RE_NOMBRE = /^[A-Za-z0-9ÁÉÍÓÚÜÑáéíóúüñ .'-]{1,17}$/;

/**
 * Validación del alta. Se hace también acá y no sólo en la Terminal porque
 * el ESP32 tiene que poder confiar en lo que le llega sin gastar memoria en
 * mensajes de error largos.
 */
export function validarAlta({ nombre, especie }, especiesValidas) {
  const errores = [];
  const n = (nombre || '').trim();
  if (!n) {
    errores.push('Poné un nombre.');
  } else if (!RE_NOMBRE.test(n)) {
    errores.push('El nombre admite hasta 17 letras, números y espacios.');
  }
  if (!especie) {
    errores.push('Elegí una especie.');
  } else if (especiesValidas && !especiesValidas.includes(especie)) {
    errores.push('Esa especie no está en el catálogo.');
  }
  return { ok: errores.length === 0, errores };
}

/**
 * Cómo presentar una identificación. Por debajo de 0,7 no se da por buena
 * sola: de esa especie salen los umbrales con los que se juzga la planta el
 * resto de su vida, así que conviene que el usuario confirme.
 */
export function interpretarIdentificacion(r) {
  if (!r || !r.especie) return { estado: 'fallo', mensaje: 'No pude reconocerla.' };
  if (r.confianza >= 0.7) {
    return { estado: 'seguro', especie: r.especie, mensaje: `Parece ${r.nombre}.` };
  }
  return {
    estado: 'dudoso',
    especie: r.especie,
    mensaje: `Puede que sea ${r.nombre}, pero no estoy seguro. Revisá antes de guardar.`,
  };
}

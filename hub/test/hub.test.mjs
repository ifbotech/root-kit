/* Pruebas del Hub. Sin dependencias: corredor de tests propio de Node.
 *
 *     node --test hub/test/
 */
import { test, describe, beforeEach } from 'node:test';
import assert from 'node:assert/strict';

import {
  formatTemp, formatLux, formatEdad, battPct, ordenarPlantas, contarAlertas,
  posicionEnRango, validarAlta, interpretarIdentificacion, MOOD_ES,
} from '../lib/model.mjs';

import { manejarApi, reset, ESPECIES, SIMBIONTES } from '../dev-server.mjs';

/* ============================================================== formato === */
describe('formato', () => {
  test('la temperatura sale de décimas a coma decimal', () => {
    assert.equal(formatTemp(236), '23,6 °C');
    assert.equal(formatTemp(0), '0,0 °C');
    assert.equal(formatTemp(450), '45,0 °C');
  });

  test('las temperaturas bajo cero no se rompen', () => {
    /* El signo tiene que ir adelante y el decimal seguir siendo positivo:
     * un -55 ingenuo imprime "-5,-5". */
    assert.equal(formatTemp(-55), '-5,5 °C');
    assert.equal(formatTemp(-150), '-15,0 °C');
  });

  test('los valores ausentes no imprimen NaN', () => {
    assert.equal(formatTemp(null), '—');
    assert.equal(formatTemp(undefined), '—');
    assert.equal(formatLux(null), '—');
    assert.equal(formatEdad(undefined), '—');
  });

  test('la luz se abrevia como la lee la gente', () => {
    assert.equal(formatLux(0), '0 lux');
    assert.equal(formatLux(850), '850 lux');
    assert.equal(formatLux(5200), '5,2k lux');
    assert.equal(formatLux(42000), '42k lux');
  });

  test('la antigüedad se redondea a la unidad util', () => {
    assert.equal(formatEdad(30), 'recién');
    assert.equal(formatEdad(600), 'hace 10 min');
    assert.equal(formatEdad(7200), 'hace 2 h');
    assert.equal(formatEdad(259200), 'hace 3 días');
  });
});

/* ============================================================== bateria === */
describe('batería', () => {
  test('coincide con la curva del firmware en los extremos', () => {
    assert.equal(battPct(4300), 100);
    assert.equal(battPct(4200), 100);
    assert.equal(battPct(3700), 55);
    assert.equal(battPct(3000), 0);
    assert.equal(battPct(2500), 0);
  });

  test('es monótona creciente', () => {
    /* Si no lo fuera, el ruido del ADC haría "subir" la batería en pantalla. */
    let prev = -1;
    for (let mv = 2800; mv <= 4300; mv += 10) {
      const p = battPct(mv);
      assert.ok(p >= prev, `bajó en ${mv} mV: ${p} < ${prev}`);
      prev = p;
    }
  });

  test('entradas invalidas devuelven cero en vez de NaN', () => {
    assert.equal(battPct(undefined), 0);
    assert.equal(battPct(NaN), 0);
  });
});

/* ================================================================ lista === */
describe('orden de la lista', () => {
  const plantas = [
    { nombre: 'ZAMIA', severity: 'OK' },
    { nombre: 'ALOE', severity: 'URGENT' },
    { nombre: 'POTUS', severity: 'WATCH' },
    { nombre: 'BONSAI', severity: 'URGENT' },
    { nombre: 'CACTUS', severity: 'OK' },
  ];

  test('lo urgente va primero', () => {
    const o = ordenarPlantas(plantas).map((p) => p.nombre);
    assert.deepEqual(o, ['ALOE', 'BONSAI', 'POTUS', 'CACTUS', 'ZAMIA']);
  });

  test('dentro de cada severidad ordena por nombre, sin bailar', () => {
    /* Dos llamadas seguidas tienen que dar el mismo orden: una lista que se
     * reacomoda sola en cada refresco es imposible de usar. */
    const a = ordenarPlantas(plantas).map((p) => p.nombre);
    const b = ordenarPlantas([...plantas].reverse()).map((p) => p.nombre);
    assert.deepEqual(a, b);
  });

  test('no muta el arreglo original', () => {
    const copia = [...plantas];
    ordenarPlantas(plantas);
    assert.deepEqual(plantas, copia);
  });

  test('tolera vacío y nulo', () => {
    assert.deepEqual(ordenarPlantas([]), []);
    assert.deepEqual(ordenarPlantas(null), []);
    assert.equal(contarAlertas(null), 0);
  });

  test('cuenta las que reclaman algo', () => {
    assert.equal(contarAlertas(plantas), 3);
  });
});

/* =============================================================== rangos === */
describe('posición en el rango', () => {
  test('mapea el rango cómodo a 0..1', () => {
    assert.equal(posicionEnRango(25, 25, 60), 0);
    assert.equal(posicionEnRango(60, 25, 60), 1);
    assert.ok(Math.abs(posicionEnRango(42.5, 25, 60) - 0.5) < 0.001);
  });

  test('satura fuera del rango en vez de salirse de la barra', () => {
    assert.equal(posicionEnRango(5, 25, 60), 0);
    assert.equal(posicionEnRango(90, 25, 60), 1);
  });

  test('un rango invalido devuelve null y la barra no se dibuja', () => {
    assert.equal(posicionEnRango(30, 60, 25), null);
    assert.equal(posicionEnRango(30, 25, 25), null);
    assert.equal(posicionEnRango(NaN, 25, 60), null);
  });
});

/* =========================================================== validacion === */
describe('validación del alta', () => {
  const ids = ESPECIES.map((e) => e.id);

  test('acepta un alta correcta', () => {
    assert.equal(validarAlta({ nombre: 'MONSTERA', especie: 'monstera' }, ids).ok, true);
  });

  test('exige nombre y especie', () => {
    assert.equal(validarAlta({ nombre: '', especie: 'monstera' }, ids).ok, false);
    assert.equal(validarAlta({ nombre: '   ', especie: 'monstera' }, ids).ok, false);
    assert.equal(validarAlta({ nombre: 'X', especie: '' }, ids).ok, false);
  });

  test('rechaza una especie que no está en el catálogo', () => {
    const v = validarAlta({ nombre: 'X', especie: 'palmera-inventada' }, ids);
    assert.equal(v.ok, false);
    assert.match(v.errores.join(' '), /catálogo/);
  });

  test('acepta acentos y eñes, que en castellano no son opcionales', () => {
    assert.equal(validarAlta({ nombre: 'MALVÓN DEL ÑANDÚ', especie: 'pothos' }, ids).ok, true);
  });

  test('corta en 17, que es lo que entra en la barra de la Terminal', () => {
    const largo = 'A'.repeat(18);
    assert.equal(validarAlta({ nombre: largo, especie: 'pothos' }, ids).ok, false);
    assert.equal(validarAlta({ nombre: 'A'.repeat(17), especie: 'pothos' }, ids).ok, true);
  });
});

/* ======================================================= identificacion === */
describe('interpretación de la identificación por IA', () => {
  test('una confianza alta se da por buena', () => {
    const i = interpretarIdentificacion({ especie: 'monstera', nombre: 'Monstera', confianza: 0.93 });
    assert.equal(i.estado, 'seguro');
    assert.equal(i.especie, 'monstera');
  });

  test('una confianza baja pide confirmación, no decide sola', () => {
    /* De la especie salen los umbrales con los que se juzga la planta el
     * resto de su vida: adivinarla en silencio es el peor resultado. */
    const i = interpretarIdentificacion({ especie: 'pothos', nombre: 'Potus', confianza: 0.41 });
    assert.equal(i.estado, 'dudoso');
    assert.match(i.mensaje, /no estoy seguro/i);
  });

  test('una respuesta vacía no rompe la interfaz', () => {
    assert.equal(interpretarIdentificacion(null).estado, 'fallo');
    assert.equal(interpretarIdentificacion({}).estado, 'fallo');
  });
});

/* ================================================================== API === */
describe('API', () => {
  beforeEach(reset);

  test('el estado trae terminal y plantas', async () => {
    const [code, body] = await manejarApi('GET', '/api/state', null);
    assert.equal(code, 200);
    assert.ok(body.terminal.fw);
    assert.equal(body.plants.length, 2);
  });

  test('cada planta del estado tiene los campos que la interfaz usa', async () => {
    const [, body] = await manejarApi('GET', '/api/state', null);
    for (const p of body.plants) {
      for (const k of ['id', 'nombre', 'especie', 'simbionte', 'mood', 'severity', 'reason', 'tel']) {
        assert.ok(k in p, `falta ${k}`);
      }
      assert.ok(MOOD_ES[p.mood], `ánimo desconocido: ${p.mood}`);
      for (const k of ['soil_pct', 'temp_dc', 'rh_pct', 'lux', 'age_s']) {
        assert.equal(typeof p.tel[k], 'number', `tel.${k} no es número`);
      }
    }
  });

  test('el catálogo de especies coincide con el del firmware', async () => {
    /* El archivo se genera con tools/sync_catalog.py desde species.c, y
     * `make verify` falla si quedó desfasado. Acá sólo verificamos que el
     * resultado tenga forma de catálogo. */
    const [, body] = await manejarApi('GET', '/api/species', null);
    assert.ok(body.length >= 12, `esperaba 12 o mas especies, hay ${body.length}`);
    const m = body.find((e) => e.id === 'monstera');
    assert.equal(m.soil_min, 25);
    assert.equal(m.soil_max, 60);
    assert.equal(m.temp_min_dc, 180);
    for (const e of body) {
      assert.ok(e.soil_min < e.soil_max, `${e.id}: rango de suelo invertido`);
      assert.ok(e.temp_min_dc < e.temp_max_dc, `${e.id}: rango de temp invertido`);
      assert.ok(e.lux_min < e.lux_max, `${e.id}: rango de luz invertido`);
      assert.ok(e.dificultad >= 0 && e.dificultad <= 100, `${e.id}: dificultad fuera de rango`);
    }
  });

  test('la rareza sale de la dificultad, con los mismos cortes que el firmware', () => {
    const corte = (d) => (d >= 80 ? 'LEGENDARIO' : d >= 60 ? 'EPICO'
                        : d >= 30 ? 'RARO' : 'COMUN');
    for (const s of SIMBIONTES) {
      const e = ESPECIES.find((x) => x.id === s.especie);
      assert.equal(s.rareza, corte(e.dificultad),
        `${s.id} (${e.id}, dificultad ${e.dificultad}) deberia ser ${corte(e.dificultad)}`);
    }
  });

  test('la piramide de rarezas tiene forma de piramide', () => {
    /* Si hubiera tantos legendarios como comunes, el escalon mas alto
     * dejaria de sentirse alto. */
    const n = (r) => SIMBIONTES.filter((s) => s.rareza === r).length;
    for (const r of ['COMUN', 'RARO', 'EPICO', 'LEGENDARIO']) {
      assert.ok(n(r) > 0, `no hay ningun simbionte ${r}`);
    }
    assert.ok(n('LEGENDARIO') < n('COMUN'), 'los legendarios no son los mas raros');
  });

  test('registrar una planta la agrega y devuelve 201', async () => {
    const [code, creada] = await manejarApi('POST', '/api/plants',
      { nombre: 'FICUS', especie: 'ficus-lyrata' });
    assert.equal(code, 201);
    assert.equal(creada.nombre, 'FICUS');
    assert.equal(creada.simbionte, 'lyra');

    const [, st] = await manejarApi('GET', '/api/state', null);
    assert.equal(st.plants.length, 3);
  });

  test('una especie nueva desbloquea su simbionte', async () => {
    const [, creada] = await manejarApi('POST', '/api/plants',
      { nombre: 'ALOE', especie: 'cactus' });
    assert.equal(creada.simbionte_nuevo, true);

    const [, col] = await manejarApi('GET', '/api/collection', null);
    assert.ok(col.desbloqueados.includes('spine'));
  });

  test('registrar un bonsai desbloquea un legendario', async () => {
    /* La recompensa tiene que escalar con el trabajo: el bonsai es la
     * planta mas dificil del catalogo. */
    const [, creada] = await manejarApi('POST', '/api/plants',
      { nombre: 'BONSAI', especie: 'bonsai' });
    const sim = SIMBIONTES.find((s) => s.id === creada.simbionte);
    assert.equal(sim.rareza, 'LEGENDARIO');
  });

  test('registrar un potus desbloquea un comun', async () => {
    const [, creada] = await manejarApi('POST', '/api/plants',
      { nombre: 'OTRO POTUS', especie: 'pothos' });
    const sim = SIMBIONTES.find((s) => s.id === creada.simbionte);
    assert.equal(sim.rareza, 'COMUN');
  });

  test('una especie repetida no vuelve a desbloquear', async () => {
    const [, creada] = await manejarApi('POST', '/api/plants',
      { nombre: 'OTRA', especie: 'monstera' });
    assert.equal(creada.simbionte_nuevo, false);
  });

  test('el desbloqueo es determinista, nunca al azar', async () => {
    /* Es la garantía que mantiene la colección afuera del terreno de las
     * cajas de botín. Mismo alta, mismo simbionte, siempre. */
    for (let i = 0; i < 20; i++) {
      reset();
      const [, c] = await manejarApi('POST', '/api/plants',
        { nombre: 'X', especie: 'sansevieria' });
      assert.equal(c.simbionte, 'sable');
    }
  });

  test('hay un simbionte por especie y ninguno huérfano', async () => {
    const espIds = ESPECIES.map((e) => e.id);
    for (const s of SIMBIONTES) {
      assert.ok(espIds.includes(s.especie), `${s.id} apunta a una especie inexistente`);
    }
    assert.equal(new Set(SIMBIONTES.map((s) => s.especie)).size, ESPECIES.length);
  });

  test('rechaza altas incompletas o con especie inventada', async () => {
    assert.equal((await manejarApi('POST', '/api/plants', { nombre: 'X' }))[0], 400);
    assert.equal((await manejarApi('POST', '/api/plants', { especie: 'monstera' }))[0], 400);
    assert.equal((await manejarApi('POST', '/api/plants',
      { nombre: 'X', especie: 'inventada' }))[0], 400);
  });

  test('el nombre se recorta a lo que entra en la Terminal', async () => {
    const [, c] = await manejarApi('POST', '/api/plants',
      { nombre: 'X'.repeat(40), especie: 'pothos' });
    assert.equal(c.nombre.length, 17);
  });

  test('asignar un Spore lo saca de los libres', async () => {
    const [, libresAntes] = await manejarApi('GET', '/api/spores', null);
    assert.equal(libresAntes.length, 1);
    await manejarApi('POST', '/api/plants',
      { nombre: 'NUEVA', especie: 'pothos', spore_id: 'a4cf129b40aa' });
    const [, libresDespues] = await manejarApi('GET', '/api/spores', null);
    assert.equal(libresDespues.length, 0);
  });

  test('editar y borrar una planta', async () => {
    const [, p] = await manejarApi('PATCH', '/api/plants/p1', { nombre: 'RENOMBRADA' });
    assert.equal(p.nombre, 'RENOMBRADA');
    assert.equal((await manejarApi('PATCH', '/api/plants/p1', { especie: 'nope' }))[0], 400);
    assert.equal((await manejarApi('DELETE', '/api/plants/p1', null))[0], 204);
    assert.equal((await manejarApi('GET', '/api/plants/p1', null))[0], 404);
  });

  test('borrar una planta no borra el simbionte de la colección', async () => {
    await manejarApi('DELETE', '/api/plants/p1', null);
    const [, col] = await manejarApi('GET', '/api/collection', null);
    assert.ok(col.desbloqueados.includes('tuga'));
  });

  test('identificar exige una imagen', async () => {
    assert.equal((await manejarApi('POST', '/api/identify', {}))[0], 400);
    const [code, r] = await manejarApi('POST', '/api/identify', { image_b64: 'AAAA' });
    assert.equal(code, 200);
    assert.ok(ESPECIES.some((e) => e.id === r.especie));
    assert.ok(r.confianza > 0 && r.confianza <= 1);
  });

  test('el historial devuelve 48 puntos ordenados del mas nuevo al mas viejo', async () => {
    const [code, h] = await manejarApi('GET', '/api/history/p1', null);
    assert.equal(code, 200);
    assert.equal(h.puntos.length, 48);
    for (let i = 1; i < h.puntos.length; i++) {
      assert.ok(h.puntos[i].t < h.puntos[i - 1].t, 'el historial no está ordenado');
    }
  });

  test('las rutas desconocidas dan 404 en vez de romper', async () => {
    assert.equal((await manejarApi('GET', '/api/nada', null))[0], 404);
    assert.equal((await manejarApi('GET', '/api/history/noexiste', null))[0], 404);
    assert.equal((await manejarApi('GET', '/api/plants/noexiste', null))[0], 404);
  });
});

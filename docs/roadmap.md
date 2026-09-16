# Checklist y roadmap

Dónde está el producto y qué falta, en orden. Cada casilla es algo que se
puede verificar; las que están marcadas tienen prueba o captura que lo
respalda.

**Dos repositorios:**

- [`rootkit`](https://github.com/ifbotech/root-kit) — firmware, hardware y
  carcasas.
- [`root-lab`](https://github.com/ifbotech/root-lab) — la app, la nube, las
  notificaciones, la IA y el emulador.

## El producto, en una línea

Una maceta con sensores y una pantalla que muestra **sólo dos cosas: un QR al
principio y unos ojos después**. El personaje lo pone la carcasa impresa en
3D; la cara le hace juego. Todo lo demás —números, tareas, avisos,
colección— vive en la app del teléfono.

```
 encender ─► QR ─► app: instalar ─► avisos ─► wifi ─► vincular
                                                          │
 la cara ◄─ foto + IA ◄─ nombre ◄─ la maceta abre ◄─ cofre en la app
    │                              los ojos
    └─► sensores ─► ánimo ─► nube ─► tablero y notificaciones
```

---

## Fase 0 — La base (hecho)

### Firmware

- [x] Caras en ilustración plana tipo Duolingo, con bordes suavizados en
      punto fijo (`gfx/aa.c`), para los dos paneles
- [x] Seis personajes × once ánimos, con parpadeo, mirada y respiración
- [x] Cara dormida antes del cofre, sin delatar al personaje
- [x] Despertar: los ojos se abren en dos intentos cuando se abre el cofre
- [x] Pantalla del QR con código legible y estado del aparato sin palabras
- [x] Máquina de estados del vínculo (`core/enlace.c`): QR, wifi, vínculo,
      cofre, despertar, cara, desvincular, botón largo
- [x] Código de vinculación que cambia con cada desvinculación
- [x] Token del aparato derivado de un secreto de fábrica (HMAC-SHA256)
- [x] Portal cautivo para pasar el wifi desde cualquier teléfono
- [x] Lectura de sensores: capacitivo, AHT20, BH1750, DS18B20, riel/USB
- [x] Un sensor caído no inventa problemas
- [x] Historial de lecturas en flash que sobrevive cortes de internet y de luz
- [x] Muestreo adaptativo: mide seguido cerca de un umbral, transmite poco
- [x] Protocolo JSON con la nube, tolerante a respuestas rotas u hostiles
- [x] Deep sleep con despertar por toque o por reloj
- [x] Compila para C3 SuperMini y ESP32 DevKit, con 1,44" y 2,2"
- [x] 1085 comprobaciones en el escritorio, regresión visual de las 66 caras
- [x] El renderer compilado a WebAssembly para la app y el emulador

### App y nube (`root-lab`)

- [x] Flujo de alta completo: QR → instalar → avisos → wifi → vincular →
      cofre → nombre → foto → listo
- [x] Cofre con probabilidades públicas; revela la persona de fábrica o tira
- [x] Identificación de la especie por foto con Claude (simulada sin clave)
- [x] Diagnóstico por foto cruzado con los sensores
- [x] Tablero: tareas del día, caras vivas, contadores, nivel y racha
- [x] Detalle: medidores con rango, gráfico de 24 h / 48 h / 7 días,
      vínculo, especie, pantalla siempre encendida, brillo, desvincular
- [x] Colección con siluetas y logros
- [x] Notificaciones push: por estado, con espera, calladas de noche
- [x] Cuentas con email y contraseña: cada persona ve sólo sus plantas y
      entra desde cualquier teléfono
- [x] Base SQLite con las plantas y lecturas de todas las cuentas, sin
      borrar historial; respaldo diario
- [x] Emulador de ROOTKIT en el navegador con el firmware real
- [x] 142 pruebas, incluido el flujo completo de punta a punta y el
      aislamiento entre cuentas
- [x] En línea para probar desde el teléfono: https://ifbotech.com/rootkit/

---

## Fase 1 — Prototipo en el banco (2,2" + C3 SuperMini)

Objetivo: el flujo completo con placa real, enchufada a USB.

- [ ] Comprar lo de la [lista de compras](hardware.md#lista-de-compras-del-prototipo)
- [ ] Armar en protoboard siguiendo [las conexiones](hardware.md#conexiones)
- [ ] Flashear `c3-22` (por defecto sincroniza con `https://ifbotech.com/rootkit`)
- [ ] **La pantalla:** colores, orientación y que el QR se lea desde un
      iPhone y un Android a 30 cm
- [ ] **Framerate real** de la cara en 240×320. Si baja de 10 fps: subir el
      bus a 80 MHz, repintar sólo la zona de los ojos, o dibujar a 120×120 y
      escalar ×2
- [ ] **Portal cautivo** en iPhone y Android: que la página abra sola y que
      la maceta se conecte con la clave
- [ ] Vínculo, cofre y despertar con la app en el teléfono, contra el VPS
- [ ] **Calibrar el capacitivo**: seco, regado y sumergido, tres unidades
- [ ] Comparar AHT20 y BH1750 contra un termohigrómetro y un luxómetro
- [ ] Sellar el borde del capacitivo y dejarlo una semana en tierra
- [ ] Una semana en una planta real, enchufado: que no se cuelgue, que no
      duplique lecturas, que las notificaciones lleguen y no molesten

## Fase 2 — A batería y dentro de la carcasa

- [ ] Circuito de carga: TP4056 + carga compartida + divisor
- [ ] **Medir el consumo real** (medidor USB o PPK2) en deep sleep, midiendo,
      transmitiendo y con pantalla, y actualizar `nodo/power.c` con los
      números medidos
- [ ] Desoldar el LED de encendido de la SuperMini y volver a medir
- [ ] TTP223 detrás de 2–3 mm de PLA/PETG: sensibilidad (capacitor Cs)
- [ ] Ajustar `PANTALLA_OCIOSA_MS` y el brillo por defecto con uso real
- [ ] Carcasa cruda para verificar encastres ([carcasas.md](carcasas.md))
- [ ] Ventana de la pantalla contra el módulo real
- [ ] Antena del C3 fuera del plástico grueso y lejos de la tierra húmeda:
      medir RSSI dentro de la carcasa
- [ ] Versión mini: 1,44" + LiPo, corrimientos del panel, carcasa chica
- [ ] Dos semanas a batería: comparar la autonomía con la estimación

## Fase 3 — La nube en serio

- [x] HTTPS (la app instalable y las notificaciones lo exigen): hoy en
      `ifbotech.com/rootkit`
- [ ] Dominio propio para la app (pasos en `root-lab/docs/despliegue.md`)
- [x] Despliegue de `root-lab` en el VPS, con su propio Node y servicio
- [x] Base de datos SQLite con cuentas, plantas y lecturas
- [x] Respaldos diarios en el VPS, incluidas las claves VAPID
- [ ] Respaldos fuera del VPS (snapshots del proveedor o un bucket)
- [ ] Recuperar la contraseña y verificar el email (necesita un servicio de
      envío de correo)
- [ ] `ANTHROPIC_API_KEY` con límite de gasto y alertas
- [ ] **Seguridad del aparato:** fijar el certificado raíz del servidor en
      el firmware (`RK_NUBE_CA`) y dejar de usar `setInsecure()`
- [ ] **Registro de fábrica:** apagar la confianza al primer uso
      (`ROOTLAB_TOFU=0`) y registrar cada token desde la estación de fábrica
- [ ] Actualizaciones por aire (OTA): la tabla de particiones ya deja lugar
- [ ] Política de privacidad y términos (fotos de plantas, datos de la casa)
- [ ] Monitoreo: aparatos caídos, errores de IA, notificaciones fallidas

## Fase 4 — Piloto con 20 macetas

- [ ] 20 unidades con carcasas de los seis modelos
- [ ] Estación de fábrica: grabar secreto y persona en NVS, imprimir la
      etiqueta con el código de respaldo
- [ ] Medir: cuántos terminan el alta, dónde abandonan, cuántas
      notificaciones silencian
- [ ] Ajustar umbrales de especies con plantas reales y ampliar el catálogo
- [ ] Iterar el arte con Rocío: proporciones y paletas en `core/persona.c`
- [ ] Accesibilidad: lector de pantalla, contraste, tamaño de letra

## Fase 5 — Producción

- [ ] PCB propia: C3 en módulo, cargador (TP4056 o BQ24074), buck-boost
      TPS63802, conectores para sensores y pantalla
- [ ] Antena y pruebas de emisiones
- [ ] **Homologación ENACOM** (equipo con radio vendido en Argentina)
- [ ] Caja ciega: empaque, QR de respaldo impreso, instructivo de una página
- [ ] Plan de reposición de carcasas y del modelo secreto

---

## Riesgos conocidos

| Riesgo | Qué se hace |
|---|---|
| El C3 dibuja lento la cara grande | Medir en Fase 1; hay tres salidas listas (ver arriba) |
| Capacitivos con NE555 | Revisar el chip al comprar; el firmware detecta lecturas imposibles |
| iPhone: la app instalada no comparte datos con Safari | El alta pide instalar antes que nada; el manifest abre en el mismo código; código de transferencia |
| Wifi de 5 GHz solamente | La app lo explica en el paso del wifi |
| TLS sin verificar en el prototipo | Tarea explícita de la Fase 3 |
| El personaje de fábrica no coincide con la carcasa | Grabar la persona en la misma estación que ensambla la carcasa |
| Costo de la IA por foto | Límite por cuenta (30/h), catálogo curado primero, alertas de gasto |

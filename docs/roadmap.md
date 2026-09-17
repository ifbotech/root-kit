# Checklist y roadmap

Dónde está el producto y qué falta, en orden. Cada casilla es algo que se
puede verificar; las que están marcadas tienen prueba o captura que lo
respalda.

**Nombres:** **ROOTKIT** es el hardware (la maceta con su placa y su
carcasa). Cada personaje es un **Rooti** (plural: Rooties). **ROOTLAB** es la
app y la nube.

**Dos repositorios:**

- [`root-kit`](https://github.com/ifbotech/root-kit) — firmware, hardware y
  carcasas.
- [`root-lab`](https://github.com/ifbotech/root-lab) — ROOTLAB: la app, la
  nube, las cuentas, la IA, el correo, las notificaciones y el emulador.

## El producto, en una línea

Una maceta con sensores y una pantalla que muestra **sólo dos cosas: un QR al
principio y unos ojos después**. El Rooti lo pone la carcasa impresa en 3D; la
cara le hace juego. Todo lo demás —números, tareas, avisos, colección, la
charla con la planta— vive en ROOTLAB, en el teléfono.

```
 encender ─► QR ─► ROOTLAB: instalar ─► cuenta ─► avisos ─► wifi ─► vincular
                                                                       │
 charla ◄─ ficha ◄─ foto + IA ◄─ nombre ◄─ el Rooti abre ◄─ cofre (la app se
    │                                        los ojos        pinta con sus colores)
    └─► sensores ─► ánimo ─► nube ─► tablero, notificaciones y lo que dice la planta
```

**En línea para probar:** https://ifbotech.com/rootkit/ · emulador en
https://ifbotech.com/rootkit/emulador/

---

## Software de alta gama (hecho)

Quince mejoras de software y firmware sin tocar el hardware ni lo que muestra
la pantalla (sólo el QR y los ojos):

- [x] La cara no salta de ánimo: transición de un tercio de segundo con
      parpadeo en el medio (`rk_face_draw_mezcla`), en la placa, el
      simulador y la app
- [x] El riego que se escurre se detecta (`nodo/soil.c`) y viaja como
      `escurre`: tarea y aviso en ROOTLAB
- [x] Modo escritorio (`/desk/<id>`): la cara a pantalla completa, sin que
      se apague, con modo nocturno
- [x] La cara se ve con la luz que mide el BH1750 (penumbra, sol)
- [x] Caricias: ^ ^ y ronroneo (`rk_face_draw_mimo`), vibración, corazones
- [x] La voz de cada Rooti mientras escribe (Web Audio, sin archivos)
- [x] Regar antes: Open-Meteo + la velocidad de secado; VPD y DLI
- [x] El cuidador: `/sitter/<token>` con "ya regué" y push al dueño
- [x] El invernadero: los Rooties se miran (`rk_face_draw_mirada`)
- [x] Álbum con fantasma de encuadre, antes/después y GIF de evolución;
      pasaporte botánico en A4
- [x] Paletas OLED Midnight, Cristal y Solar Gold, que se ganan cuidando
- [x] Sin red: local primero con IndexedDB y cola de cambios; insignia y
      atajos del ícono

## Fase 0 — La base (hecho)

### Firmware

- [x] Caras en ilustración plana tipo Duolingo, con bordes suavizados en
      punto fijo (`gfx/aa.c`)
- [x] **Cinco Rooties botánicos** (Brote, Musgo, Pinchito, Bulbo, Champi) ×
      once ánimos, estilo libro de cuentos, con parpadeo, mirada, guiño y
      respiración
- [x] **Tres pieles por Rooti** (común, rara, épica) con sus paletas y
      adornos (brillos, aura, corona, luces); la rareza llega en el sync y
      queda en NVS
- [x] Cara dormida antes del cofre: el Rooti en gris, sin su piel
- [x] Despertar: los ojos se abren en dos intentos cuando se abre el cofre
- [x] Pantalla del QR con código legible y estado del aparato sin palabras
- [x] Máquina de estados del vínculo (`core/enlace.c`): QR, wifi, vínculo,
      cofre, despertar, cara, desvincular, botón largo
- [x] Código de vinculación que cambia con cada desvinculación
- [x] Token del aparato derivado de un secreto de fábrica (HMAC-SHA256)
- [x] Portal cautivo para pasar el wifi desde cualquier teléfono, con la
      identidad de ROOTLAB
- [x] Lectura de sensores: capacitivo, AHT20, BH1750, DS18B20, riel/USB
- [x] Un sensor caído no inventa problemas
- [x] Historial de lecturas en flash que sobrevive cortes de internet y de luz
- [x] Muestreo adaptativo: mide seguido cerca de un umbral, transmite poco
- [x] Protocolo JSON con la nube, tolerante a respuestas rotas u hostiles
- [x] **HTTPS verificando el certificado**: sólo raíces de Let's Encrypt y
      ZeroSSL (`esp32/certificados.h`); sin `setInsecure()`
- [x] Sincroniza por defecto con el VPS (`https://ifbotech.com/rootkit`)
- [x] Deep sleep con despertar por toque o por reloj
- [x] Compila para C3 SuperMini (el producto) y ESP32 DevKit (el banco), con
      el TFT de 1,44". El de 2,2" del primer prototipo se retiró
- [x] **Actualizaciones por aire firmadas** (ECDSA P-256), con vuelta atrás
      si la versión nueva no logra hablar con la nube ([ota.md](ota.md)); falta
      probarlas en placa (Fase 1)
- [x] **Protocolo de fábrica** por el puerto serie y `tools/fabrica.py`
      ([fabrica.md](fabrica.md))
- [x] **Modo calibración**: con la app calibrando, mide y cuenta cada 5 s
- [x] 1884 comprobaciones en el escritorio, regresión visual de las 165 caras
- [x] El renderer compilado a WebAssembly para la app y el emulador

### ROOTLAB: app y nube (`root-lab`)

- [x] Flujo de alta completo: QR → instalar → cuenta → avisos → wifi →
      vincular → cofre → nombre → foto → listo
- [x] La app reconoce al Rooti de la figura al vincular; el **cofre sortea la
      piel** con probabilidades públicas (70 / 25 / 5)
- [x] **Paletas dinámicas**: Vibrant Tones por defecto; al abrir el cofre la
      app entera se pinta con una de las quince pieles (temas claros), con
      una animación que sale del cofre. Elegibles en Ajustes; bloqueadas si
      no te salió esa piel. Motor con contraste WCAG AA garantizado y
      probado en todas
- [x] **El Rooti entero en la app**, con la silueta que se imprime (voladizo
      de 45°, base plana, centro de masa bajo, verificados por test)
- [x] **Mascota**: salud de sensores y felicidad de mimos; caricias, polvo y
      esponja, snacks de gotas de rocío, y de noche se sienta con gorrito
- [x] Emulador con **Probar lo nuevo**: pieles, riego que se escurre, 48 h de
      historial, luces, noche, polvo y gotas, varios aparatos
- [x] Reconocer la especie por foto con Claude, **sólo con un Rooti
      registrado**
- [x] **Ficha de cuidados** que nace con el primer reconocimiento (riego,
      luz, sustrato, abono, plagas, toxicidad...)
- [x] **Charla con la planta**: contesta con su nombre, la voz de su Rooti y
      sus sensores en vivo; sólo habla de su cuidado
- [x] **Tope de gasto de la IA** (diario y mensual) con alertas por email, y
      **cuotas del plan gratis**: 3 mensajes por día, 3 reconocimientos y 2
      diagnósticos por Rooti; todo auditado (`tools/uso-ia.mjs`)
- [x] Diagnóstico por foto cruzado con los sensores
- [x] Tablero: tareas del día, caras vivas, contadores, nivel y racha
- [x] Detalle: medidores con rango, gráfico de 24 h / 48 h / 7 días,
      vínculo, especie, cuidados, pantalla siempre encendida, brillo
- [x] Colección de Rooties con siluetas, paletas y logros
- [x] Notificaciones push: por estado, con espera, calladas de noche
- [x] Cuentas con email y contraseña: cada persona ve sólo sus plantas y
      entra desde cualquier teléfono
- [x] **Recuperar la contraseña y verificar el email** (Nodemailer + Brevo,
      enlaces de un solo uso, sin revelar qué emails tienen cuenta)
- [x] **Datos personales cifrados** (AES-256-GCM, índice ciego) y
      contraseñas con **Argon2id y pimienta**; clave maestra fuera de la base
- [x] Base SQLite con las plantas, lecturas y charlas de todas las cuentas;
      migraciones; respaldo diario
- [x] **Seguridad web**: CSP estricta, anti-iframe, HSTS, nada de terceros
      (la fuente se sirve desde la app); servicio systemd encerrado
- [x] Emulador de Rooti en el navegador con el firmware real
- [x] **Accesibilidad WCAG 2.1 AA**: auditoría con axe sin violaciones, de día
      y de noche; el contraste lo garantiza el motor de paletas
- [x] **Lo que el teléfono no gasta**: todo el texto comprimido (el armazón
      pasó de 281 a 89 KB) y con etiqueta, así la segunda carga son 6 KB y un
      tablero que no cambió vuelve vacío
- [x] La ficha y Ajustes con lo de todos los días adelante y lo de una vez
      plegado: de cuatro pantallas de scroll a una
- [x] 469 pruebas, y verificación de punta a punta contra producción
- [x] En línea: https://ifbotech.com/rootkit/

### El servidor (VPS)

- [x] ROOTLAB con su propio Node 24, usuario sin privilegios y respaldo diario
- [x] Sitio principal endurecido: puerto 3000 cerrado a internet, `.env`
      fuera de la imagen Docker, chequeo de salud arreglado, Next.js sin la
      vulnerabilidad crítica, nodemailer actualizado
- [x] HSTS para todo ifbotech.com

---

## Ahora mismo

- [ ] **Clave de Anthropic nueva**: la del servidor está revocada (401).
      Mientras tanto ROOTLAB funciona con la IA simulada. Cargarla en
      `/etc/root-lab.env` y en el `.env` del sitio principal, y poner un
      límite de gasto para esa clave en la consola de Anthropic
- [ ] **Guardar la clave maestra** (`ROOTLAB_SECRETO`) en un gestor de
      contraseñas: sin ella la base no se puede leer
- [ ] Revisar que el email de prueba de ROOTLAB llegó a la bandeja de
      entrada y no a spam

## Fase 1 — Prototipo en el banco (1,44" + C3 SuperMini)

Objetivo: el flujo completo con placa real, enchufada a USB.

- [ ] Comprar lo de la [lista de compras](hardware.md#lista-de-compras-del-prototipo)
- [ ] Armar en protoboard siguiendo [las conexiones](hardware.md#conexiones)
- [ ] Pasar la placa por `tools/fabrica.py` (la nube de producción ya no
      acepta placas sin registrar) y flashear `c3-144`
- [ ] **Una actualización por aire de punta a punta**: publicar en beta, ver
      el log `[ota]`, cortar el wifi a mitad de la descarga, publicar un
      binario que no sincroniza y ver la vuelta atrás
- [ ] **HTTPS real**: que la placa valide el certificado del VPS con las
      raíces fijadas (primer sync en el monitor serie)
- [ ] **La pantalla:** colores, orientación y que el QR se lea desde un
      iPhone y un Android a 30 cm
- [ ] **Framerate real** de la cara en la placa. Si baja de 10 fps: subir el
      bus a 80 MHz, repintar sólo la zona de los ojos, o dibujar a 120×120 y
      escalar ×2
- [ ] **Portal cautivo** en iPhone y Android: que la página abra sola y que
      el Rooti se conecte con la clave
- [ ] Vínculo, cofre y despertar con ROOTLAB en el teléfono, contra el VPS
- [ ] **Calibrar el capacitivo**: seco, regado y sumergido, tres unidades
- [ ] Comparar AHT20 y BH1750 contra un termohigrómetro y un luxómetro
- [ ] Sellar el borde del capacitivo y dejarlo una semana en tierra
- [ ] Una semana en una planta real, enchufado: que no se cuelgue, que no
      duplique lecturas, que las notificaciones lleguen y no molesten, y que
      la charla con la planta diga cosas ciertas

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
- [ ] Corrimientos del panel de 1,44" (`RK_TFT_OFS_X/Y`) contra el módulo real
- [ ] Dos semanas a batería: comparar la autonomía con la estimación

## Fase 2b — Sonido: el Rooti hace ruiditos

**Postergada hasta después del piloto.** Suma lista de materiales, consumo y
una rejilla en una carcasa que no puede tener aperturas hacia arriba; y el
sonido ya vive en la app (la voz de cada Rooti, el ronroneo, las burbujas).
Se retoma con datos de uso.

Un parlantito para que el Rooti se exprese también con sonido: un gorjeo al
despertar del cofre, un quejido cuando tiene sed, un "gracias" cuando lo
riegan, una respuesta al tocarlo. Cada Rooti con su voz, como con la cara.
Opciones de hardware y pines en [hardware.md](hardware.md#sonido).

- [ ] **Diseñar los sonidos como datos**, igual que las caras: secuencias de
      notas (frecuencia, duración, volumen) por Rooti y por evento, en un
      módulo portable (`ui/sonido.c`) con pruebas en el escritorio
- [ ] **Escucharlos en el emulador** antes de tener hardware: el WASM
      devuelve las notas y el navegador las toca con Web Audio. Rocío puede
      iterar los sonidos igual que las caras
- [ ] Banco: **buzzer pasivo** por PWM (LEDC) para validar la idea y el
      volumen
- [ ] Producto: **PAM8302 + parlante de 8 Ω, 0,5–1 W, 20–28 mm**, por PWM
      con filtro RC; liberar un pin del C3 atando CS de la pantalla a GND
- [ ] Apagar el amplificador en deep sleep (pin SD) y medir el consumo
- [ ] **Respetar la noche y la persona**: silencio de 23 a 8 como las
      notificaciones, volumen y "silencio" por Rooti en la app
- [ ] Qué suena y cuándo: despertar, sed urgente (una vez por episodio),
      riego detectado, toque, batería baja
- [ ] Carcasa: rejilla del parlante **sin aperturas hacia arriba** (agua) y
      cámara de resonancia; medir cuánto se escucha a 2 m
- [ ] En el DevKit (sobran pines): probar I2S con MAX98357A para sonido de
      mejor calidad y decidir si vale la pena en la PCB

## Fase 3 — La nube en serio

- [x] HTTPS (la app instalable y las notificaciones lo exigen): hoy en
      `ifbotech.com/rootkit`
- [x] Despliegue de ROOTLAB en el VPS, con su propio Node y servicio
- [x] Base de datos SQLite con cuentas, plantas, lecturas y charlas
- [x] Respaldos diarios en el VPS, incluidas las claves VAPID
- [x] Recuperar la contraseña y verificar el email
- [x] Tope de gasto de la IA con alertas, y cuotas por plan
- [x] Datos personales cifrados y contraseñas con Argon2id
- [x] **Seguridad del aparato:** certificado del servidor verificado en el
      firmware (raíces fijadas); falta probarlo en placa (Fase 1)
- [ ] Dominio propio para la app (pasos en `root-lab/docs/despliegue.md`),
      con SPF, DKIM y DMARC para el correo
- [x] Respaldos cifrados para sacar del VPS, con prueba de restauración
      mensual que avisa por email
- [ ] Elegir el destino de esos respaldos (`ROOTLAB_RESPALDO_DESTINO`)
- [ ] Rotación de la clave maestra (`tools/rotar-secreto.mjs`)
- [ ] DMARC en `p=quarantine` cuando los reportes de Brevo estén limpios
- [x] **Registro de fábrica:** en producción sólo entran las placas que
      registró la estación de fábrica (`ROOTLAB_TOFU=emulador`); un aparato o
      un lote se pueden deshabilitar
- [x] Actualizaciones por aire firmadas, por canales (beta y estable)
- [x] Administración (`/api/admin/*`), vigía de caídas masivas, latido
      opcional y métricas anónimas sin terceros
- [x] Sin una IA de verdad, la app esconde las funciones de IA en vez de
      simularlas
- [ ] Passkeys como segundo factor opcional; cambiar el email de la cuenta
- [ ] Política de privacidad y términos (fotos de plantas, charlas, datos de
      la casa), y exportar los datos de una cuenta
- [x] **Qué queda escrito:** errores, frenos por límite, lo lento y un resumen
      cada diez minutos en el journal, sin IPs ni ids
      ([operacion.md](https://github.com/ifbotech/root-lab/blob/main/docs/operacion.md))
- [ ] Monitoreo que falta: errores de IA, emails rebotados, notificaciones
      fallidas; y un latido externo configurado (`ROOTLAB_LATIDO_URL`)
- [ ] Sitio principal: `@anthropic-ai/sdk` y `mercadopago` a sus versiones
      mayores nuevas (avisos moderados), probando los flujos de pago

## Fase 4 — Piloto con 20 Rooties

- [ ] 20 unidades con carcasas de los cinco Rooties
- [ ] **Modelar las cinco carcasas** a partir de las siluetas de
      `root-lab/public/lib/cuerpo.mjs`, con la ventana biselada y la 18650
      parada; imprimir una de cada una sin soportes
- [ ] Estación de fábrica: grabar secreto y Rooti en NVS, imprimir la
      etiqueta con el código de respaldo
- [ ] **Iterar el arte con Rocío**: ajustar caras y pieles
      (`core/persona.c`), siluetas y relieves (`public/lib/cuerpo.mjs`), voces
      (`public/lib/voz.mjs`) y personalidades (`server/ficha.mjs`)
- [ ] Medir: cuántos terminan el alta, dónde abandonan, cuántas
      notificaciones silencian, cuánto se usa la charla y cuánto cuesta
- [ ] Revisar con botánicos las respuestas del chat en las especies más
      comunes
- [ ] Ajustar umbrales de especies con plantas reales y ampliar el catálogo
- [ ] Accesibilidad: lector de pantalla, tamaño de letra, movimiento reducido
      (el contraste ya está garantizado por el motor de paletas)

## Fase 4b — ROOTLAB Pro

- [ ] Qué incluye: charla sin límite con todas las plantas, más
      reconocimientos y diagnósticos, historial largo exportable, paletas
      extra
- [ ] Cobro (MercadoPago y Stripe, como el sitio principal) y el campo
      `cuentas.plan` que ya existe
- [ ] Precio contra el costo real medido de la IA por usuario activo
- [ ] Mantener el tope global de gasto también para Pro

## Fase 5 — Producción

- [ ] PCB propia: C3 en módulo, cargador (TP4056 o BQ24074), buck-boost
      TPS63802, amplificador y parlante, conectores para sensores y pantalla
- [ ] Antena y pruebas de emisiones
- [ ] **Homologación ENACOM** (equipo con radio vendido en Argentina)
- [ ] Cifrado de la flash del ESP32 (la clave del wifi hoy va en NVS sin cifrar)
- [ ] Caja ciega: empaque, QR de respaldo impreso, instructivo de una página
- [ ] Plan de reposición de carcasas y del modelo secreto
- [ ] Auditoría de seguridad externa de ROOTLAB

---

## Riesgos conocidos

| Riesgo | Qué se hace |
|---|---|
| El C3 dibuja lento la cara grande | Medir en Fase 1; hay tres salidas listas (ver arriba) |
| Capacitivos con NE555 | Revisar el chip al comprar; el firmware detecta lecturas imposibles |
| iPhone: la app instalada no comparte datos con Safari | El alta pide instalar antes de crear la cuenta; las plantas están en la cuenta, no en el teléfono |
| Wifi de 5 GHz solamente | La app lo explica en el paso del wifi |
| El certificado fijado queda viejo en placas que duran años | Van las raíces actuales y las de la próxima generación de Let's Encrypt; OTA en Fase 3 |
| El Rooti de fábrica no coincide con la carcasa | Grabar el Rooti en la misma estación que ensambla la carcasa |
| Costo de la IA | Tope global diario y mensual, cuotas por plan, sólo con Rooti, caché de prompts, alertas por email, auditoría en `ia_uso` |
| Alguien inventa Rooties con el emulador para usar la IA | El tope global acota el gasto; el registro de fábrica lo cierra |
| **Se pierde la clave maestra** | Sin ella los datos cifrados no se recuperan: copia en un gestor de contraseñas, fuera del VPS |
| Se pierde el VPS | Respaldos fuera del servidor (pendiente) + la clave maestra guardada aparte |
| Los emails caen en spam | Relay con reputación (Brevo), SPF/DKIM/DMARC, texto + HTML, nunca a dominios de prueba |
| La charla dice algo incorrecto o peligroso | Prompt acotado a su cuidado, rangos curados mandan sobre la IA, advertencia de toxicidad, revisión con botánicos en el piloto |
| El sitio principal comparte el VPS con ROOTLAB | Servicios separados, cada uno detrás de Caddy en 127.0.0.1; ROOTLAB con usuario y Node propios |

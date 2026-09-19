# Actualizaciones por aire

Un ROOTKIT se queda años en una casa. Todo lo que se mejore del firmware
—una cara, el detector de riego, un arreglo de seguridad— tiene que poder
llegar sin cable y sin que nadie haga nada. Este documento es el lado del
aparato; publicar, canales y claves están en
[root-lab/docs/operacion.md](https://github.com/ifbotech/root-lab/blob/main/docs/operacion.md).

## El camino de una versión

```
 quien publica                    la nube                        el aparato
 ─────────────                    ───────                        ──────────
 pio run -e c3-144
 firma el .bin (ECDSA P-256) ──►  verifica la firma con la
                                  pública; guarda el binario
                                  por placa y canal
                                                          ◄────  sync: "corro la 0.6.0"
                                  "hay 0.6.1 para tu placa  ──►  ¿conviene? (core/ota.c)
                                   y tu canal": manifiesto       baja con su token, en otra tarea
                                                                 SHA-256 == el del manifiesto
                                                                 firma válida con la pública
                                                                 compilada (mbedTLS)
                                                                 escribe la partición inactiva
                                                                 reinicia
                                                          ◄────  sync: "corro la 0.6.1" -> confirmada
```

El manifiesto llega en la respuesta del sync ([nube.md](nube.md)):

```json
"firmware": { "version": "0.6.1",
              "url": "https://ifbotech.com/rootkit/api/d/firmware/12",
              "sha256": "<64 hex>", "firma": "<DER en base64>", "tamano": 1159640 }
```

## Qué decide el aparato (`core/ota.c`)

Todo lo que decide es C99 puro y se prueba en el escritorio (suite
`ota y fabrica`):

| Pregunta | Respuesta |
|---|---|
| ¿El manifiesto sirve? | versión `X.Y.Z`, URL http(s), hash de 32 bytes, firma DER de 64 a 80 bytes, tamaño entre 1 y 1,9 MB (lo que entra en una partición). Uno a medias es como si no hubiera |
| ¿Ya la corro? | misma versión: nada |
| ¿Es más vieja? | **se instala igual**: los aparatos siguen "la vigente de su canal". Así se vuelve atrás: publicando la anterior |
| ¿Ya falló? | cada versión se intenta como mucho **3 veces** (`RK_OTA_INTENTOS_MAX`), contadas en NVS: un binario que siempre falla no deja a la maceta bajando y reiniciando para siempre |
| ¿Hay batería? | enchufado, siempre. A batería, sólo con la celda arriba de **3,7 V** |

## Qué toca el hardware (`esp32/ota.cpp`)

- Baja por HTTPS con las mismas raíces que el sync (`certificados.h`) y el
  **token del aparato**: el binario no es público.
- Corre en su propia tarea: la cara sigue animándose. Mientras baja o
  verifica, el aparato no se duerme.
- Va escribiendo la partición inactiva mientras calcula el SHA-256. Al final
  compara el hash y **verifica la firma** (ECDSA P-256 sobre ese hash) con la
  clave pública de `esp32/ota_clave.h`. Si algo no da, aborta: la partición
  activa nunca se tocó.
- Un corte de wifi a mitad de camino (15 s sin un byte) es un intento
  fallido; el próximo sync lo vuelve a ofrecer.

**Por qué firma además de HTTPS.** HTTPS prueba que el binario viene del
servidor. La firma prueba que lo publicó quien tiene la clave privada, que
**no está en el servidor**. Tomar el servidor no alcanza para instalarle algo
a una maceta.

## Si la versión nueva no anda

El gestor de arranque del ESP32 deja la imagen nueva "pendiente de
verificar". El firmware la confirma (`esp_ota_mark_app_valid_cancel_rollback`)
recién cuando **un sync salió bien**: eso prueba wifi, TLS, JSON y el camino
entero. Si en **10 minutos** no lo logra, se marca inválida y el aparato
reinicia en la anterior. Si la nueva ni siquiera arranca, el gestor de
arranque vuelve solo.

El aparato le cuenta a la nube cómo le fue en cada sync
(`"ota": { "version", "estado" }`: `bajando`, `verificando`, `ok`, `fallo`),
y la app lo muestra en la ficha de la planta.

## La clave pública

`esp32/ota_clave.h` se genera (no se edita):

```bash
# en root-lab
node tools/publicar-firmware.mjs cabecera deploy/firmware-publica.pem > ../rootkit/firmware/esp32/ota_clave.h
```

Cambiar de par de claves deja sin actualizaciones por aire a los aparatos
que tengan la pública vieja: se hace una sola vez, antes de fabricar. Una
prueba de root-lab (`test/firma-cifrada.test.mjs`) compara esta cabecera con
`deploy/firmware-publica.pem`: si se separan, el servidor aceptaría binarios
que los aparatos rechazan, y nadie se enteraría hasta que una actualización
no llega.

## Quién firma

**Una persona, en su computadora.** La clave privada vive en
`~/.rootkit/firmware.key`, cifrada con una frase
(`publicar-firmware.mjs cifrar-clave`), y no está en el servidor, ni en
ningún repositorio, ni en GitHub. Es la clave de todos los aparatos vendidos:
con ella se instala cualquier cosa en cualquiera.

El CI (`.github/workflows/ci.yml`) compila cada commit y deja
`firmware-c3-144` como artefacto, con `firmware.bin.sha256` al lado. Se
puede firmar ese binario (comprobando la huella) o uno compilado en la
computadora; las dos cosas dan lo mismo, porque la compilación es la misma.

Hubo un flujo, `publicar-firmware.yml`, que firmaba en un runner de GitHub
con la clave como secreto del repositorio. Se borró antes de usarse: en ese
runner corren `pip install platformio`, las toolchains que baja PlatformIO,
las pruebas del repo y un clon de root-lab, y cualquiera de esas piezas
comprometida se llevaba la clave. Además recibía la nube como parámetro, así
que quien pudiera dispararlo podía mandar la clave de administración a otro
lado. El paso "Ningún flujo usa secretos" del CI falla si vuelve algo así.

Cómo se publica: [root-lab/docs/operacion.md](https://github.com/ifbotech/root-lab/blob/main/docs/operacion.md),
"Actualizaciones por aire".

## Particiones

`min_spiffs.csv`: dos particiones de aplicación de 1,9 MB (el firmware mide
~1,2 MB) y 190 KB de LittleFS para el historial. Flashear por USB sigue
andando igual.

## Probarlo

- `make test`: manifiestos hostiles, versiones, base64 y hex, la decisión, los
  tres intentos y el arranque con vuelta atrás.
- Sin placa: el emulador de root-lab hace el mismo camino (baja con su token,
  SHA-256 y firma con WebCrypto) con cualquier archivo publicado para la
  placa `emulador`.
- Con placa: publicar en `beta`, pasar ese aparato a beta
  (`PATCH /api/admin/aparatos/<id>`), mirar el log por USB (`[ota] …`).

# La estación de fábrica

Cada ROOTKIT sale de la caja con dos cosas grabadas que no vienen en el
firmware: su **secreto** (16 bytes al azar, de donde salen el token con el
que habla con la nube y el código de su QR) y su **Rooti** (la figura que
lleva puesta: Kip, Nori, Blink, Plum o Blink). Y la nube tiene que
conocerlo de antemano: en producción, una placa que no pasó por acá **no
entra** (`ROOTLAB_TOFU=emulador`).

## El paso a paso

```bash
set ROOTLAB_ADMIN_CLAVE=...       # la del servidor; nunca en la línea de comandos
~/.platformio/penv/Scripts/python tools/fabrica.py --puerto COM5 \
    --persona kip --lote L2609 --nube https://ifbotech.com/rootkit --flashear
```

1. **Flashea** el firmware (`--flashear`, con PlatformIO; se puede saltear si
   ya está).
2. **Genera el secreto** y se lo manda al aparato por el puerto serie, con el
   Rooti y el lote.
3. **Le pregunta quién es**: la MAC (su id) y el código del QR.
4. **Lo registra en la nube** (`POST /api/admin/aparatos`) con el **hash de
   su token**. La nube nunca ve el secreto, y el programa tampoco lo guarda:
   vive sólo en la NVS del aparato.
5. **Escribe la etiqueta** (`build/etiquetas/<id>.svg`, 50 × 30 mm): el
   Rooti, el lote y el código de respaldo, para cuando la cámara no lee el
   QR.

`--consultar` sólo pregunta (para reimprimir una etiqueta); `--canal beta`
deja al aparato en el canal beta de actualizaciones; `--sin-nube` graba sin
registrar (banco de pruebas); `--autoprueba` verifica, sin placa, que la
derivación del token es la misma que la del firmware y la de la nube (corre
en CI).

## El protocolo (`core/fabrica.h`)

Una línea por el puerto serie, a 115200:

```
FABRICA {"secreto":"3a917c05ee4218b69d602fc3710e845b","persona":"kip","lote":"L2609"}
  -> {"fabrica":true,"grabado":true}
FABRICA?
  -> {"fabrica":true,"id":"A1B2C3D4E5F6","persona":"kip","lote":"L2609","codigo":"K7Q2M9XA","fw":"0.6.0","vinculado":false}
```

El aparato valida: secreto de 32 hexadecimales y que no sea todo ceros, un
Rooti que exista, un lote de letras, números y guiones. Cualquier otra cosa:
`{"fabrica":false,"error":"orden invalida"}`. Al grabar, recalcula el token y
el código, y el QR de la pantalla cambia en el acto.

**Sólo sin vincular.** Una maceta que ya es de alguien contesta
`{"fabrica":false,"error":"vinculado"}` y no cambia nada. Quien tiene el
cable y la placa en la mano tiene todo de todas formas (puede reflashear);
esta regla evita el accidente, no al atacante.

## Qué gana la nube

- **Nadie registra aparatos inventados**: sin el hash del token cargado por
  la fábrica, el sync de una placa desconocida responde `401`.
- **Lo que grabó la fábrica manda**: el Rooti y el lote no se pueden cambiar
  desde el firmware.
- **Un lote se puede retirar entero** (`PATCH /api/admin/lotes/<lote>
  { "deshabilitado": true }`) o pasar a beta.

Las placas de desarrollo siguen andando sin pasar por acá contra un servidor
local (`ROOTLAB_TOFU=1`): si la NVS no tiene secreto, el firmware genera uno.

## Probarlo sin placa

`make test` cubre el protocolo (suite `ota y fabrica`): órdenes válidas,
secretos cortos o en cero, Rooties que no existen, lotes raros, líneas del
log que no son órdenes, y las respuestas. `tools/fabrica.py --autoprueba`
cubre la derivación. En root-lab, `test/fabrica.test.mjs` cubre el registro,
los modos de confianza y deshabilitar.

# El sustrato impreso

El núcleo común de ROOTKIT: una placa de PETG impresa en 3D con canaletas,
cinta de cobre pegada y soldada dentro de ellas, y los módulos montados
encima. Uno solo para los cinco Rooties.

**Qué es cada archivo:**

| | |
|---|---|
| `nucleo.json` | **El dato.** Módulos, posiciones, redes, pistas, puentes y reglas. Es lo único que se edita a mano |
| `sustrato.scad` | El modelo paramétrico. No tiene ni un número del diseño: los lee de `generado/sustrato_datos.scad` |
| `generado/sustrato_datos.scad` | Los datos que come el modelo |
| `generado/nucleo-sustrato.stl` | La pieza para imprimir, en PETG, sin soportes |
| `generado/plantilla-cinta.svg` | La plantilla **1:1** para imprimir en papel y cortar la cinta |

Todo lo de `generado/` sale de `nucleo.json`, y también `docs/conexiones.md`
y `firmware/test/redes.h`. Se regenera con:

```bash
make pcb
```

y CI falla si quedó desfasado.

**Antes de tocar `nucleo.json`:** leé [docs/pcb.md](../../docs/pcb.md) (qué
es el sustrato, qué se resolvió y por qué) y acordate de que los pines tienen
una sola verdad, que es `firmware/esp32/placa.h`. Si cambiás un pin, cambia
también ahí y en `docs/hardware.md`, en el mismo commit: `make test` lo
verifica.

Para armar una unidad: [docs/armado.md](../../docs/armado.md).

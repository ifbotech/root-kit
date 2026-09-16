# La app

La app se mudó a su propio repositorio: **[root-lab](https://github.com/ifbotech/root-lab)**,
junto con la nube, la identificación por IA, las notificaciones y un emulador
del aparato que corre este mismo firmware compilado a WebAssembly.

Desde este repositorio, lo que le importa a la app es:

- **El contrato con la nube:** [nube.md](nube.md).
- **Las caras:** `make wasm` genera `firmware/build/rootkit_caras.wasm`; en
  root-lab, `npm run firmware` lo copia, renderiza las imágenes de las
  notificaciones y regenera la tabla de personajes desde `core/persona.c`.
- **El flujo del aparato:** [arquitectura.md](arquitectura.md#el-flujo) y
  [firmware.md](firmware.md#del-encendido-a-la-cara).

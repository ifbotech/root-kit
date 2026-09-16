/* assert.h minimo para WebAssembly: las verificaciones de qrcodegen se
 * apagan, como en cualquier build de produccion con NDEBUG. */
#ifndef ROOTKIT_WASM_ASSERT_H
#define ROOTKIT_WASM_ASSERT_H
#define assert(x) ((void)0)
#endif

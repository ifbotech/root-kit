/* string.h mínimo para compilar el renderer a WebAssembly sin libc.
 * Sólo lo que usan gfx/, art/, core/persona.c y core/species.c. */
#ifndef ROOTKIT_WASM_STRING_H
#define ROOTKIT_WASM_STRING_H

#include <stddef.h>

void  *memset(void *s, int c, size_t n);
void  *memcpy(void *d, const void *s, size_t n);
int    strcmp(const char *a, const char *b);
size_t strlen(const char *s);
char  *strncpy(char *d, const char *s, size_t n);
void  *memmove(void *d, const void *s, size_t n);
char  *strchr(const char *s, int c);
int    memcmp(const void *a, const void *b, size_t n);

#endif

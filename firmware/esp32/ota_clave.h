/* ota_clave.h — la clave PÚBLICA con la que el aparato verifica cada
 * actualización (ECDSA P-256). GENERADA con
 *
 *   node tools/publicar-firmware.mjs generar-clave      (en root-lab)
 *
 * La privada no está en ningún repositorio ni en el servidor: la tiene quien
 * publica. Si se pierde, los aparatos en la calle no se pueden actualizar
 * más por aire (habría que flashearlos por USB con una pública nueva), así
 * que se guarda como la clave maestra: en un gestor de contraseñas.
 */
#ifndef ROOTKIT_OTA_CLAVE_H
#define ROOTKIT_OTA_CLAVE_H

static const char RK_OTA_CLAVE_PUBLICA[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEFvgVIZu6qAQWgCoZKZ29bv7/olqJ\n"
    "WNeaf+o71rLbn6SeWTJ8XqlOBtjIDCkQcupsRBqUKx2zDsOu3Gdm/Wsb9Q==\n"
    "-----END PUBLIC KEY-----\n";

#endif /* ROOTKIT_OTA_CLAVE_H */


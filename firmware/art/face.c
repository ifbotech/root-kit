#include "face.h"
#include "look.h"
#include <stddef.h>

/* Todas las medidas de la persona vienen en centésimas del ancho del panel.
 * Esta macro es la única conversión a pixeles que hay en el archivo. */
#define PC(w, v)  ((int)(w) * (int)(v) / 100)

/* Dónde vive cada rasgo, en centésimas del alto. Son las proporciones de la
 * cara y no dependen del modelo: lo que cambia entre personas es la FORMA de
 * los rasgos, no dónde están. Mantenerlas fijas es lo que hace que los seis
 * se lean como el mismo producto. */
#define Y_OJOS   44
#define Y_BOCA   73

typedef struct {
    int w, h;
    int cx;
    int y_ojos, y_boca;
    int ojo_rx, ojo_ry, ojo_dx;
    const rk_persona_t *p;
    rk_color_t trazo, blanco, iris, acento;
} cara_t;

/* Raíz entera, igual que la de gfx/fb.c. Se repite acá —cuatro líneas— en
 * vez de exportarla, para no ensuciar la interfaz del framebuffer con un
 * detalle que sólo necesita el recorte de un párpado. */
static int isqrt_i_face(int v)
{
    int x;
    if (v <= 0) {
        return 0;
    }
    x = v > 255 ? 255 : v;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    while (x * x > v)             { x--; }
    while ((x + 1) * (x + 1) <= v) { x++; }
    return x;
}

/* ------------------------------------------------------------- apertura -- */
/* Cuánto está abierto el ojo, de 0 (cerrado) a 100, y qué tan grande la
 * pupila. Sale del ánimo y es lo que la familia de ojos después interpreta a
 * su manera. */
static void apertura_de(rk_ojo_t forma, int *abre, int *pupila)
{
    switch (forma) {
    case RK_OJO_BLINK:  *abre =   0; *pupila = 100; break;
    case RK_OJO_HAPPY:  *abre =  30; *pupila = 100; break;
    case RK_OJO_WIDE:   *abre = 130; *pupila = 145; break;
    case RK_OJO_SLEEPY: *abre =  42; *pupila =  85; break;
    case RK_OJO_DEAD:   *abre = 100; *pupila =   0; break;
    case RK_OJO_DIZZY:  *abre = 100; *pupila = 110; break;
    case RK_OJO_GLITCH: *abre = 100; *pupila = 100; break;
    default:            *abre = 100; *pupila = 100; break;
    }
}

/* --------------------------------------------------------------- ojos ---- */
/* El iris con su brillo. Es el único lugar donde se dibuja una pupila, así
 * que las seis familias comparten el mismo look de mirada y el producto se
 * mantiene reconocible entre modelos. */
static void dibujar_iris(rk_fb_t *fb, const cara_t *c,
                         int cx, int cy, int r, int mira_x, int mira_y)
{
    if (r <= 1) {
        return;
    }
    rk_disc(fb, cx + mira_x, cy + mira_y, r, c->iris);
    rk_disc(fb, cx + mira_x, cy + mira_y, r / 2, c->trazo);
    /* El brillo arriba a la izquierda, siempre en el mismo lugar: es lo que
     * convierte un círculo en un ojo húmedo. */
    rk_disc(fb, cx + mira_x - r / 3, cy + mira_y - r / 3,
            r / 4 + 1, c->blanco);
}

/* Ojos redondos: el caso base. Un óvalo blanco con el iris adentro. */
static void ojos_redondos(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                          int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int lado;

    for (lado = -1; lado <= 1; lado += 2) {
        int ex = c->cx + lado * c->ojo_dx;
        int ry = c->ojo_ry * abre / 100;

        if (ry < 2) {
            /* Cerrado: un trazo, con la curva hacia abajo. */
            rk_arco(fb, ex, c->y_ojos, c->ojo_rx, c->ojo_ry / 2,
                    true, 3, c->trazo);
            continue;
        }
        rk_elipse(fb, ex, c->y_ojos, c->ojo_rx, ry, c->blanco);
        if (forma == RK_OJO_DEAD) {
            /* Las equis se dibujan en vez del iris. */
            rk_linea(fb, ex - c->ojo_rx / 2, c->y_ojos - ry / 2,
                     ex + c->ojo_rx / 2, c->y_ojos + ry / 2, 3, c->trazo);
            rk_linea(fb, ex + c->ojo_rx / 2, c->y_ojos - ry / 2,
                     ex - c->ojo_rx / 2, c->y_ojos + ry / 2, 3, c->trazo);
        } else if (forma == RK_OJO_DIZZY) {
            int k;
            for (k = 2; k < c->ojo_rx; k += 3) {
                rk_elipse_ring(fb, ex, c->y_ojos, k, k * ry / c->ojo_rx + 1,
                               1, c->iris);
            }
        } else {
            dibujar_iris(fb, c, ex, c->y_ojos,
                         c->ojo_rx * pupila / 190, mira_x, 0);
        }
        rk_elipse_ring(fb, ex, c->y_ojos, c->ojo_rx, ry, 2, c->trazo);
    }
    (void)t_ms;
}

/* Ojos rasgados: almendrados, arqueados hacia arriba. El kawaii vive acá. */
static void ojos_rasgados(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                          int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int lado;

    for (lado = -1; lado <= 1; lado += 2) {
        int ex = c->cx + lado * c->ojo_dx;
        int ry = c->ojo_ry * abre / 100;

        /* Feliz y cerrado son el mismo gesto en esta familia: el arco ^^.
         * Es exactamente lo que hace que se lea como kawaii y no como
         * "ojos chiquitos". */
        if (forma == RK_OJO_HAPPY || ry < 3) {
            /* El arco ^ : mitad SUPERIOR de la elipse, que sube en el medio.
             * Con la mitad inferior queda una ∪ y la cara se lee triste,
             * que es justo lo contrario de lo que este modelo es. */
            rk_arco(fb, ex, c->y_ojos + c->ojo_ry / 2,
                    c->ojo_rx, c->ojo_ry, true, 4, c->trazo);
            continue;
        }
        if (forma == RK_OJO_DEAD) {
            rk_linea(fb, ex - c->ojo_rx / 2, c->y_ojos - ry / 2,
                     ex + c->ojo_rx / 2, c->y_ojos + ry / 2, 3, c->trazo);
            rk_linea(fb, ex + c->ojo_rx / 2, c->y_ojos - ry / 2,
                     ex - c->ojo_rx / 2, c->y_ojos + ry / 2, 3, c->trazo);
            continue;
        }
        rk_elipse(fb, ex, c->y_ojos, c->ojo_rx, ry, c->blanco);
        dibujar_iris(fb, c, ex, c->y_ojos, c->ojo_rx * pupila / 165,
                     mira_x, 0);
        /* El párpado superior baja recto y deja el ojo almendrado. */
        rk_elipse_ring(fb, ex, c->y_ojos, c->ojo_rx, ry, 2, c->trazo);
        rk_arco(fb, ex, c->y_ojos - ry + 2, c->ojo_rx, ry / 2,
                false, 3, c->trazo);
    }
    (void)t_ms;
}

/* Ojos fieros: angostos, inclinados, con el párpado comiéndose el iris. */
static void ojos_fieros(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                        int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int lado;

    for (lado = -1; lado <= 1; lado += 2) {
        int ex = c->cx + lado * c->ojo_dx;
        int ry = c->ojo_ry * abre / 100;
        int caida = lado * PC(c->w, c->p->ojo_inclina) / 4;

        if (ry < 2) {
            rk_linea(fb, ex - c->ojo_rx, c->y_ojos - caida,
                     ex + c->ojo_rx, c->y_ojos + caida, 3, c->trazo);
            continue;
        }
        rk_elipse(fb, ex, c->y_ojos, c->ojo_rx, ry, c->blanco);
        if (forma == RK_OJO_DEAD) {
            rk_linea(fb, ex - c->ojo_rx / 2, c->y_ojos - ry,
                     ex + c->ojo_rx / 2, c->y_ojos + ry, 3, c->trazo);
            rk_linea(fb, ex + c->ojo_rx / 2, c->y_ojos - ry,
                     ex - c->ojo_rx / 2, c->y_ojos + ry, 3, c->trazo);
        } else {
            dibujar_iris(fb, c, ex, c->y_ojos, c->ojo_rx * pupila / 210,
                         mira_x, 0);
        }
        rk_elipse_ring(fb, ex, c->y_ojos, c->ojo_rx, ry, 2, c->trazo);
        /* El párpado inclinado, DIBUJADO y no borrado: una cuña opaca del
         * color del trazo que baja desde la esquina interna. Antes se pintaba
         * del color del fondo para "borrar" el ojo, pero el fondo es un
         * degradé y a la altura del ojo ya no es ese color: quedaba un
         * bloque plano encima de la cara. Un párpado pesado es un rasgo, no
         * una ausencia. */
        {
            int i;
            for (i = 0; i <= c->ojo_rx * 2; i++) {
                int x = ex - lado * (c->ojo_rx + 1) + lado * i;
                int alto = (c->ojo_rx * 2 - i) * (ry + caida) /
                           (c->ojo_rx * 2) - ry / 3;
                if (alto > 0) {
                    rk_vline(fb, x, c->y_ojos - ry - 1, alto + 2, c->trazo);
                }
            }
        }
    }
    (void)t_ms;
}

/* Visor: una banda horizontal. La ONDA es la expresión: plana cuando está
 * bien, dentada cuando hay alerta, un punto cuando duerme. */
static void ojos_visor(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                       int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int x, w = c->ojo_rx, h = c->ojo_ry;
    int x0 = c->cx - w, x1 = c->cx + w;

    (void)pupila;
    (void)mira_x;

    /* La carcasa del visor tiene una ranura, así que el marco se dibuja
     * siempre: es el hueco negro que se ve incluso con la cara apagada. */
    rk_fill_round(fb, x0 - 4, c->y_ojos - h - 4, w * 2 + 8, h * 2 + 8,
                  h / 2 + 2, c->trazo);

    if (abre < 20) {
        rk_hline(fb, x0, c->y_ojos, w * 2, c->iris);
        return;
    }
    for (x = x0; x <= x1; x++) {
        int t = (x - x0) * 256 / (w * 2);
        int y = c->y_ojos;
        int alto = 2;

        switch (forma) {
        case RK_OJO_WIDE:    /* alerta: picos altos y rápidos */
            y += rk_sin8((uint8_t)(t * 6 + t_ms / 8)) * h / 127;
            alto = 3;
            break;
        case RK_OJO_HAPPY:   /* una onda amable y lenta */
            y += rk_sin8((uint8_t)(t * 2 + t_ms / 40)) * (h / 2) / 127;
            break;
        case RK_OJO_SLEEPY:
            y += rk_sin8((uint8_t)(t + t_ms / 90)) * (h / 4) / 127;
            break;
        case RK_OJO_DEAD:    /* linea plana: el chiste del monitor */
            break;
        case RK_OJO_DIZZY:
        case RK_OJO_GLITCH:  /* la onda se rompe en tramos */
            y += (int)(rk_hash((uint16_t)(x + t_ms / 100)) %
                       (unsigned)(h * 2 + 1)) - h;
            break;
        default:
            y += rk_sin8((uint8_t)(t * 3 + t_ms / 24)) * (h / 3) / 127;
            break;
        }
        rk_fill_rect(fb, x, y - alto / 2, 1, alto, c->iris);
    }
}

/* Un solo ojo, enorme y centrado. Casi toda la actuación es la pupila. */
static void ojos_unico(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                       int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int ry = c->ojo_ry * abre / 100;

    if (ry < 3) {
        rk_arco(fb, c->cx, c->y_ojos, c->ojo_rx, c->ojo_ry / 2,
                true, 4, c->trazo);
        return;
    }
    rk_elipse(fb, c->cx, c->y_ojos, c->ojo_rx, ry, c->blanco);

    if (forma == RK_OJO_DEAD) {
        rk_linea(fb, c->cx - c->ojo_rx / 2, c->y_ojos - ry / 2,
                 c->cx + c->ojo_rx / 2, c->y_ojos + ry / 2, 5, c->trazo);
        rk_linea(fb, c->cx + c->ojo_rx / 2, c->y_ojos - ry / 2,
                 c->cx - c->ojo_rx / 2, c->y_ojos + ry / 2, 5, c->trazo);
    } else if (forma == RK_OJO_DIZZY) {
        int k;
        for (k = 3; k < c->ojo_rx; k += 5) {
            rk_elipse_ring(fb, c->cx, c->y_ojos, k,
                           k * ry / c->ojo_rx + 1, 2, c->iris);
        }
    } else {
        /* La pupila deriva despacio: un ojo enorme perfectamente quieto se
         * ve muerto, y este modelo no tiene otro rasgo que lo disimule. */
        int deriva = rk_sin8((uint8_t)(t_ms / 70)) * c->ojo_rx / 900;
        dibujar_iris(fb, c, c->cx, c->y_ojos, c->ojo_rx * pupila / 190,
                     mira_x + deriva, deriva / 2);
    }
    rk_elipse_ring(fb, c->cx, c->y_ojos, c->ojo_rx, ry, 3, c->trazo);
}

/* Párpados caídos: siempre a media asta, y el ánimo los mueve poco. */
static void ojos_pesados(rk_fb_t *fb, const cara_t *c, rk_ojo_t forma,
                         int abre, int pupila, int mira_x, uint32_t t_ms)
{
    int lado;
    /* Se comprime el rango: entre el más despierto y el más dormido de este
     * modelo hay mucha menos diferencia que en los demás. Eso ES el modelo. */
    int a = 52 + abre * 40 / 130;

    for (lado = -1; lado <= 1; lado += 2) {
        int ex = c->cx + lado * c->ojo_dx;
        int ry = c->ojo_ry * a / 100;

        if (abre < 20) {
            rk_arco(fb, ex, c->y_ojos, c->ojo_rx, c->ojo_ry / 2,
                    true, 3, c->trazo);
            continue;
        }
        rk_elipse(fb, ex, c->y_ojos, c->ojo_rx, ry, c->blanco);
        if (forma == RK_OJO_DEAD) {
            rk_linea(fb, ex - c->ojo_rx / 2, c->y_ojos - ry,
                     ex + c->ojo_rx / 2, c->y_ojos + ry, 3, c->trazo);
            rk_linea(fb, ex + c->ojo_rx / 2, c->y_ojos - ry,
                     ex - c->ojo_rx / 2, c->y_ojos + ry, 3, c->trazo);
        } else {
            dibujar_iris(fb, c, ex, c->y_ojos + ry / 3,
                         c->ojo_rx * pupila / 200, mira_x, 0);
        }
        rk_elipse_ring(fb, ex, c->y_ojos, c->ojo_rx, ry, 2, c->trazo);
        /* El párpado grueso, opaco, cubriendo el tercio de arriba del ojo.
         * Se dibuja recortado a la elipse para que no quede un rectángulo
         * flotando sobre la cara. */
        {
            int dy;
            for (dy = -ry; dy <= -ry / 2; dy++) {
                int t = 1024 - (dy * dy * 1024) / (ry * ry);
                int dx;
                if (t < 0) {
                    continue;
                }
                dx = c->ojo_rx * isqrt_i_face(t * 16) / 128;
                rk_hline(fb, ex - dx, c->y_ojos + dy, dx * 2 + 1, c->trazo);
            }
        }
    }
    (void)t_ms;
}

/* --------------------------------------------------------------- cejas --- */
static void cejas(rk_fb_t *fb, const cara_t *c, int angulo)
{
    int lado;
    int alto = PC(c->w, c->p->ceja_alto);
    int largo = c->ojo_rx;
    int grosor = (c->p->ceja == RK_CEJA_GRUESA) ? 5
               : (c->p->ceja == RK_CEJA_DESPEINADA) ? 4 : 2;

    if (c->p->ceja == RK_CEJA_NINGUNA) {
        return;
    }
    for (lado = -1; lado <= 1; lado += 2) {
        int ex = c->cx + lado * c->ojo_dx;
        int y  = c->y_ojos - c->ojo_ry - alto;
        /* El ángulo baja la punta INTERNA: es lo que se lee como enojo.
         * Positivo la sube, y eso se lee como preocupación. */
        int dy = angulo * largo / 40;

        if (c->p->ceja == RK_CEJA_DESPEINADA) {
            /* Tres trazos sueltos en vez de una línea: la ceja despeinada
             * que pidió la dirección de arte. Los desplazamientos son fijos
             * y no aleatorios para que la cara no titile entre cuadros. */
            static const int8_t OFF[3][2] = { { -6, 1 }, { 0, -2 }, { 6, 2 } };
            int k;
            for (k = 0; k < 3; k++) {
                int kx = ex + lado * OFF[k][0] * largo / 12;
                int ky = y + OFF[k][1] + dy * OFF[k][0] / 12;
                rk_linea(fb, kx - largo / 4, ky + 2,
                         kx + largo / 4, ky - 2, grosor, c->trazo);
            }
        } else {
            rk_linea(fb, ex - lado * largo, y - dy,
                     ex + lado * largo, y + dy, grosor, c->trazo);
        }
    }
}

/* --------------------------------------------------------------- boca ---- */
static void boca(rk_fb_t *fb, const cara_t *c, rk_boca_t forma, uint32_t t_ms)
{
    int w = PC(c->w, c->p->boca_ancho);
    int y = c->y_boca;
    int curva = 0;

    /* La curva sale del ánimo; el ESTILO sale de la persona. Un mismo
     * "sonríe" es una w felina en el kawaii y una dentadura en el cresta. */
    switch (forma) {
    case RK_BOCA_SMILE: curva =  1; break;
    case RK_BOCA_FROWN: curva = -1; break;
    case RK_BOCA_OPEN:
    case RK_BOCA_PANT:  curva =  2; break;
    case RK_BOCA_WAVY:  curva =  3; break;
    default:            curva =  0; break;
    }

    switch (c->p->boca) {
    case RK_BOCA_GATO:
        if (curva >= 0) {
            /* La w: dos arcos chicos que se tocan. */
            rk_arco(fb, c->cx - w / 2, y, w / 2, w / 3, false, 3, c->trazo);
            rk_arco(fb, c->cx + w / 2, y, w / 2, w / 3, false, 3, c->trazo);
        } else {
            rk_arco(fb, c->cx, y + w / 3, w, w / 2, true, 3, c->trazo);
        }
        break;

    case RK_BOCA_DIENTES:
        if (curva >= 2) {
            int i;
            rk_fill_round(fb, c->cx - w, y - w / 3, w * 2, w * 2 / 3,
                          4, c->trazo);
            for (i = -w + 4; i < w - 2; i += 7) {
                rk_fill_rect(fb, i + c->cx, y - w / 3, 4, 5, c->blanco);
            }
        } else if (curva < 0) {
            rk_arco(fb, c->cx, y + w / 3, w, w / 2, true, 4, c->trazo);
            rk_fill_rect(fb, c->cx - w / 3, y + w / 4, 4, 5, c->blanco);
        } else {
            int i;
            rk_linea(fb, c->cx - w, y, c->cx + w, y, 4, c->trazo);
            for (i = -w + 5; i < w - 3; i += 9) {
                rk_fill_rect(fb, i + c->cx, y - 4, 4, 4, c->blanco);
            }
        }
        break;

    case RK_BOCA_ONDA: {
        int x;
        for (x = -w; x <= w; x++) {
            int t = (x + w) * 256 / (w * 2);
            int dy = rk_sin8((uint8_t)(t * (curva == 3 ? 5 : 2)
                                       + t_ms / 30)) * (w / 5) / 127;
            rk_fill_rect(fb, c->cx + x, y + dy * (curva < 0 ? -1 : 1),
                         1, 3, c->trazo);
        }
        break;
    }

    case RK_BOCA_CHICA:
        if (curva > 0) {
            rk_arco(fb, c->cx, y, w, w, false, 3, c->trazo);
        } else if (curva < 0) {
            rk_arco(fb, c->cx, y + w / 2, w, w, true, 3, c->trazo);
        } else {
            rk_elipse(fb, c->cx, y, w / 2 + 1, w / 2 + 1, c->trazo);
        }
        break;

    default: /* RK_BOCA_LINEA */
        if (curva > 0) {
            rk_arco(fb, c->cx, y - w / 4, w, w / 2, false, 3, c->trazo);
        } else if (curva < 0) {
            rk_arco(fb, c->cx, y + w / 4, w, w / 2, true, 3, c->trazo);
        } else {
            rk_linea(fb, c->cx - w, y, c->cx + w, y, 3, c->trazo);
        }
        break;
    }
}

/* ------------------------------------------------------------ adornos ---- */
static void adornos(rk_fb_t *fb, const cara_t *c, uint8_t set, uint32_t t_ms)
{
    int i;

    if (set & RK_ADORNO_RUBOR) {
        int lado;
        for (lado = -1; lado <= 1; lado += 2) {
            int rx = c->cx + lado * (c->ojo_dx + c->ojo_rx / 3);
            int ry = c->y_ojos + c->ojo_ry + c->h / 10;
            rk_elipse(fb, rx, ry, c->w / 11, c->h / 34, c->acento);
        }
    }
    if (set & RK_ADORNO_COLMILLO) {
        int fx = c->cx - PC(c->w, c->p->boca_ancho) * 2 / 3;
        rk_fill_rect(fb, fx, c->y_boca, 4, 7, c->blanco);
        rk_fill_rect(fb, fx + 1, c->y_boca + 7, 2, 3, c->blanco);
    }
    if (set & RK_ADORNO_BRILLOS) {
        /* Cuatro destellos girando despacio alrededor de la cara. Fijos en
         * ángulo y variables en radio: se leen como que flotan, no como que
         * orbitan, que sería mareador. */
        for (i = 0; i < 3; i++) {
            uint8_t ph = (uint8_t)(t_ms / 22 + i * 85);
            int r  = c->w / 3 + rk_sin8(ph) * (c->w / 22) / 127;
            int ax = c->cx + rk_sin8((uint8_t)(i * 64 + 64)) * r / 127;
            int ay = c->y_ojos + rk_sin8((uint8_t)(i * 64)) * r / 127;
            /* Estrella de cuatro puntas: el brazo vertical mide el doble
             * que el horizontal. Con los cuatro brazos iguales se lee como
             * un signo de más flotando sobre la cara, que es exactamente lo
             * que pasaba antes. La asimetría es lo que lo vuelve destello. */
            int l  = c->w / 22 + (rk_sin8(ph) > 40 ? 1 : 0);
            int k;
            for (k = 0; k <= l; k++) {
                int an = 1 + (l - k) / 3;          /* la punta adelgaza */
                rk_fill_rect(fb, ax - an / 2, ay - k, an, 1, c->acento);
                rk_fill_rect(fb, ax - an / 2, ay + k, an, 1, c->acento);
            }
            for (k = 0; k <= l / 2; k++) {
                int an = 1 + (l / 2 - k) / 2;
                rk_fill_rect(fb, ax - k, ay - an / 2, 1, an, c->acento);
                rk_fill_rect(fb, ax + k, ay - an / 2, 1, an, c->acento);
            }
        }
    }
    if (set & RK_ADORNO_ESPORAS) {
        for (i = 0; i < 9; i++) {
            uint16_t r = rk_hash((uint16_t)(i * 733 + 5));
            int ax = (int)(r % (unsigned)c->w);
            int sube = (int)((t_ms / 26 + (r >> 4) % 256u) % 256u);
            int ay = c->h - sube * c->h / 256;
            rk_fill_rect(fb, ax, ay, 2, 2,
                         rk_mix(c->p->fondo2, c->acento, 170));
        }
    }
    if (set & RK_ADORNO_SCANLINE) {
        int y = (int)((t_ms / 12) % (uint32_t)c->h);
        rk_hline(fb, 0, y, c->w, rk_mix(c->p->fondo2, c->acento, 60));
        rk_hline(fb, 0, y + 1, c->w, rk_mix(c->p->fondo2, c->acento, 30));
    }
    if (set & RK_ADORNO_ESTATICA) {
        for (i = 0; i < 5; i++) {
            uint16_t r = rk_hash((uint16_t)(i * 911 + t_ms / 90));
            int by = (int)(r % (unsigned)c->h);
            int bh = 1 + (int)((r >> 5) % 3u);
            rk_fill_rect(fb, 0, by, c->w, bh,
                         rk_mix(c->p->fondo2, c->acento, 34));
        }
    }
    if (set & RK_ADORNO_AURA) {
        /* De adentro hacia afuera, apagándose. Los discos se pintan de mayor
         * a menor y cada uno tapa al anterior, así que la mezcla tiene que
         * CRECER a medida que el radio baja o queda una dona. */
        int r0 = c->w * 46 / 100;
        int pasos = 8;
        for (i = 0; i < pasos; i++) {
            int r = r0 - i * (c->w / 60 + 1);
            if (r <= 0) {
                break;
            }
            rk_elipse_ring(fb, c->cx, c->h / 2, r, r, 2,
                           rk_mix(c->p->fondo, c->acento,
                                  (uint8_t)(6 + i * 3)));
        }
    }
    if (set & RK_ADORNO_CORONA) {
        int k;
        int y = c->h / 10;
        for (k = -1; k <= 1; k++) {
            int kx = c->cx + k * c->w / 7;
            int alto = (k == 0) ? c->h / 9 : c->h / 14;
            rk_linea(fb, kx, y + alto, kx, y, 3, c->acento);
            rk_disc(fb, kx, y - 2, 3, c->acento);
        }
    }
}

/* -------------------------------------------------------- pictograma ----- */
bool rk_face_pictograma(rk_fb_t *fb, int x, int y, int lado,
                        rk_mood_t mood, rk_color_t c)
{
    int m = lado / 2;

    if (fb == NULL || lado < 6) {
        return false;
    }
    switch (mood) {
    case RK_MOOD_THIRSTY:      /* una gota */
        rk_disc(fb, x + m, y + lado * 2 / 3, lado / 3, c);
        rk_linea(fb, x + m, y + 1, x + m - lado / 3, y + lado * 2 / 3, 2, c);
        rk_linea(fb, x + m, y + 1, x + m + lado / 3, y + lado * 2 / 3, 2, c);
        return true;
    case RK_MOOD_DROWNING:     /* gota tachada */
        rk_disc(fb, x + m, y + lado * 2 / 3, lado / 3, c);
        rk_linea(fb, x, y, x + lado, y + lado, 2, c);
        return true;
    case RK_MOOD_COLD:         /* copo: tres trazos cruzados */
        rk_linea(fb, x + m, y, x + m, y + lado, 2, c);
        rk_linea(fb, x, y + m / 2, x + lado, y + lado - m / 2, 2, c);
        rk_linea(fb, x, y + lado - m / 2, x + lado, y + m / 2, 2, c);
        return true;
    case RK_MOOD_HOT:          /* termómetro */
        rk_disc(fb, x + m, y + lado - lado / 4, lado / 4, c);
        rk_fill_rect(fb, x + m - 1, y, 3, lado - lado / 4, c);
        return true;
    case RK_MOOD_SCORCHED:     /* sol con rayos */
        rk_disc(fb, x + m, y + m, lado / 4, c);
        rk_linea(fb, x + m, y, x + m, y + 2, 2, c);
        rk_linea(fb, x + m, y + lado - 2, x + m, y + lado, 2, c);
        rk_linea(fb, x, y + m, x + 2, y + m, 2, c);
        rk_linea(fb, x + lado - 2, y + m, x + lado, y + m, 2, c);
        return true;
    case RK_MOOD_DARK:         /* luna */
        rk_disc(fb, x + m, y + m, lado / 2, c);
        return true;
    case RK_MOOD_PARCHED_AIR:  /* tres rayas de viento */
        rk_linea(fb, x, y + m - 3, x + lado - 2, y + m - 3, 2, c);
        rk_linea(fb, x + 2, y + m, x + lado, y + m, 2, c);
        rk_linea(fb, x, y + m + 3, x + lado - 3, y + m + 3, 2, c);
        return true;
    case RK_MOOD_OFFLINE:      /* antena tachada */
        rk_linea(fb, x + m, y + lado, x + m, y + 2, 2, c);
        rk_linea(fb, x, y, x + lado, y + lado, 2, c);
        return true;
    default:
        return false;
    }
}

/* ------------------------------------------------------------- etapas ---- */
uint8_t rk_face_adornos_etapa(int etapa)
{
    /* El modelo te toca por azar; cómo se ve se gana cuidando la planta.
     * Es lo que quedó de la mecánica de mérito cuando la rareza se mudó a
     * la caja física, y a diferencia de la rareza, esto no se compra. */
    if (etapa >= 4) { return RK_ADORNO_AURA | RK_ADORNO_CORONA; }
    if (etapa >= 3) { return RK_ADORNO_AURA; }
    if (etapa >= 2) { return RK_ADORNO_BRILLOS; }
    return 0u;
}

/* --------------------------------------------------------------- cara ---- */
static void armar(cara_t *c, rk_fb_t *fb, const rk_persona_t *p, int ancho)
{
    c->p = p;
    c->w = ancho;
    c->h = fb->h;
    c->cx = fb->w / 2;
    c->y_ojos = fb->h * Y_OJOS / 100;
    c->y_boca = fb->h * Y_BOCA / 100;
    c->ojo_rx = PC(ancho, p->ojo_rx);
    c->ojo_ry = PC(ancho, p->ojo_ry);
    c->ojo_dx = PC(ancho, p->ojo_dx);
    c->trazo  = p->trazo;
    c->blanco = p->blanco;
    c->iris   = p->iris;
    c->acento = p->acento;
}

static void rasgos(rk_fb_t *fb, cara_t *c, rk_mood_t mood,
                   uint8_t set, uint32_t t_ms)
{
    const rk_look_t *lk = rk_look(mood);
    rk_ojo_t forma = rk_look_parpadea(lk, t_ms) ? RK_OJO_BLINK
                                                : (rk_ojo_t)lk->ojo;
    int abre, pupila;
    int mira_x = 0;
    int angulo = c->p->ceja_angulo;

    apertura_de(forma, &abre, &pupila);

    /* La mirada deriva un poco salvo cuando el ánimo pide quietud. Es el
     * detalle más barato que separa "un dibujo" de "algo vivo". */
    if (forma != RK_OJO_DEAD && forma != RK_OJO_BLINK) {
        mira_x = rk_sin8((uint8_t)(t_ms / 55)) * c->ojo_rx / 500;
    }
    /* El ánimo inclina las cejas sobre la inclinación de fábrica: un modelo
     * enojado preocupado sigue siendo más enojado que un kawaii preocupado. */
    switch (mood) {
    case RK_MOOD_THIRSTY:
    case RK_MOOD_HOT:
    case RK_MOOD_SCORCHED:  angulo -= 10; break;
    case RK_MOOD_DROWNING:
    case RK_MOOD_COLD:
    case RK_MOOD_DARK:      angulo += 10; break;
    case RK_MOOD_HAPPY:     angulo += 4;  break;
    default: break;
    }

    if (set & RK_ADORNO_AURA)   { adornos(fb, c, RK_ADORNO_AURA, t_ms); }
    if (set & RK_ADORNO_ESPORAS){ adornos(fb, c, RK_ADORNO_ESPORAS, t_ms); }

    switch (c->p->familia) {
    case RK_OJOS_RASGADOS: ojos_rasgados(fb, c, forma, abre, pupila, mira_x, t_ms); break;
    case RK_OJOS_FIEROS:   ojos_fieros(fb, c, forma, abre, pupila, mira_x, t_ms);   break;
    case RK_OJOS_VISOR:    ojos_visor(fb, c, forma, abre, pupila, mira_x, t_ms);    break;
    case RK_OJOS_UNICO:    ojos_unico(fb, c, forma, abre, pupila, mira_x, t_ms);    break;
    case RK_OJOS_PESADOS:  ojos_pesados(fb, c, forma, abre, pupila, mira_x, t_ms);  break;
    default:               ojos_redondos(fb, c, forma, abre, pupila, mira_x, t_ms); break;
    }

    cejas(fb, c, angulo);
    boca(fb, c, (rk_boca_t)lk->boca, t_ms);

    adornos(fb, c, (uint8_t)(set & (uint8_t)~(RK_ADORNO_AURA |
                                              RK_ADORNO_ESPORAS)), t_ms);
}

void rk_face_rasgos(rk_fb_t *fb, int cx, int cy, int ancho,
                    const rk_persona_t *p, rk_mood_t mood,
                    uint8_t adornos_extra, uint32_t t_ms)
{
    cara_t c;

    if (fb == NULL || ancho < 16) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    armar(&c, fb, p, ancho);
    c.cx = cx;
    c.y_ojos = cy;
    /* En modo ficha la boca cuelga del ANCHO de la cara y no del alto del
     * panel: es lo que la mantiene compacta cuando se dibuja chica dentro de
     * una lista en vez de ocupando la pantalla entera. */
    c.y_boca = cy + ancho * 42 / 100;
    rasgos(fb, &c, mood, (uint8_t)(p->adornos | adornos_extra), t_ms);
}

void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p,
                  rk_mood_t mood, rk_severity_t sev,
                  uint8_t adornos_extra, uint32_t t_ms)
{
    const rk_look_t *lk;
    cara_t c;
    rk_color_t f1, f2;
    int bob, jitter;

    if (fb == NULL) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    lk = rk_look(mood);
    armar(&c, fb, p, fb->w);

    /* ---- fondo: el ánimo tiñe la paleta de la persona, no la reemplaza --
     * Así un cresta con frío sigue siendo verde ácido, apenas azulado. Si el
     * fondo lo pusiera el ánimo, los seis modelos se verían iguales en los
     * estados malos, que es justo cuando más hay que distinguirlos. */
    f1 = p->fondo;
    f2 = p->fondo2;
    if (lk->tint_amt > 0u) {
        uint8_t k = (uint8_t)(lk->tint_amt / 2);
        f1 = rk_mix(f1, lk->tint, k);
        f2 = rk_mix(f2, lk->tint, k);
    }
    if (lk->dim > 0u) {
        f1 = rk_dim(f1, lk->dim);
        f2 = rk_dim(f2, lk->dim);
    }
    rk_vgradient(fb, 0, 0, fb->w, fb->h, f1, f2);
    c.p = p;

    /* La respiración mueve la cara entera, y el tiritar la sacude. Como no
     * hay cuerpo, este es el único movimiento global que queda: sin él la
     * cara flota quieta en el medio y se ve impresa. */
    bob = lk->bob_amp
        ? rk_sin8((uint8_t)(t_ms * lk->bob_speed / 1000)) * lk->bob_amp / 127
        : 0;
    jitter = lk->shiver ? (((t_ms / 60) % 2) ? lk->shiver : -lk->shiver) : 0;
    c.cx     += jitter * 2;
    c.y_ojos += bob;
    c.y_boca += bob;

    {
        uint8_t set = (uint8_t)(p->adornos | adornos_extra);
        /* Una alerta urgente agrega brillos rojos... no: agrega latido al
         * acento, que es más sobrio y funciona en los seis modelos. */
        if (sev == RK_SEV_URGENT) {
            int pulso = rk_sin8((uint8_t)(t_ms / 5));
            c.acento = rk_mix(p->acento, RK_RGB(255, 90, 70),
                              (uint8_t)(128 + pulso / 2));
        }
        rasgos(fb, &c, mood, set, t_ms);
    }

    /* ---- pictograma: la cara dice que algo anda mal, esto dice qué ------ */
    if (sev != RK_SEV_OK) {
        int lado = fb->w / 9;
        int m    = fb->w / 24;
        rk_color_t ic = (sev == RK_SEV_URGENT) ? RK_RGB(255, 120, 100)
                                               : RK_RGB(250, 200, 110);
        if (rk_face_pictograma(fb, fb->w - lado - m, m, lado, mood, ic)) {
            /* nada mas: el pictograma ya se dibujo */
        }
    }
}

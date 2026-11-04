#ifndef BARREIRA_ATIVA_H
#define BARREIRA_ATIVA_H

#include <stdatomic.h>

typedef struct {
    _Alignas(64) atomic_int contador;
    _Alignas(64) atomic_int sentido;
    int n;
} barreira_ativa_t;

/* Barreira com espera ativa (spin-wait) e inversao de sentido.
   Cada thread inverte seu sentido local, incrementa o contador.
   A ultima a chegar zera o contador e publica o novo sentido.
   As demais giram lendo 'sentido' ate mudar. */
static inline void barreira_ativa_espera(barreira_ativa_t *b, int *sentido_local)
{
    int meu_sentido = *sentido_local;
    *sentido_local = !meu_sentido;

    /* Incrementa contador de chegadas. */
    int chegadas = atomic_fetch_add(&b->contador, 1) + 1;

    if (chegadas == b->n) {
        /* Sou a ultima: zera o contador e publica o novo sentido. */
        atomic_store(&b->contador, 0);
        atomic_store(&b->sentido, *sentido_local);
    } else {
        /* Nao sou a ultima: giro esperando o sentido mudar. */
        while (atomic_load(&b->sentido) == meu_sentido) {
            /* espera ativa (spin-wait) */
            ; /* vazio propositalmente */
        }
    }
}

#endif // BARREIRA_ATIVA_H
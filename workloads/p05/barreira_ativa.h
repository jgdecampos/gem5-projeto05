#ifndef BARREIRA_ATIVA_H
#define BARREIRA_ATIVA_H

#include <stdatomic.h>

typedef struct {
    _Alignas(64) atomic_int contador;
    _Alignas(64) atomic_int sentido;
    int n;
} barreira_ativa_t;

/* TODO(aluno): barreira com inversão de sentido (capítulo 4):
 *              cada thread inverte o seu sentido local, incrementa o contador;
 *              a última zera o contador e publica o novo sentido; as demais
 *              giram lendo 'sentido' até que ele mude. Sem PAUSE: no gem5 ela
 *              seria uma instrução nula de qualquer forma. */
void barreira_ativa_espera(barreira_ativa_t *b, int *sentido_local);

#endif // BARREIRA_ATIVA_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef USE_M5OPS
#include <gem5/m5ops.h>
#endif
#ifdef BARREIRA_ATIVA
#include "barreira_ativa.h" /* variante W3 */
#endif

enum modo { UNIFORME, DESBALANCEADO, CORRIGIDO };

typedef struct {
    int id, P; long N, R; enum modo m;
    long unidades; /* trabalho atribuído (impresso) */
    double resultado; /* contribui para a verificação */
} arg_t;


static double custo_linha(long i, long c);
    /* TODO(aluno): laço de c unidades, resultado usado */

static void *trabalhador(void *p) {
    arg_t *a = p;
    for (long r = 0; r < a->R; r++) {
    /* TODO(aluno): percorrer as linhas desta thread conforme o
     * modo: blocos contíguos (UNIFORME e
     * DESBALANCEADO) ou cíclico (CORRIGIDO); custo
     * 1 por linha no UNIFORME e i nos demais. */
    /* TODO(aluno): barreira (pthread ou ativa). */
    }
    return NULL;
}

int main(int argc, char **argv) {
    /* TODO(aluno): ler P, N, R e modo; criar P-1 threads; a thread
    * principal é o trabalhador 0. m5_reset_stats entre
    * duas barreiras iniciais; m5_dump_stats depois da
    * barreira final; imprimir resultado e unidades. */
    return 0;
}

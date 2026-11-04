#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int sentido_local; /* para barreira ativa */
} arg_t;

static pthread_barrier_t barreira_pthread;
static barreira_ativa_t barreira_ativa;

/* Executa c unidades de trabalho "umido" (soma simples) e retorna resultado. */
static double custo_linha(long i, long c) {
    double acc = 0.0;
    long k;
    for (k = 0; k < c; k++)
        acc += (double)(i + k) * 0.000001;
    return acc;
}

static void *trabalhador(void *p) {
    arg_t *a = p;
    long r, i;

    for (r = 0; r < a->R; r++) {
        if (a->m == UNIFORME) {
            /* Blocos contiguos de igual tamanho. */
            long bloco = a->N / a->P;
            long inicio = a->id * bloco;
            long fim = (a->id == a->P - 1) ? a->N : inicio + bloco;
            for (i = inicio; i < fim; i++)
                a->resultado += custo_linha(i, 1);
            a->unidades += (fim - inicio);
        }
        else if (a->m == DESBALANCEADO) {
            /* Laço triangular dividido em blocos contiguos.
               thread t recebe bloco com trabalho proporcional a (2t+1). */
            long bloco = a->N / a->P;
            long inicio = a->id * bloco;
            long fim = (a->id == a->P - 1) ? a->N : inicio + bloco;
            for (i = inicio; i < fim; i++)
                a->resultado += custo_linha(i, (long)(i + 1));
            a->unidades += (fim - inicio);
        }
        else if (a->m == CORRIGIDO) {
            /* Divisao ciclica: thread pega linhas id, id+P, id+2P, ... */
            for (i = a->id; i < a->N; i += a->P)
                a->resultado += custo_linha(i, (long)(i + 1));
            a->unidades += (a->N - a->id + a->P - 1) / a->P;
        }

        /* Barreira entre rodadas. */
#ifdef BARREIRA_ATIVA
        barreira_ativa_espera(&barreira_ativa, &a->sentido_local);
#else
        pthread_barrier_wait(&barreira_pthread);
#endif
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <P> <N> <R> <modo>\n", argv[0]);
        fprintf(stderr, "  P   = numero de threads (nucleos)\n");
        fprintf(stderr, "  N   = tamanho do problema (linhas)\n");
        fprintf(stderr, "  R   = numero de rodadas\n");
        fprintf(stderr, "  modo = 0 uniformo, 1 desbalanceado, 2 corrigido\n");
        return 1;
    }

    int P = atoi(argv[1]);
    long N = atol(argv[2]);
    long R = atol(argv[3]);
    int modo = atoi(argv[4]);

    if (P < 1 || P > 64) {
        fprintf(stderr, "P deve estar entre 1 e 64\n");
        return 1;
    }

    pthread_t threads[64];
    arg_t args[64];
    int t;

#ifdef BARREIRA_ATIVA
    barreira_ativa.contador = 0;
    barreira_ativa.sentido = 0;
    barreira_ativa.n = P;
#else
    pthread_barrier_init(&barreira_pthread, NULL, P);
#endif

    for (t = 0; t < P; t++) {
        args[t].id = t;
        args[t].P = P;
        args[t].N = N;
        args[t].R = R;
        args[t].m = (enum modo)modo;
        args[t].unidades = 0;
        args[t].resultado = 0.0;
        args[t].sentido_local = 0;
    }

    /* Primeira barreira: sincronizacao antes da regiao de interesse. */
#ifdef BARREIRA_ATIVA
    /* na versão ativa cada thread chama dentro de trabalhador; aqui só inicializa */
#else
    pthread_barrier_wait(&barreira_pthread);
#endif

#ifdef USE_M5OPS
    m5_reset_stats(0, 0);
#endif

#ifdef BARREIRA_ATIVA
    /* segunda barreira ativa dentro de cada thread apos o reset */
#else
    pthread_barrier_wait(&barreira_pthread);
#endif

    /* Cria P-1 threads; a main é a trabalhadora 0. */
    for (t = 1; t < P; t++)
        pthread_create(&threads[t], NULL, trabalhador, &args[t]);

    trabalhador(&args[0]);

    for (t = 1; t < P; t++)
        pthread_join(threads[t], NULL);

#ifdef USE_M5OPS
    m5_dump_stats(0, 0);
#endif

    /* Imprime resultado e unidades por thread (para validacao cruzada). */
    printf("Resultado triang.c (modo %d, %d threads, N=%ld, R=%ld):\n",
           modo, P, N, R);
    long total_unidades = 0;
    double total_resultado = 0.0;
    for (t = 0; t < P; t++) {
        printf("  thread %d: unidades = %ld, resultado = %.6f\n",
               t, args[t].unidades, args[t].resultado);
        total_unidades += args[t].unidades;
        total_resultado += args[t].resultado;
    }
    printf("  total unidades = %ld, total resultado = %.6f\n",
           total_unidades, total_resultado);

#ifdef BARREIRA_ATIVA
#else
    pthread_barrier_destroy(&barreira_pthread);
#endif

    return 0;
}
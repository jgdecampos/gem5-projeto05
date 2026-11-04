#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef USE_M5OPS
#include <gem5/m5ops.h>
#endif
#ifdef BARREIRA_ATIVA
#include "barreira_ativa.h"
#endif

typedef struct {
    int id, P;
    long trabalho; /* unidades de trabalho desta thread */
    long unidades_executadas;
    double resultado;
    int sentido_local;
} arg_t;

static pthread_barrier_t barreira_pthread;
static barreira_ativa_t barreira_ativa;

/* Trabalho umido: soma simples por 'unidades' iteracoes. */
static double fazer_trabalho(long unidades) {
    double acc = 0.0;
    long i;
    for (i = 0; i < unidades; i++)
        acc += (double)i * 0.000001;
    return acc;
}

static void *trabalhador(void *p) {
    arg_t *a = p;
    long r;

    for (r = 0; r < 10; r++) { /* 10 fases para gerar serie temporal */
        a->unidades_executadas += a->trabalho;
        a->resultado += fazer_trabalho(a->trabalho);

#ifdef BARREIRA_ATIVA
        barreira_ativa_espera(&barreira_ativa, &a->sentido_local);
#else
        pthread_barrier_wait(&barreira_pthread);
#endif
    }
    return NULL;
}

int main(int argc, char **argv) {
    /* Uso: pesos.c <P> <t0> <t1> ... <t(P-1)> <modo_barreira>
       modo_barreira: 0 = pthread, 1 = ativa */
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <P> <t0> <t1> ... <t(P-1)> <barreira>\n", argv[0]);
        fprintf(stderr, "  barreira: 0=pthread, 1=ativa\n");
        return 1;
    }

    int P = atoi(argv[1]);
    int modo_barreira = atoi(argv[argc - 1]);

    if (P < 1 || P > 64) {
        fprintf(stderr, "P deve estar entre 1 e 64\n");
        return 1;
    }

    pthread_t threads[64];
    arg_t args[64];
    int t;

    long trabalhos[64];
    for (t = 0; t < P; t++)
        trabalhos[t] = atol(argv[2 + t]);

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
        args[t].trabalho = trabalhos[t];
        args[t].unidades_executadas = 0;
        args[t].resultado = 0.0;
        args[t].sentido_local = 0;
    }

#ifdef BARREIRA_ATIVA
#else
    pthread_barrier_wait(&barreira_pthread);
#endif

#ifdef USE_M5OPS
    m5_reset_stats(0, 0);
#endif

#ifdef BARREIRA_ATIVA
#else
    pthread_barrier_wait(&barreira_pthread);
#endif

    for (t = 1; t < P; t++)
        pthread_create(&threads[t], NULL, trabalhador, &args[t]);

    trabalhador(&args[0]);

    for (t = 1; t < P; t++)
        pthread_join(threads[t], NULL);

#ifdef USE_M5OPS
    m5_dump_stats(0, 0);
#endif

    printf("Resultado pesos.c (P=%d):\n", P);
    long total = 0;
    for (t = 0; t < P; t++) {
        printf("  thread %d: trabalho = %ld, executou = %ld, resultado = %.6f\n",
               t, args[t].trabalho, args[t].unidades_executadas, args[t].resultado);
        total += args[t].unidades_executadas;
    }
    printf("  total executado = %ld\n", total);

#ifdef BARREIRA_ATIVA
#else
    pthread_barrier_destroy(&barreira_pthread);
#endif

    return 0;
}
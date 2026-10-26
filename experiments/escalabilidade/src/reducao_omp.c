#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N (1UL << 25)

int main(void) {
    int *v = malloc(N * sizeof(int));
    if (v == NULL) return 1;

    for (unsigned long i = 0; i < N; i++)
        v[i] = (int)(i % 97);

    uint64_t soma = 0;

    #pragma omp parallel for reduction(+:soma) schedule(static)
    for (unsigned long i = 0; i < N; i++)
        soma += (uint64_t)v[i] * (uint64_t)(v[i] + 3);

    printf("threads=%d soma=%llu\n", omp_get_max_threads(),
           (unsigned long long)soma);
    free(v);
    return 0;
}

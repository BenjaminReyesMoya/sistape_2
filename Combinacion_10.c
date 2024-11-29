#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define LIMITE 5000000 // Límite para la suma de números primos

// Función para verificar si un número es primo
int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Función para calcular la suma de números primos hasta un límite
long long calculate_primes() {
    long long sum = 0;
    for (int i = 2; i < LIMITE; i++) {
        if (is_prime(i)) {
            sum += i;
        }
    }
    return sum;
}

// Función para configurar la política de planificación y afinidad
void establecer_afinidad_y_politica(int algoritmo, int nucleos_cpu, int prioridad) {
    struct sched_param param;
    param.sched_priority = prioridad;
    sched_setscheduler(0, algoritmo, &param);

    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (int i = 0 ; i < nucleos_cpu; i++){
        CPU_SET(i, &mask);
    }
    sched_setaffinity(0, sizeof(mask), &mask);
}

int main() {
    int pipes[4][2]; // Pipes para comunicación entre procesos padre e hijos
    pid_t pids[4];
    int algoritmos[4] = {SCHED_FIFO, SCHED_RR, SCHED_OTHER, SCHED_BATCH};
    int nucleos_cpu[4] = {2, 3, 4, 1}; // Núcleos a asignar
    int prioridades[4] = {50, 50, 0, 0}; // Prioridades (solo FIFO y RR)

    for (int i = 0; i < 4; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) { // Proceso hijo
            close(pipes[i][0]); // Cierra el lado de lectura

            establecer_afinidad_y_politica(algoritmos[i], nucleos_cpu[i], prioridades[i]);

            struct timeval start, end;
            gettimeofday(&start, NULL);

            long long sum = calculate_primes();

            gettimeofday(&end, NULL);

            double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

            // Escribe los resultados en el pipe
            write(pipes[i][1], &sum, sizeof(sum));
            write(pipes[i][1], &elapsed_time, sizeof(elapsed_time));
            close(pipes[i][1]);
            exit(0);
        } else {
            close(pipes[i][1]); // Cierra el lado de escritura
        }
    }

    // Proceso padre
    for (int i = 0; i < 4; i++) {
        long long sum;
        double elapsed_time;

        read(pipes[i][0], &sum, sizeof(sum));
        read(pipes[i][0], &elapsed_time, sizeof(elapsed_time));
        close(pipes[i][0]);

        printf("Proceso %d:\n", i + 1);
        printf("  Política: %s\n", (algoritmos[i] == SCHED_FIFO) ? "FIFO" :
                                   (algoritmos[i] == SCHED_RR) ? "Round Robin" :
                                   (algoritmos[i] == SCHED_OTHER) ? "Normal" : "Batch");
        printf("  Núcleos: %d\n", nucleos_cpu[i]);
        printf("  Suma de primos: %lld\n", sum);
        printf("  Tiempo de ejecución: %.6f segundos\n\n", elapsed_time);

        waitpid(pids[i], NULL, 0); // Espera a que termine el hijo
    }

    return 0;
}
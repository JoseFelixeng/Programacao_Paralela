#include <stdio.h>
#include <omp.h>

void tarefa_A() { printf("Tarefa A na thread %d\n", omp_get_thread_num()); }
void tarefa_B() { printf("Tarefa B na thread %d\n", omp_get_thread_num()); }
void tarefa_C() { printf("Tarefa C na thread %d\n", omp_get_thread_num()); }

int main() {
    #pragma omp parallel sections num_threads(3)
    {
        #pragma omp section
        {
            tarefa_A();
        }

        #pragma omp section
        {
            tarefa_B();
        }

        #pragma omp section
        {
            tarefa_C();
        }
    }
    return 0;
}
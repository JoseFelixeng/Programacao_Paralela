#include <stdio.h>
#include <omp.h>
#define N 10000000
#define TOL  0.0000001

int main(){
    float a[N], b[N], c[N], res[N];
    int err=0;

    double init_time, compute_time, test_time;
    init_time    = -omp_get_wtime();

   // preenchendo o array 
   #pragma omp parallel for
   for (int i=0; i<N; i++){
      a[i] = (float)i;
      b[i] = 2.0*(float)i;
      c[i] = 0.0;
      res[i] = i + 2*i;
   }

   init_time    +=  omp_get_wtime();
   compute_time  = -omp_get_wtime();
   
   // operando a soma de vetores
   #pragma omp target
   #pragma omp loop
   for (int i=0; i<N; i++){
      c[i] = a[i] + b[i];
   }

   compute_time +=  omp_get_wtime();
   test_time     = -omp_get_wtime();

   // testando os resuldados
   #pragma omp parallel for reduction(+:err)
   for(int i=0;i<N;i++){
      float val = c[i] - res[i];
      val = val*val;
      if(val>TOL) err++;
   }

   test_time    +=  omp_get_wtime();
   
   // saidas
   printf(" vectors added with %d errors\n",err);
   printf("Init time:    %.6fs\n", init_time);
   printf("Compute time: %.6fs\n", compute_time);
   printf("Test time:    %.6fs\n", test_time);
   printf("Total time:   %.6fs\n", init_time + compute_time + test_time);
   return 0;
}
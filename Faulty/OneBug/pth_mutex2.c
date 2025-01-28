/* Arquivo:  
 *    pth_mutex1.c
 *
 * PropÃ³sito:
 *    
 * Input:
 *    nenhum
 * Output:
 *
 * Compile:  gcc -g -Wall -o pth_mutex1 pth_mutex1.c -lpthread
 * Usage:    ./pth_mutex1 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>


int publico = 0;

pthread_mutex_t mutex;

void incPublico(){
   
   publico++;
   
}

void *execute() {
   int i;

   for  (i = 1; i <= 100000; i++){
      incPublico();
   }
   return NULL;
} 


/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   pthread_t t1, t2, t3, t4; 
   
   pthread_mutex_init(&mutex, NULL);

   // CriaÃ§Ã£o e execuÃ§Ã£o das threads
   pthread_create(&t1, NULL, execute, NULL);  
   pthread_create(&t2, NULL, execute, NULL);  
   pthread_create(&t3, NULL, execute, NULL);  
   pthread_create(&t4, NULL, execute, NULL);  
   
   // Espera pela finalizaÃ§Ã£o das threads
   pthread_join(t1, NULL); 
   pthread_join(t2, NULL); 
   pthread_join(t3, NULL); 
   pthread_join(t4, NULL); 

   printf("PÃºblico final: %d\n", publico);
   pthread_mutex_destroy(&mutex);

   return 0;
}  /* main */



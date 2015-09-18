#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <err.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

/*Prototypes*/
static void generate_rnd_array(int *array, int element_count, int seed);
static void *greatest_element(void *arguments);
static void *calculate_sum(void *arguments);

/*Global variables*/
static int sum = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*Structure of arguments to pass to threads*/
typedef struct args {
  int *array;
  int start_point, number_of_elements;
} Args;

struct timeval tv_delta(struct timeval start, struct timeval end){
  struct timeval delta = end;

  delta.tv_sec -= start.tv_sec;
  delta.tv_usec -= start.tv_usec;
  if(delta.tv_usec < 0){
    delta.tv_usec += 1000000;
    delta.tv_usec--;
  }

  return delta;
}

int main(int argc, char *argv[]){
  int *array, element_count, thread_count, seed, task, array_chunk_size;
  int i = 0, *chunk_element_count, *t_returns, max_val = 0;
  char results;
  pthread_t *t_ids;
  Args *arguments;
  void *t_return = NULL;

  struct rusage start_ru, end_ru;
  struct timeval start_wall, end_wall;
  struct timeval diff_ru_utime, diff_wall, diff_ru_stime;
  
  /*Exit if not enough command line args*/
  if(argc != 6){
    printf("Not enough command line args\n");
    exit(EX_USAGE);
  }

  /*Parse command line arguments into usable data*/
  sscanf(argv[1], "%d", &element_count);
  sscanf(argv[2], "%d", &thread_count);
  sscanf(argv[3], "%d", &seed);
  sscanf(argv[4], "%d", &task);
  sscanf(argv[5], "%c", &results);

  /*Check for invalid arguments*/
  if(task != 1 && task != 2){
    printf("Invalid task number\n");
    exit(EX_USAGE);
  }
  
  if(results != 'Y' && results != 'N'){
    printf("Invalid print result argument\n");
    exit(EX_USAGE);
  }
  
  /*Handle distributing elements among the threads*/
  array_chunk_size = element_count / thread_count;
  chunk_element_count = malloc(thread_count * sizeof(int));
  for(i = 0; i < thread_count; i++){
    chunk_element_count[i] = array_chunk_size;
  }
  chunk_element_count[thread_count - 1] += (element_count - (thread_count * array_chunk_size));
  
  /*Create space for the array*/
  array = malloc(element_count * sizeof(int));
  
  /*Fill the array with random values based off of a seed*/
  generate_rnd_array(array, element_count, seed);

  /*Print the array*/
  /*printf("Start of array\n");
  for(i = 0; i < element_count; i++){
    printf("Element: %d\n", array[i]);
  }
  printf("End of array\n");*/
  
  /*Start the timing*/
  gettimeofday(&start_wall, NULL);
  getrusage(RUSAGE_SELF, &start_ru);
  
  /*Create space for the threads, their return values and their arguments*/
  t_ids = malloc(thread_count * sizeof(pthread_t));
  t_returns = malloc(thread_count * sizeof(int));
  arguments = malloc(thread_count * sizeof(Args));
  
  /*Check which task to perform*/
  if(task == 1){
    /*Create threads to do tasks*/
    for(i = 0; i < thread_count; i++){
      /*Set up the arguments for each thread*/
      arguments->array = array;
      arguments->start_point = i * array_chunk_size;
      arguments->number_of_elements = chunk_element_count[i];

      /*Create the threads and increment the pointer to traverse array*/
      if(pthread_create(t_ids, NULL, greatest_element, arguments)){
	printf("Error creating threads\n");
      }
      t_ids++;
      arguments++;
    }
    /*Put the pointer back at the beginning of the array*/
    t_ids -= thread_count;
    arguments -= thread_count;
    
    /*Reap the threads*/
    /*Put their return values in an array for computation of max*/
    /*Free the memory associated with the allocated max value*/
    for(i = 0; i < thread_count; i++){
      if(pthread_join(t_ids[i], &t_return)){
	printf("Error joining threads\n");
      }
      t_returns[i] = * (int *) t_return;
      free(t_return);
    }

    /*Find the maximum value*/
    for(i = 0; i < thread_count; i++){
      if(t_returns[i] > max_val){
	max_val = t_returns[i];
      }
    }

    /*End timing and find the difference*/
    gettimeofday(&end_wall, NULL);
    getrusage(RUSAGE_SELF, &end_ru);
    diff_ru_utime = tv_delta(start_ru.ru_utime, end_ru.ru_utime);
    diff_ru_stime = tv_delta(start_ru.ru_stime, end_ru.ru_stime);
    diff_wall = tv_delta(start_wall, end_wall);

    if(results == 'Y'){
      /*Print out the results*/
      printf("Max value: %d\n", max_val);
    } else {
      printf("No results printed\n");
    }

    /*Print the difference in time*/
    printf("User time: %ld.%06ld\n", diff_ru_utime.tv_sec, diff_ru_utime.tv_usec);
    printf("System time: %ld.%06ld\n", diff_ru_stime.tv_sec, diff_ru_stime.tv_usec);
    printf("Wall time: %ld.%06ld\n", diff_wall.tv_sec, diff_wall.tv_usec);
  } else if(task == 2){
    /*Create threads*/
    for(i = 0; i < thread_count; i++){
      /*Set up arguments for each thread*/
      arguments->array = array;
      arguments->start_point = i * array_chunk_size;
      arguments->number_of_elements = chunk_element_count[i];

      /*Create threads and pass arguments*/
      /*Increment pointer to traverse the array*/
      if(pthread_create(t_ids, NULL, calculate_sum, arguments)){
	printf("Error creating threads\n");
      }
      t_ids++;
      arguments++;
    }

    /*Put the pointer back at the beginning of the array*/
    t_ids -= thread_count;
    arguments -= thread_count;
    
    /*Reap the threads*/
    for(i = 0; i < thread_count; i++){
      if(pthread_join(t_ids[i], NULL)){
	printf("Error joining threads\n");
      }
    }

    /*End timing and find difference*/
    gettimeofday(&end_wall, NULL);
    getrusage(RUSAGE_SELF, &end_ru);
    diff_ru_utime = tv_delta(start_ru.ru_utime, end_ru.ru_utime);
    diff_ru_stime = tv_delta(start_ru.ru_stime, end_ru.ru_utime);
    diff_wall = tv_delta(start_wall, end_wall);

    /*Print results*/
    if(results == 'Y'){
      printf("Sum: %d\n", sum);
    } else {
      printf("No results printed\n");
    }

    /*Print the difference in time*/
    printf("User time: %ld.%06ld\n", diff_ru_utime.tv_sec, diff_ru_utime.tv_sec);
    printf("System time: %ld.%06ld\n", diff_ru_stime.tv_sec, diff_ru_stime.tv_usec);
    printf("Wall time: %ld.%06ld\n", diff_wall.tv_sec, diff_wall.tv_usec);
  }
  
  /*Free dynamically allocated memory*/
  free(array);
  free(t_ids);
  free(chunk_element_count);
  free(t_returns);
  free(arguments);
  
  return 0;
}

static void generate_rnd_array(int *array, int element_count, int seed){
  int i = 0;

  /*Set the seed*/
  srand(seed);

  /*Fill the each element of the array with random integer*/
  for(i = 0; i < element_count; i++){
    array[i] = rand() % 500;
  }
}

static void *greatest_element(void *arguments){
  int i = 0, *max_value;
  
  struct args argues = * (struct args *) arguments;

  max_value = malloc(sizeof(int));
  *max_value = -1;
  for(i = argues.start_point; i < argues.start_point + argues.number_of_elements; i++){
    if(argues.array[i] > *max_value){
      *max_value = *(argues.array + i);
    }
  }
  
  return max_value;
}

static void *calculate_sum(void *arguments){
  int i = 0, local_sum = 0;

  struct args argues = * (struct args *) arguments;
  
  for(i = argues.start_point; i < argues.start_point + argues.number_of_elements; i++){
    local_sum += argues.array[i];
  }
  
  pthread_mutex_lock(&mutex);
  sum += local_sum;
  pthread_mutex_unlock(&mutex);

  return NULL;
}

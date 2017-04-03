#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include "phases.h"

pthread_barrier_t barrier;
struct timeval startTime, endPhase1Time, endPhase2Time, endPhase3Time,  endTime;

float getTime(struct timeval start, struct timeval end) {
  return ((float) (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec)) / 1000000;
}

void *thread(void *threadData) {;
  struct threadData *data = threadData;

  pthread_barrier_wait(&barrier);
  if (data->id == 0) {
    gettimeofday(&startTime, 0);
  }
  pthread_barrier_wait(&barrier);
  phase1(data);
  pthread_barrier_wait(&barrier);
  // if there is only 1 thread then the list is already sorted
  if (data->numThreads != 1) {
    if (data->id == 0) {
      gettimeofday(&endPhase1Time, 0);
      phase2(data);
    }
    pthread_barrier_wait(&barrier);
    if (data->id == 0) {
      gettimeofday(&endPhase2Time, 0);
    }
    phase3(data);
    pthread_barrier_wait(&barrier);
    if (data->id == 0) {
      gettimeofday(&endPhase3Time, 0);
    }
    phase4(data);
    pthread_barrier_wait(&barrier);
    if (data->id == 0) { 
      // CONCATENATE THE LISTS TOGETHER
      int count = 0;
      for (int i = 0; i < data->numThreads; i++) {
	for (int j = 0; j < data->phase4Lists.sizes[i]; j++) {
	  data->unsortedArray[count] = data->phase4Lists.subLists[i][j];
	  count += 1;
	}
      }
      gettimeofday(&endTime, 0);
    }
    pthread_barrier_wait(&barrier);
    return NULL;
  }
  gettimeofday(&endTime, 0);
  return NULL;
}

void psrs(long *unsortedArray, long size, int numThreads) {

  // set up the arrays needed for intermediate values
  long *phase1Pivots;
  long *phase2Pivots;
  struct phase3Lists phase3Lists;
  struct phase4Lists phase4Lists;

  phase1Pivots = malloc(sizeof(long) * numThreads * numThreads);
  phase2Pivots = malloc(sizeof(long) * (numThreads - 1));
  phase3Lists.numLists = numThreads;
  phase3Lists.subLists = malloc(sizeof(long **) * phase3Lists.numLists);
  for (int i = 0; i < phase3Lists.numLists; i++) {
    phase3Lists.subLists[i] = malloc(sizeof(long *) * phase3Lists.numLists);
    for (int j = 0; j < phase3Lists.numLists; j++) {
      phase3Lists.subLists[i][j] = NULL;
    }
  }
  phase3Lists.sizes = malloc(sizeof (int *) * phase3Lists.numLists);
  for (int i = 0; i < phase3Lists.numLists; i++) {
    phase3Lists.sizes[i] = malloc(sizeof (int) * phase3Lists.numLists);
    for (int j = 0; j < phase3Lists.numLists; j++) {
      phase3Lists.sizes[i][j] = 0;
    }
  }
  phase4Lists.numLists= numThreads;
  phase4Lists.subLists = malloc(sizeof(long *) * phase4Lists.numLists);
  for (int i = 0; i < phase4Lists.numLists; i++) {
    phase4Lists.subLists[i] = NULL;
  }
  phase4Lists.sizes = malloc(sizeof(int) * phase4Lists.numLists);

  pthread_t threads[numThreads];
  struct threadData data[numThreads];

  // fill the data for the threads
  for (int i = 0; i < numThreads; i++) {
    data[i].id = i;
    data[i].threads = threads;
    data[i].unsortedArray = unsortedArray;
    data[i].phase1Pivots = phase1Pivots;
    data[i].phase2Pivots = phase2Pivots;
    data[i].phase3Lists = phase3Lists;
    data[i].phase4Lists = phase4Lists;
    data[i].size = size;
    data[i].numThreads = numThreads;
  }

  // create the threads
  for (int i = 0; i < numThreads; i++) {
    int value = pthread_create(&(threads[i]), NULL, thread, &(data[i]));
    if (value) {
      printf("Error in pthread_create\n");
      exit(-1);
    }
  }

  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  
  // free all memory
  for (int i = 0; i < phase4Lists.numLists; i++) {
    free(phase4Lists.subLists[i]);
  }
  free(phase4Lists.subLists);
  free(phase4Lists.sizes);
  for (int i = 0; i < phase3Lists.numLists; i++) {
    for (int j = 0; j < phase3Lists.numLists; j++) {
      free(phase3Lists.subLists[i][j]);
    }
    free(phase3Lists.subLists[i]);
    free(phase3Lists.sizes[i]);
  }
  free(phase3Lists.sizes);
  free(phase3Lists.subLists);
  free(phase2Pivots);
  free(phase1Pivots);
  return;
}

int main() {
  long  arraySizes[] = {10000000, 15000000, 20000000, 30000000, 50000000, 100000000};
  int numArrays = 6;
  int maxThreads = 16;
  long *unsortedArray;
  float timings[5];

  for (int i = 0; i < numArrays; i++) {
    for (int j = 1; j <= maxThreads; j *= 2) {
      pthread_setconcurrency(j);
      // initialize the barrier
      if(pthread_barrier_init(&barrier, NULL, j) != 0) {
	printf("Error initializing barrier\n");
	exit(-1);
      }
      // create the arrays
      unsortedArray = malloc(arraySizes[i]*sizeof(arraySizes[0]));
      for (int k = 0; k < 5; k++) {
	timings[k] = 0;
      }
      
      for (int run  = 0; run < 5; run++) {
	srandom(101);
	for (int k = 0; k <arraySizes[i]; k++) {
	  unsortedArray[k] = random();
	}

	// call psrs
	psrs(unsortedArray, arraySizes[i], j);
	for (int i = 0; i < arraySizes[i] - 1; i++) {
	  assert(unsortedArray[i] <= unsortedArray[i + 1]);
	}
	// get the timings
      
	if (j > 1) {
	  timings[0] += getTime(startTime, endPhase1Time);
	  timings[1] += getTime(endPhase1Time, endPhase2Time);
	  timings[2] += getTime(endPhase2Time, endPhase3Time);
	  timings[3] += getTime(endPhase3Time, endTime);
	  timings[4] += getTime(startTime, endTime);	
	}
	else {
	  timings[4] += getTime(startTime, endTime);
	  
	}
      }
      free(unsortedArray);
      // get the average timings
      for (int k = 0; k < 6; k++) {
	timings[k] = timings[k] / 5;
      }
      if (j > 1) {
	printf("NUM PROCESSORS: %i   TOTAL TIME: %f   P1 TIME: %f   P2 TIME: %f   P3 TIME: %f   P4 TIME: %f\n", j, timings[4], timings[0], timings[1], timings[2], timings[3]);
      }
      else {
	printf("-----------ARRAY SIZE: %i-----------\n", (int) arraySizes[i]);
	printf("NUM PROCESSORS: %i   TOTAL TIME: %f\n", j, timings[4]);
      }
    }
    printf("\n");
  }
  return 0;
}


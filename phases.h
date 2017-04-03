#pragma once

struct phase4Lists {
  long **subLists;
  int *sizes;
  int numLists;
};

struct phase3Lists {
  long ***subLists;
  int **sizes;
  int numLists;
};

struct threadData {
  int id;
  pthread_t *threads;
  long *unsortedArray;
  long *phase1Pivots;
  long *phase2Pivots;
  struct phase3Lists phase3Lists;
  struct phase4Lists phase4Lists;
  int size;
  int numThreads;
};

int compareFunction(const void * a, const void * b);
void phase1(struct threadData *data);
void phase2(struct threadData *data);
void phase3(struct threadData *data);
void phase4(struct threadData *data);

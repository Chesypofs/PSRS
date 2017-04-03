#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include "phases.h"

// taken from:
// www.cplusplus.com/reference/cstdlib/qsort/
int compareFunction(const void * a, const void * b) {
  if (*(long*)a < *(long*)b) return -1;
  if (*(long*)a > *(long*)b) return 1;
  else return 0;
}

void phase1(struct threadData *data) {
  int startIndex, endIndex;
  startIndex = data->id * data->size / data->numThreads;

  // if this is the last thread then assign it the rest of the list
  // it may have more items then the other threads but no more then twice as many
  if (data->numThreads == (data->id + 1)) {
    endIndex = data->size;
  } 
  else {
    endIndex = (data->id + 1) * data->size / data->numThreads;
  }

  // sort the subarray
  qsort((data->unsortedArray) + startIndex, endIndex - startIndex, sizeof(data->unsortedArray[0]), compareFunction);

  // find the pivots
  for (int i = 0; i < data->numThreads; i++) {
    data->phase1Pivots[(data->id * data->numThreads) + i] = data->unsortedArray[startIndex + (i * (data->size / (data->numThreads * data->numThreads)))];
  }
  return;
}

void phase2(struct threadData *data) {
  // sort the pivots
  qsort(data->phase1Pivots, data->numThreads * data->numThreads, sizeof(data->phase1Pivots[0]), compareFunction);

  // find the new pivots
  for (int i = 0; i < data->numThreads -1; i++) {
    data->phase2Pivots[i] = data->phase1Pivots[(((i+1) * data->numThreads) + (data->numThreads / 2)) - 1];
  }
  return;
}

void phase3(struct threadData *data) {
  int startIndex, endIndex;
  startIndex = data->id * data->size / data->numThreads;

  // if this is the last thread then assign it the rest of the list
  // it may have more items then the other threads but no more then twice as many
  if (data->numThreads == (data->id + 1)) {
    endIndex = data->size;
  } 
  else {
    endIndex = (data->id + 1) * data->size / data->numThreads;
  }

  for (int i = startIndex; i < endIndex; i++) {
    for (int j = 0; j < data->numThreads; j++) {
      if (data->unsortedArray[i] <= data->phase2Pivots[j]) {
	data->phase3Lists.sizes[data->id][j] += 1;
	break;
      }
    }
  }
  int count = 0;
  for (int i = 0; i < data->numThreads; i++) {
    count += data->phase3Lists.sizes[data->id][i];
  }
  if (count != (endIndex - startIndex)) {
    data->phase3Lists.sizes[data->id][data->numThreads - 1] = endIndex - startIndex - count;
  }

  int totalCounts = 0;
  for (int i = 0; i < data->numThreads; i++) {
    if (data->phase3Lists.sizes[data->id][i] > 0) {
      data->phase3Lists.subLists[data->id][i] = malloc(sizeof(long) * data->phase3Lists.sizes[data->id][i]);
      for (int j = 0; j < data->phase3Lists.sizes[data->id][i]; j++) {
	data->phase3Lists.subLists[data->id][i][j] = data->unsortedArray[startIndex + totalCounts + j];
      }
      totalCounts += data->phase3Lists.sizes[data->id][i];
    }
  }

  return;
}

void phase4(struct threadData *data) {
  int totalListSize = 0;
  for (int i = 0; i < data->numThreads; i++) {
    totalListSize += data->phase3Lists.sizes[i][data->id];
  }
  data->phase4Lists.subLists[data->id] = malloc(sizeof(long) * totalListSize);
  data->phase4Lists.sizes[data->id] = totalListSize;
  int indexes[data->phase4Lists.numLists];
  for (int i = 0; i < data->phase4Lists.numLists; i++) {
    indexes[i] = 0;
  }
  for (int i = 0; i < totalListSize; i++) {
    long lowest = LONG_MAX;
    int ind = -1;
    for (int j = 0; j < data->phase4Lists.numLists; j++) {
      if ((indexes[j] < data->phase3Lists.sizes[j][data->id]) && (data->phase3Lists.subLists[j][data->id][indexes[j]] < lowest)) {
	lowest = data->phase3Lists.subLists[j][data->id][indexes[j]];
	ind = j;
      }
    }
    data->phase4Lists.subLists[data->id][i] = lowest;
    indexes[ind] += 1;
  }
  return;
}

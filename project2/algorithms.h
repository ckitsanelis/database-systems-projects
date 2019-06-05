#ifndef ALGORITHMS_H_
#define ALGORITHMS_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sort.h"

void binary_search(int firstRecord, int lastRecord, int* foundRecords, int fileDesc, int fieldNo, void* value, int lastBlock, int recordsinLastBlock, int maxRecords, int blocksArray[]);
void Merge_Sort(int fd1, int fd2, int fd3);
void Bubble_Sort(int fileDesc, char *fileName, int fieldNo, int initialBlocks, int recordsinLastBlock);

#endif

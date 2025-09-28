// Queues.h - Header file for queue implementations
#ifndef QUEUES_H
#define QUEUES_H

#include <stdbool.h>
#include "sched_structs.h"

// -----------------------------------------------------------------------------
// FIFO Queue (Circular Buffer) Function Declarations
// -----------------------------------------------------------------------------
void initQueue(PCBQueue *q);
bool isQueueEmpty(const PCBQueue *q);
bool isQueueFull(const PCBQueue *q);
bool enqueuePCB(PCBQueue *q, PCB pcb);
bool dequeuePCB(PCBQueue *q, PCB *out);
void printQueue(PCBQueue *q);
void previewQueue(const PCBQueue *q, int outArr[]);

// -----------------------------------------------------------------------------
// Min-Heap Priority Queue Function Declarations
// -----------------------------------------------------------------------------
void initMinPQ(PCBMinPQ *pq);
bool isMinPQEmpty(const PCBMinPQ *pq);
bool isMinPQFull(const PCBMinPQ *pq);
bool minPQInsert(PCBMinPQ *pq, PCB *pcb);
bool minPQPop(PCBMinPQ *pq, PCB *out);

#endif // QUEUES_H
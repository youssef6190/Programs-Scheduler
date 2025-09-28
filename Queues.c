// Queues.c
// Implements PCBQueue (FIFO) and PCBMinPQ (min-heap priority queue with FIFO tie-breakers)
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "Queues.h"
// -----------------------------------------------------------------------------
// FIFO Queue (Circular Buffer)
// -----------------------------------------------------------------------------

void initQueue(PCBQueue *q) {
    q->head = q->tail = q->size = 0;
}

bool isQueueEmpty(const PCBQueue *q) {
    return q->size == 0;
}

bool isQueueFull(const PCBQueue *q) {
    return q->size == QUEUE_CAPACITY;
}

bool enqueuePCB(PCBQueue *q, PCB pcb) {
    if (isQueueFull(q))
        return false;
    q->data[q->tail] = pcb;
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->size++;
    return true;
}

bool dequeuePCB(PCBQueue *q, PCB *out) {
    if (isQueueEmpty(q))
        return false;
    *out = q->data[q->head];
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->size--;
    return true;
}
void printQueue(PCBQueue *q) {
    if (isQueueEmpty(q)) {
        printf("Queue is empty\n");
        return;
    }
    printf("Queue contents:\n");
    for (int i = 0; i < q->size; i++) {
        PCB pcb = q->data[(q->head + i) % QUEUE_CAPACITY];
        printf("Process ID: %d, State: %d, Priority: %d\n", pcb.process_id, pcb.state, pcb.priority);
    }
}
void previewQueue(const PCBQueue *q, int outArr[]) {
    for (int i = 0; i < q->size; i++) {
        outArr[i] = q->data[(q->head + i) % QUEUE_CAPACITY].process_id;
    }
}

// -----------------------------------------------------------------------------
// Min-Heap Priority Queue with FIFO tie-breaking
// -----------------------------------------------------------------------------

// Note: The PCBMinPQ struct (in Structures.h) must be extended as follows:
//   typedef struct {
//       PCB      heap[HEAP_CAPACITY];
//       int      size;
//       unsigned long seq_counter;            // next sequence number
//       unsigned long seqs[HEAP_CAPACITY];    // insertion sequence per entry
//   } PCBMinPQ;

void initMinPQ(PCBMinPQ *pq) {
    pq->size = 0;
    pq->seq_counter = 0;
}

bool isMinPQEmpty(const PCBMinPQ *pq) {
    return pq->size == 0;
}

bool isMinPQFull(const PCBMinPQ *pq) {
    return pq->size == HEAP_CAPACITY;
}

// Swap both PCB and its sequence number
static void swapEntry(PCBMinPQ *pq, int i, int j) {
    PCB           tmp_pcb  = pq->heap[i];
    unsigned long tmp_seq  = pq->seqs[i];

    pq->heap[i]   = pq->heap[j];
    pq->seqs[i]   = pq->seqs[j];

    pq->heap[j]   = tmp_pcb;
    pq->seqs[j]   = tmp_seq;
}

// Bubble-up insertion with tie-break on insertion order
bool minPQInsert(PCBMinPQ *pq, PCB *pcb) {
    if (isMinPQFull(pq))
        return false;

    int idx = pq->size++;
    pq->heap[idx] = *pcb;
    pq->seqs[idx] = pq->seq_counter++;

    while (idx > 0) {
        int parent = (idx - 1) / 2;
        bool higherPri = pq->heap[parent].priority > pq->heap[idx].priority;
        bool tieButOlder = (
            pq->heap[parent].priority == pq->heap[idx].priority &&
            pq->seqs[parent] > pq->seqs[idx]
        );
        if (!(higherPri || tieButOlder))
            break;
        swapEntry(pq, parent, idx);
        idx = parent;
    }
    return true;
}

// Trickle-down (heapify) with FIFO tie-break
static void minHeapify(PCBMinPQ *pq, int idx) {
    while (true) {
        int left     = 2*idx + 1;
        int right    = 2*idx + 2;
        int smallest = idx;

        if (left < pq->size) {
            bool leftBetter = pq->heap[left].priority < pq->heap[smallest].priority;
            bool tieButEarlier = (
                pq->heap[left].priority == pq->heap[smallest].priority &&
                pq->seqs[left] < pq->seqs[smallest]
            );
            if (leftBetter || tieButEarlier)
                smallest = left;
        }
        if (right < pq->size) {
            bool rightBetter = pq->heap[right].priority < pq->heap[smallest].priority;
            bool tieButEarlier = (
                pq->heap[right].priority == pq->heap[smallest].priority &&
                pq->seqs[right] < pq->seqs[smallest]
            );
            if (rightBetter || tieButEarlier)
                smallest = right;
        }
        if (smallest == idx)
            break;
        swapEntry(pq, idx, smallest);
        idx = smallest;
    }
}

// Pop the PCB with smallest priority (and oldest seq on ties)
bool minPQPop(PCBMinPQ *pq, PCB *out) {
    if (isMinPQEmpty(pq))
        return false;
    *out = pq->heap[0];
    pq->heap[0]  = pq->heap[--pq->size];
    pq->seqs[0]  = pq->seqs[pq->size];
    minHeapify(pq, 0);
    return true;
}
void previewMinPQ(const PCBMinPQ *pq, PCB *outArr, int* idx) {
    // Make a local copy to avoid destroying the original
    PCBMinPQ temp = *pq;
    PCB item;
    idx = 0;

    // Repeatedly pop from the temp heap in sorted order
    while (minPQPop(&temp, &item)) {
        outArr[(*idx)++] = item;
    }
}

 #define MEMORY_SIZE 60
 #define MAX_PROCESSES 60
 #define MAX_PATH_LENGTH 256

 #include <stdbool.h>

#ifndef QUEUE_CAPACITY
#define QUEUE_CAPACITY  60
#endif

#ifndef HEAP_CAPACITY
#define HEAP_CAPACITY   60
#endif

  // Enum for process states
typedef enum {
    READY,
    RUNNING,
    BLOCKED, 
    FINISHED
} ProcessState;

// Enum for scheduling algorithms
typedef enum {
    FCFS,
    ROUND_ROBIN,
    MULTILEVEL_FEEDBACK
} SchedulingAlgorithm;

// Struct for Process Control Block
typedef struct {
    int process_id;
    ProcessState state;
    int priority;
    int program_counter;
    int memory_lower_bound;
    int memory_upper_bound;
    char* program_instructions[20]; // Assuming max 20 instructions per program
    int instruction_count;
    int arrival_time;
    int currentMLFQueue;
    bool shiftDown; // For MLFQ
} PCB;
typedef struct {
    PCB    data[QUEUE_CAPACITY];
    int    head;   // index for next dequeue
    int    tail;   // index for next enqueue
    int    size;   // current number of elements
} PCBQueue;

// Priority Queue structure
typedef struct {
    PCB            heap[HEAP_CAPACITY];   // the binary-heap array
    int            size;                  // current number of elements
    unsigned long  seq_counter;           // next sequence number to assign
    unsigned long  seqs[HEAP_CAPACITY];   // per-element insertion sequence
} PCBMinPQ;

// Struct for Memory
typedef struct {
    char* name;
    char* value;
    int allocated;
} MemoryWord;

// Struct for Resource
typedef struct {
    char* name;
    int available;
    PCB* holder_process;
    PCBMinPQ* blocked;
    int waiting_processes[MAX_PROCESSES];
    int waiting_count;
} Resource;

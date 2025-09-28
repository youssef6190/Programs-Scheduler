#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Queues.h"
#include <stdio.h>
#include <glib.h>


// Global variables
extern PCB processes[MAX_PROCESSES];
extern int process_count;
extern MemoryWord memory[MEMORY_SIZE];
extern int clock_cycle;
extern SchedulingAlgorithm current_algorithm;
extern int quantum;
extern Resource resources[3]; // userInput, userOutput, file
extern int running_process_index;
extern int ready_queue[MAX_PROCESSES];
extern int ready_queue_count;
extern int blocked_queue[MAX_PROCESSES];
extern int blocked_queue_count;
extern gboolean simulation_running;
extern int mode;
extern int filecount;
extern char file_names[MAX_PROCESSES][256];
extern bool step;
extern int idleCount;
extern int quantum;
//imported from backend vars
extern PCBQueue firstLevelQueue;
extern PCBQueue secondLevelQueue;
extern PCBQueue thirdLevelQueue;
extern PCBQueue readyQueue; // Also works as fourth queue in MLFQ (Round Robin)

extern void executeInstructionUI(char* line, PCB* process);
extern void append_log(const char* message);
//global varunctions
Resource* mutex_converter(char* name){
    if(strcmp(name,"userInput")==0){
        //printf("Mutex taken: input_mutex\n");
        return &resources[0];
    }else if(strcmp(name,"userOutput")==0){
        //return &output_mutex;
        return &resources[1];
    }else {
        //return &access_file_mutex;
        return &resources[2];
    }
}

bool signalMutex(Resource* m) {
    if(m->available) {
        return false; // Error: signaling an unused mutex
    }
    if(isMinPQEmpty(m->blocked)) {
        m->available = true;
        m->holder_process = NULL;
        printf("Mutex released\n");
        return true;
    } else {
        printf("started in signal if");
        PCB* nextProcess = malloc(sizeof(PCB));
        printf("passes pointer");
        minPQPop(m->blocked, nextProcess);
        nextProcess->state = READY ; // Ready state
        m->holder_process = nextProcess;
        if(current_algorithm == MULTILEVEL_FEEDBACK) {
            printf("Process Current Queue is %d\n", nextProcess->currentMLFQueue);
            if(nextProcess->currentMLFQueue == 1) {
                if (nextProcess->shiftDown) {
                    nextProcess->currentMLFQueue = 2;
                    nextProcess->shiftDown = false;
                    enqueuePCB(&secondLevelQueue, *nextProcess);
                }
                else enqueuePCB(&firstLevelQueue, *nextProcess);
            } else if(nextProcess->currentMLFQueue == 2) {
                if (nextProcess->shiftDown) {
                    nextProcess->currentMLFQueue = 3;
                    nextProcess->shiftDown = false;
                    enqueuePCB(&thirdLevelQueue, *nextProcess);
                }
                else enqueuePCB(&secondLevelQueue, *nextProcess);
            } else if(nextProcess->currentMLFQueue == 3) {
                if (nextProcess->shiftDown) {
                    nextProcess->currentMLFQueue = 4;
                    nextProcess->shiftDown = false;
                    enqueuePCB(&readyQueue, *nextProcess);
                }
                else enqueuePCB(&thirdLevelQueue, *nextProcess);
            } else {
                if (nextProcess->shiftDown) {
                    nextProcess->currentMLFQueue = 4;
                    nextProcess->shiftDown = false;
                    enqueuePCB(&readyQueue, *nextProcess);
                }
                else enqueuePCB(&readyQueue, *nextProcess);
            }
        }else {
            printf("trying to enqueu");
            enqueuePCB(&readyQueue, *nextProcess);
            printf("enqueue successful");
        }
        return true;
    }
}

bool waitMutex(Resource* m, PCB* pcb) {
    if(pcb == NULL) {
        printf("Error: PCB is NULL\n");
        return false;
    }
    if(m->available) {
        m->available = false;
        m->holder_process = pcb;
        m->holder_process->state = READY;
        printf("Process %d acquired mutex\n", pcb->process_id);
        return true;
    } else {
        minPQInsert(m->blocked, pcb);
        pcb->state = BLOCKED;
        printf("Process %d is blocked on mutex\n", pcb->process_id);
        return false;
    }
}
// Set Variable in memory
void setVariable(PCB* process, char* name, char* value) {
    // If variable already exists
    for (int i = process->memory_lower_bound; i < process->memory_upper_bound; i++) {
        if (memory[i].allocated && strcmp(memory[i].name, name) == 0) {
            memory[i].value = value;
            return;
        }
    }
    // If variable does not exist
    for (int i = process->memory_lower_bound; i < process->memory_upper_bound; i++) {
        if (strcmp(memory[i].value, "NULL") == 0) {
            memory[i].name = strdup(name);
            memory[i].value = strdup(value);
            memory[i].allocated = 1;
            return;
        }
    }
}


// Get Variable from memory
char* getVariable(PCB* process, char* name) {
    for (int i = process->memory_lower_bound; i < process->memory_upper_bound; i++) {
        if (memory[i].allocated && strcmp(memory[i].name, name) == 0) {
            return memory[i].value;
        }
    }
    return NULL;
}



// File Reader
void fileReader(const char* fileName, PCB* process) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL)
        printf("Error opening file\n");
    else {
        char line[256];
        while (fgets(line, sizeof(line), file))
            executeInstructionUI(line, process);
    }
    fclose(file);
}

int countInstructions(const char *filename) {
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: could not open file %s\n", filename);
        return -1; // error
    }

    int lines = 0;
    char c;
    int last_char_was_newline = 1; // Start as if last char was newline

    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            lines++;
            last_char_was_newline = 1;
        } else {
            last_char_was_newline = 0;
        }
    }

    fclose(file);

    // If the last character was not newline and file was not empty, add one more line
    if (!last_char_was_newline) {
        lines++;
    }
    return lines;
}

void executeLineFromFile(const char *filename, int program_counter, PCB *process) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    char line[256];
    int current_line = 0;

    // Read the specific line corresponding to the program_counter
    while (fgets(line, sizeof(line), file)) {
        if (current_line == program_counter) {
        executeInstructionUI(line, process); // Execute the instruction
            break;
        }
        current_line++;
    }

    fclose(file);
}



void fcfs() {
    current_algorithm = FCFS;
    static bool arrived [MAX_PROCESSES] = {false};
    
    static int finished_processes = 0;


    static int total_instructions[MAX_PROCESSES];
    static int arrival_times[MAX_PROCESSES];

    // Count instructions in each program
    for (int i = 0; i < process_count; i++) {
        total_instructions[i] = processes[i].instruction_count;
    }
    // Count instructions in each program
    for (int i = 0; i < process_count; i++) {
        arrival_times[i] = processes[i].arrival_time;
    }

    while (finished_processes < process_count) {
        // Handle process arrivals
        for (int i = 0; i < process_count; i++) {
            if (arrival_times[i] == clock_cycle && !arrived[i]) {
                arrived[i] = true;
                processes[i].state = READY;
                enqueuePCB(&readyQueue, processes[i]);
            }
        }
        printQueue(&readyQueue);
        if (!isQueueEmpty(&readyQueue)) {
            idleCount = 0;
            PCB current = readyQueue.data[readyQueue.head];
            int pid = current.process_id - 1;
            processes[pid].state = RUNNING;
            running_process_index = current.process_id;
            append_log("Reached this line");
            executeLineFromFile(file_names[pid], processes[pid].program_counter, &processes[pid]);
            append_log("Reached this line too");
            processes[pid].program_counter++;

            clock_cycle++;
            // Check if process is finished
            if (processes[pid].program_counter >= total_instructions[pid]) {
                processes[pid].state = FINISHED;
                finished_processes++;
                dequeuePCB(&readyQueue, &current);
                } else {
                processes[pid].state = READY;
            } 

            // Debug pause if in step-by-step mode
            if (mode == 2) {
                printf("ready queue after exec");
                printQueue(&readyQueue);
                return;
            }

            // Handle arrivals during execution
            for (int i = 0; i < process_count; i++) {
                if (arrival_times[i] == clock_cycle && !arrived[i]) {
                    arrived[i] = true;
                    processes[i].state = READY;
                    enqueuePCB(&readyQueue, processes[i]);
                }
            }
            
            
        } else {
            append_log("No current Processes to run yet.");
            clock_cycle++;
            idleCount++;
            if (mode == 2) {
                printQueue(&readyQueue);
                return;
            }
        }
    }
    append_log("All processes completed");
}

void roundRobin() {
    current_algorithm = ROUND_ROBIN;

    static bool arrived[MAX_PROCESSES] = {false};
    static int finished_processes = 0;


    static int total_instructions[MAX_PROCESSES];
    static int arrival_times[MAX_PROCESSES];

    // Count instructions in each program
    for (int i = 0; i < process_count; i++) {
        total_instructions[i] = processes[i].instruction_count;
    }
    // Count instructions in each program
    for (int i = 0; i < process_count; i++) {
        arrival_times[i] = processes[i].arrival_time;
    }


    while (finished_processes < process_count) {

        // Handle process arrivals
        for (int i = 0; i < process_count; i++) {
            if (arrival_times[i] == clock_cycle && !arrived[i]) {
                arrived[i] = true;
                processes[i].state = READY;
                enqueuePCB(&readyQueue, processes[i]);
            }
        }

        printQueue(&readyQueue);

        if (!isQueueEmpty(&readyQueue)) {
            append_log("Haga bet run");
            PCB current;
            dequeuePCB(&readyQueue, &current);
            PCB *currentProcess = &processes[current.process_id-1];
            running_process_index = currentProcess->process_id;
            currentProcess->state = RUNNING;

            printf("Scheduling Process %d (quantum: %d)\n", currentProcess->process_id, quantum);

            int quantum_used = 0;

            while (quantum_used < quantum &&
                   currentProcess->program_counter < total_instructions[currentProcess->process_id - 1] &&
                   currentProcess->state != BLOCKED) {

                    
                executeLineFromFile(
                    file_names[current.process_id-1],
                    currentProcess->program_counter,
                    currentProcess
                );

                currentProcess->program_counter++;
                quantum_used++;

                printf("Process %d executed instruction %d/%d\n",
                       currentProcess->process_id,
                       currentProcess->program_counter,
                       total_instructions[currentProcess->process_id - 1]);
                    //arrival of processes
                for (int i = 0; i < process_count; i++) {
                        if (arrival_times[i] == clock_cycle && !arrived[i]) {
                            arrived[i] = true;
                            processes[i].state = READY;
                            enqueuePCB(&readyQueue, processes[i]);
                        }
                    }
                    printQueue(&readyQueue);

                clock_cycle++;

                if (quantum_used < quantum && 
                    currentProcess->program_counter < total_instructions[currentProcess->process_id - 1] &&
                    currentProcess->state != BLOCKED) {
                    //printf("\n<<<<<<<<<<<<<<<< [CLK %d] >>>>>>>>>>>>>>>>>>\n", clock_cycle);
                }

                if (currentProcess->state == BLOCKED) {
                    printf("Process %d became BLOCKED.\n", currentProcess->process_id);
                    return;
                }

                
            }
            if (currentProcess->state != BLOCKED) {
                if (currentProcess->program_counter >= total_instructions[currentProcess->process_id - 1]) {
                    currentProcess->state = FINISHED;
                    finished_processes++;
                    printf("Process %d completed at CLK %d\n", currentProcess->process_id, clock_cycle - 1);
                } else {
                    currentProcess->state = READY;
                    enqueuePCB(&readyQueue, *currentProcess);
                    printf("Process %d not finished, re-enqueued.\n", currentProcess->process_id);
                }
            }
            if (mode == 2) {
                printf("ready queue after exec");
                printQueue(&readyQueue);
                return;
            }
        } else {
            append_log("No current Processes to run yet.");
            clock_cycle++;
            idleCount++;
            if (mode == 2) {
                printQueue(&readyQueue);
                return;
            }
        }
    }

    append_log("All processes completed");
}

void mlfq() {
    current_algorithm = MULTILEVEL_FEEDBACK;
    static bool arrived[MAX_PROCESSES] = {false};
    static int finished_processes = 0;
    static int total_instructions[MAX_PROCESSES];
    static int arrival_times[MAX_PROCESSES];

    for (int i = 0; i < process_count; i++) {
        total_instructions[i] = processes[i].instruction_count;
        arrival_times[i] = processes[i].arrival_time;
    }

    while (finished_processes < process_count) {
        // Arrival handling at the start of the cycle
        for (int i = 0; i < process_count; i++) {
            if (!arrived[i] && arrival_times[i] == clock_cycle) {
                arrived[i] = true;
                processes[i].state = READY;
                processes[i].currentMLFQueue = 1;
                enqueuePCB(&firstLevelQueue, processes[i]);
            }
        }

        // Select process based on MLFQ level
        PCB *currentProcess = NULL;
        int quantum_length = 0;
        int nextQueueLevel = 0;

        if (!isQueueEmpty(&firstLevelQueue)) {
            PCB temp; dequeuePCB(&firstLevelQueue, &temp);
            currentProcess = &processes[temp.process_id - 1];
            quantum_length = 1; nextQueueLevel = 2;
        } else if (!isQueueEmpty(&secondLevelQueue)) {
            PCB temp; dequeuePCB(&secondLevelQueue, &temp);
            currentProcess = &processes[temp.process_id - 1];
            quantum_length = 2; nextQueueLevel = 3;
        } else if (!isQueueEmpty(&thirdLevelQueue)) {
            PCB temp; dequeuePCB(&thirdLevelQueue, &temp);
            currentProcess = &processes[temp.process_id - 1];
            quantum_length = 4; nextQueueLevel = 4;
        } else if (!isQueueEmpty(&readyQueue)) {
            PCB temp; dequeuePCB(&readyQueue, &temp);
            currentProcess = &processes[temp.process_id - 1];
            quantum_length = 8; nextQueueLevel = 4;
        }

        if (currentProcess) {
            currentProcess->state = RUNNING;
            running_process_index = currentProcess->process_id;
            int pid = currentProcess->process_id - 1;
            int quantum_used = 0;

            while (quantum_used < quantum_length &&
                   currentProcess->program_counter < total_instructions[pid] &&
                   currentProcess->state != BLOCKED) {

                // Handle new arrivals during execution
                for (int i = 0; i < process_count; i++) {
                    if (!arrived[i] && arrival_times[i] == clock_cycle) {
                        arrived[i] = true;
                        processes[i].state = READY;
                        processes[i].currentMLFQueue = 1;
                        enqueuePCB(&firstLevelQueue, processes[i]);
                    }
                }

                // If last cycle of quantum, mark for demotion
                if (quantum_used == quantum_length - 1)
                    currentProcess->shiftDown = true;

                executeLineFromFile(file_names[pid], currentProcess->program_counter, currentProcess);
                currentProcess->program_counter++;

                // Log clock
                char log_msg[64];
                sprintf(log_msg, "Clock cycle: %d Completed", clock_cycle);
                append_log(log_msg);

                // Advance clock
                clock_cycle++;
                quantum_used++;

                if (currentProcess->state == BLOCKED)
                    break;
            }

            if (currentProcess->state == BLOCKED) {
                continue;
            }

            if (currentProcess->program_counter >= total_instructions[pid]) {
                currentProcess->state = FINISHED;
                finished_processes++;
            } else {
                currentProcess->state = READY;
                if (currentProcess->shiftDown) {
                    currentProcess->shiftDown = false;
                    currentProcess->currentMLFQueue = nextQueueLevel;
                }
                switch (currentProcess->currentMLFQueue) {
                    case 1: enqueuePCB(&firstLevelQueue, *currentProcess); break;
                    case 2: enqueuePCB(&secondLevelQueue, *currentProcess); break;
                    case 3: enqueuePCB(&thirdLevelQueue, *currentProcess); break;
                    default: enqueuePCB(&readyQueue, *currentProcess); break;
                }
            }
        } else {
            append_log("No current Processes to run yet.");
            clock_cycle++;
            idleCount++;
        }

        if (mode == 2) return; // Step mode
    }

    append_log("All processes completed");
}

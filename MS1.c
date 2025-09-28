#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <sys/resource.h>

// Structure to store various performance metrics for a thread
struct ThreadMetrics {
    long int release_time;      // Time when the thread was released/created
    long int start_time;        // Time when the thread started execution
    long int finish_time;       // Time when the thread finished execution
    long int exec_time;         // CPU execution time (useful work time)
    long int waiting_time;      // Time spent waiting (turnaround time minus exec time)
    long int response_time;     // Time between release and start of execution
    long int turnaround_time;   // Total time from release to finish
    long int cpu_utilization;   // Percentage of CPU time utilized
    long int cpu_useful_work;   // Actual CPU time spent on computations
    long int memory_consumption;// Memory usage in kilobytes
};

struct ThreadMetrics metric[3]; // Array to hold metrics for 3 threads

// Function to print the collected thread metrics
void printThreadMetrics(struct ThreadMetrics metrics) {
    printf("\nThread Metrics:\n");
    printf("  Release Time: %ld microseconds\n", metrics.release_time);
    printf("  Start Time: %ld microseconds\n", metrics.start_time);
    printf("  Finish Time: %ld microseconds\n", metrics.finish_time);
    printf("  Execution Time (CPU time): %ld microseconds\n", metrics.exec_time);
    printf("  Waiting Time: %ld microseconds\n", metrics.waiting_time);
    printf("  Response Time: %ld microseconds\n", metrics.response_time);
    printf("  Turnaround Time: %ld microseconds\n", metrics.turnaround_time);
    printf("  CPU Useful Work: %ld microseconds\n", metrics.cpu_useful_work);
    printf("  CPU Utilization: %ld%%\n", metrics.cpu_utilization);
    printf("  Memory Consumption: %ld kilobytes\n\n", metrics.memory_consumption);
}

// Returns the current time in microseconds using CLOCK_MONOTONIC
long int get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Thread function to prompt user for a range of alphabets and print them
void* printAlphabetRange(void* arg) {
    struct ThreadMetrics *metrics = &metric[0];
    
    // Record CPU start time for this thread's computation
    struct timespec cpu_start, cpu_end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_start);
    
    metrics->start_time = get_time(); // Record the start time

    char ch1, ch2;
    printf("Enter first alphabetic character: ");
    scanf(" %c", &ch1);
    printf("Enter second alphabetic character: ");
    scanf(" %c", &ch2);

    // Swap characters if they are in reverse order
    if (ch1 > ch2) {
        char temp = ch1;
        ch1 = ch2;
        ch2 = temp;
    }

    // Print characters in the specified range
    printf("Characters between %c and %c: ", ch1, ch2);
    for (char c = ch1; c <= ch2; c++) {
        printf("%c ", c);
    }
    printf("\n");

    metrics->finish_time = get_time(); // Record the finish time

    // Calculate CPU time spent on useful work for this thread
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_end);
    metrics->cpu_useful_work = (cpu_end.tv_sec - cpu_start.tv_sec) * 1000000 +
                               (cpu_end.tv_nsec - cpu_start.tv_nsec) / 1000;

    metrics->exec_time = metrics->cpu_useful_work;
    
    // Calculate turnaround and waiting times along with response time
    metrics->turnaround_time = metrics->finish_time - metrics->release_time;
    metrics->waiting_time = metrics->turnaround_time - metrics->exec_time;
    metrics->response_time = metrics->start_time - metrics->release_time;
    metrics->cpu_utilization = (metrics->turnaround_time > 0) ?
                               (metrics->exec_time * 100) / metrics->turnaround_time : 0;
    
    // Retrieve memory usage for this thread
    struct rusage usage;
    getrusage(RUSAGE_THREAD, &usage);
    metrics->memory_consumption = usage.ru_maxrss;

    pthread_exit(NULL); // Terminate the thread
}

// Thread function to print the thread ID multiple times
void* printIDs(void* arg) {
    // Artificial delay to simulate workload
    for (size_t i = 0; i < 10000000; i++)
    {}
    
    struct ThreadMetrics *metrics = &metric[1];

    // Record CPU start time for this thread's work
    struct timespec cpu_start, cpu_end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_start);
    
    metrics->start_time = get_time(); // Record the start time

    // Print the thread's ID three times
    for (int i = 0; i < 3; i++) {
        printf("The Thread ID is: %lu\n", (unsigned long)pthread_self());
    }

    metrics->finish_time = get_time(); // Record the finish time
    
    // Calculate CPU time used by this thread for computations
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_end);
    metrics->cpu_useful_work = (cpu_end.tv_sec - cpu_start.tv_sec) * 1000000 +
                               (cpu_end.tv_nsec - cpu_start.tv_nsec) / 1000;

    metrics->exec_time = metrics->cpu_useful_work;
    
    // Compute turnaround, waiting, and response times
    metrics->turnaround_time = metrics->finish_time - metrics->release_time;
    metrics->waiting_time = metrics->turnaround_time - metrics->exec_time;
    metrics->response_time = metrics->start_time - metrics->release_time;
    metrics->cpu_utilization = (metrics->turnaround_time > 0) ?
                               (metrics->exec_time * 100) / metrics->turnaround_time : 0;
    
    // Get memory usage details for this thread
    struct rusage usage;
    getrusage(RUSAGE_THREAD, &usage);
    metrics->memory_consumption = usage.ru_maxrss;

    pthread_exit(NULL); // Terminate the thread
}

// Thread function to calculate the sum, product, and average of a range of integers
void* calculateSumAndProduct(void* arg) {

    // Artificial delay loop to simulate workload
    for (int i = 0; i < 100000000; i++) {}

    struct ThreadMetrics *metrics = &metric[2];
    
    // Record CPU start time for this thread's computation
    struct timespec cpu_start, cpu_end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_start);
    
    metrics->start_time = get_time(); // Record the start time

    int num1, num2;
    int sum = 0;
    int product = 1;
    printf("Enter first Integer number: ");
    scanf("%d", &num1);
    printf("Enter second Integer number: ");
    scanf("%d", &num2);
    // Calculate sum and product for the range between num1 and num2
    for (int i = num1; i <= num2; i++) {
        sum += i;
        product *= i;
    }
    
    // Print the results: sum, product, and average
    printf("THE SUM OF ALL NUMBERS BETWEEN %d and %d is %d\n", num1, num2, sum);
    printf("THE PRODUCT OF ALL NUMBERS BETWEEN %d and %d is %d\n", num1, num2, product);
    printf("THE AVERAGE OF ALL NUMBERS BETWEEN %d and %d is %d\n", num1, num2, sum/(num2-num1+1));

    metrics->finish_time = get_time(); // Record the finish time
    
    // Compute the CPU time used for the thread's useful work
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_end);
    metrics->cpu_useful_work = (cpu_end.tv_sec - cpu_start.tv_sec) * 1000000 +
                               (cpu_end.tv_nsec - cpu_start.tv_nsec) / 1000;

    metrics->exec_time = metrics->cpu_useful_work;
    
    // Calculate turnaround time, waiting time, and response time
    metrics->turnaround_time = metrics->finish_time - metrics->release_time;
    metrics->waiting_time = metrics->turnaround_time - metrics->exec_time;
    metrics->response_time = metrics->start_time - metrics->release_time;
    metrics->cpu_utilization = (metrics->turnaround_time > 0) ?
                               (metrics->exec_time * 100) / metrics->turnaround_time : 0;
    
    // Retrieve memory consumption details for this thread
    struct rusage usage;
    getrusage(RUSAGE_THREAD, &usage);
    metrics->memory_consumption = usage.ru_maxrss;

    pthread_exit(NULL); // Terminate the thread
}

// Set CPU affinity to bind the process to CPU core 0 for consistent scheduling
void set_cpu_affinity() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    // Apply the CPU affinity setting; exit on failure
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity failed");
        exit(1);
    }
}

int main() {
    set_cpu_affinity(); // Bind process to a specific CPU core (CPU 0)
    pthread_t threads[3];
    pthread_attr_t attr;
    struct sched_param param;
    int choice, policy, ret;
    
    // Prompt user to select a scheduling algorithm
    printf("Select scheduling algorithm:\n");
    printf("1. Round-Robin Scheduling \n");
    printf("2. First Come First Serve Scheduling \n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    
    // Set scheduling policy based on user's selection
    if (choice == 1) {
        policy = SCHED_RR;
    } else if (choice == 2) {
        policy = SCHED_FIFO;
    } else {
        printf("Invalid choice\n");
        exit(1);
    }
    int max_priority = sched_get_priority_max(policy);
    param.sched_priority = max_priority;
    pthread_attr_init(&attr);
    
    // Set the thread scheduling policy and priority
    ret = pthread_attr_setschedpolicy(&attr, policy);
    if(ret != 0){
        perror("pthread_attr_setschedpolicy");
    }
    ret = pthread_attr_setschedparam(&attr, &param);
    if(ret != 0){
        perror("pthread_attr_setschedparam");
    }

    // Create thread for printing alphabet range and record its release time
    metric[0].release_time = get_time();
    pthread_create(&threads[0], &attr, printAlphabetRange, NULL);

    // Create thread for printing thread IDs and record its release time
    metric[1].release_time = get_time();
    pthread_create(&threads[1], &attr, printIDs, NULL);

    // Create thread for calculating sum and product and record its release time
    metric[2].release_time = get_time();
    pthread_create(&threads[2], &attr, calculateSumAndProduct, NULL);

    // Wait for all threads to finish execution
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_attr_destroy(&attr);
    
    // Print the performance metrics for each thread
    printThreadMetrics(metric[0]);
    printThreadMetrics(metric[1]);
    printThreadMetrics(metric[2]);

    // Display overall memory consumption for the process
    struct rusage overall_usage;
    getrusage(RUSAGE_SELF, &overall_usage);
    printf("Overall Process Memory Consumption: %ld kilobytes\n", overall_usage.ru_maxrss);

    return 0;
}

/**
 * OS Scheduler Simulation UI
 * GTK3-based interface for Operating Systems Project - Milestone 2
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Queues.h"

// Global variables
PCB processes[MAX_PROCESSES];
int process_count = 0;
MemoryWord memory[MEMORY_SIZE];
int clock_cycle = 0;
SchedulingAlgorithm current_algorithm = FCFS;
int quantum = 2;
Resource resources[3]; // userInput, userOutput, file
int running_process_index = -1;
int blocked_queue[MAX_PROCESSES];
int blocked_queue_count = 0;
gboolean simulation_running = FALSE;
int mode;
int filecount;
char file_names[MAX_PROCESSES][256]; // Change to array of char arrays instead of array of pointers - DONT CHANGE SIZE
bool step = false;
int idleCount = 0;
int blocked_priority[MAX_PROCESSES];
char * blocked_locations[MAX_PROCESSES];

//imported from backend vars
PCBQueue firstLevelQueue;
PCBQueue secondLevelQueue;
PCBQueue thirdLevelQueue;
PCBQueue readyQueue; // Also works as fourth queue in MLFQ (Round Robin)
// GUI Components
GtkWidget *window;
GtkWidget *main_box;

// Dashboard Components
GtkWidget *dashboard_frame;
GtkWidget *process_list_view;
GtkListStore *process_list_store;
GtkWidget *ready_queue_view;
GtkListStore *ready_queue_store;
GtkWidget *blocked_queue_view;
GtkListStore *blocked_queue_store;

GtkWidget *clock_cycle_label;
GtkWidget *algo_label;

// Control Panel Components
GtkWidget *control_panel_frame;
GtkWidget *algorithm_combo;
GtkWidget *quantum_spin;
GtkWidget *start_button;
GtkWidget *stop_button;
GtkWidget *reset_button;
GtkWidget *step_button;

// Resource Panel Components
GtkWidget *resource_panel_frame;
GtkWidget *user_input_status;
GtkWidget *user_output_status;
GtkWidget *file_status;

// Memory Viewer Components
GtkWidget *memory_frame;
GtkWidget *memory_view;
GtkListStore *memory_store;

// Log & Console Components
GtkWidget *log_frame;
GtkWidget *log_view;
GtkTextBuffer *log_buffer;

// Process Creation Components
GtkWidget *process_creation_frame;
GtkWidget *file_chooser_button;
GtkWidget *arrival_time_spin;
GtkWidget *priority_spin;  // Add this line
GtkWidget *add_process_button;

// Function declarations
void initialize_ui();
void setup_dashboard();
void setup_control_panel();
void setup_resource_panel();
void setup_memory_viewer();
void setup_log_console();
void setup_process_creation();
void update_ui();
void append_log(const char* message);
void start_simulation();
void stop_simulation();
void reset_simulation();
void step_simulation();
void add_process();
void change_algorithm();
void initialize_resources();
void update_blocked_queue();
void update_ready_queue_table();
void update_blocked_queue_table();
void executeInstructionUI(char* line, PCB* process);
void update_ready_queue_table();
bool checkFNS();


//backend functions
extern Resource* mutex_converter(char* name);
extern bool signalMutex(Resource* m);
extern bool waitMutex(Resource* m, PCB* pcb);
extern void setVariable(PCB* process, char* name, char* value);
extern char* getVariable(PCB* process, char* name);
extern void fileReader(const char* fileName, PCB* process);
extern int countInstructions(const char *filename);
extern void executeLineFromFile(const char *filename, int program_counter, PCB *process);
extern void round_Robin();
extern void mlfq();
extern void fcfs();

// Signal handlers
void on_start_button_clicked(GtkWidget *widget, gpointer data);
void on_stop_button_clicked(GtkWidget *widget, gpointer data);
void on_reset_button_clicked(GtkWidget *widget, gpointer data);
void on_step_button_clicked(GtkWidget *widget, gpointer data);
void on_add_process_clicked(GtkWidget *widget, gpointer data);
void on_algorithm_changed(GtkWidget *widget, gpointer data);
void on_quantum_changed(GtkWidget *widget, gpointer data);
void on_file_set(GtkWidget *widget, gpointer data);

int main(int argc, char *argv[]) {
    filecount = 0;
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Initialize resources
    initialize_resources();
    
    // Initialize UI
    initialize_ui();
    
    // Start the GTK main loop
    gtk_main();
    
    return 0;
}

void executeInstructionUI(char* line, PCB* process) {
    char command[20], arg1[20], arg2[20];
    int parsed = sscanf(line, "%s %s %s", command, arg1, arg2);
    if (parsed < 2) {
        append_log("Invalid command format");
        return;
    }

    if (strcmp(command, "assign") == 0) {
        if (strcmp(arg2, "input") == 0) {
            // Create input dialog
            GtkWidget *dialog = gtk_dialog_new_with_buttons("User Input Required",
                                                          GTK_WINDOW(window),
                                                          GTK_DIALOG_MODAL,
                                                          "OK",
                                                          GTK_RESPONSE_ACCEPT,
                                                          "Cancel",
                                                          GTK_RESPONSE_CANCEL,
                                                          NULL);

            GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
            GtkWidget *entry = gtk_entry_new();
            GtkWidget *label = gtk_label_new("Please enter a value:");

            gtk_container_add(GTK_CONTAINER(content_area), label);
            gtk_container_add(GTK_CONTAINER(content_area), entry);
            gtk_widget_show_all(dialog);

            gint result = gtk_dialog_run(GTK_DIALOG(dialog));
            if (result == GTK_RESPONSE_ACCEPT) {
                const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry));
                setVariable(process, arg1, (char*)value);
                char log_msg[128];
                sprintf(log_msg, "Process %d: Assigned user input '%s' to variable %s", 
                        process->process_id, value, arg1);
                append_log(log_msg);
            }
            gtk_widget_destroy(dialog);
            
        } else if (strcmp(arg2, "readFile") == 0) {
            char nextArg[20];
            sscanf(line, "%*s %*s %*s %s", nextArg);
            char* fileName = getVariable(process, nextArg);
            if (fileName == NULL) {
                append_log("Variable not found in memory");
                return;
            }
            
            FILE* file = fopen(fileName, "r");
            if (file == NULL) {
                append_log("Error opening file");
            } else {
                char content[10000] = {0};
            size_t totalRead = 0, bytesRead;

            // Read repeatedly until EOF or buffer is full
            while ((bytesRead = fread(content + totalRead,
                                    1,
                                    sizeof(content) - 1 - totalRead,
                                    file)) > 0) {
                totalRead += bytesRead;
                if (totalRead >= sizeof(content) - 1) {
                    // buffer full, stop reading
                    break;
                }
            }
            content[totalRead] = '\0';  // ensure null-terminated
            fclose(file);
                
                // Store file content in memory
                setVariable(process, arg1, content);
                
                char log_msg[128];
                sprintf(log_msg, "Process %d: Read from file '%s' into variable %s", 
                        process->process_id, fileName, arg1);
                append_log(log_msg);
            }
        } else {
            setVariable(process, arg1, arg2);
            char log_msg[128];
            sprintf(log_msg, "Process %d: Assigned value '%s' to variable %s", 
                    process->process_id, arg2, arg1);
            append_log(log_msg);
        }
    }
    else if (strcmp(command, "printFromTo") == 0) {
        char output[1024] = "";
        char* start_str = getVariable(process, arg1);
        char* end_str = getVariable(process, arg2);
        
        if (!start_str || !end_str) {
            append_log("Error: Variables not found in memory");
            return;
        }
        
        int start = atoi(start_str);
        int end = atoi(end_str);
        
        for (int i = start; i <= end; i++) {
            char num[16];
            sprintf(num, "%d\n", i);
            strcat(output, num);
        }
        
        char log_msg[128];
        sprintf(log_msg, "Process %d: Printing range %d to %d", process->process_id, start, end);
        append_log(log_msg);
        
        // Show output in dialog
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Output",
                                                      GTK_WINDOW(window),
                                                      GTK_DIALOG_MODAL,
                                                      "OK",
                                                      GTK_RESPONSE_ACCEPT,
                                                      NULL);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *text_view = gtk_text_view_new();
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_set_text(buffer, output, -1);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

        gtk_container_add(GTK_CONTAINER(content_area), text_view);
        gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } 
    else if (strcmp(command, "print") == 0) {
        char* value = getVariable(process, arg1);
        if (value != NULL) {
            char log_msg[256];
            sprintf(log_msg, "Process %d output: %s", process->process_id, value);
            append_log(log_msg);
            // Show value in a dialog window
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Output",
            GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            "OK",
            GTK_RESPONSE_ACCEPT,
            NULL);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *text_view = gtk_text_view_new();
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_set_text(buffer, value, -1);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

        gtk_container_add(GTK_CONTAINER(content_area), text_view);
        gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        } else {
            char log_msg[256];
            sprintf(log_msg, "Process %d: Error - Variable %s not found in memory", 
                    process->process_id, arg1);
            append_log(log_msg);
        }
    } 
    else if (strcmp(command, "writeFile") == 0) {
        char* fileName = getVariable(process, arg1);
        char* data = getVariable(process, arg2);
        
        if (!fileName || !data) {
            append_log("Error: Variables not found in memory");
            return;
        }
        
        FILE* f = fopen(fileName, "w");
        if (f) {
            fprintf(f, "%s", data);
            fclose(f);
            char log_msg[128];
            sprintf(log_msg, "Process %d: Wrote data to file %s", process->process_id, fileName);
            append_log(log_msg);
        } else {
            char log_msg[128];
            sprintf(log_msg, "Process %d: Error writing to file %s", process->process_id, fileName);
            append_log(log_msg);
        }
    } 
    else if (strcmp(command, "readFile") == 0) {
        char* fileName = getVariable(process, arg1);
        if (!fileName) {
            append_log("Error: File name variable not found in memory");
            return;
        }
        
        FILE* f = fopen(fileName, "r");
        if (f) {
            char content[1000] = {0};
            fgets(content, sizeof(content), f);
            fclose(f);
            
            char log_msg[1128];
            sprintf(log_msg, "Process %d read from file %s: %s", 
                    process->process_id, fileName, content);
            append_log(log_msg);
        } else {
            char log_msg[128];
            sprintf(log_msg, "Process %d: Error reading file %s", process->process_id, fileName);
            append_log(log_msg);
        }
    } 
    else if (strcmp(command, "semWait") == 0) {
        char log_msg[128];
        sprintf(log_msg, "Process %d: Waiting for mutex %s", process->process_id, arg1);
        append_log(log_msg);
        waitMutex(mutex_converter(arg1), process);
    } 
    else if (strcmp(command, "semSignal") == 0) {
        char log_msg[128];
        sprintf(log_msg, "Process %d: Signaling mutex %s", process->process_id, arg1);
        append_log(log_msg);
        signalMutex(mutex_converter(arg1));
    }
    
    update_ui();
}

bool checkFNS(){
    for (int i = 0;i<process_count;i++){
        if(processes[i].state!=FINISHED) return true;
    }
    return false;
}



void update_ready_queue_table() {
    // Clear the existing ready queue store
    gtk_list_store_clear(ready_queue_store);
    // Arrays to hold the preview data
    int first_queue[QUEUE_CAPACITY];
    int second_queue[QUEUE_CAPACITY];
    int third_queue[QUEUE_CAPACITY];
    int rr_queue[QUEUE_CAPACITY];
    
    // Get the queue contents (without modifying them)
    previewQueue(&firstLevelQueue, first_queue);
    previewQueue(&secondLevelQueue, second_queue);
    previewQueue(&thirdLevelQueue, third_queue);
    previewQueue(&readyQueue, rr_queue);
    
    // Add processes from first level queue
    for (int i = 0; i < firstLevelQueue.size; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(ready_queue_store, &iter);
        
        int pid = first_queue[i];
        // Find process by PID
        for (int j = 0; j < process_count; j++) {
            if (processes[j].process_id == pid) {
                char* current_instruction = "N/A";
                if (processes[j].program_counter < processes[j].instruction_count) {
                    current_instruction = processes[j].program_instructions[processes[j].program_counter];
                }
                
                gtk_list_store_set(ready_queue_store, &iter,
                    0, pid,  // PID
                    1, current_instruction,  // Current instruction
                    2, "First Level Queue",  // Queue name
                    -1);
                break;
            }
        }
    }
    
    // Add processes from second level queue
    for (int i = 0; i < secondLevelQueue.size; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(ready_queue_store, &iter);
        
        int pid = second_queue[i];
        // Find process by PID
        for (int j = 0; j < process_count; j++) {
            if (processes[j].process_id == pid) {
                char* current_instruction = "N/A";
                if (processes[j].program_counter < processes[j].instruction_count) {
                    current_instruction = processes[j].program_instructions[processes[j].program_counter];
                }
                
                gtk_list_store_set(ready_queue_store, &iter,
                    0, pid,  // PID
                    1, current_instruction,  // Current instruction
                    2, "Second Level Queue",  // Queue name
                    -1);
                break;
            }
        }
    }
    
    // Add processes from third level queue
    for (int i = 0; i < thirdLevelQueue.size; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(ready_queue_store, &iter);
        
        int pid = third_queue[i];
        // Find process by PID
        for (int j = 0; j < process_count; j++) {
            if (processes[j].process_id == pid) {
                char* current_instruction = "N/A";
                if (processes[j].program_counter < processes[j].instruction_count) {
                    current_instruction = processes[j].program_instructions[processes[j].program_counter];
                }
                
                gtk_list_store_set(ready_queue_store, &iter,
                    0, pid,  // PID
                    1, current_instruction,  // Current instruction
                    2, "Third Level Queue",  // Queue name
                    -1);
                break;
            }
        }
    }
    
    // Add processes from ready queue (RR queue)
    for (int i = 0; i < readyQueue.size; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(ready_queue_store, &iter);
        
        int pid = rr_queue[i];
        // Find process by PID
        for (int j = 0; j < process_count; j++) {
            if (processes[j].process_id == pid) {
                char* current_instruction = "N/A";
                if (processes[j].program_counter < processes[j].instruction_count) {
                    current_instruction = processes[j].program_instructions[processes[j].program_counter];
                }
                
                // Determine queue name based on algorithm
                const char* queue_name = (current_algorithm == MULTILEVEL_FEEDBACK) ? 
                                       "Fourth Level Queue" : "Ready Queue";
                
                gtk_list_store_set(ready_queue_store, &iter,
                    0, pid,  // PID
                    1, current_instruction,  // Current instruction
                    2, queue_name,  // Queue name
                    -1);
                break;
            }
        }
    }
}

// Initialize resources
void initialize_resources() {
    // Initialize 3 PCB instances with process_id = -1
    PCB pcb1 = {
        .process_id = -1,
        .state = READY,
        .priority = 0,
        .program_counter = 0,
        .memory_lower_bound = 0,
        .memory_upper_bound = 0,
        .program_instructions = {NULL},
        .instruction_count = 0,
        .arrival_time = 0,
        .currentMLFQueue = 0
    };

    PCB pcb2 = {
        .process_id = -1,
        .state = READY,
        .priority = 1,
        .program_counter = 0,
        .memory_lower_bound = 0,
        .memory_upper_bound = 0,
        .program_instructions = {NULL},
        .instruction_count = 0,
        .arrival_time = 0,
        .currentMLFQueue = 1
    };

    PCB pcb3 = {
        .process_id = -1,
        .state = READY,
        .priority = 2,
        .program_counter = 0,
        .memory_lower_bound = 0,
        .memory_upper_bound = 0,
        .program_instructions = {NULL},
        .instruction_count = 0,
        .arrival_time = 0,
        .currentMLFQueue = 2
    };

    PCBMinPQ* pq1 = malloc(sizeof(PCBMinPQ));
    initMinPQ(pq1);
    PCBMinPQ* pq2 = malloc(sizeof(PCBMinPQ));
    initMinPQ(pq2);
    PCBMinPQ* pq3 = malloc(sizeof(PCBMinPQ));
    initMinPQ(pq3);
    
    // Initialize userInput resource
    resources[0].name = "userInput";
    resources[0].available = 1;
    resources[0].holder_process = &pcb1;
    resources[0].waiting_count = 0;
    resources[0].blocked = pq1;
    
    // Initialize userOutput resource
    resources[1].name = "userOutput";
    resources[1].available = 1;
    resources[1].holder_process = &pcb2;
    resources[1].waiting_count = 0;
    resources[1].blocked = pq2;
    
    // Initialize file resource
    resources[2].name = "file";
    resources[2].available = 1;
    resources[2].holder_process = &pcb3;
    resources[2].waiting_count = 0;
    resources[2].blocked = pq3;
    
    // Initialize memory
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i].name = NULL;
        memory[i].value = NULL;
        memory[i].allocated = 0;
    }
}

// Initialize UI components
void initialize_ui() {
    // Create the main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OS Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create the main container
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 10);
    
    // Setup components
    setup_dashboard();
    setup_control_panel();
    setup_resource_panel();
    setup_memory_viewer();
    setup_log_console();
    setup_process_creation();
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    // Initial UI update
    update_ui();
}

// Setup dashboard section
void setup_dashboard() {
    // Create dashboard frame
    dashboard_frame = gtk_frame_new("Dashboard");
    gtk_box_pack_start(GTK_BOX(main_box), dashboard_frame, TRUE, TRUE, 0);
    
    GtkWidget *dashboard_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_box);
    
    // Create overview section
    GtkWidget *overview_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(dashboard_box), overview_box, FALSE, FALSE, 0);
    
    // Clock cycle label
    GtkWidget *clock_label = gtk_label_new("Clock Cycle:");
    gtk_box_pack_start(GTK_BOX(overview_box), clock_label, FALSE, FALSE, 5);
    
    clock_cycle_label = gtk_label_new("0");
    gtk_box_pack_start(GTK_BOX(overview_box), clock_cycle_label, FALSE, FALSE, 5);
    
    // Algorithm label
    GtkWidget *algo_text_label = gtk_label_new("Algorithm:");
    gtk_box_pack_start(GTK_BOX(overview_box), algo_text_label, FALSE, FALSE, 5);
    
    algo_label = gtk_label_new("FCFS");
    gtk_box_pack_start(GTK_BOX(overview_box), algo_label, FALSE, FALSE, 5);
    
    // Process count label
    GtkWidget *process_count_label = gtk_label_new("Total Processes:");
    gtk_box_pack_start(GTK_BOX(overview_box), process_count_label, FALSE, FALSE, 5);
    
    GtkWidget *process_num_label = gtk_label_new("0");
    gtk_box_pack_start(GTK_BOX(overview_box), process_num_label, FALSE, FALSE, 5);
    
    // Create tables box
    GtkWidget *tables_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(dashboard_box), tables_box, TRUE, TRUE, 0);
    
    // Process list view
    GtkWidget *process_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(tables_box), process_list_box, TRUE, TRUE, 0);
    
    GtkWidget *process_list_label = gtk_label_new("All Processes");
    gtk_box_pack_start(GTK_BOX(process_list_box), process_list_label, FALSE, FALSE, 0);
    
    process_list_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    
    process_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(process_list_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("PC", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Lower Bound", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Upper Bound", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(process_list_view), column);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), process_list_view);
    gtk_box_pack_start(GTK_BOX(process_list_box), scrolled_window, TRUE, TRUE, 0);
    
    // Queue section
    GtkWidget *queue_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(tables_box), queue_box, TRUE, TRUE, 0);
    
        
    // Ready queue
    GtkWidget *ready_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(queue_box), ready_box, TRUE, TRUE, 0);
    
    GtkWidget *ready_label = gtk_label_new("Ready Queue");
    gtk_box_pack_start(GTK_BOX(ready_box), ready_label, FALSE, FALSE, 0);
    
    ready_queue_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    
    ready_queue_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ready_queue_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Current Instruction", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Queue", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ready_queue_view), column);
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), ready_queue_view);
    gtk_box_pack_start(GTK_BOX(ready_box), scrolled_window, TRUE, TRUE, 0);
    
    // Blocked queue
    GtkWidget *blocked_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(queue_box), blocked_box, TRUE, TRUE, 0);
    
    GtkWidget *blocked_label = gtk_label_new("Blocked Queue");
    gtk_box_pack_start(GTK_BOX(blocked_box), blocked_label, FALSE, FALSE, 0);
    
    blocked_queue_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
    
    blocked_queue_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(blocked_queue_store));
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Waiting for Resource", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(blocked_queue_view), column);
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), blocked_queue_view);
    gtk_box_pack_start(GTK_BOX(blocked_box), scrolled_window, TRUE, TRUE, 0);
}

// Setup control panel section
void setup_control_panel() {
    // Create control panel frame
    control_panel_frame = gtk_frame_new("Scheduler Control Panel");
    gtk_box_pack_start(GTK_BOX(main_box), control_panel_frame, FALSE, FALSE, 0);
    
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(control_panel_frame), control_box);
    
    // Algorithm selection
    GtkWidget *algo_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(control_box), algo_box, FALSE, FALSE, 5);
    
    GtkWidget *algo_label = gtk_label_new("Select Algorithm:");
    gtk_box_pack_start(GTK_BOX(algo_box), algo_label, FALSE, FALSE, 0);
    
    algorithm_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algorithm_combo), "Multilevel Feedback Queue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algorithm_combo), 0);
    g_signal_connect(algorithm_combo, "changed", G_CALLBACK(on_algorithm_changed), NULL);
    gtk_box_pack_start(GTK_BOX(algo_box), algorithm_combo, FALSE, FALSE, 0);
    
    // Quantum adjustment
    GtkWidget *quantum_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(control_box), quantum_box, FALSE, FALSE, 5);
    
    GtkWidget *quantum_label = gtk_label_new("Quantum:");
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_label, FALSE, FALSE, 0);
    
    quantum_spin = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(quantum_spin), quantum);
    g_signal_connect(quantum_spin, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_spin, FALSE, FALSE, 0);
    
    // Control buttons
    GtkWidget *buttons_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(control_box), buttons_box, TRUE, TRUE, 5);
    
    start_button = gtk_button_new_with_label("Start");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_box), start_button, TRUE, TRUE, 0);
    
    stop_button = gtk_button_new_with_label("Stop");
    gtk_widget_set_sensitive(stop_button, FALSE);
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_box), stop_button, TRUE, TRUE, 0);
    
    reset_button = gtk_button_new_with_label("Reset");
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_box), reset_button, TRUE, TRUE, 0);
    
    step_button = gtk_button_new_with_label("Step");
    g_signal_connect(step_button, "clicked", G_CALLBACK(on_step_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_box), step_button, TRUE, TRUE, 0);
}



void setup_resource_panel() {
    // Create resource panel frame
    resource_panel_frame = gtk_frame_new("Resource Management Panel");
    gtk_box_pack_start(GTK_BOX(main_box), resource_panel_frame, FALSE, FALSE, 0);
    
    GtkWidget *resource_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(resource_panel_frame), resource_box);
    
    // Mutex status
    GtkWidget *mutex_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(resource_box), mutex_box, FALSE, FALSE, 5);
    
    GtkWidget *mutex_label = gtk_label_new("Mutex Status:");
    gtk_box_pack_start(GTK_BOX(mutex_box), mutex_label, FALSE, FALSE, 5);
    
    // User input status
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(mutex_box), input_box, TRUE, TRUE, 0);
    
    GtkWidget *input_label = gtk_label_new("userInput:");
    gtk_box_pack_start(GTK_BOX(input_box), input_label, FALSE, FALSE, 0);
    
    user_input_status = gtk_label_new("Available");
    gtk_box_pack_start(GTK_BOX(input_box), user_input_status, FALSE, FALSE, 0);
    
    // User output status
    GtkWidget *output_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(mutex_box), output_box, TRUE, TRUE, 0);
    
    GtkWidget *output_label = gtk_label_new("userOutput:");
    gtk_box_pack_start(GTK_BOX(output_box), output_label, FALSE, FALSE, 0);
    
    user_output_status = gtk_label_new("Available");
    gtk_box_pack_start(GTK_BOX(output_box), user_output_status, FALSE, FALSE, 0);
    
    // File status
    GtkWidget *file_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(mutex_box), file_box, TRUE, TRUE, 0);
    
    GtkWidget *file_label = gtk_label_new("file:");
    gtk_box_pack_start(GTK_BOX(file_box), file_label, FALSE, FALSE, 0);
    
    file_status = gtk_label_new("Available");
    gtk_box_pack_start(GTK_BOX(file_box), file_status, FALSE, FALSE, 0);
}

// Setup memory viewer section
void setup_memory_viewer() {
    // Create memory frame
    memory_frame = gtk_frame_new("Memory Viewer");
    gtk_box_pack_start(GTK_BOX(main_box), memory_frame, TRUE, TRUE, 0);
    
    GtkWidget *memory_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(memory_frame), memory_box);
    
    // Memory view
    memory_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    
    memory_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(memory_store));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Address", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_view), column);
    
    column = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(memory_view), column);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), memory_view);
    gtk_box_pack_start(GTK_BOX(memory_box), scrolled_window, TRUE, TRUE, 0);
}

// Setup log console section
void setup_log_console() {
    // Create log frame
    log_frame = gtk_frame_new("Log & Console Panel");
    gtk_box_pack_start(GTK_BOX(main_box), log_frame, TRUE, TRUE, 0);
    
    GtkWidget *log_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(log_frame), log_box);
    
    // Log view
    log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD);
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), log_view);
    gtk_box_pack_start(GTK_BOX(log_box), scrolled_window, TRUE, TRUE, 0);
}

// Setup process creation section
void setup_process_creation() {
    // Create process creation frame
    process_creation_frame = gtk_frame_new("Process Creation");
    gtk_box_pack_start(GTK_BOX(main_box), process_creation_frame, FALSE, FALSE, 0);
    
    GtkWidget *process_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(process_creation_frame), process_box);
    
    // File chooser
    GtkWidget *file_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(process_box), file_box, TRUE, TRUE, 5);
    
    GtkWidget *file_label = gtk_label_new("Program File:");
    gtk_box_pack_start(GTK_BOX(file_box), file_label, FALSE, FALSE, 0);
    
    file_chooser_button = gtk_file_chooser_button_new("Select a File", GTK_FILE_CHOOSER_ACTION_OPEN);
    g_signal_connect(file_chooser_button, "file-set", G_CALLBACK(on_file_set), NULL);
    gtk_box_pack_start(GTK_BOX(file_box), file_chooser_button, FALSE, FALSE, 0);
    
    // Arrival time
    GtkWidget *arrival_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(process_box), arrival_box, FALSE, FALSE, 5);
    
    GtkWidget *arrival_label = gtk_label_new("Arrival Time:");
    gtk_box_pack_start(GTK_BOX(arrival_box), arrival_label, FALSE, FALSE, 0);
    
    arrival_time_spin = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_box_pack_start(GTK_BOX(arrival_box), arrival_time_spin, FALSE, FALSE, 0);

    // Priority selection - Add this section
    GtkWidget *priority_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(process_box), priority_box, FALSE, FALSE, 5);
    
    GtkWidget *priority_label = gtk_label_new("Priority:");
    gtk_box_pack_start(GTK_BOX(priority_box), priority_label, FALSE, FALSE, 0);
    
    priority_spin = gtk_spin_button_new_with_range(0, 10, 1);
    gtk_box_pack_start(GTK_BOX(priority_box), priority_spin, FALSE, FALSE, 0);
    
    // Add process button
    GtkWidget *add_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(process_box), add_box, FALSE, FALSE, 5);
    
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(add_box), spacer, FALSE, FALSE, 0);
    
    add_process_button = gtk_button_new_with_label("Add Process");
    g_signal_connect(add_process_button, "clicked", G_CALLBACK(on_add_process_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(add_box), add_process_button, FALSE, FALSE, 0);
}

void update_blocked_queue_table(){
    blocked_queue_count = 0;
    // Clear the existing ready queue store
    
    gtk_list_store_clear(blocked_queue_store);
    
    // For each resource, get processes from its blocked queue
    for (int i = 0; i < 3; i++) {
        PCBMinPQ temp = *(resources[i].blocked); // Make a copy to preserve original -> could not be the issue based on chatgpt
        PCB blocked_pcb;
        
        // Extract processes from the MinPQ
        while (!isMinPQEmpty(&temp)) {
            minPQPop(&temp, &blocked_pcb);
            // Find process index by PID and add to blocked queue
            for (int j = 0; j < process_count; j++) {
                if (processes[j].process_id == blocked_pcb.process_id) {
                    blocked_queue[blocked_queue_count++] = j;
                    blocked_locations[blocked_queue_count-1] = resources[i].name;
                    blocked_priority[blocked_queue_count-1] = processes[j].priority;
                    break;
                }
            }
        } 
    }
    
    for(int i = 0;i<blocked_queue_count;i++){
        GtkTreeIter iter;
        gtk_list_store_append(blocked_queue_store, &iter);
        gtk_list_store_set(
            blocked_queue_store,&iter,
            0, blocked_queue[i]+1,
            1, blocked_locations[i],
            2, blocked_priority[i],
            -1
        );
    }
}

// Update the UI with current data
void update_ui() {
    // Update clock cycle label
    char clock_str[32];
    sprintf(clock_str, "%d", clock_cycle);
    gtk_label_set_text(GTK_LABEL(clock_cycle_label), clock_str);
    
    // Update algorithm label
    switch(current_algorithm) {
        case FCFS:
            gtk_label_set_text(GTK_LABEL(algo_label), "FCFS");
            break;
        case ROUND_ROBIN:
            gtk_label_set_text(GTK_LABEL(algo_label), "Round Robin");
            break;
        case MULTILEVEL_FEEDBACK:
            gtk_label_set_text(GTK_LABEL(algo_label), "Multilevel Feedback Queue");
            break;
    }
    
    // Update process list
    gtk_list_store_clear(process_list_store);
    for (int i = 0; i < process_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(process_list_store, &iter);
        
        char state_str[10];
        switch (processes[i].state) {
            case READY:
                strcpy(state_str, "Ready");
                break;
            case RUNNING:
                strcpy(state_str, "Running");
                break;
            case BLOCKED:
                strcpy(state_str, "Blocked");
                break;
            case FINISHED:
                strcpy(state_str, "Finished");
                break;
        }
        
        gtk_list_store_set(
            process_list_store, &iter,
            0, processes[i].process_id,
            1, state_str,
            2, processes[i].priority,
            3, processes[i].program_counter,
            4, processes[i].memory_lower_bound,
            5, processes[i].memory_upper_bound,
            -1
        );
    }
    
    // Update running process label
    
    for (int i = 0; i < 3; i++) {
        GtkWidget* status_label;
        char status_str[32];
        
        switch (i) {
            case 0: // userInput
                status_label = user_input_status;
                break;
            case 1: // userOutput
                status_label = user_output_status;
                break;
            case 2: // file
                status_label = file_status;
                break;
        }
        
        if (resources[i].available) {
            strcpy(status_str, "Available");
        } else {
            sprintf(status_str, "Held by P%d", resources[i].holder_process->process_id);
        }
        
        gtk_label_set_text(GTK_LABEL(status_label), status_str);
    }
    
    // Update memory view
    gtk_list_store_clear(memory_store);
    for (int i = 0; i < MEMORY_SIZE; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(memory_store, &iter);
        
        char* name = memory[i].allocated ? memory[i].name : "Free";
        char* value = memory[i].allocated ? memory[i].value : "-";
        
        gtk_list_store_set(
            memory_store, &iter,
            0, i,
            1, name,
            2, value,
            -1
        );
    }
    update_ready_queue_table();
    update_blocked_queue_table();
}

// Append message to log
void append_log(const char* message) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(log_buffer, &iter);
    
    // Add timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", t);
    
    // Append log with timestamp
    gtk_text_buffer_insert(log_buffer, &iter, timestamp, -1);
    gtk_text_buffer_insert(log_buffer, &iter, message, -1);
    gtk_text_buffer_insert(log_buffer, &iter, "\n", -1);
    
    // Scroll to the end
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(log_view), &iter, 0.0, FALSE, 0.0, 0.0);
}

// Updated start_simulation() to schedule simulation_tick periodically (e.g., every 1000ms)
void start_simulation() {
    mode = 1;
    if (!simulation_running) {
        simulation_running = TRUE;
        append_log("Simulation started");
    }
    gtk_widget_set_sensitive(start_button, FALSE);
    gtk_widget_set_sensitive(stop_button, TRUE);
    while(simulation_running&&checkFNS()){
        step_simulation();        
    }
    gtk_widget_set_sensitive(start_button, TRUE);
    gtk_widget_set_sensitive(stop_button, FALSE);
}

// Updated stop_simulation() to cancel~ the simulation timer
void stop_simulation() {
    if (simulation_running) {
        simulation_running = FALSE;
        append_log("Simulation stopped");
    }
    gtk_widget_set_sensitive(start_button, TRUE);
    gtk_widget_set_sensitive(stop_button, FALSE);
}

// Reset simulation
void reset_simulation() {
    // Stop simulation if running
    if (simulation_running) {
        stop_simulation();
    }
    
    // Reset clock
    clock_cycle = 0;
    
    // Clear processes
    process_count = 0;
    running_process_index = -1;
    
    // Reset all queues
    initQueue(&firstLevelQueue);
    initQueue(&secondLevelQueue);
    initQueue(&thirdLevelQueue);
    initQueue(&readyQueue);
    
    // Reset resources
    initialize_resources();
    
    // Reset memory
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i].allocated) {
            free(memory[i].name);
            free(memory[i].value);
        }
        memory[i].name = NULL;
        memory[i].value = NULL;
        memory[i].allocated = 0;
    }
    idleCount = 0;
    for(int i = 0;i<filecount;i++){
        memset(file_names[i], 0, 244);
    }
    filecount = 0;
    // Update UI
    update_ui();
    
    // Clear log
    gtk_text_buffer_set_text(log_buffer, "", -1);
    
    append_log("Simulation reset - All data structures cleared");
}

// Step simulation by one clock cycle
void step_simulation() {
    //condition to end start -> nothing to run
   // simulation_running = false;
   // return;

    step = true;
    mode = 2;
    // Increment clock cycle
   // clock_cycle++;
    
    // TODO: Implement actual scheduling logic
    switch (current_algorithm)
    {
    case ROUND_ROBIN:
        roundRobin();
        break;
    case FCFS:
        fcfs();
        break;
    case MULTILEVEL_FEEDBACK:
        mlfq();
        break;
    default:
        append_log("No such scheduling algorithm ya 3am");
        break;
        return;
    }
    update_ui();
    if (current_algorithm!= MULTILEVEL_FEEDBACK) 
    {
        char log_message[64];
    sprintf(log_message, "Clock cycle %d completed", clock_cycle-1);
    append_log(log_message);
    }
    
    
}

// Updated step button handler to run one simulation tick when not running continuously
void on_step_button_clicked(GtkWidget *widget, gpointer data) {
    step_simulation();
}

// Add a new process
void add_process() {
    if (process_count >= MAX_PROCESSES) {
        append_log("Error: Maximum number of processes reached");
        return;
    }
    
    // Get file path
    char* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser_button));
    if (!file_path) {
        append_log("Error: No file selected");
        return;
    }
    
    // Create a copy of the file in our directory
    const char* filename = strrchr(file_path, '/') ? strrchr(file_path, '/') + 1 : file_path;
    char new_path[256];
    snprintf(new_path, sizeof(new_path), "/home/seif/UbTrial/%s", filename);
    
    // Copy the file
    FILE* source = fopen(file_path, "r");
    FILE* dest = fopen(new_path, "w");
    if (!source || !dest) {
        append_log("Error: Could not copy program file to project directory");
        if (source) fclose(source);
        if (dest) fclose(dest);
        g_free(file_path);
        return;
    }
    
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    fclose(source);
    fclose(dest);
    strncpy(file_names[filecount++], filename, 255); // Change to use strncpy
    file_names[filecount-1][255] = '\0'; // Ensure null termination
    char filename_log[256];
    sprintf(filename_log, "filename is %s", file_names[filecount-1]);
    append_log(filename_log);
    
    // Update file_path to use the new location
    g_free(file_path);
    file_path = strdup(new_path);
    
    // Get arrival time
    int arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_time_spin));
    
    // Get priority - Add this line
    int priority = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(priority_spin));
    
    // Create new process with dummy data for demonstration
    PCB new_process;
    new_process.process_id = process_count + 1;
    new_process.state = READY;
    new_process.priority = priority; // Use selected priority instead of default
    new_process.program_counter = 0;
    new_process.memory_lower_bound = process_count * 20;
    new_process.memory_upper_bound = (process_count + 1) * 20 - 1;
    new_process.arrival_time = arrival_time;

    // Read instructions from file
    FILE* file = fopen(file_path, "r");
    char line[256];
    int idx = 0;
    while (fgets(line, sizeof(line), file) && idx < 20) {  // Add bound check
        // Remove newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        char* instruction = strdup(line);
        if (instruction == NULL) {  // Check for allocation failure
            append_log("Error: Memory allocation failed for instruction");
            fclose(file);
            g_free(file_path);
            // Clean up previously allocated instructions
            for (int i = 0; i < idx; i++) {
                free(new_process.program_instructions[i]);
            }
            return;
        }
        new_process.program_instructions[idx] = instruction;
        idx++;
    }
    // Set the actual amount of instructions read
    new_process.instruction_count = idx;

    // Set remaining instruction pointers to NULL
    for (int i = idx; i < 20; i++) {
        new_process.program_instructions[i] = NULL;
    }

    fclose(file);
    
    // Add process to list
    processes[process_count] = new_process;
    process_count++;
    
    
    // Allocate memory for process
    for (int i = new_process.memory_lower_bound; i <= new_process.memory_upper_bound; i++) {
        memory[i].allocated = 1;
        
        // Set memory values
        if (i == new_process.memory_lower_bound) {
            memory[i].name = strdup("State");
            memory[i].value = strdup("Ready");
        } else if (i == new_process.memory_lower_bound + 1) {
            memory[i].name = strdup("PC");
            memory[i].value = strdup("0");
        } else if (i == new_process.memory_lower_bound + 2) {
            memory[i].name = strdup("Priority");
            char priority_str[16];
            sprintf(priority_str, "%d", new_process.priority);
            memory[i].value = strdup(priority_str);
        } else if (i < new_process.memory_lower_bound + 3 + new_process.instruction_count) {
            char var_name[32];
            sprintf(var_name, "Inst%d", i - new_process.memory_lower_bound - 2);
            memory[i].name = strdup(var_name);
            memory[i].value = strdup(new_process.program_instructions[i - new_process.memory_lower_bound - 3]);
        } else {
            char var_name[32];
            sprintf(var_name, "Var%d", i - new_process.memory_lower_bound - (new_process.instruction_count+2));
            memory[i].name = strdup(var_name);
            memory[i].value = strdup("NULL");
        }
    }
    
    update_ui();
    
    char log_message[128];
    sprintf(log_message, "Added process PID=%d with arrival time=%d from file %s", 
            new_process.process_id, arrival_time, 
            strrchr(file_path, '/') ? strrchr(file_path, '/') + 1 : file_path);
    append_log(log_message);
    
    g_free(file_path);
}

// Change scheduling algorithm
void change_algorithm() {
    int active = gtk_combo_box_get_active(GTK_COMBO_BOX(algorithm_combo));
    
    switch (active) {
        case 0:
            current_algorithm = FCFS;
            append_log("Scheduling algorithm changed to First Come First Serve");
            gtk_widget_set_sensitive(quantum_spin, FALSE);
            break;
        case 1:
            current_algorithm = ROUND_ROBIN;
            append_log("Scheduling algorithm changed to Round Robin");
            gtk_widget_set_sensitive(quantum_spin, TRUE);
            break;
        case 2:
            current_algorithm = MULTILEVEL_FEEDBACK;
            append_log("Scheduling algorithm changed to Multilevel Feedback Queue");
            gtk_widget_set_sensitive(quantum_spin, FALSE);
            break;
    }
    //reset_simulation();
    update_ui();
}

// Signal handler for start button
void on_start_button_clicked(GtkWidget *widget, gpointer data) {
    start_simulation();
}


// Signal handler for stop button
void on_stop_button_clicked(GtkWidget *widget, gpointer data) {
    stop_simulation();
}

// Signal handler for reset button
void on_reset_button_clicked(GtkWidget *widget, gpointer data) {
    reset_simulation();
}

// Signal handler for add process button
void on_add_process_clicked(GtkWidget *widget, gpointer data) {
    add_process();
}

// Signal handler for algorithm changed
void on_algorithm_changed(GtkWidget *widget, gpointer data) {
    change_algorithm();
}

// Signal handler for quantum changed
void on_quantum_changed(GtkWidget *widget, gpointer data) {
    quantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(quantum_spin));
    
    char log_message[32];
    sprintf(log_message, "Quantum changed to %d", quantum);
    append_log(log_message);
}

// Signal handler for file chooser
void on_file_set(GtkWidget *widget, gpointer data) {
    char* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    if (file_path) {
        char log_message[128];
        sprintf(log_message, "Selected file: %s", 
                strrchr(file_path, '/') ? strrchr(file_path, '/') + 1 : file_path);
        append_log(log_message);
        g_free(file_path);
    }
}

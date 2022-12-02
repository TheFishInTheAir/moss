#pragma once
#include <stdint.h>
#include <spinlock.h>

// Not sure how to handle process storage, dynamic allocation will likely not be worth it. 
#define MAX_PROCS 16
#define NUM_CORES 2
#define PROC_ID_MAX 256

typedef enum 
{
    MOSS_PROCESS_UNSTARTED,
    MOSS_PROCESS_READY,
    MOSS_PROCESS_WAITING,
    MOSS_PROCESS_RUNNING,
    MOSS_PROCESS_STOPPED,
} moss_process_state;

struct _moss_process;
struct _moss_process_node;
struct _moss_scheduler_context;


void moss_init_tick_divisor();

typedef struct _moss_process
{
    // In convention with FreeRTOS top_of_stack pointer is first in process struct so it has
    // a zero offset. 
    volatile uint8_t* top_of_stack;

    // Flag for context save completion TODO: phase this out
    volatile int32_t primed;


    uint8_t* stack;
    char identifier[PROC_ID_MAX];
    uint8_t pid;

    void* user_ptr;    

    void* instruction_ptr;

    void(*entry_point)();

    moss_process_state state;    
    
} moss_process;

extern volatile moss_process* moss_active_process[NUM_CORES];
extern volatile uint8_t* moss_idle_stack[NUM_CORES];

int moss_create_proc(struct _moss_scheduler_context* ctx, moss_process** proc, char* identifier, void(*entry_point)(), void* user_ptr);


// Implement around a ring buf (maybe switch this up later... or just make it a list
//  based queue)
// TODO: convert this into a multi-level priority queue eventually
// TODO: Refactor Naming
typedef struct _moss_process_exec_queue
{
    struct _moss_process* processes[MAX_PROCS];
    volatile uint32_t ring_size;
    volatile uint32_t num_elems;
    volatile uint32_t start_off;

    spinlock_t lock;

} moss_process_exec_queue;

int moss_process_exec_queue_init(moss_process_exec_queue* queue);
int moss_process_exec_queue_pop(moss_process_exec_queue* queue, moss_process** proc);
int moss_process_exec_queue_push(moss_process_exec_queue* queue, moss_process* proc);

typedef struct _moss_scheduler_context
{
    // Global Process List (non-hierarchical)
    struct _moss_process* procs[MAX_PROCS];
    volatile uint32_t num_procs;
    spinlock_t global_sched_lock;


    // Execution Queue (for now)
    moss_process_exec_queue queue;
    uint8_t init_flag;
} moss_scheduler_context;

moss_scheduler_context* moss_sched();
moss_process* moss_scheduler_find_proc_id(moss_scheduler_context* ctx, char* id);
moss_process* moss_scheduler_find_proc_pid(moss_scheduler_context* ctx, uint8_t pid);
int moss_scheduler_start_proc(moss_scheduler_context* ctx, moss_process* proc);
void moss_process_exec_queue_debug_dump(moss_process_exec_queue* queue);
int moss_scheduler_start(moss_scheduler_context* ctx);
int moss_scheduler_init();

// General Functions
int moss_instantiate_proc(moss_scheduler_context* ctx, moss_process** proc,
                          char* identifier, void(*entry_point)(), void* user_ptr);

int moss_delete_process(moss_scheduler_context* ctx, moss_process* proc);
void moss_terminate();


uint64_t moss_get_context_switches();
void moss_prime_context_switch();
void moss_yield();
void moss_yield_without_queue();
moss_process* moss_current_process();
void* moss_current_process_user_ptr();

void moss_dump_current_process_debug();

void _moss_dispatch();
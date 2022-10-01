#pragma once
#include <stdint.h>

// Two kernel level PIDs
// Six User Level PIDs (hopefully enough)
#define MAX_PROCS 8

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


// moss Process Definition
typedef struct _moss_process
{
    char* identifier;
    uint8_t pid;

    

    void* instruction_ptr;

    void(*entry_point)();

    moss_process_state state;

    // Registers

    
    // Ptr to Top of Stack
    // Stack information

    // Instruction Address
    // Process Execution state

    
    
} moss_process;

int moss_create_proc(struct _moss_scheduler_context* ctx, moss_process** proc, char* identifier, void(*entry_point)());


// Implement around a ring buf (maybe switch this up later... or just make it a list
//  based queue)
// Likely more efficient to make this coalesced
typedef struct _moss_process_exec_queue
{
    // TODO: Remove *2
    struct _moss_process* processes[MAX_PROCS*2];
    uint32_t ring_size;
    uint32_t num_elems;
    uint32_t start_off;

} moss_process_exec_queue;

int moss_process_exec_queue_init(moss_process_exec_queue* queue);
int moss_process_exec_queue_pop(moss_process_exec_queue* queue, moss_process** proc);
int moss_process_exec_queue_push(moss_process_exec_queue* queue, moss_process* proc);

typedef struct _moss_scheduler_context
{
    // Global Process List (non-hierarchical)
    struct _moss_process* procs[MAX_PROCS];
    uint32_t num_procs;

    // Execution Queue (for now)
    moss_process_exec_queue queue;
    uint8_t init_flag;
    struct moss_process* active_proc;
} moss_scheduler_context;

moss_scheduler_context* moss_sched();
moss_process* moss_scheduler_find_proc_id(moss_scheduler_context* ctx, char* id);
moss_process* moss_scheduler_find_proc_pid(moss_scheduler_context* ctx, uint8_t pid);
int moss_scheduler_start_proc(moss_scheduler_context* ctx, moss_process* proc);
int moss_scheduler_start(moss_scheduler_context* ctx);
int moss_scheduler_init();

int moss_instantiate_proc(moss_scheduler_context* ctx, moss_process** proc,
                          char* identifier, void(*entry_point)());

#include <scheduler.h>
#include <moss.h>

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include <esp_log.h>

#include <stdio.h>

#include <xtensa/config/core.h>
#include <xtensa/xtensa_context.h>

#include "esp_heap_caps.h"


#define STACK_DEPTH 1024*10
#define IDLE_STACK_DEPTH 1024

#define TAG "moss_scheduler"

moss_scheduler_context sched_ctx;
volatile uint8_t* moss_idle_stack[NUM_CORES] = {NULL};

volatile moss_process* moss_active_process[NUM_CORES] = {NULL};
volatile moss_process* moss_process_backbuffer[NUM_CORES] = {NULL};

int _moss_scheduler_register_proc(moss_scheduler_context* ctx, moss_process* proc);
int _moss_scheduler_preempt(moss_scheduler_context* ctx);

void _moss_prime_context_switch();
void moss_dump_current_process_debug();

/***********
 * PROCESS *
 ***********/

// Create a new process and add add it to execution queue
// Proc ref can be null
int moss_instantiate_proc(moss_scheduler_context* ctx, moss_process** proc_ref,
                          char* identifier, void(*entry_point)(), void* user_ptr)
{                 
    moss_process* proc;
    PROP(moss_create_proc(ctx, &proc, identifier, entry_point, user_ptr));

    PROP(moss_process_exec_queue_push(&ctx->queue, proc));

    if(proc_ref != NULL)
        *proc_ref = proc;

    return MOSS_SUCCESS;
}

void _test_exit()
{
    assert(0 && "Something Really Bad Happened :)");
}

// Interrupt Exit Handler
extern void _xt_user_exit();

// TODO: Cleanup
uint8_t* _moss_init_stack(moss_process* proc)
{
    // Heavy Inspo from the freertos port code but simplified

    uint8_t *sp, *tos;
    tos = (uint8_t*) ((int)proc->top_of_stack & ~0xf);
    sp  = (uint8_t*) (((moss_address)tos-XT_STK_FRMSZ)& ~0xf);

    // Set Stack Frame to recognizable pattern
    for(uint8_t* tp = sp; tp <= tos; tp++)
    {
        *tp = 0xBF;
    }

    // Possible that there are some allignment issues which are leading to the 
    // Instruction failing

    XtExcFrame* frame = (XtExcFrame*) sp;
    frame->pc   = (moss_address) proc->entry_point;
    frame->a0   = 0;
    frame->a1   = (moss_address) tos;
    frame->a10  = (moss_address) proc->entry_point; //testing purposes

    // This is the final call that loads PS PC and A0 
    frame->exit = (moss_address) _xt_user_exit; 

    // TODO: figure out which flags to use.
    //frame->ps = PS_UM | PS_EXCM;
    frame->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1);

    //@Cleanup
    //_moss_log("Stack Ptr addr %#08X\n", sp);
    //_moss_log("Current Stack Ptr addr %#08X\n", moss_sp());

    return sp;
}

int moss_create_proc(moss_scheduler_context* ctx, moss_process** proc_ref, 
                               char* identifier, void(*entry_point)(), void* user_ptr)
{
    moss_process* proc;
    proc = (moss_process*) malloc(sizeof(moss_process));

    if(proc_ref!=NULL)
    {
        (*proc_ref) = proc;
    }
    
    strcpy(proc->identifier, identifier);
    proc->entry_point = (void*)entry_point;
    proc->instruction_ptr = NULL;
    proc->state = MOSS_PROCESS_UNSTARTED;
    proc->user_ptr = user_ptr;

    // So arbitrary its incredible TODO: replace stack allocation
    proc->stack = (uint8_t*) malloc(STACK_DEPTH);

    proc->top_of_stack = proc->stack+(STACK_DEPTH-1);


    // Initialise stack frame.
    proc->top_of_stack = _moss_init_stack(proc);
    proc->primed = 1;

    PROP(_moss_scheduler_register_proc(ctx, proc));


    return MOSS_SUCCESS;
}

int moss_delete_process(moss_scheduler_context* ctx, moss_process* proc)
{
    spinlock_acquire(&ctx->global_sched_lock, SPINLOCK_WAIT_FOREVER);

    assert(proc->state==MOSS_PROCESS_STOPPED);
    if(proc->state!=MOSS_PROCESS_STOPPED)
        return MOSS_FAIL;


    //@Cleanup
    _moss_log("Freeing proc Stack of PID: %d ... total procs left:  %d \n", proc->pid, ctx->num_procs);
    //for(int i = 0; i < ctx->num_procs; i++)
        //_moss_log(" -- procs: ID '%s' PID %d State %d \n", ctx->procs[i]->identifier, ctx->procs[i]->pid, ctx->procs[i]->state);


    char found = 0;

    // problematic
    for(int i = 0; i < ctx->num_procs; i++)
    {
        if(found)
        {
            ctx->procs[i-1] = ctx->procs[i];
        }

        if(ctx->procs[i]->pid==proc->pid)
            found=true;
    }

    ctx->num_procs--;

    free(proc->stack);
    free(proc);


    spinlock_release(&ctx->global_sched_lock);

    return MOSS_SUCCESS;
}

//TODO: delete process and free stack after this!
// Can't delete stack here though, should have a cleanup phase.
void moss_terminate()
{
    moss_current_process()->state = MOSS_PROCESS_STOPPED;

    //@Cleanup
    //moss_delete_process(moss_sched(), moss_current_process());
    //moss_yield_without_queue();
    moss_ctx_switch();

    // the call0 in dispatch is leading to some weird stuff
    //_moss_dispatch();


    assert(0 && "what the hell");
    //_moss_dispatch();
}


/*************
 * SCHEDULER *
 *************/


moss_scheduler_context* moss_sched()
{
    assert(sched_ctx.init_flag==MOSS_INIT_CONST);
    return &sched_ctx;
}

// NOTE: Not sure how we want to handle global things just yet.
int moss_scheduler_init()
{
    sched_ctx.num_procs = 0;
    sched_ctx.init_flag = MOSS_INIT_CONST;
    spinlock_initialize(&sched_ctx.global_sched_lock);
    PROP(moss_process_exec_queue_init(&sched_ctx.queue));

    for(int i = 0; i < NUM_CORES; i++)
    {
        int bos = (int) malloc(IDLE_STACK_DEPTH);
        int tos = bos+(IDLE_STACK_DEPTH-1);
        moss_idle_stack[i] = (uint8_t*) (tos& ~0xf);
    }
    
    return MOSS_SUCCESS;
}

//TODO: maybe just clean up for the current core because that removes the null check
//      and we can be lazy with cleaning up processes 
void _moss_scheduler_cleanup_dead_procs(moss_scheduler_context* ctx)
{   
    for(int i = 0; i < ctx->num_procs; i++)
    {
        if(ctx->procs[i]->state==MOSS_PROCESS_STOPPED)
        {
            char abort = 0;
            for(int core = 0; core < NUM_CORES; core++)
            {
                if(moss_active_process[core]==NULL)
                    continue;
                // Prevent deleting stack if the core is still moving onto next process.
                if(moss_active_process[core]->pid==ctx->procs[i]->pid)
                    abort = 1;
            }

            if(abort)
                continue;

            moss_delete_process(ctx, ctx->procs[i]);
            i = 0;
        }
    }
}

static int uuid_count = 0;
// Adds into process array and assigns PID
int _moss_scheduler_register_proc(moss_scheduler_context* ctx, moss_process* proc)
{
    if(ctx->num_procs == MAX_PROCS)
    {
        assert(0 && "Reached Max Processes.");
        return MOSS_FAIL;
    }

    _moss_scheduler_cleanup_dead_procs(ctx);

    //TODO: make a critical section
    spinlock_acquire(&ctx->global_sched_lock, SPINLOCK_WAIT_FOREVER);
    proc->pid = ++uuid_count;
    ctx->procs[ctx->num_procs++] = proc; //TODO: might need synch primitives..
    spinlock_release(&ctx->global_sched_lock);

    return MOSS_SUCCESS;
}

moss_process* moss_scheduler_find_proc_id(moss_scheduler_context* ctx, char* id)
{
    for(int i = 0; i < ctx->num_procs; i++)
    {
        if(!strcmp(ctx->procs[i]->identifier, id))
            return ctx->procs[i];
    }
    return NULL;
}

moss_process* moss_schedzuler_find_proc_pid(moss_scheduler_context* ctx, uint8_t pid)
{
    for(int i = 0; i < ctx->num_procs; i++)
    {
        if(ctx->procs[i]->pid==pid)
            return ctx->procs[i];
    }
    return NULL;
}

int moss_scheduler_start_proc(moss_scheduler_context* ctx, moss_process* proc)
{
    //TODO: Remove Logging eventually.
    //_moss_log("Starting Process %s\n", proc->identifier);
    PROP(moss_process_exec_queue_push(&ctx->queue, proc));
    return MOSS_SUCCESS;

}

/**************************
 * PRIMARY SCHEDULER LOOP *
 **************************/

int moss_scheduler_start(moss_scheduler_context* ctx)
{
    _moss_dispatch();

    // Should never reach here. TODO: make this a void func
    return MOSS_SUCCESS;
}


/**********************
 * PROCESS EXEC QUEUE *
 **********************/

int moss_process_exec_queue_init(moss_process_exec_queue* queue)
{
    queue->ring_size = MAX_PROCS;
    queue->num_elems = 0;
    queue->start_off = 0;
    
    spinlock_initialize(&queue->lock);

    return MOSS_SUCCESS;
}

moss_process** _moss_process_exec_queue_get_elem(moss_process_exec_queue* queue, uint32_t indx)
{
    return queue->processes+(indx+queue->start_off) % queue->ring_size;
}


//TODO: Refactor and make it just assume the default execution queue.
void moss_process_exec_queue_debug_dump(moss_process_exec_queue* queue)
{
    spinlock_acquire(&queue->lock, SPINLOCK_WAIT_FOREVER);
    
    _moss_log("Dumping Process Exec Queue State: \n");

    if(moss_active_process[0]!=NULL && moss_active_process[1]!=NULL)
    {
        _moss_log("  --- CPU 0: %s\n", moss_active_process[0]->identifier);
        _moss_log("  --- CPU 1: %s\n", moss_active_process[1]->identifier);
    }
    for(int i = 0; i < queue->num_elems; i++)
    {
        moss_process* proc = *_moss_process_exec_queue_get_elem(queue, i);
        //printf("  (%d)  state %d  entry (%d)  instr (%d)\n", i, proc->state, (int)proc->entry_point, (int) proc->instruction_ptr);

        //printf("Wow ok did that \n\n\n");
        _moss_log("  (%d) %s (STATE: %d, ENTRY: %#04X, INSTR: %#04X)\n", i, proc->identifier, proc->state,  (moss_address)proc->entry_point, *(int*)proc->top_of_stack);
    }
    spinlock_release(&queue->lock);
}


int moss_process_exec_queue_pop(moss_process_exec_queue* queue, moss_process** proc)
{
    spinlock_acquire(&queue->lock, SPINLOCK_WAIT_FOREVER);

    if(queue->num_elems==0)
    {
        spinlock_release(&queue->lock);
        return MOSS_FAIL;
    }
    
    *proc = *_moss_process_exec_queue_get_elem(queue, 0);
    queue->start_off += 1;
    queue->start_off %= queue->ring_size;
    queue->num_elems--;

    spinlock_release(&queue->lock);

    return MOSS_SUCCESS;
}

// A process should never be entered twice into the exec queue (I think at least!)
int moss_process_exec_queue_push(moss_process_exec_queue* queue, moss_process* proc)
{
    spinlock_acquire(&queue->lock, SPINLOCK_WAIT_FOREVER);

    // TODO: make sure this never goes off
    // TODO: should be more liberal with asserts
    assert(queue->num_elems<queue->ring_size);

    moss_process** queue_slot = _moss_process_exec_queue_get_elem(queue, queue->num_elems);
    *queue_slot = proc;
    queue->num_elems++;

    spinlock_release(&queue->lock);

    return MOSS_SUCCESS;
}

// @Refactor Name
void _moss_push_active_process()
{
    moss_current_process()->state = MOSS_PROCESS_WAITING;
    moss_process_exec_queue_push(&moss_sched()->queue, moss_current_process());
}

uint64_t switches = 0;

uint64_t moss_get_context_switches()
{
    return switches;
}

// @Refactor Name
void _moss_prime_context_switch()
{
    // This currently Functions as idle task.
    // Could be a bad idea to put that here, some of the issues that have arrisen
    // are kind of due to this being blocking.

    int idling = 0;

    while(1)
    {
        moss_process* next_proc;
        if(moss_process_exec_queue_pop(&moss_sched()->queue, &next_proc))
        {
            if(idling)
                _moss_log("- Exiting Idle State on Core %d\n", moss_core_id());

            while(next_proc->primed==0)
            {_moss_nop();}
            
            next_proc->state = MOSS_PROCESS_RUNNING;

            //if(moss_current_process()!=NULL)
                //moss_dump_current_process_debug();

            // After this gets switched cant guarantee access to a stack.
            moss_active_process[moss_core_id()] = next_proc;

            /*

            moss_active_process[moss_core_id()]->primed=0;

            if(moss_active_process[moss_core_id()]->pid == 1)
            {
                _moss_log("Going to main thread\n");

            }*/
            //moss_dump_current_process_debug();
            //moss_process_exec_queue_debug_dump(&moss_sched()->queue);

            return;
        }

        //TODO: only switch stack pointer for this

        if(!idling)
            _moss_log("- No More Processes, Idling on Core %d\n", moss_core_id());

        idling = 1;
        _moss_log("- Continuous Idling on Core %d\n", moss_core_id());

        for(unsigned int i = 0; i < 100000; i++)
        {_moss_nop();}

    }
}


//TODO: @Refactor
void moss_yield_without_queue()
{
    switches++;
 
    moss_current_process()->state = MOSS_PROCESS_WAITING;
    
    moss_ctx_switch();

    // Janky spot for this tbh. It does work for now though.
}

void _moss_prime_proc()
{
    moss_current_process()->primed = 1;
}

void _moss_dump_current_stack()
{
    _moss_log_acquire();
    ESP_LOG_BUFFER_HEXDUMP(TAG, (void*)moss_current_process()->top_of_stack, 256, ESP_LOG_INFO);
    _moss_log_release();
}

void moss_yield()
{
    switches++;
    _moss_push_active_process();
    //_moss_log("YIELD: Current Stack Ptr addr %#08X  TOS: %#08X SP: %#08X\n", moss_sp(), moss_current_process()->top_of_stack, moss_current_process()->stack);
    moss_ctx_switch();
}

void _moss_dispatch()
{
    switches++;
    __asm__ volatile ("call0    _moss_xt_dispatch\n");
}

moss_process* moss_current_process()
{
    return (moss_process*) moss_active_process[moss_core_id()];
}

void* moss_current_process_user_ptr()
{
    return moss_current_process()->user_ptr;
}

//TODO: Create a stack frame helper function.
void moss_dump_current_process_debug()
{
    moss_process* active = moss_current_process();

    /*_moss_log("Proc: '%s' TOS: %#08X SP: %#08X\n", 
    *            active->identifier, 
    *            (moss_address) active->top_of_stack, 
    *            (moss_address) active->stack);
    */
    XtExcFrame* frame = (XtExcFrame*) active->top_of_stack;
    _moss_log("Proc: '%s' PID: %d TOS: %08x SP: %08x PC: %08x a10: %08x EXIT: %08x (CORE %d)\n", 
                active->identifier, 
                active->pid,
                (moss_address) active->top_of_stack, 
                (moss_address) active->stack,
                (moss_address) frame->pc,
                (moss_address) frame->a10,
                (moss_address) frame->exit,
                moss_core_id());
}
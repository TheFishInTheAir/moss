#include <scheduler.h>
#include <moss.h>

#include <string.h>
#include <stddef.h>
//TODO: dont do this shit (directly include allocator tbh)
#include <stdlib.h>

#include <esp_log.h>

#include <stdio.h>

#include <xtensa/config/core.h>
#include <xtensa/xtensa_context.h>

#define STACK_DEPTH 256*4

#define TAG "moss_scheduler"

moss_scheduler_context sched_ctx;
moss_process* moss_active_process;

int _moss_scheduler_register_proc(moss_scheduler_context* ctx, moss_process* proc);
int _moss_scheduler_preempt(moss_scheduler_context* ctx);

void moss_init_tick_divisor()
{

}

/***********
 * PROCESS *
 ***********/

// Create a new process and add add it to execution queue
// Proc ref can be null
int moss_instantiate_proc(moss_scheduler_context* ctx, moss_process** proc_ref,
                          char* identifier, void(*entry_point)())
{
    moss_process* proc;
    PROP(moss_create_proc(ctx, &proc, identifier, entry_point));

    PROP(moss_process_exec_queue_push(&ctx->queue, proc));

    if(proc_ref != NULL)
        *proc_ref = proc;

    return MOSS_SUCCESS;
}

void _test_exit()
{
    assert(0 && "The Fuck");
}

extern void _xt_user_exit();

// TODO: Cleanup
uint8_t* _moss_init_stack(moss_process* proc)
{
    // Heavy Inspo from the freertos port code

    uint8_t *sp, *tos;
    tos = proc->top_of_stack;
    sp  = (uint8_t*) (((long)tos-XT_STK_FRMSZ-XT_CP_SIZE) & ~0xf); // allignmet


    // Zero Stack Frame
    for(uint8_t* tp = sp; tp <= tos; tp++)
    {
        // This is for debugging
        *tp = 0;
    }

    // Possible that there are some allignment issues which are leading to the 
    // Instruction failing

    XtExcFrame* frame = (XtExcFrame*) sp;
    frame->pc = (long*) proc->entry_point;
    frame->a0 = 0;
    frame->a1 = tos;
    frame->a10 = (long*) proc->entry_point; //testing purposes

    // This is the final call that loads PS PC and A0 
    frame->exit = (long*) _xt_user_exit; 

    // TODO: figure out which flags to use.
    //frame->ps = PS_UM | PS_EXCM;
    frame->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1);

    return sp;
}

// Proc Ref cant be null
int moss_create_proc(moss_scheduler_context* ctx, moss_process** proc_ref, 
                               char* identifier, void(*entry_point)())
{
    moss_process* proc;
    proc = (moss_process*) malloc(sizeof(moss_process));
    *proc_ref = proc;

    
    strcpy(proc->identifier, identifier);
    proc->entry_point = (void*)entry_point;
    proc->instruction_ptr = NULL;
    proc->state = MOSS_PROCESS_UNSTARTED;

    proc->stack = (uint8_t*) malloc(STACK_DEPTH); // So arbitrary its incredible TODO: replace

    proc->top_of_stack = proc->stack+(STACK_DEPTH-1);

    // Initialise stack frame.
    proc->top_of_stack = _moss_init_stack(proc);

    PROP(_moss_scheduler_register_proc(ctx, proc));


    return MOSS_SUCCESS;
}


/*************
 * SCHEDULER *
 *************/

moss_scheduler_context* moss_sched()
{
    assert(sched_ctx.init_flag==MOSS_INIT_CONST);
    return &sched_ctx;
}

// NOTE: Not sure how we want to handle global things.
int moss_scheduler_init()
{
    sched_ctx.num_procs = 0;
    sched_ctx.init_flag = MOSS_INIT_CONST;
    PROP(moss_process_exec_queue_init(&sched_ctx.queue));
    
    return MOSS_SUCCESS;
}

// TODO: Implement this complex ass function
int _moss_scheduler_preempt(moss_scheduler_context* ctx)
{
    //moss_process* next_proc = moss_process_exec_queue_pop(&ctx->queue);
    //assert(next_proc!=NULL);


    return MOSS_SUCCESS;
}

// Adds into process array and assigns PID
int _moss_scheduler_register_proc(moss_scheduler_context* ctx, moss_process* proc)
{
    if(ctx->num_procs == MAX_PROCS)
        return MOSS_FAIL;

    proc->pid = ctx->num_procs;
    ctx->procs[ctx->num_procs++] = proc;
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

moss_process* moss_scheduler_find_proc_pid(moss_scheduler_context* ctx, uint8_t pid)
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
    PROP(_moss_scheduler_register_proc(ctx, proc));
    return MOSS_SUCCESS;

}


/**************************
 * PRIMARY SCHEDULER LOOP *
 **************************/

int moss_scheduler_start(moss_scheduler_context* ctx)
{
    // very stupid lil thing here.
    

    // This is very much not how schedulers work.

    ESP_LOGI(TAG, "Beginning Scheduler");

    //moss_prime_context_switch();
    //_moss_xt_dispatch();
    __asm__ volatile ("call0    _moss_xt_dispatch\n");

    //moss_yield();
    //moss_interrupt_yield();

    /*
    while(1)
    {
        moss_process* next_proc;
        while(moss_process_exec_queue_pop(&ctx->queue, &next_proc))
        {
            if(next_proc->state==MOSS_PROCESS_UNSTARTED)
            {
                next_proc->entry_point();
                next_proc->state = MOSS_PROCESS_STOPPED;
            }
        }
    }*/

    return MOSS_SUCCESS;
}


/**********************
 * PROCESS EXEC QUEUE *
 **********************/

int moss_process_exec_queue_init(moss_process_exec_queue* queue)
{
    // TODO: again such a stupidly arbitrary size
    queue->ring_size = MAX_PROCS*2;
    queue->num_elems = 0;
    queue-> start_off = 0;

    return MOSS_SUCCESS;
}

moss_process** _moss_process_exec_queue_get_elem(moss_process_exec_queue* queue, uint32_t indx)
{
    return queue->processes+(indx+queue->start_off) % queue->ring_size;
}


int moss_process_exec_queue_pop(moss_process_exec_queue* queue, moss_process** proc)
{
    if(queue->num_elems==0)
        return MOSS_FAIL;
    
    *proc = *_moss_process_exec_queue_get_elem(queue, 0);
    queue->start_off += 1;
    queue->start_off %= queue->ring_size;
    queue->num_elems--;
    return MOSS_SUCCESS;
}

// A process should never be entered twice into the exec queue (I think at least!)
int moss_process_exec_queue_push(moss_process_exec_queue* queue, moss_process* proc)
{
    // TODO: make sure this never goes off
    assert(queue->num_elems<queue->ring_size);

    // This is assuming no duplicate processes (for now)

    moss_process** queue_slot = _moss_process_exec_queue_get_elem(queue, queue->num_elems);
    *queue_slot = proc;
    queue->num_elems++;

    return MOSS_SUCCESS;
}

// @Refactor Name
void _moss_push_active_process()
{
    moss_active_process->state = MOSS_PROCESS_WAITING;
    moss_process_exec_queue_push(&moss_sched()->queue, moss_active_process);
}

// @Refactor Name
void _moss_prime_context_switch()
{
    // Likely the previous process state should be determined somewhere else
    moss_process* next_proc;
    if(moss_process_exec_queue_pop(&moss_sched()->queue, &next_proc))
    {
        //printf("Next Task: %s\n", next_proc->identifier);
        next_proc->state = MOSS_PROCESS_RUNNING;
        moss_active_process = next_proc;
    }
    else
    {
        //while(1){_moss_nop();}
        ESP_LOGI(TAG, "No More Processes");
        assert(0 && "Process Queue is empty??");
    }
}
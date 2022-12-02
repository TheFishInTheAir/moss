#include <matrix.h>
#include <stdlib.h>
#include <esp_timer.h>

#include <moss.h>
#include <synch.h>
#include <scheduler.h>
#include "esp_heap_caps.h"


static int matrix_size = 0;

static int* mat_a;
static int* mat_b;
static int* mat_c;

static moss_semaphore lock;
static moss_semaphore completion;

typedef struct _matrix_load_descriptor
{
    uint32_t start_idx;
    uint32_t num_ops;
} matrix_load_descriptor;

static inline int* indx_matrix(int* mat, int col, int row)
{
    return mat + (col+row*matrix_size);
}

void matrix_test_init(int size)
{

    _moss_log("Left heap bytes: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    matrix_size = size;

    mat_a = (int*) malloc(sizeof(int) * size * size);
    mat_b = (int*) malloc(sizeof(int) * size * size);
    mat_c = (int*) malloc(sizeof(int) * size * size);
    
    assert(mat_a!=NULL);
    assert(mat_b!=NULL);
    assert(mat_c!=NULL);

    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            *indx_matrix(mat_a, i, j) = rand() % 10;
            *indx_matrix(mat_b, i, j) = rand() % 10;
        }
    }

}

void matrix_test_cleanup()
{
    free(mat_a);
    free(mat_b);
    free(mat_c);
}

void matrix_test_single_threaded()
{
    int i, j;
    for(int x=0; x<matrix_size*matrix_size; x++)
    {
        for(int k=0; k<matrix_size; k++) 
        {
            i = (x) % matrix_size;
            j = (x) / matrix_size;
            *indx_matrix(mat_c, i, j)   +=  (*indx_matrix(mat_a, i, k)) * 
                                            (*indx_matrix(mat_b, k, j));
        }
    }
}

void _matrix_test_multiply()
{
    matrix_load_descriptor descriptor = *(matrix_load_descriptor*)(moss_current_process_user_ptr());
    
    int i, j;
    for(int x=0; x < descriptor.num_ops; x++)
    {
        for(int k=0; k<matrix_size; k++) 
        {
            //This is adding overhead
            i = (x+descriptor.start_idx) % matrix_size;
            j = (x+descriptor.start_idx) / matrix_size;

            *indx_matrix(mat_c, i, j)   +=  (*indx_matrix(mat_a, i, k)) * 
                                            (*indx_matrix(mat_b, k, j));

        }
    }    

    moss_semaphore_signal(&completion);

    moss_terminate();
}

void matrix_test_multi_threaded()
{
    matrix_load_descriptor* descriptors[MAT_THREADS];
    moss_semaphore_init(&completion, 0);
    moss_semaphore_init(&lock, 1);

    _moss_log("Got past semaphroes\n");

    for(int i = 0; i < MAT_THREADS; i++)
    {

        descriptors[i] = (matrix_load_descriptor*) malloc(sizeof(matrix_load_descriptor));
        descriptors[i]->num_ops = (matrix_size*matrix_size)/MAT_THREADS;
        descriptors[i]->start_idx = i*descriptors[i]->num_ops;

        // Hacky way of naming processes for now.
        char id[] = "matrix_task0";
        id[11] = i+'0';

        _moss_log("Creating new Process %s\n", id);
        moss_instantiate_proc(moss_sched(), NULL, id, _matrix_test_multiply, descriptors[i]);
    }


    int complete = 0;
    while(complete!=MAT_THREADS)
    {
        moss_semaphore_wait(&completion);
        complete++;       
    }
}

void matrix_test_run_test(int size)
{
    matrix_test_init(size);
    if(matrix_size<8)
    {
        _moss_log("Matrix A:\n");
        for(int i = 0; i < matrix_size; i++)
        {
            _moss_log("| ");
            for(int j = 0; j < matrix_size; j++)
            {
                _moss_log("%d ", indx_matrix(mat_a, i, j));
            }
            _moss_log("|\n");
        }
        _moss_log("\n");

        _moss_log("Matrix B:\n");
        for(int i = 0; i < matrix_size; i++)
        {
            _moss_log("| ");
            for(int j = 0; j < matrix_size; j++)
            {
                _moss_log("%d ", indx_matrix(mat_b, i, j));
            }
            _moss_log("|\n");
        }
    }
    else
    {
        _moss_log("%dx%d Matrix\n", matrix_size, matrix_size);
    }

    int64_t sstart  = esp_timer_get_time();
    matrix_test_single_threaded();
    int64_t send    = esp_timer_get_time();

    _moss_log("Doing multithreaded test now\n");


    int64_t mstart  = esp_timer_get_time();
    matrix_test_multi_threaded();
    int64_t mend    = esp_timer_get_time();

    int64_t sdif = send-sstart;
    int64_t mdif = mend-mstart;

    _moss_log("Single-Threaded Time: %lld us\n", sdif);
    _moss_log("Multi-Threaded Time:  %lld us\n", mdif);
    _moss_log("Multi-Threaded Performance Speedup: %f%%\n", (double)sdif/(double)mdif*100.0);

    matrix_test_cleanup();
}


void matrix_test_run_suite()
{
    _moss_log("---Running Matrix Multiplication Test Suite---\n");
    matrix_test_run_test(16);
    matrix_test_run_test(32);
    matrix_test_run_test(42);
    matrix_test_run_test(64);
    matrix_test_run_test(80);
}
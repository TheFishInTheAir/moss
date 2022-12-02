#include <startup.h>
#include <scheduler.h>
#include <sdkconfig.h>

#include <moss.h>

#include <esp_private/esp_clk.h>
#include <esp_private/esp_int_wdt.h>
#include <esp_private/startup_internal.h>


#include <esp_timer.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_cpu.h>
//#include <esp_vfs_dev.h>
#include <esp_vfs_console.h>
#include <esp_xt_wdt.h>
#include <esp_newlib.h>
#include <esp_app_desc.h>
#include <esp_heap_caps_init.h>

#include <hal/wdt_hal.h>

static const char* TAG = "moss_startup";

static volatile bool _sys_init_complete = 0;

// Overrides start_cpu0_default
void moss_portlvl_init()
{
    ESP_EARLY_LOGI(TAG, "Overriding Default Startup");

    ESP_EARLY_LOGI(TAG, "Pro cpu start use code");

    int cpu_freq = esp_clk_cpu_freq();
    ESP_EARLY_LOGI(TAG, "CPU FREQ: %d", cpu_freq);
  
    // Heap Allocator
    heap_caps_init();

    // Timer
    esp_timer_early_init();

    // NewLib
    esp_newlib_init();
    esp_newlib_time_init();

    // Setup VFS Console
    {
        esp_err_t vfs_err = esp_vfs_console_register();
        assert(vfs_err == ESP_OK && "Failed to register vfs console");
        
        const static char *default_stdio_dev = "/dev/console/";
        esp_reent_init(_GLOBAL_REENT);
        _GLOBAL_REENT->_stdin  = fopen(default_stdio_dev, "r");
        _GLOBAL_REENT->_stdout = fopen(default_stdio_dev, "w");
        _GLOBAL_REENT->_stderr = fopen(default_stdio_dev, "w");
    }
    
    // Ignoring RTC init for now.

    // Disable the Boot Watchdog
    {
        wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};

        
        wdt_hal_write_protect_disable(&rtc_wdt_ctx);
        wdt_hal_disable(&rtc_wdt_ctx);
        wdt_hal_write_protect_enable(&rtc_wdt_ctx);
    }
    
    ESP_EARLY_LOGI(TAG, "Disabled Boot Watchdog");

    ESP_EARLY_LOGI(TAG, "Unblocking Other Cores");

    // This is defined in esp_system startup
    startup_resume_other_cores();


    moss_kernel_init();
}

void moss_cpu1_init()
{
    // Wait for system init to finish  
    while(!_sys_init_complete)
    {
        for(int i = 0; i < (10^4); i++)
        {
            _moss_nop();
        }
    }
    moss_scheduler_start(moss_sched());

}

extern void app_main();
void moss_kernel_init()
{
    ESP_EARLY_LOGI(TAG, "Made it to kernel Init");

    // Init internal components
    moss_scheduler_init();

    // Create Main Task
    moss_process* main_proc;
    moss_instantiate_proc(moss_sched(), &main_proc, "moss_main", app_main, NULL);

    // moss_process_exec_queue_debug_dump(&moss_sched()->queue);

    // Scheduler Init State has been setup, can now let other core resume operation.
    _sys_init_complete = 1;

    // Start Scheduler
    moss_scheduler_start(moss_sched());
    
    printf("Should not have gotten to this point!");
    while(1){};

}

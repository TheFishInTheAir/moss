#include <startup.h>
#include <scheduler.h>
#include <sdkconfig.h>

#include <esp_private/esp_clk.h>
#include <esp_private/esp_int_wdt.h>

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

// Override start_cpu0_default
void moss_portlvl_init()
{
    ESP_EARLY_LOGI(TAG, "Overriding Default Startup");

    ESP_EARLY_LOGI(TAG, "Pro cpu start use code");
    int cpu_freq = esp_clk_cpu_freq();

    ESP_EARLY_LOGI(TAG, "CPU FREQ: %d", cpu_freq);
    
    char buf[17];
    esp_app_get_elf_sha256(buf, sizeof(buf));
    ESP_EARLY_LOGI(TAG, "ELF file SHA256:  %s...", buf);
    //    ESP_EARLY_LOGI(TAG, "ESP-IDF:          %s", app_desc->idf_ver);


    // TODO: add second core init before following init steps.

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
    
    ESP_LOGI(TAG, "Disabled Boot Watchdog");

    moss_kernel_init();
}

void _moss_interrupt_init()
{
    // Setup Interrupts HERE!
    esp_int_wdt_init();

    
}


void test_process_twoo()
{
    printf("Executed a second cool process\n");
}

void test_process_entry()
{
    printf("Executed a cool process\n");
    moss_process* secondary_proc;
    moss_instantiate_proc(moss_sched(), &secondary_proc, "testingggg", test_process_twoo); //TODO: should allow process to be null
}

void moss_kernel_init()
{
    ESP_LOGI(TAG, "Made it to kernel Init");

    // TODO: deal with the interrupt nightmare later
    //_moss_interrupt_init();

    
    // And interrupt watchdog

    // Init internal components
    moss_scheduler_init();

    // Create Main Task
    moss_process* main_proc;
    moss_instantiate_proc(moss_sched(), &main_proc, "moss_main", test_process_entry);

    // Start Scheduler
    moss_scheduler_start(moss_sched());
    
    printf("Should not have gotten to this point!");
    while(1){};

}

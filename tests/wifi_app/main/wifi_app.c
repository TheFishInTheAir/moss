#include <stdio.h>
#include <wifi.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <moss_net.h>
#include <network.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ping_test()
{

  while(1)
  {
    moss_udp_ping_latency_test();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    moss_wifi_init();
    moss_wifi_connect_default();


    // Do ping test instead
    ping_test();

    printf("the fuck\n");


    while(1)
    {
      moss_pull_test_prog();
      
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#include <stdio.h>
#include <wifi.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <network.h>

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    printf("Got to Main.\n");
    moss_wifi_init();
    moss_wifi_connect_default();

    http_get_test();

    while(1){};
}
#include <wifi.h>
#include <moss_net.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_log.h>


// Free RTOS includes TODO: remove dependency
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>



#define MOSS_DEFAULT_WIFI_SSID "Apt 3"
#define MOSS_DEFAULT_WIFI_PASS "augslambo"

#define MOSS_WIFI_CONNECTED_BIT BIT0
#define MOSS_WIFI_FAIL_BIT      BIT1

#define TAG "moss_net_wifi"

typedef struct _moss_wifi_context
{
    EventGroupHandle_t s_wifi_event_group;
} moss_wifi_context;

void _moss_wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

// Wifi Context
static moss_wifi_context ctx = {NULL};

int moss_wifi_init()
{    



    ctx.s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Default NETIF config for wifi station
    esp_netif_create_default_wifi_sta();


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_moss_wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &_moss_wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_LOGI(TAG, "Wifi init completed.");

    return MOSS_SUCCESS;
}

int moss_wifi_connect_default()
{

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MOSS_DEFAULT_WIFI_SSID,
            .password = MOSS_DEFAULT_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH, //TODO: Figure out what this is
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );


    // This is not super necessary to be honest.
    // Entirely just to hold up execution until succesful connection which may or may not be desirable.
    EventBits_t bits = xEventGroupWaitBits(ctx.s_wifi_event_group,
            MOSS_WIFI_CONNECTED_BIT | MOSS_WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & MOSS_WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Succesfully connected to Default Network.");
    } else if (bits & MOSS_WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to Default Netowrk.");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED ERROR (oopsies)");
    }

    return MOSS_SUCCESS;
}


void _moss_wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        ESP_LOGI(TAG,"Failed to connect to AP '%s'", MOSS_DEFAULT_WIFI_SSID);
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Succesfully connected to AP '%s' Assigned ip:" IPSTR, MOSS_DEFAULT_WIFI_SSID, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(ctx.s_wifi_event_group, MOSS_WIFI_CONNECTED_BIT);
    }
}
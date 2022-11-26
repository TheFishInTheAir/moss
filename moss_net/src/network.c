#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "Moss WIFI App";


#define WEB_SERVER "10.0.0.171"
#define WEB_PORT "8000"
#define WEB_PATH "/text.txt"

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";
// Start with the simplest thing


void weird_delay()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void http_get_test()
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {


        // Some mysterious ass function right here
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        // res seems like a resolved address

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            weird_delay();
            continue;
        }

        // Code to print the resolved IP.

        // Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code 
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        // START OF THE REAL NETCODEEEE BABYYYYYYYYYYY

        // So what exactly is a socket


        // Socket allocation
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            weird_delay();
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");


        // Socket Connection
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            weird_delay();
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);


        // Send our request
        // this stuff is super straightforward tbh.

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            weird_delay();
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");


        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            weird_delay();
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");



        // Read HTTP response 
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                // Print the contents of HTTP Response Character by character
                putchar(recv_buf[i]);
            }
        } while(r > 0);





        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            weird_delay();
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

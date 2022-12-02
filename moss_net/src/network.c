#include <string.h>
#include <network.h>
#include <moss_net.h>
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
static const char *TAG = "moss_network";



#define TEST_WEB_SERVER "10.0.0.171"
#define TEST_WEB_PORT "8000"
#define TEST_WEB_PATH "/text.txt"

static const char *REQUEST = "GET " TEST_WEB_PATH " HTTP/1.0\r\n"
    "Host: "TEST_WEB_SERVER":"TEST_WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";


static const char *REQUEST_FORMAT = "GET %s HTTP/1.0\r\n"
    "Host: %s:%s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

void arbitrary_delay()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

int _moss_get_server_connection(char* server, char* port, int* s)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;

    int err = getaddrinfo(TEST_WEB_SERVER, TEST_WEB_PORT, &hints, &res);
    if(err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        return MOSS_FAIL;
    }

    // Code to print the resolved IP.
    // Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code 
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    // Socket allocation
    *s = socket(res->ai_family, res->ai_socktype, 0);
    if(*s < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        return MOSS_FAIL;
    }
    ESP_LOGI(TAG, "... allocated socket");

    // Socket Connection
    if(connect(*s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(*s);
        freeaddrinfo(res);
        return MOSS_FAIL;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    return MOSS_SUCCESS;
}

int moss_download_http_get(char* server, char* port, char* path, void* buf, uint32_t buf_size)
{
    char request[256*4];
    sprintf(request, REQUEST_FORMAT, path, server, port);
    int err;
    int s;

    err = _moss_get_server_connection(server, port, &s);
    if(err!=MOSS_SUCCESS)
    {
        ESP_LOGE(TAG, "Failed to conenct to server - Aborting Download.");
        return MOSS_FAIL;
    }
    
    // Send our request
    if (write(s, request, strlen(request)) < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        return MOSS_FAIL;
    }
    ESP_LOGI(TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(s);
        return MOSS_FAIL;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");


    // Skip HTTP Headers, definetly not the best way to do this.
    {
        char mini_buf[4];

        while(1)
        {
            // NOTE: Possible inifinite loop if malformed response
            // TODO: Add Timeout Watchdog

            mini_buf[0] = mini_buf[1];
            mini_buf[1] = mini_buf[2];
            mini_buf[2] = mini_buf[3];
            read(s, mini_buf+3, 1);
            if( mini_buf[0]=='\r' &&
                mini_buf[1]=='\n' &&
                mini_buf[2]=='\r' &&
                mini_buf[3]=='\n')
                {
                    break;
                }
        }
    }

    // Read HTTP response 
    read(s, buf, buf_size);

    close(s);
    return MOSS_SUCCESS;
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

        int err = getaddrinfo(TEST_WEB_SERVER, TEST_WEB_PORT, &hints, &res);

        // res is the resolved address
        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            arbitrary_delay();
            continue;
        }

        // Print the resolved IP.
        // Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code 
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        // Socket allocation
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            arbitrary_delay();
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        // Socket Connection
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            arbitrary_delay();
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);


        // Send the request

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            arbitrary_delay();
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
            arbitrary_delay();
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
            arbitrary_delay();
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

idf_component_register(SRCS "freertos_profiler.c" "freertos_wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_timer esp_wifi nvs_flash)

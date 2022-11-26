idf_component_register(SRCS "freertos-profiler.c" "freertos-wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_timer esp_wifi nvs_flash)

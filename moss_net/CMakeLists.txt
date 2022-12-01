idf_component_register(SRCS "src/moss_net.c" "src/network.c" "src/wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_wifi nvs_flash elfloader)

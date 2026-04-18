idf_component_register(
    SRCS "comm_mqtt.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos log esp_timer esp_event esp_netif esp_wifi mqtt nvs_flash lwip
)

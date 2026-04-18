idf_component_register(
    SRCS "comm_lorawan.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos log esp_timer
)


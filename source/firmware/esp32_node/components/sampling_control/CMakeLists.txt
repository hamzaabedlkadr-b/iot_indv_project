idf_component_register(
    SRCS "sampling_control.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos esp_timer log
)

idf_component_register(
    SRCS "signal_processing.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos esp_timer log
)


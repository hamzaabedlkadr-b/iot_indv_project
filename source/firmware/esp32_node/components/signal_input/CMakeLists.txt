idf_component_register(
    SRCS "signal_input.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos esp_timer log
)


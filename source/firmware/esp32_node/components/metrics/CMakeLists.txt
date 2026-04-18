idf_component_register(
    SRCS "metrics.c"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES freertos esp_timer log
)


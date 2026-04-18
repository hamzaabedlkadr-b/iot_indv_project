idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "." "../include"
    REQUIRES signal_input signal_processing sampling_control comm_mqtt comm_lorawan metrics app_display freertos log
)


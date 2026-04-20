set(heltec_library_dir "$ENV{HELTEC_ESP32_DEV_BOARDS_DIR}")

if(NOT heltec_library_dir)
    set(heltec_library_dir "$ENV{USERPROFILE}/Documents/Arduino/libraries/Heltec_ESP32_Dev-Boards")
endif()

file(TO_CMAKE_PATH "${heltec_library_dir}" heltec_library_dir)
file(TO_CMAKE_PATH "$ENV{IDF_PATH}/components/esp_netif/include" idf_esp_netif_include_dir)

set(heltec_src_dir "${heltec_library_dir}/src")
set(heltec_prebuilt_lorawan "${heltec_src_dir}/esp32s3/liblorawan.a")
set(heltec_driver_dir "${heltec_src_dir}/driver")
set(heltec_radio_dir "${heltec_src_dir}/radio")
set(heltec_loramac_dir "${heltec_src_dir}/loramac")
set(heltec_loramac_region_dir "${heltec_loramac_dir}/region")

if(NOT EXISTS "${heltec_src_dir}/LoRaWan_APP.cpp")
    message(FATAL_ERROR
            "Heltec_ESP32_Dev-Boards was not found. Install the library locally or set "
            "HELTEC_ESP32_DEV_BOARDS_DIR to its folder before building.")
endif()

if(NOT EXISTS "${heltec_prebuilt_lorawan}")
    message(FATAL_ERROR
            "Heltec prebuilt LoRaWAN archive was not found at ${heltec_prebuilt_lorawan}.")
endif()

set(heltec_lorawan_support_srcs
    "${heltec_src_dir}/clk.c"
    "${heltec_driver_dir}/board.c"
    "${heltec_driver_dir}/delay.c"
    "${heltec_driver_dir}/gpio-board.c"
    "${heltec_driver_dir}/gpio.c"
    "${heltec_driver_dir}/sx1262-board.c"
    "${heltec_driver_dir}/sx126x.c"
    "${heltec_radio_dir}/radio.c"
    "${heltec_loramac_dir}/aes.c"
    "${heltec_loramac_dir}/cmac.c"
    "${heltec_loramac_dir}/LoRaMac.c"
    "${heltec_loramac_dir}/LoRaMacClassB.c"
    "${heltec_loramac_dir}/LoRaMacConfirmQueue.c"
    "${heltec_loramac_dir}/LoRaMacCrypto.c"
    "${heltec_loramac_dir}/utilities.c"
    "${heltec_loramac_region_dir}/Region.c"
    "${heltec_loramac_region_dir}/RegionCommon.c"
    "${heltec_loramac_region_dir}/RegionEU868.c"
)

idf_component_register(
    SRCS "comm_lorawan.cpp"
         "${heltec_src_dir}/LoRaWan_APP.cpp"
         "${heltec_src_dir}/HT_Display.cpp"
         "${heltec_src_dir}/HT_DisplayUi.cpp"
         ${heltec_lorawan_support_srcs}
    INCLUDE_DIRS "include"
                 "../../include"
                 "${heltec_src_dir}"
                 "${heltec_driver_dir}"
                 "${heltec_radio_dir}"
                 "${heltec_loramac_dir}"
                 "${heltec_loramac_region_dir}"
                 "${idf_esp_netif_include_dir}"
    REQUIRES arduino
             freertos
             log
             esp_timer
             esp_netif
             driver
             spi_flash
             esp_partition
             mbedtls
             wpa_supplicant
             esp_adc
             esp_eth
             http_parser
             esp_ringbuf
             esp_driver_gptimer
             esp_driver_usb_serial_jtag
             esp_http_client
             esp_https_ota
             esp_driver_touch_sens
)

add_prebuilt_library(heltec_lorawan "${heltec_prebuilt_lorawan}" REQUIRES arduino)
target_link_libraries(${COMPONENT_LIB} PUBLIC heltec_lorawan)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    HELTEC_WIFI_LORA_32_V3
    HELTEC_BOARD=30
    WIFI_LORA_32_V3
    MCU_ESP32_S3
    RADIO_CHIP_SX1262
    REGION_EU868
    ACTIVE_REGION=LORAMAC_REGION_EU868
    SLOW_CLK_TPYE=0
    LoRaWAN_DEBUG_LEVEL=2
    LORAWAN_PREAMBLE_LENGTH=8
    LORAWAN_DEVEUI_AUTO=0
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)

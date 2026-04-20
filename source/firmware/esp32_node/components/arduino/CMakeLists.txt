file(TO_CMAKE_PATH
     "$ENV{LOCALAPPDATA}/Arduino15/packages/esp32/hardware/esp32/3.3.7"
     arduino_framework_dir)

if(NOT EXISTS "${arduino_framework_dir}/CMakeLists.txt")
    message(FATAL_ERROR
            "Arduino core 3.3.7 was not found at ${arduino_framework_dir}. "
            "Install the ESP32 Arduino core locally before building this project.")
endif()

set(arduino_core_dir "${arduino_framework_dir}/cores/esp32")
set(arduino_variant_dir "${arduino_framework_dir}/variants/${CONFIG_ARDUINO_VARIANT}")
file(TO_CMAKE_PATH "$ENV{IDF_PATH}/components/esp_netif/include" idf_esp_netif_include_dir)

set(srcs
    "${arduino_core_dir}/base64.cpp"
    "${arduino_core_dir}/cbuf.cpp"
    "${arduino_core_dir}/ColorFormat.c"
    "${arduino_core_dir}/chip-debug-report.cpp"
    "${arduino_core_dir}/esp32-hal-adc.c"
    "${arduino_core_dir}/esp32-hal-bt.c"
    "${arduino_core_dir}/esp32-hal-cpu.c"
    "${arduino_core_dir}/esp32-hal-dac.c"
    "${arduino_core_dir}/esp32-hal-gpio.c"
    "${arduino_core_dir}/esp32-hal-hosted.c"
    "${arduino_core_dir}/esp32-hal-i2c.c"
    "${arduino_core_dir}/esp32-hal-i2c-ng.c"
    "${arduino_core_dir}/esp32-hal-i2c-slave.c"
    "${arduino_core_dir}/esp32-hal-ledc.c"
    "${arduino_core_dir}/esp32-hal-log-wrapper.c"
    "${arduino_core_dir}/esp32-hal-matrix.c"
    "${arduino_core_dir}/esp32-hal-misc.c"
    "${arduino_core_dir}/esp32-hal-periman.c"
    "${arduino_core_dir}/esp32-hal-psram.c"
    "${arduino_core_dir}/esp32-hal-rgb-led.c"
    "${arduino_core_dir}/esp32-hal-rmt.c"
    "${arduino_core_dir}/esp32-hal-sigmadelta.c"
    "${arduino_core_dir}/esp32-hal-spi.c"
    "${arduino_core_dir}/esp32-hal-time.c"
    "${arduino_core_dir}/esp32-hal-timer.c"
    "${arduino_core_dir}/esp32-hal-tinyusb.c"
    "${arduino_core_dir}/esp32-hal-touch.c"
    "${arduino_core_dir}/esp32-hal-touch-ng.c"
    "${arduino_core_dir}/esp32-hal-uart.c"
    "${arduino_core_dir}/Esp.cpp"
    "${arduino_core_dir}/freertos_stats.cpp"
    "${arduino_core_dir}/FunctionalInterrupt.cpp"
    "${arduino_core_dir}/HardwareSerial.cpp"
    "${arduino_core_dir}/HashBuilder.cpp"
    "${arduino_core_dir}/HEXBuilder.cpp"
    "${arduino_core_dir}/idf_openthread_mpool_wrapper.c"
    "${arduino_core_dir}/IPAddress.cpp"
    "${arduino_core_dir}/libb64/cdecode.c"
    "${arduino_core_dir}/libb64/cencode.c"
    "${arduino_core_dir}/MacAddress.cpp"
    "${arduino_core_dir}/main.cpp"
    "${arduino_core_dir}/MD5Builder.cpp"
    "${arduino_core_dir}/Print.cpp"
    "${arduino_core_dir}/stdlib_noniso.c"
    "${arduino_core_dir}/Stream.cpp"
    "${arduino_core_dir}/StreamString.cpp"
    "${arduino_core_dir}/Tone.cpp"
    "${arduino_core_dir}/HWCDC.cpp"
    "${arduino_core_dir}/USB.cpp"
    "${arduino_core_dir}/USBCDC.cpp"
    "${arduino_core_dir}/USBMSC.cpp"
    "${arduino_core_dir}/FirmwareMSC.cpp"
    "${arduino_core_dir}/firmware_msc_fat.c"
    "${arduino_core_dir}/wiring_pulse.c"
    "${arduino_core_dir}/wiring_shift.c"
    "${arduino_core_dir}/WMath.cpp"
    "${arduino_core_dir}/WString.cpp"
    "${arduino_framework_dir}/libraries/SPI/src/SPI.cpp"
    "${arduino_framework_dir}/libraries/Wire/src/Wire.cpp"
)

set(includedirs
    "${arduino_variant_dir}"
    "${arduino_core_dir}"
    "${arduino_framework_dir}/libraries/SPI/src"
    "${arduino_framework_dir}/libraries/Wire/src"
    "${idf_esp_netif_include_dir}"
)

set(priv_includes "${arduino_core_dir}/libb64")
set(requires
    spi_flash
    esp_partition
    mbedtls
    wpa_supplicant
    esp_adc
    esp_eth
    esp_netif
    http_parser
    esp_ringbuf
    esp_driver_gptimer
    esp_driver_usb_serial_jtag
    driver
    esp_http_client
    esp_https_ota
)
set(priv_requires fatfs nvs_flash app_update spiffs bootloader_support bt esp_hid usb esp_psram)

if(IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s2" OR IDF_TARGET STREQUAL "esp32s3" OR IDF_TARGET STREQUAL "esp32p4")
    list(APPEND requires esp_driver_touch_sens)
endif()

idf_component_register(
    INCLUDE_DIRS ${includedirs}
    PRIV_INCLUDE_DIRS ${priv_includes}
    SRCS ${srcs}
    REQUIRES ${requires}
    PRIV_REQUIRES ${priv_requires}
)

if(NOT CONFIG_FREERTOS_HZ EQUAL 1000 AND NOT "$ENV{ARDUINO_SKIP_TICK_CHECK}")
    message(FATAL_ERROR
            "esp32-arduino requires CONFIG_FREERTOS_HZ=1000 "
            "(currently ${CONFIG_FREERTOS_HZ})")
endif()

string(TOUPPER ${CONFIG_ARDUINO_VARIANT} idf_target_caps)
string(REPLACE "-" "_" idf_target_for_macro "${idf_target_caps}")
target_compile_options(${COMPONENT_TARGET} PUBLIC
    -DARDUINO=10812
    -DARDUINO_${idf_target_for_macro}_DEV
    -DARDUINO_ARCH_ESP32
    -DARDUINO_BOARD="${idf_target_caps}_DEV"
    -DARDUINO_VARIANT="${CONFIG_ARDUINO_VARIANT}"
    -DESP32
)

if(NOT CONFIG_DIAG_USE_EXTERNAL_LOG_WRAP)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_log_write" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_log_writev" APPEND)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __wrap_esp_log_write" "-u __wrap_esp_log_writev")
endif()

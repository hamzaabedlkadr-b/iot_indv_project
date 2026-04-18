# Secure MQTT Setup

This note explains how the firmware can switch from plain local `MQTT` to verified `MQTTS`.

## Current Situation

The real home-network validation in this repository was done with a plain local broker:

- scheme: `mqtt://`
- transport: local LAN
- purpose: functional validation of the edge path

That is enough for development, but it does not fully satisfy the grading item about secure MQTT transport.

## What The Firmware Supports Now

The firmware in [`../firmware/esp32_node/components/comm_mqtt/comm_mqtt.c`](../firmware/esp32_node/components/comm_mqtt/comm_mqtt.c) now supports:

- plain `MQTT`
- `MQTTS` with certificate verification through the ESP-IDF certificate bundle
- optional broker username / password
- optional server common-name override

The control switches are in [`../firmware/esp32_node/include/project_config.h`](../firmware/esp32_node/include/project_config.h).

## Config Switches

### Plain Local Broker

Use:

```c
#define PROJECT_MQTT_SECURITY_MODE PROJECT_MQTT_SECURITY_PLAINTEXT
#define PROJECT_MQTT_USERNAME ""
#define PROJECT_MQTT_PASSWORD ""
```

This keeps the currently working home setup unchanged.

### Secure Broker With TLS

Use:

```c
#define PROJECT_MQTT_SECURITY_MODE PROJECT_MQTT_SECURITY_TLS
#define PROJECT_DEFAULT_MQTT_BROKER_HOST "your-broker-hostname"
#define PROJECT_DEFAULT_MQTT_BROKER_PORT 8883
#define PROJECT_MQTT_USERNAME "your-user"
#define PROJECT_MQTT_PASSWORD "your-password"
#define PROJECT_MQTT_TLS_COMMON_NAME ""
#define PROJECT_MQTT_TLS_SKIP_COMMON_NAME_CHECK 0U
```

Recommended rules:

- prefer a hostname, not a raw IP, when using TLS
- keep `PROJECT_MQTT_TLS_SKIP_COMMON_NAME_CHECK = 0U`
- use a broker certificate that chains to a CA included in the ESP-IDF certificate bundle

## What Is Verified

When `PROJECT_MQTT_SECURITY_MODE` is set to `PROJECT_MQTT_SECURITY_TLS`, the client uses:

- `mqtts://`
- broker certificate verification via `esp_crt_bundle_attach`

So the board is not just encrypting blindly; it is also verifying the remote broker identity against the trusted CA bundle.

## Current Limitation

This repo does not yet include a live saved run against a TLS-enabled broker, so the secure path is implemented but not yet fully demonstrated in the captured evidence.

That means the secure-MQTT grading item should be described honestly as:

- `implemented in firmware`
- `ready for validation`
- `not yet proven with a saved live TLS run`

## Best Final Evidence

If you later validate secure MQTT, the best proof would be:

1. serial log showing connection to the secure broker
2. listener output showing real messages arriving
3. one note stating that the transport was `mqtts://` and certificate verification was enabled

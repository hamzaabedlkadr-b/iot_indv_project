# Secure MQTT Setup

This note explains how the firmware can switch from plain local `MQTT` to verified `MQTTS`.

## Validated Situation

The project has both a plain local MQTT validation run and a secure live `MQTTS` validation run.

Plain local broker evidence was used for development:

- scheme: `mqtt://`
- transport: local LAN
- purpose: functional validation of the edge path

Secure MQTT evidence was captured on `2026-04-22`:

- scheme: `mqtts://`
- broker: `broker.emqx.io:8883`
- topic: `iot_indv_project/hamza/adaptive-sampling-node/secure`
- listener: `tls=enabled verify=required`
- ESP32 log: `Certificate validated`
- final proof: [`../results/secure_mqtt_evidence_2026-04-22.md`](../results/secure_mqtt_evidence_2026-04-22.md)

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

## Final Evidence

Use these files when presenting the secure MQTT requirement:

- [`../results/secure_mqtt_evidence_2026-04-22.md`](../results/secure_mqtt_evidence_2026-04-22.md)
- [`../results/summaries/secure_mqtt_listener_2026-04-22.out.txt`](../results/summaries/secure_mqtt_listener_2026-04-22.out.txt)
- [`../results/summaries/secure_mqtt_summary_final_2026-04-22.md`](../results/summaries/secure_mqtt_summary_final_2026-04-22.md)
- [`../pics/2026-04-22_secure_mqtt_listener_tls.png`](../pics/2026-04-22_secure_mqtt_listener_tls.png)
- [`../pics/2026-04-22_secure_mqtt_cert_validated.png`](../pics/2026-04-22_secure_mqtt_cert_validated.png)
- [`../pics/2026-04-22_secure_mqtt_heltec_sent.png`](../pics/2026-04-22_secure_mqtt_heltec_sent.png)

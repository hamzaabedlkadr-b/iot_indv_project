# Secure MQTT Evidence - 2026-04-22

This run validates the secure MQTT rubric item with the real `Heltec WiFi LoRa 32 V3` firmware and the Python edge listener.

## Test Setup

| Item | Value |
| --- | --- |
| Broker | `broker.emqx.io` |
| Port | `8883` |
| Transport | `MQTTS` / TLS |
| Topic | `iot_indv_project/hamza/adaptive-sampling-node/secure` |
| Firmware mode | `MQTT_ONLY`, `clean_reference`, adaptive sampling enabled |
| TLS verification | required on the listener; ESP32 certificate validation enabled through the ESP-IDF certificate bundle |

Listener command:

```powershell
python source\edge_server\mqtt_listener\listen_aggregates.py --host broker.emqx.io --port 8883 --tls --topic iot_indv_project/hamza/adaptive-sampling-node/secure --client-id hamza-secure-edge-listener --csv source\results\summaries\secure_mqtt_run_2026-04-22.csv --jsonl source\results\summaries\secure_mqtt_run_2026-04-22.jsonl --limit 3
```

## Proof Screenshots

| Proof | Screenshot |
| --- | --- |
| Listener connected with `tls=enabled verify=required` and received secure aggregate messages | [`../pics/2026-04-22_secure_mqtt_listener_tls.png`](../pics/2026-04-22_secure_mqtt_listener_tls.png) |
| ESP32 WiFi connected, MQTT started after IP assignment, and broker certificate was validated | [`../pics/2026-04-22_secure_mqtt_cert_validated.png`](../pics/2026-04-22_secure_mqtt_cert_validated.png) |
| ESP32 heartbeat showed `wifi=1`, `mqtt=1`, no pending messages, and sent aggregates | [`../pics/2026-04-22_secure_mqtt_heltec_sent.png`](../pics/2026-04-22_secure_mqtt_heltec_sent.png) |

## Saved Output

Important listener lines:

```text
connecting to broker broker.emqx.io:8883 and waiting for topic iot_indv_project/hamza/adaptive-sampling-node/secure tls=enabled verify=required
subscribed to iot_indv_project/hamza/adaptive-sampling-node/secure on broker.emqx.io:8883 as hamza-secure-edge-listener tls=enabled
message window=9 profile=clean_reference fs=40.0Hz dominant=5.0Hz avg=-0.0 anomalies=0 listener_latency_us=624484 end_to_end_latency_us=956607
message window=10 profile=clean_reference fs=40.0Hz dominant=5.0Hz avg=0.0 anomalies=0 listener_latency_us=555125 end_to_end_latency_us=886945
message window=11 profile=clean_reference fs=40.0Hz dominant=5.0Hz avg=-0.0 anomalies=0 listener_latency_us=547704 end_to_end_latency_us=879133
```

Artifacts:

- [`summaries/secure_mqtt_listener_2026-04-22.out.txt`](./summaries/secure_mqtt_listener_2026-04-22.out.txt)
- [`summaries/secure_mqtt_run_final_2026-04-22.csv`](./summaries/secure_mqtt_run_final_2026-04-22.csv)
- [`summaries/secure_mqtt_summary_final_2026-04-22.md`](./summaries/secure_mqtt_summary_final_2026-04-22.md)
- [`summaries/secure_mqtt_summary_final_2026-04-22.json`](./summaries/secure_mqtt_summary_final_2026-04-22.json)

## Result

| Metric | Value |
| --- | ---: |
| Messages received | `3` |
| Unique windows | `3` |
| Missing windows | `0` |
| Average sampling frequency | `40.000 Hz` |
| Average dominant frequency | `5.000 Hz` |
| Average payload size | `446.333 B` |
| Average listener latency | `575,771 us` |
| Average end-to-end latency | `907,561.667 us` |

Conclusion: the project now has live proof that aggregate values are transmitted to an MQTT server using TLS, with certificate verification enabled.

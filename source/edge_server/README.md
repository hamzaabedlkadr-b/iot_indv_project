# Edge Server

This folder contains the nearby server-side pieces used for the assignment.

Current local-validation path:

- run or connect to an `MQTT` broker,
- receive aggregate values from the ESP32 over `WiFi`,
- log incoming payloads with receive timestamps,
- support latency and network-volume measurement before campus `LoRaWAN` testing.

Available tooling:

- [`mqtt_listener/listen_aggregates.py`](./mqtt_listener/listen_aggregates.py) subscribes to the aggregate topic and can write `CSV` and `JSONL` logs for Phase 8 and Phase 10.

Suggested workflow:

1. Start a local or LAN `MQTT` broker.
2. Run the listener on the machine acting as the nearby edge server.
3. Flash the ESP32 with `MQTT` enabled in `project_config.h`.
4. Verify that each completed window appears at the listener with timestamps.

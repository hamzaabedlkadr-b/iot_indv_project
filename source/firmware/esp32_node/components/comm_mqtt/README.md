# MQTT Communication

This module owns the working edge path used for local validation.

It currently:

- connects the board to WiFi and waits for a valid IP before starting MQTT
- connects to the local broker and retries cleanly after disconnects
- publishes one JSON aggregate per completed window
- adds both monotonic and Unix `SNTP` timestamps so the listener can compute real latency

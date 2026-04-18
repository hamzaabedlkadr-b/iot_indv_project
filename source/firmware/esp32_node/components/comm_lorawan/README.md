# LoRaWAN Communication

This module prepares the cloud-side payload path.

It currently:

- packs each aggregate into the compact binary payload intended for `TTN`
- logs a JSON preview alongside the binary payload for debugging
- keeps the newest prepared uplinks even when the queue is full
- stays in `stub_ready` mode until the board is tested near the campus gateway

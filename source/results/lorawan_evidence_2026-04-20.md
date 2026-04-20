# LoRaWAN Evidence Note

This note groups the `2026-04-20` artifacts that prove the integrated main application reached `TTN` through the Heltec `LoRaWAN` radio path.

## What This Run Proves

- The main `ESP-IDF` firmware joined the `LoRaWAN` network and reported `joined=1`.
- The aggregate produced by the same pipeline used for `MQTT` was packed into the compact `10-byte` uplink format.
- The radio queued the payload, completed transmission, and `TTN` received repeated uplinks on `FPort 1`.
- The cloud-side device page showed fresh activity, counters, and real RSSI/SNR metadata.

## Core Artifacts

- Main-app serial proof: [`../pics/2026-04-20_serial_lorawan_join_tx.png`](../pics/2026-04-20_serial_lorawan_join_tx.png)
- Payload serialization proof: [`../pics/2026-04-20_serial_lorawan_payload.png`](../pics/2026-04-20_serial_lorawan_payload.png)
- `TTN` Live Data proof: [`../pics/2026-04-20_ttn_live_data_uplink.png`](../pics/2026-04-20_ttn_live_data_uplink.png)
- `TTN` uplink details proof: [`../pics/2026-04-20_ttn_uplink_decoded.png`](../pics/2026-04-20_ttn_uplink_decoded.png)
- `TTN` device overview proof: [`../pics/2026-04-20_ttn_device_overview.png`](../pics/2026-04-20_ttn_device_overview.png)

## Key Facts To Cite

- Join state observed in serial: `joined=1`
- Confirmed radio action: `LoRaWAN uplink queued to radio` followed by `Event : Tx Done`
- Payload format: compact `10-byte` aggregate on `FPort 1`
- Cloud-side proof: repeated uplinks visible in `TTN` Live Data with RSSI/SNR metadata and device activity counters

## Suggested README Placement

- Use the serial `joined=1` screenshot in the detailed `LoRaWAN` explanation if a firmware-focused figure is needed.
- Use the `TTN` Live Data screenshot in the root README evidence gallery.
- Link this note anywhere the repository needs a single source of truth for the validated `LoRaWAN + TTN` evidence bundle.

# TTN Payloads

This folder documents the compact payload prepared by the current firmware.

Current payload layout:

- bytes `0-1`: `window_id` as unsigned 16-bit big-endian
- bytes `2-3`: `sample_count` as unsigned 16-bit big-endian
- bytes `4-5`: `sampling_frequency_hz * 10` as unsigned 16-bit big-endian
- bytes `6-7`: `dominant_frequency_hz * 100` as unsigned 16-bit big-endian
- bytes `8-9`: `average_value * 1000` as signed 16-bit big-endian

Current firmware behavior:

- prepares a hex representation of this payload for logs and queue inspection,
- drives the integrated Heltec `LoRaWAN` radio path from the main application when credentials are present,
- uses `FPort 1` by default.

Included offline artifact:

- [`ttn_decoder.js`](./ttn_decoder.js) decodes the current 10-byte aggregate payload inside `TTN`.

Validation bundle saved on `2026-04-20`:

- [`../../results/lorawan_evidence_2026-04-20.md`](../../results/lorawan_evidence_2026-04-20.md)
- [`../../pics/2026-04-20_serial_lorawan_join_tx.png`](../../pics/2026-04-20_serial_lorawan_join_tx.png)
- [`../../pics/2026-04-20_serial_lorawan_payload.png`](../../pics/2026-04-20_serial_lorawan_payload.png)
- [`../../pics/2026-04-20_ttn_live_data_uplink.png`](../../pics/2026-04-20_ttn_live_data_uplink.png)
- [`../../pics/2026-04-20_ttn_uplink_decoded.png`](../../pics/2026-04-20_ttn_uplink_decoded.png)
- [`../../pics/2026-04-20_ttn_device_overview.png`](../../pics/2026-04-20_ttn_device_overview.png)

If the payload layout changes later:

- update [`ttn_decoder.js`](./ttn_decoder.js),
- repeat the same screenshot set so the decoded values still match the serialized aggregate fields.

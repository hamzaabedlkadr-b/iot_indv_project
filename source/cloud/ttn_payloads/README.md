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
- keeps radio transmission disabled until the real `TTN` test on campus,
- uses `FPort 1` by default.

Included offline artifact:

- [`ttn_decoder.js`](./ttn_decoder.js) decodes the current 10-byte aggregate payload inside `TTN`.

Next step:

- verify during the campus test that the decoded `TTN` values match the original aggregate fields.

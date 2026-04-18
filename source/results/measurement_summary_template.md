# Measurement Summary Template

Use this file as the skeleton for the final evaluation write-up.

## Run Metadata

- Date:
- Location:
- Board:
- Firmware communication mode:
- Signal profile:
- Synthetic anomalies per window:
- Broker host:
- `TTN` gateway available:

## Baseline Configuration

- Sampling mode:
- Configured sampling frequency:
- Window duration:
- Communication path:

## Adaptive Configuration

- Adaptive rule:
- Minimum sampling frequency:
- Maximum sampling frequency:
- Window duration:
- Communication path:

## Required Results Table

| Metric | Baseline | Adaptive | Notes |
| --- | --- | --- | --- |
| Maximum stable sampling frequency |  |  |  |
| Average selected sampling frequency |  |  |  |
| Aggregate average correctness |  |  |  |
| End-to-end latency |  |  |  |
| Transmitted bytes |  |  |  |
| Energy or power measurement |  |  |  |

## Energy Measurement Method

- Instrument or method used:
- Whether the result is direct power measurement or indirect comparison:
- Limitations:

## MQTT Evidence

- CSV file:
- JSONL file:
- Summary report:
- Serial log:

## LoRaWAN / TTN Evidence

- Decoder used:
- Live data screenshot:
- Uplink timestamps:
- Notes:

## Interpretation

- Did adaptive sampling preserve signal quality?
- Did communication volume go down?
- Did latency change meaningfully?
- Did energy usage improve?

## Bonus Signal Matrix

| Signal profile | Notes | CSV / summary artifacts |
| --- | --- | --- |
| `clean_reference` |  |  |
| `noisy_reference` |  |  |
| `anomaly_stress` |  |  |

## Open Issues

- 

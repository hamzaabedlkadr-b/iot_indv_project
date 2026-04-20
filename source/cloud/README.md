# Cloud Integration

This folder tracks the `LoRaWAN + TTN` communication path.

Current state:

- the firmware now prepares compact aggregate uplink payloads from real analysis windows,
- the integrated main application has already transmitted those payloads through `TTN` on real hardware,
- the screenshot-backed validation bundle now lives in [`../results/lorawan_evidence_2026-04-20.md`](../results/lorawan_evidence_2026-04-20.md) and [`../pics/`](../pics/),
- the remaining cloud-side work is only to keep the decoder notes aligned if the payload layout changes later.

Evidence contents:

- `TTN` setup notes
- payload format documentation
- decoder examples
- saved serial and `TTN` screenshots proving successful uplinks

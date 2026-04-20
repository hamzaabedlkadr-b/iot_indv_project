# Screenshot Inventory

This file lists the evidence screenshots that were generated from the saved project artifacts.

## 2026-04-18 MQTT And Plotter Evidence

- `screenshots/mqtt_listener_received_message_2026-04-18.png`
  Caption: `Edge listener receiving real aggregate messages from the ESP32 over MQTT.`

- `screenshots/mqtt_summary_clean_profile_2026-04-18.png`
  Caption: `Summary of MQTT latency and payload metrics for the clean profile run.`

- `screenshots/signal_profile_comparison_2026-04-18.png`
  Caption: `Comparison of the three saved virtual signal profiles using real-board MQTT runs.`

- `screenshots/anomaly_profile_nonzero_count_2026-04-17.png`
  Caption: `Anomaly-stress signal profile producing windows with non-zero anomaly counts.`

## Existing Visual Evidence

- `../pics/2026-04-18_better_serial_plotter_live_view.png`
  Caption: `BetterSerialPlotter receiving the live adaptive-sampling stream from the Heltec board, showing sampling frequency, dominant frequency, and aggregate average.`

## 2026-04-20 LoRaWAN And TTN Evidence

- `../pics/2026-04-20_serial_lorawan_join_tx.png`
  Caption: `Integrated main-app serial log showing joined=1, LoRaWAN radio queuing, and Tx Done on the Heltec board.`

- `../pics/2026-04-20_serial_lorawan_payload.png`
  Caption: `Integrated main-app serial log showing the compact 10-byte LoRaWAN payload and the aggregate fields used to build it.`

- `../pics/2026-04-20_ttn_live_data_uplink.png`
  Caption: `TTN Live Data view showing fresh uplinks from the integrated main app on FPort 1.`

- `../pics/2026-04-20_ttn_uplink_decoded.png`
  Caption: `TTN event details showing the uplink message and raw frm_payload for the validated LoRaWAN transmission.`

- `../pics/2026-04-20_ttn_device_overview.png`
  Caption: `TTN device overview showing recent activity and repeated cloud-side visibility for the Heltec node.`

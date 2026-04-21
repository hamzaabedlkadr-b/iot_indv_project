# Repo Audit - 2026-04-18

This file is kept as a historical audit checkpoint. It records the gaps that were identified on `2026-04-18` and how they were closed before the final repository polish.

For the current submission state, use:

- [`SUBMISSION_SNAPSHOT.md`](./SUBMISSION_SNAPSHOT.md)
- [`GRADING_EVIDENCE_MATRIX.md`](./GRADING_EVIDENCE_MATRIX.md)
- [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md)

## Resolved Items

| Original audit item | Current status | Evidence |
| --- | --- | --- |
| LoRaWAN / TTN proof missing | Resolved | [`../results/lorawan_evidence_2026-04-20.md`](../results/lorawan_evidence_2026-04-20.md), TTN screenshots in [`../pics/`](../pics/) |
| Energy measurement missing | Resolved | [`../results/summaries/ina219_comparison_2026-04-21.md`](../results/summaries/ina219_comparison_2026-04-21.md), [`../results/summaries/ina219_three_mode_comparison_2026-04-21.md`](../results/summaries/ina219_three_mode_comparison_2026-04-21.md) |
| Communication-volume comparison incomplete | Resolved | [`../results/summaries/communication_volume_comparison_2026-04-21.md`](../results/summaries/communication_volume_comparison_2026-04-21.md) |
| Screenshot evidence incomplete | Resolved for core rubric | Raw sampling, MQTT, TTN, INA219, hardware, and plotter screenshots are linked from [`../../README.md`](../../README.md) |
| Secrets mixed with public config | Resolved | Public config uses placeholders; local credentials live in ignored `project_config_local.h` |
| Generated cache/build files visible | Resolved | `.gitignore` excludes PlatformIO builds, Python caches, logs, local configs, smoke-test workspaces, and reference material |
| Bonus signal evidence needed | Resolved | Three signal profiles plus anomaly-filter evaluation are linked from the final evidence index |

## Remaining Known Limitation

The only intentionally retained weaker item is live secure-MQTT proof. The firmware supports `MQTTS`, certificate verification, and optional credentials, but a saved live TLS broker run has not been captured yet.

Evidence / setup:

- [`SECURE_MQTT_SETUP.md`](./SECURE_MQTT_SETUP.md)

## Final Audit Conclusion

The original `2026-04-18` blockers have been closed except for optional live secure-MQTT validation. The repository is now organized around a clean submission path:

```text
README -> source/README -> evidence matrix -> final evidence index -> raw artifacts
```

This is the structure to use during the workshop and in the GitHub submission.

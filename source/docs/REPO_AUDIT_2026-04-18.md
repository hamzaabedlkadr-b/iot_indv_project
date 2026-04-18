# Repo Audit - 2026-04-18

This note records what is still missing, incomplete, or worth cleaning before the final submission.

It is intentionally stricter than the normal progress report: if a claim is not backed by a saved live run, a final comparison table, or a clean submission-ready artifact, it is treated as still open.

## 1. Core Technical Blockers

### 1.1 LoRaWAN / TTN Is Not Finished Yet

Current status:

- the firmware prepares compact LoRa-style payloads,
- the decoder exists in `cloud/ttn_payloads/ttn_decoder.js`,
- runtime logs prove payload preparation in `stub_ready` mode.

What is still missing:

- board-specific LoRaWAN / TTN radio transmission integration,
- real device credentials and final radio configuration,
- one live uplink under real gateway coverage,
- `TTN` console screenshots showing the uplink and decoded payload.

Important code-level reason:

- `components/comm_lorawan/comm_lorawan.c` still logs that the board-specific `TTN` driver is not integrated yet when radio TX is enabled.

Impact:

- the cloud path is only partially complete,
- the project should not be presented as fully validated for `LoRaWAN + TTN` yet.

### 1.2 Energy Measurement Is Still Missing

Current status:

- the method is documented,
- baseline/adaptive firmware switching exists,
- the `INA219` setup notes are prepared.

What is still missing:

- real baseline power or energy run,
- real adaptive power or energy run,
- repeated runs under the same conditions,
- final comparison table,
- final energy screenshots / setup photo.

Impact:

- the `energy savings` grading item is still open.

### 1.3 Secure MQTT Is Implemented But Not Proven

Current status:

- the firmware now supports `mqtt://` and `mqtts://`,
- certificate-bundle support and optional credentials are in place.

What is still missing:

- one live run against a TLS-capable broker,
- one saved proof artifact showing `MQTTS` in action.

Impact:

- the secure-MQTT grading item is still only partial.

## 2. Measurement Gaps

### 2.1 Communication-Volume Comparison Is Incomplete

Current status:

- payload size is measured for the adaptive runs,
- payload sizes are available for the three signal profiles.

What is still missing:

- one fixed-baseline run with `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U`,
- a final baseline-vs-adaptive comparison table,
- one short explanation that aggregate payload size remains almost constant because one compact summary is sent per window.

Impact:

- the `network volume` item is only partially supported.

### 2.2 Latency Is Measured, But Final Packaging Can Be Stronger

Current status:

- MQTT latency numbers are saved and usable,
- the summary artifacts are already present.

What is still missing:

- nothing critical in implementation,
- only final workshop packaging and figure placement.

Impact:

- not a blocker, but still worth presenting cleanly.

## 3. Evidence Still Missing

The repository already contains some screenshots, but several checklist items are still not represented by final saved figures.

### 3.1 Missing Or Still-Weaker Visual Evidence

Still missing or not yet clearly saved as final evidence:

- architecture / system-overview figure,
- maximum-stable-sampling benchmark screenshot,
- FFT and adaptive-update serial screenshot,
- aggregate-result screenshot,
- WiFi + MQTT connected serial screenshot,
- dedicated communication-volume screenshot,
- `TTN` uplink screenshot,
- `TTN` decoded-payload screenshot,
- energy setup photo,
- energy baseline graph,
- energy adaptive graph.

### 3.2 Bonus-Signal Evidence Is Good Enough

Already supported:

- clean profile summary,
- noisy profile summary,
- anomaly profile summary,
- non-zero anomaly screenshot,
- combined comparison screenshot.

Impact:

- the three-signal bonus path is already in good shape.

## 4. Documentation Gaps

### 4.1 Prompt Log Is Still Empty

Current status:

- the structure exists in `prompts/`,
- the template is ready.

What is still missing:

- the actual curated prompt history,
- reflections on accepted / corrected / rejected outputs.

Impact:

- not a firmware blocker,
- but it is still a final-submission gap if the assignment expects LLM usage documentation.

### 4.2 Cloud Folder Is Still Mostly Preparation Material

Current status:

- `cloud/README.md` and `cloud/ttn_payloads/README.md` describe the plan and decoder.

What is still missing:

- actual campus-test outputs,
- final `TTN` setup notes from the real run,
- uploaded console screenshots or logs.

### 4.3 App Display Is Still Placeholder-Only

Current status:

- `components/app_display/app_display.c` only prints serial heartbeats.

Impact:

- not a grading blocker for the core assignment,
- but the OLED path is still placeholder-level if someone expects a real board display.

## 5. Repo Hygiene Problems To Fix Before Sharing

### 5.1 Real Secrets Are Still In The Repo

Current status:

- `firmware/esp32_node/include/project_config.h` still contains the current WiFi SSID and password,
- the same file contains the local broker host,
- runtime notes also mention the home-network values.

What should be fixed:

- replace live credentials with placeholders before pushing or sharing,
- keep a private local copy outside the final public repo if needed.

### 5.2 Python Cache Files Should Not Be In The Final Repo

Current status:

- `source/edge_server/mqtt_listener/__pycache__/`
- `source/results/__pycache__/`
- several `*.pyc` files

What should be fixed:

- remove these generated files,
- add a `.gitignore` so they do not come back.

### 5.3 No `.gitignore` Was Found

Current status:

- the repo currently has no `.gitignore`.

Recommended additions:

- `__pycache__/`
- `*.pyc`
- `.pio/`
- `.vscode/`
- `*.log`
- local secret overrides if a private config file is added later.

### 5.4 Top-Level Workspace Noise

Current status:

- the workspace root includes files such as `imgui.ini` and `MAHI.log`.

Impact:

- not a project blocker,
- but they should stay out of the final submission unless they are intentionally part of the project.

## 6. What Is Already Strong

The following areas are already in a strong state:

- virtual signal generation,
- FFT / dominant-frequency estimation,
- adaptive sampling from `50 Hz` to `40 Hz`,
- aggregate computation,
- end-to-end `MQTT/WiFi` validation on real hardware,
- synchronized latency measurements,
- three-signal bonus support,
- BetterSerialPlotter live visualization,
- detailed current report in markdown, LaTeX, and PDF.

## 7. Priority Order From Here

If the goal is to maximize submission readiness, the best order is:

1. sanitize credentials and clean generated junk,
2. perform campus `LoRaWAN/TTN` integration and proof run,
3. collect the missing screenshots tied to the rubric,
4. run baseline-vs-adaptive energy measurements,
5. validate `MQTTS` if a TLS broker is available,
6. fill the final prompt log and short final report.

## 8. Honest Bottom Line

The repo is not missing the core pipeline anymore.

What is still missing is concentrated in:

- cloud validation,
- energy evaluation,
- secure-transport proof,
- a few final evidence assets,
- final repo hygiene before public sharing.

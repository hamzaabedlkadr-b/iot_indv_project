# Presentation Playbook

This file captures the presentation strategy we want to use for the final repository and workshop demo.

It is based on reviewing several public student repositories shared by the instructor:

- `albertoDonof/IoT-Individual-Assignment`
- `afk-codze/heltec-lora-signal-analytics`
- `MarceloJimenez/IoTSignalProcessing`
- `volpeffervescente/IOT-course-individual-assignment`

The goal here is not to copy their content, but to extract the strongest presentation patterns and apply them more cleanly in this repository.

## Main Goal

Present the project in a way that is:

- easy to follow quickly during the workshop
- aligned with the grading rubric
- backed by concrete evidence
- honest about what is already validated and what still requires campus testing

## Best Patterns To Reuse

### 1. Rubric-first structure

The strongest repositories make it easy to see how each assignment requirement is addressed.

What we should do:

- organize the main `README.md` in the same order as the assignment
- make each major section answer one grading item directly
- avoid burying key results in long narrative paragraphs

Recommended section order:

1. Project overview
2. Architecture / pipeline
3. Input signal and signal profiles
4. Maximum sampling frequency
5. FFT and optimal sampling frequency
6. Aggregate computation
7. MQTT over WiFi
8. LoRaWAN + TTN
9. Performance evaluation
10. Bonus signals
11. Setup and run guide
12. LLM usage / prompts / limitations

### 2. Phase-based storytelling

The cleanest repositories present the project as a sequence of phases or steps instead of a random collection of features.

What we should do:

- keep the code modular as it already is
- explain the firmware pipeline as a staged flow
- use short diagrams and checklists instead of too much prose

### 3. Evidence beside every claim

The best submission-style repos do not just explain what the system should do; they show screenshots, logs, plots, or result files.

What we should do:

- every major technical claim should point to one saved artifact
- every screenshot should have a short caption saying exactly what it proves
- result tables should reference the artifact filenames used to generate them

### 4. Clear separation between final implementation and side experiments

Some student repos become harder to follow because many sketches or prototypes coexist without a clear "final path."

What we should do:

- keep one clearly identified final implementation path
- label older or side experiments as intermediate or archived
- always say which code path was used for the actual final measurements

## What To Avoid

### 1. One huge README with no quick map

Long reports are useful, but only if the reader can find things quickly.

Avoid:

- long unstructured text walls
- repeating the same explanation in multiple sections
- hiding the setup steps after many screens of theory

### 2. Talking about planned work as if it is already done

This is risky in a workshop evaluation.

Avoid:

- saying LoRaWAN is "done" before the live campus test exists
- saying MQTT is secure before TLS is actually in place
- presenting future measurement work as completed evidence

### 3. Scattered evidence

Avoid:

- screenshots with unclear names
- result summaries not linked to the raw data
- graphs without units or captions

## Recommended Final README Style

The top-level `README.md` should feel like a guided tour, not a raw lab notebook.

Recommended opening structure:

1. one-paragraph project summary
2. one architecture figure or pipeline sketch
3. one short "current status" block
4. one short "where to find evidence" block

Then for each grading item:

- what the system does
- how we measured it
- what result we got
- what artifact proves it

## Workshop Presentation Strategy

The live explanation should follow the same order every time.

Recommended demo flow:

1. show the architecture and signal path
2. show that the board boots and identifies the active signal profile
3. show max sampling frequency evidence
4. show FFT result and adaptive rate update
5. show aggregate generation
6. show MQTT message reception and saved summary
7. show the three signal profiles and their saved summaries
8. show energy and latency evidence
9. show LoRaWAN / TTN evidence on campus

This keeps the presentation aligned with the rubric and avoids jumping around.

## How To Present Our Repo Better Than The Reference Repos

We already have an advantage: this repo is more modular and easier to extend.

To make that advantage visible:

- keep the repo structure clean and intentional
- use short cross-links between `docs/`, `results/`, and the firmware
- present one polished path instead of many partially overlapping sketches
- use saved result summaries and runtime notes to support the final README

## Current Repo Assets We Should Reuse In The Final Presentation

- `docs/IMPLEMENTATION_PLAN.md`
- `docs/MEASUREMENT_PLAN.md`
- `docs/RUNTIME_VALIDATION_CHECKLIST.md`
- `results/runtime_notes_2026-04-17.md`
- `results/summaries/`
- `edge_server/mqtt_listener/`
- `cloud/ttn_payloads/`

## Final README Checklist

Before submission, the main `README.md` should explicitly answer:

- what signal is used
- how maximum sampling frequency was measured
- how the optimal sampling rate is computed
- how the aggregate is computed
- how MQTT transmission works
- how LoRaWAN / TTN transmission works
- how energy was measured
- how latency was measured
- how communication volume was measured
- what the three signal profiles are
- which parts were validated at home and which on campus
- what prompts and LLM limitations are being reported

## Closing Rule

Whenever we add a new result, we should ask:

- what claim does this support?
- where is the raw artifact?
- where is the summarized artifact?
- where will the reader find it from the main README?

If those four answers are clear, the presentation will feel strong and deliberate.

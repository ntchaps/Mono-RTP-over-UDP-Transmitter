# Architecture

This document explains the current structure of the Mono RTP over UDP Transmitter project.

The project has been refactored so that `main.c` stays focused on STM32 startup code, while the project-specific logic lives in separate application files. This makes the firmware easier to read, debug, and expand as the project moves from basic UDP transmission toward RTP-based mono audio streaming.

## Overview

The project uses an **STM32 Nucleo-F446RE** with a **W5500 Ethernet module**.

The STM32 generates or prepares audio data, formats it into packets, and sends it to the W5500 over SPI. The W5500 handles the Ethernet and UDP transmission.

```text
Audio source
    |
    | current: generated sine wave / test data
    | future: external audio ADC
    v
STM32 Nucleo-F446RE
    |
    | packet formatting / timing
    v
W5500 Ethernet Module
    |
    | UDP over Ethernet
    v
PC receiver / Wireshark
```

## Design Goals

The refactor was done to:

* Keep `main.c` simple
* Keep STM32CubeMX-generated code mostly untouched
* Move project-specific logic into separate modules
* Make the code easier to debug
* Make future RTP and real audio input support easier to add

## Current Firmware Flow

`main.c` now acts mostly as the firmware entry point.

The intended flow is:

```text
main.c
  |
  |-- HAL_Init()
  |-- SystemClock_Config()
  |-- CubeMX peripheral init
  |
  |-- App_Init()
  |
  v
while (1)
  |
  v
App_Run()
```

This keeps startup code separate from the main project behavior.

## Current File Structure

```text
Core/
├── Inc/
│   ├── main.h
│   ├── app.h
│   └── ...
│
├── Src/
│   ├── main.c
│   ├── app.c
│   └── ...
```

## Module Roles

### `main.c`

Handles STM32 startup and generated initialization code.

This file is responsible for:

* HAL initialization
* Clock configuration
* CubeMX-generated peripheral initialization
* Calling `App_Init()`
* Calling `App_Run()` inside the main loop

Most project-specific behavior should stay out of `main.c`.

### `app.c` / `app.h`

Controls the main application logic.

This module is responsible for:

* Project-specific initialization
* W5500 setup
* UDP transmit behavior
* Main application loop behavior
* Keeping the custom firmware logic separate from generated STM32 code

As the project grows, more logic can be moved out of `app.c` into smaller modules.

## Planned Module Split

The current refactor created the main application layer. Future cleanup can split the application code into more focused modules.

Possible future files:

```text
Core/
├── Inc/
│   ├── audio_sine.h
│   ├── udp_stream.h
│   ├── rtp_packet.h
│   └── w5500_port.h
│
├── Src/
│   ├── audio_sine.c
│   ├── udp_stream.c
│   ├── rtp_packet.c
│   └── w5500_port.c
```

## Future Module Roles

### `audio_sine.c` / `audio_sine.h`

Would handle generated test audio data.

This module would be responsible for:

* Creating sine wave sample data
* Filling audio buffers
* Providing predictable test data before real audio input is added

### `udp_stream.c` / `udp_stream.h`

Would handle UDP transmission.

This module would be responsible for:

* Destination IP and port configuration
* UDP socket setup
* Sending packet buffers through the W5500

### `rtp_packet.c` / `rtp_packet.h`

Would handle RTP packet formatting.

This module would be responsible for:

* RTP sequence numbers
* RTP timestamps
* Payload type
* SSRC
* Combining RTP headers with audio payload data

### `w5500_port.c` / `w5500_port.h`

Would isolate the W5500 hardware interface.

This module would be responsible for:

* SPI read/write callbacks
* Chip select control
* W5500 reset logic if needed
* Connecting the W5500 library to STM32 HAL functions

## Data Flow

The current and planned data flow is:

```text
audio/test data
    |
    v
application logic
    |
    v
UDP packet buffer
    |
    v
W5500 over SPI
    |
    v
Ethernet / UDP
    |
    v
PC receiver or Wireshark
```

Once RTP support is added, the flow will become:

```text
audio/test data
    |
    v
RTP packet builder
    |
    v
UDP sender
    |
    v
W5500 over SPI
    |
    v
PC receiver or Wireshark
```

## Refactor Status

* [x] Create `app.c` and `app.h`
* [x] Move application-level flow out of `main.c`
* [x] Keep `main.c` focused on startup and initialization
* [ ] Split W5500 setup into its own module
* [ ] Split UDP sending into its own module
* [ ] Split sine wave generation into its own module
* [ ] Add RTP packet builder module
* [ ] Add real audio input later

## Main Rule

```text
main.c starts the system.
app.c controls the application.
future modules should handle audio, UDP, RTP, and W5500 details.
```

The current refactor is a good first step because it separates generated startup code from custom project behavior. Future refactors can continue breaking `app.c` into smaller modules as the project grows.

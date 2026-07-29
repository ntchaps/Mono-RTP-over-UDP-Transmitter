# Firmware Architecture

This document describes how the custom firmware is organized and how data moves through the project.

The main goal of the refactor is to keep hardware control, networking, audio, and application logic in separate modules.

## Current File Structure

```text
Core/
├── Inc/
│   ├── app.h
│   ├── audio.h
│   ├── audio_sine.h
│   ├── debug_uart.h
│   ├── network.h
│   ├── udp_stream.h
│   └── w5500_port.h
│
└── Src/
    ├── app.c
    ├── audio.c
    ├── audio_sine.c
    ├── debug_uart.c
    ├── main.c
    ├── network.c
    ├── udp_stream.c
    └── w5500_port.c
```

STM32CubeMX-generated files remain in their normal locations.

## Module Responsibilities

### `main.c`

Handles STM32 startup and CubeMX-generated peripheral initialization.

After initialization, it calls the application module.

### `app.c` / `app.h`

Controls the top-level application flow.

It initializes the custom modules and decides when data should be generated or transmitted.

Temporary test packets also belong here.

### `audio.c` / `audio.h`

Controls the audio peripherals.

This includes:

* Starting the DAC
* Starting DMA
* Starting the sample timer
* Selecting the audio sample buffer

### `audio_sine.c` / `audio_sine.h`

Stores the sine-wave lookup table used for testing.

Keeping the lookup table in its own module separates test sample data from DAC and DMA control.

### `network.c` / `network.h`

Configures the W5500 network interface.

This includes:

* W5500 initialization
* Socket memory allocation
* MAC address
* Local IP address
* Subnet mask
* Gateway

This module describes the transmitter's local network configuration.

### `udp_stream.c` / `udp_stream.h`

Manages the UDP socket used for transmission.

This includes:

* W5500 socket selection
* Local UDP port
* Destination IP address
* Destination UDP port
* Socket initialization
* Packet transmission

The module accepts arbitrary packet buffers, allowing it to send test data now and RTP packets later.

### `w5500_port.c` / `w5500_port.h`

Connects the Wiznet ioLibrary to the STM32 HAL.

This includes:

* SPI communication
* Chip-select control
* W5500 reset control
* Wiznet callback registration

### `debug_uart.c` / `debug_uart.h`

Provides UART debug output.

This keeps UART-specific code separate from the application and networking modules.

## Current Data Flow

The current audio test path is:

```text
audio_sine
    ↓
audio
    ↓
DAC + DMA
    ↓
Analog output
```

The current UDP test path is:

```text
app
    ↓
udp_stream
    ↓
Wiznet socket API
    ↓
w5500_port
    ↓
W5500
    ↓
PC receiver
```

The audio output and UDP test paths are currently separate.

## Planned RTP Data Flow

The planned audio transmission path is:

```text
Audio samples
    ↓
RTP packet builder
    ↓
udp_stream
    ↓
W5500
    ↓
PC receiver
```

A future `rtp_packet.c` and `rtp_packet.h` module will create RTP packets before passing them to `UDP_Stream_Send()`.

## Planned RTP Module

### `rtp_packet.c` / `rtp_packet.h`

Will format audio samples as RTP packets.

This will include:

* RTP header creation
* Sequence number tracking
* Timestamp tracking
* Payload type
* SSRC
* Audio payload placement

The completed RTP packet will be passed to the UDP stream module.

## Module Dependencies

```text
main
  └── app
      ├── audio
      │   └── audio_sine
      ├── network
      │   └── w5500_port
      ├── udp_stream
      └── debug_uart
```

`udp_stream` uses the Wiznet socket API, which communicates with the W5500 through callbacks registered by `w5500_port`.

## Configuration Ownership

### `network.c`

Owns:

* Local MAC address
* Local IP address
* Subnet mask
* Gateway
* W5500 socket memory configuration

### `udp_stream.c`

Owns:

* Socket number
* Local UDP port
* Destination IP address
* Destination UDP port

### `audio_sine.c`

Owns:

* Sine-wave lookup table
* Lookup-table length

### Future `rtp_packet.c`

Will own:

* RTP sequence number
* RTP timestamp
* Payload type
* SSRC

## Refactor Status

* [x] Move application logic out of `main.c`
* [x] Create `app.c` and `app.h`
* [x] Move W5500 hardware access into `w5500_port.c`
* [x] Move local network configuration into `network.c`
* [x] Move UDP socket handling into `udp_stream.c`
* [x] Move DAC and DMA control into `audio.c`
* [x] Move sine-wave data into `audio_sine.c`
* [x] Separate UART debugging into `debug_uart.c`
* [ ] Connect audio samples to the UDP transmit path
* [ ] Add fixed-rate packet transmission
* [ ] Create the RTP packet module
* [ ] Verify RTP packets in Wireshark
* [ ] Add real audio input

## Summary

```text
main.c
    Starts the firmware

app.c
    Controls the application

audio.c
    Controls the audio peripherals

audio_sine.c
    Stores sine-wave test samples

network.c
    Configures the local network

udp_stream.c
    Manages and sends UDP packets

w5500_port.c
    Communicates with the W5500 hardware

debug_uart.c
    Provides debug output

rtp_packet.c
    Will build RTP packets
```

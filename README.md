# Mono RTP over UDP Transmitter

An embedded audio-networking project that uses an STM32 Nucleo-F446RE and a W5500 Ethernet controller to transmit mono audio as RTP packets over UDP.

The firmware currently generates a sine-wave test signal, converts it to signed 16-bit PCM, packages the samples into RTP packets, and sends the stream to a PC over Ethernet. Wireshark is used to verify packet timing, sequence numbers, timestamps, and payload data.

## Project Goals

* Build a real-time mono audio transmitter
* Implement RTP packetization over UDP
* Interface an STM32 microcontroller with a W5500 Ethernet controller
* Practice embedded C, SPI, DMA, timers, networking, and modular firmware design
* Verify real-time packet transmission using Wireshark
* Eventually replace generated test data with external digital audio input

## Current Features

* [x] Sine-wave generation using a lookup table
* [x] DAC output using DMA and a hardware timer
* [x] Conversion from unsigned 12-bit DAC samples to signed 16-bit PCM
* [x] SPI communication between the STM32 and W5500
* [x] Static MAC and IPv4 configuration
* [x] UDP socket initialization and packet transmission
* [x] RTP version 2 header generation
* [x] RTP sequence-number tracking
* [x] RTP timestamp tracking
* [x] RTP SSRC and dynamic payload type
* [x] Fixed-rate transmission of 160 samples every 20 ms
* [x] RTP packet verification using Wireshark
* [x] UART debug output
* [x] Modular separation of application, audio, RTP, UDP, network, and hardware-interface code

## In Progress

* [ ] Build a PC receiver that plays the RTP audio stream
* [ ] Add stream discovery and receiver subscription
* [ ] Replace the generated sine wave with external audio input
* [ ] Add I2S audio capture
* [ ] Support 24-bit, 48 kHz mono audio
* [ ] Improve packet scheduling using timer- or DMA-driven buffering
* [ ] Add configurable destination IP, port, sample rate, and payload format

## Current Audio Format

The current RTP test stream uses:

```text
Format:             Signed 16-bit linear PCM
Channels:           1
Sample rate:        8 kHz
Samples per packet: 160
Packet interval:    20 ms
RTP payload type:   96
RTP header size:    12 bytes
Audio payload size: 320 bytes
UDP payload size:   332 bytes
```

The 12-bit sine lookup table is also used for DAC output. Before network transmission, each sample is centered around zero and scaled into a signed 16-bit PCM range.

## Firmware Structure

```text
Core/
├── Inc/
│   ├── app.h
│   ├── audio.h
│   ├── audio_sine.h
│   ├── debug_uart.h
│   ├── network.h
│   ├── rtp.h
│   ├── udp_stream.h
│   └── w5500_port.h
│
└── Src/
    ├── main.c
    ├── app.c
    ├── audio.c
    ├── audio_sine.c
    ├── debug_uart.c
    ├── network.c
    ├── rtp.c
    ├── udp_stream.c
    └── w5500_port.c
```

### Main modules

* `main.c` — STM32 startup and CubeMX-generated peripheral initialization
* `app.c` — Top-level initialization, packet scheduling, and application control
* `audio.c` — General audio initialization
* `audio_sine.c` — DAC sine-wave output and signed PCM sample generation
* `rtp.c` — RTP header construction, PCM serialization, sequence numbers, and timestamps
* `udp_stream.c` — UDP socket management and destination configuration
* `network.c` — W5500 network configuration
* `w5500_port.c` — STM32 HAL interface for W5500 SPI and chip select
* `debug_uart.c` — UART debug output

## Data Flow

### Analog test output

```text
Sine lookup table
        |
        v
DAC DMA
        |
        v
Analog sine-wave output
```

### RTP network stream

```text
Sine lookup table
        |
        v
Signed PCM conversion
        |
        v
RTP packet builder
        |
        v
UDP socket
        |
        v
W5500 over SPI
        |
        v
Ethernet
        |
        v
PC receiver / Wireshark
```

## RTP Packet Layout

Each UDP payload contains one RTP packet:

```text
Bytes 0-1     RTP version, flags, marker, and payload type
Bytes 2-3     Sequence number
Bytes 4-7     Timestamp
Bytes 8-11    SSRC
Bytes 12-331  160 signed 16-bit PCM samples
```

The sequence number increases by one per packet. The timestamp increases by 160 because each packet represents 160 audio samples.

## Network Configuration

The current implementation uses statically configured network information.

The W5500 has its own local IP address, while `udp_stream.c` contains the destination PC address and UDP port.

```text
Local UDP port:       5000
Destination UDP port: 8080
```

The destination IP must match the receiving PC's IPv4 address.

## Verification

The RTP stream can be inspected in Wireshark using:

```text
udp.port == 8080
```

If Wireshark initially identifies the packets only as UDP, use **Analyze → Decode As → RTP**.

A valid stream should show:

```text
Packet interval:       approximately 20 ms
Sequence increment:    1
Timestamp increment:   160
RTP payload type:      96
RTP payload length:    320 bytes
UDP payload length:    332 bytes
```

## Hardware

* STM32 Nucleo-F446RE
* W5500 Ethernet module
* Ethernet-connected PC
* UART connection for debugging
* Optional oscilloscope or speaker circuit for DAC testing

## Technologies and Skills

* Embedded C
* STM32 HAL
* STM32CubeIDE and STM32CubeMX
* SPI
* UART
* DAC
* DMA
* Hardware timers
* UDP sockets
* RTP packet formatting
* PCM audio representation
* W5500 Ethernet controller
* Wireshark packet analysis
* Modular firmware architecture

## Documentation

See [`docs/architecture.md`](docs/architecture.md) for module responsibilities and the complete firmware data flow.

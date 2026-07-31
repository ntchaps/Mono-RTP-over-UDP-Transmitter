# Architecture

This document describes the current firmware architecture of the Mono RTP over UDP Transmitter.

The project uses an STM32 Nucleo-F446RE and a W5500 Ethernet controller to generate a mono sine-wave test signal and transmit it as signed 16-bit PCM audio in RTP packets over UDP.

## System Overview

```text
Sine-wave source
        |
        v
STM32 Nucleo-F446RE
        |
        | PCM generation
        | RTP packetization
        | packet scheduling
        v
W5500 Ethernet controller
        |
        | UDP over IPv4
        v
PC receiver / Wireshark
```

The STM32 handles audio generation, RTP formatting, timing, and application control. The W5500 handles Ethernet, IPv4, ARP, and UDP socket transmission.

## Design Goals

The firmware is organized to:

* Keep STM32CubeMX-generated startup code mostly unchanged
* Keep `main.c` focused on system initialization
* Separate audio generation from packet formatting
* Separate RTP formatting from UDP transmission
* Isolate the W5500 hardware interface
* Make future I2S input and PC receiver support easier to add
* Allow each module to be tested and debugged independently

## Startup and Main Loop

```text
main.c
  |
  |-- HAL_Init()
  |-- SystemClock_Config()
  |-- CubeMX peripheral initialization
  |-- App_Init()
  |
  v
while (1)
  |
  v
App_Run()
```

`main.c` remains the firmware entry point. Project-specific behavior is controlled by the application layer.

## Current File Structure

```text
Core/
├── Inc/
│   ├── app.h
│   ├── audio.h
│   ├── audio_sine.h
│   ├── debug_uart.h
│   ├── main.h
│   ├── network.h
│   ├── rtp.h
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
    ├── rtp.c
    ├── udp_stream.c
    └── w5500_port.c
```

The WIZnet ioLibrary is included separately as a Git submodule.

## Module Responsibilities

### `main.c`

Handles STM32 startup and generated initialization.

Responsibilities:

* HAL initialization
* Clock configuration
* GPIO initialization
* SPI initialization
* UART initialization
* DAC initialization
* Timer initialization
* Calling `App_Init()`
* Calling `App_Run()` continuously

Project-specific networking and audio behavior should remain outside this file.

### `app.c` / `app.h`

Controls the top-level application.

Responsibilities:

* Initializing the audio system
* Initializing the W5500 network
* Opening the UDP socket
* Initializing RTP state
* Scheduling one audio packet every 20 ms
* Requesting the next sine-wave sample block
* Passing the sample block to the RTP module
* Reporting transmission failures through UART

The application layer connects the individual modules without implementing their internal details.

### `audio.c` / `audio.h`

Provides general audio initialization.

Responsibilities:

* Starting the selected audio test source
* Keeping general audio setup separate from the application layer
* Providing a future integration point for I2S or external audio input

### `audio_sine.c` / `audio_sine.h`

Provides the current test signal.

Responsibilities:

* Storing the 128-entry sine lookup table
* Starting DAC output through DMA
* Starting the hardware timer used for DAC triggering
* Tracking the lookup-table position for network audio
* Converting unsigned 12-bit DAC values into signed 16-bit PCM
* Filling an audio packet buffer with sine-wave samples

The lookup-table values range from approximately `0` to `4095` and are centered around `2048`.

For network transmission, each sample is converted approximately as:

```text
signed PCM = (DAC sample - 2048) × 8
```

This produces a signed range near `-16384` to `+16376`.

### `rtp.c` / `rtp.h`

Builds and sends RTP audio packets.

Responsibilities:

* Creating the fixed 12-byte RTP version 2 header
* Setting the dynamic payload type
* Writing the sequence number
* Writing the RTP timestamp
* Writing the SSRC
* Converting 16-bit PCM samples to network byte order
* Appending PCM payload data
* Passing completed packets to `UDP_Stream_Send()`
* Increasing the sequence number after a successful send
* Increasing the timestamp by the number of transmitted samples

The RTP module receives already prepared signed PCM samples. It does not know whether those samples came from a sine-wave generator, ADC, microphone, or I2S peripheral.

### `udp_stream.c` / `udp_stream.h`

Manages the UDP transport layer.

Responsibilities:

* Selecting the W5500 hardware socket
* Opening the socket in UDP mode
* Configuring the local UDP port
* Storing the destination IP and destination port
* Verifying that the socket is in the `SOCK_UDP` state
* Reopening the socket if it becomes closed
* Calling the WIZnet `sendto()` function

The current configuration uses:

```text
W5500 socket:          0
Local UDP port:        5000
Destination UDP port:  8080
Destination IP:        configured in udp_stream.c
```

UDP is connectionless, but every datagram still requires a destination IP address and destination port.

### `network.c` / `network.h`

Configures the W5500 network interface.

Responsibilities:

* Registering W5500 hardware callbacks
* Initializing the Ethernet controller
* Setting the MAC address
* Setting the local IPv4 address
* Setting the subnet mask
* Setting the gateway
* Reading back or printing network configuration for debugging

This module configures the local network identity. It does not build RTP packets or manage the audio destination.

### `w5500_port.c` / `w5500_port.h`

Connects the WIZnet ioLibrary to STM32 HAL.

Responsibilities:

* SPI byte transfer
* SPI burst transfer
* Chip-select control
* W5500 reset control, if used
* Registration of hardware callback functions

This module isolates hardware-specific W5500 access from higher-level networking code.

### `debug_uart.c` / `debug_uart.h`

Provides debug output.

Responsibilities:

* Formatted UART messages
* Network configuration output
* Socket and transmission error reporting
* Runtime diagnostic information

## Audio Data Paths

The project currently has two related but independent audio paths.

### DAC path

```text
Wave_LUT
    |
    v
HAL_DAC_Start_DMA()
    |
    v
DAC channel 1
    |
    v
Analog output
```

The DAC path uses the original unsigned 12-bit values.

### RTP path

```text
Wave_LUT
    |
    v
Audio_Sine_Generate()
    |
    | subtract 2048
    | scale by 8
    v
Signed int16_t PCM buffer
    |
    v
RTP_SendAudio()
    |
    v
UDP_Stream_Send()
    |
    v
W5500
    |
    v
Ethernet
```

The DAC and RTP paths use the same waveform shape but maintain separate playback positions. They are not required to remain phase-synchronized.

## Packet Timing

The current stream uses an 8 kHz sample rate and sends 160 samples per packet.

```text
160 samples / 8000 samples per second = 0.020 seconds
```

Therefore:

```text
Packet interval:    20 ms
Packets per second: 50
```

The application schedules packet transmission using `HAL_GetTick()`.

This approach is suitable for initial testing. A future version may use timer interrupts, DMA callbacks, or double buffering for more precise packet timing.

## RTP Packet Format

```text
Byte 0
    Version:    2
    Padding:    0
    Extension:  0
    CSRC count: 0

Byte 1
    Marker:       0
    Payload type: 96

Bytes 2-3
    Sequence number

Bytes 4-7
    Timestamp

Bytes 8-11
    SSRC

Bytes 12-331
    160 signed 16-bit PCM samples
```

All multibyte RTP fields and PCM samples are written in big-endian network byte order.

## Packet Sizes

```text
RTP header:            12 bytes
Samples per packet:   160
Bytes per sample:       2
Audio payload:        320 bytes
Complete RTP packet:  332 bytes
UDP header:             8 bytes
UDP length field:      340 bytes
```

IP and Ethernet headers are added by the W5500.

## RTP State

The RTP module maintains three stream-level values.

### Sequence number

The sequence number increases once per successfully transmitted packet.

```text
0, 1, 2, 3, ...
```

A receiver can use sequence gaps to detect packet loss.

### Timestamp

The timestamp increases by the number of audio samples in each packet.

```text
0, 160, 320, 480, ...
```

The timestamp uses the audio sample clock rather than milliseconds.

### SSRC

The SSRC identifies the RTP synchronization source and remains constant during the stream.

The current implementation uses a fixed test value. A later implementation may randomize the initial SSRC, sequence number, and timestamp.

## Complete Initialization Order

```text
Audio_Init()
    |
    v
Network_Init()
    |
    v
UDP_Stream_Init()
    |
    v
RTP_Init()
    |
    v
Begin scheduled RTP transmission
```

The network must be configured before the UDP socket is opened. RTP initialization should occur once when the stream begins, not before every packet.

## Wireshark Verification

The stream can be filtered using:

```text
udp.port == 8080
```

Because RTP does not have a mandatory UDP port and the project currently has no SDP signaling, Wireshark may initially classify the packets as UDP.

Use:

```text
Analyze
    -> Decode As
    -> RTP
```

A valid stream should show:

```text
RTP version:          2
Payload type:         96
Packet interval:      approximately 20 ms
Sequence increment:   1
Timestamp increment:  160
Payload length:       320 bytes
UDP payload length:   332 bytes
```

## Current Limitations

* Destination IP and port are compiled into the firmware
* No automatic discovery or subscription protocol
* No PC playback application yet
* No SDP description for dynamic RTP payload type 96
* No RTCP support
* No packet-loss recovery
* No jitter buffer
* No external audio input
* Packet scheduling currently depends on the millisecond HAL tick
* Initial RTP state uses fixed test values

## Planned Expansion

```text
Current sine-wave source
        |
        v
PC RTP receiver and playback
        |
        v
Automatic discovery and subscription
        |
        v
External I2S audio input
        |
        v
24-bit, 48 kHz mono streaming
```

A future discovery module could advertise the STM32 stream and allow the PC receiver to send a subscription request. The STM32 could then learn the receiver's IP address and UDP port instead of relying on a hard-coded destination.

## Architecture Rule

```text
main.c starts the system.
app.c coordinates the application.
audio modules produce PCM samples.
rtp.c formats audio packets.
udp_stream.c sends datagrams.
network.c configures the W5500.
w5500_port.c handles hardware access.
```

Each module should remain focused on one responsibility so that future audio sources, receiver software, and network-control features can be added without rewriting the entire firmware.

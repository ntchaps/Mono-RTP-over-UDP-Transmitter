# Firmware Architecture

This document describes how the custom firmware is organized and how audio moves from the PCM1808 input to the RTP receiver.

The project keeps application control, audio capture, packet formatting, networking, and hardware access in separate modules. STM32CubeMX-generated startup and peripheral code remains in its normal files.

## File Structure

```text
Core/
├── Inc/
│   ├── app.h
│   ├── audio.h
│   ├── audio_input.h
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
    ├── audio_input.c
    ├── audio_sine.c
    ├── debug_uart.c
    ├── network.c
    ├── rtp.c
    ├── udp_stream.c
    └── w5500_port.c

Receiver/
├── README.md
├── requirements.txt
└── rtp_receiver.py
```

## Module Responsibilities

### `main.c`

Handles STM32 startup and CubeMX-generated initialization.

Its main responsibilities are:

* Reset and HAL startup
* System clock configuration
* GPIO initialization
* DMA initialization
* SPI initialization for the W5500
* I2S initialization for the PCM1808
* UART initialization for debug output
* Calling `App_Init()` once
* Calling `App_Run()` continuously

Custom streaming logic should remain outside this file so CubeMX regeneration is less likely to overwrite it.

### `app.c` / `app.h`

Controls the top-level application flow.

`App_Init()` initializes the custom subsystems in this order:

```text
network
   ↓
UDP stream
   ↓
RTP state
   ↓
audio input
```

`App_Run()` checks the DMA half-buffer flags. When a completed half is available, it:

1. Gets a pointer to the completed I2S buffer half.
2. Extracts one channel from each input frame.
3. Stores 480 mono signed 16-bit samples in `audio_packet`.
4. Passes the sample block to `RTP_SendAudio()`.
5. Updates send-attempt, success, and failure counters.

The application layer connects the audio subsystem to the transport subsystem without directly controlling the W5500 hardware.

### `audio.c` / `audio.h`

Provides the main audio initialization entry point.

The current `Audio_Init()` function starts the PCM1808 input path by calling `Audio_Input_Init()`.

This module can later become the common location for selecting between live input, test signals, or other audio sources.

### `audio_input.c` / `audio_input.h`

Owns the live digital-audio capture path.

Its responsibilities include:

* Allocating the circular I2S DMA buffer
* Starting `HAL_I2S_Receive_DMA()`
* Tracking DMA start status
* Tracking half-complete, full-complete, and error callback counts
* Marking the first or second buffer half as ready
* Returning pointers to completed buffer halves

The configured buffer represents two blocks of 480 I2S frames:

```text
Frames per half:       480
Words per frame:       4
Words per half:        1,920
Total buffer words:    3,840
```

The four 16-bit words per frame represent the incoming stereo sample slots as delivered by the STM32 I2S peripheral and DMA configuration.

The DMA callbacks do not build or transmit packets. They only update counters and readiness flags so the main application can process the completed memory safely.

### `audio_sine.c` / `audio_sine.h`

Contains the earlier sine-wave test support.

This module was useful for validating DAC output and the original generated-audio path. It is no longer the primary input source for network transmission, but it remains useful as a known test signal.

### `rtp.c` / `rtp.h`

Builds and sends RTP packets.

It owns:

* RTP sequence number
* RTP timestamp
* RTP SSRC
* RTP header construction
* PCM payload serialization

Each packet uses a standard 12-byte RTP header:

```text
Bytes 0-1       Version, flags, marker, and payload type
Bytes 2-3       Sequence number
Bytes 4-7       Timestamp
Bytes 8-11      SSRC
Bytes 12+       Signed 16-bit PCM payload
```

The current packet contains 480 samples. Each sample is serialized most-significant byte first, producing a 960-byte audio payload and a 972-byte RTP packet.

The sequence number and timestamp advance only after `UDP_Stream_Send()` reports success:

```text
sequence number += 1
timestamp       += sample count
```

For a normal packet, the timestamp increment is 480.

### `udp_stream.c` / `udp_stream.h`

Owns the UDP transport configuration.

Its responsibilities include:

* Selecting the W5500 socket number
* Setting the local UDP port
* Setting the destination IP address
* Setting the destination UDP port
* Opening the socket
* Reopening the socket if it was closed
* Passing completed packet buffers to `sendto()`

The module does not interpret RTP or PCM. It accepts an arbitrary byte buffer and sends it to the configured receiver.

### `network.c` / `network.h`

Configures the W5500 network interface.

It owns:

* W5500 initialization
* Wiznet socket memory allocation
* Static MAC address
* Static IPv4 address
* Subnet mask
* Gateway
* Readback and debug printing of the configured address

This module describes the transmitter's local network identity. It does not own the remote receiver address.

### `w5500_port.c` / `w5500_port.h`

Connects the Wiznet ioLibrary to the STM32 HAL.

Its responsibilities include:

* SPI byte and burst transfers
* Chip-select control
* W5500 reset control
* Critical-section callbacks
* Registration of Wiznet interface callbacks

This is the lowest custom hardware layer in the Ethernet path.

### `debug_uart.c` / `debug_uart.h`

Provides formatted UART debug output.

It keeps UART-specific code out of the application, RTP, and network modules.

### `Receiver/rtp_receiver.py`

Implements the PC side of the stream.

Its responsibilities include:

* Binding a UDP socket to port 8080
* Receiving RTP packets
* Checking RTP version and payload type
* Tracking sequence gaps
* Parsing timestamps and SSRC values
* Extracting the PCM payload
* Converting network-order PCM to host byte order
* Buffering packets in a thread-safe queue
* Playing the audio with `sounddevice`

## Complete Data Flow

```text
Analog audio
    ↓
PCM1808 ADC
    ↓
I2S stereo sample frames
    ↓
SPI2 I2S peripheral
    ↓
Circular DMA buffer
    ↓
Half-transfer or full-transfer callback
    ↓
Ready flag in audio_input
    ↓
App_Run()
    ↓
One-channel extraction
    ↓
480-sample mono int16_t buffer
    ↓
RTP_SendAudio()
    ↓
12-byte RTP header + 960-byte PCM payload
    ↓
UDP_Stream_Send()
    ↓
Wiznet socket API
    ↓
w5500_port callbacks
    ↓
W5500 over SPI
    ↓
Ethernet network
    ↓
Python UDP socket
    ↓
RTP validation and PCM byte swap
    ↓
Audio queue
    ↓
sounddevice output stream
```

## Buffering Model

### STM32 Input Buffer

The input DMA operates as a circular double buffer.

```text
DMA buffer
├── First half  -> 480 audio frames
└── Second half -> 480 audio frames
```

While DMA fills one half, the application can process the other half.

This reduces the chance of overwriting samples during packet construction, although the main loop must still process each completed half before DMA reaches it again.

### Application Packet Buffer

`app.c` contains one mono packet buffer:

```text
480 samples × 2 bytes = 960 bytes
```

The conversion function currently selects one 16-bit word from each I2S frame:

```c
output[frame] = (int16_t)input[input_index];
```

The selected word and bit alignment must match the PCM1808 output format and the STM32 I2S configuration. If the waveform is shifted or distorted, the selected word index or bit assembly may need adjustment.

### RTP Packet Buffer

`rtp.c` uses a static packet buffer large enough for:

```text
12-byte RTP header
960-byte PCM payload
972 bytes total
```

## Timing Model

At 48 kHz:

```text
480 samples / 48,000 samples per second = 10 ms
```

Therefore, the intended packet rate is:

```text
100 packets per second
```

The input DMA naturally completes one 480-frame half-buffer every 10 ms when the I2S clock is correct.

The current application sends a packet when `App_Run()` notices a ready flag. This means packet creation is driven by captured audio blocks, but the exact UDP send time can still vary because processing and W5500 transmission occur in the main loop.

A future design can reduce timing variation by separating capture from network transmission with a queue or ping-pong packet buffers.

## Dependency Structure

```text
main
  └── app
      ├── audio
      │   └── audio_input
      ├── rtp
      │   └── udp_stream
      │       └── Wiznet socket API
      ├── network
      │   └── w5500_port
      └── debug_uart
```

`rtp` depends on `udp_stream`, but `udp_stream` does not depend on `rtp`.

`network` and `udp_stream` both use the W5500 software stack, while direct STM32 hardware operations remain isolated in `w5500_port`.

## Configuration Ownership

### `network.c`

Owns:

* Local MAC address
* Local IPv4 address
* Subnet mask
* Gateway
* W5500 socket memory allocation

### `udp_stream.c`

Owns:

* W5500 socket number
* Local UDP port
* Destination IPv4 address
* Destination UDP port

### `audio_input.h`

Owns:

* Frames per DMA half
* Words per I2S frame
* DMA half size
* Total DMA buffer size

### `app.c`

Owns:

* Mono samples per RTP packet
* I2S frame-to-mono conversion
* Packet send counters

### `rtp.h` and `rtp.c`

Own:

* RTP header size
* RTP payload type
* SSRC
* Sequence number state
* Timestamp state
* PCM network serialization

### `Receiver/rtp_receiver.py`

Owns:

* Receiver UDP bind address and port
* Expected RTP payload type
* Playback sample rate and channel count
* Receiver queue size
* Startup prebuffer size

## Error and Debug State

The firmware exposes several variables that are useful in STM32CubeIDE Live Expressions:

```text
audio_input_start_status
audio_input_half_count
audio_input_full_count
audio_input_error_count
audio_input_first_half_ready
audio_input_second_half_ready
app_last_send_result
app_send_attempt_count
app_send_success_count
app_send_failure_count
```

These values make it possible to separate audio-capture problems from RTP or UDP transmission problems.

## Current Limitations

* Mono conversion currently selects one 16-bit word from each four-word I2S frame
* PCM1808 sample alignment and channel selection need final hardware validation
* Ready flags can represent only whether a half is pending, not how many blocks were missed
* Packet creation and UDP transmission occur in the main loop
* There is no STM32-side queue between audio capture and network transmission
* The Python receiver does not reorder packets
* Missing packets are not replaced with silence
* RTP timestamps are parsed but not used for playout timing
* Network and audio settings are fixed at compile time

## Recommended Next Architecture

A more robust real-time pipeline would use explicit ownership of multiple audio blocks:

```text
I2S DMA ping-pong buffer
        ↓
Captured-block queue
        ↓
PCM conversion stage
        ↓
RTP packet queue
        ↓
W5500 transmit stage
```

This would allow DMA capture to continue independently while the W5500 is busy sending the previous packet.

On the receiver side, packets should enter a timestamp-aware jitter buffer that outputs fixed-size audio blocks at a stable 48 kHz playback rate.

## Summary

```text
main.c
    Starts the firmware and initializes peripherals

app.c
    Connects completed audio blocks to RTP transmission

audio.c
    Initializes the active audio source

audio_input.c
    Captures PCM1808 I2S data with circular DMA

audio_sine.c
    Provides the earlier test-signal path

rtp.c
    Builds RTP headers and serializes PCM samples

udp_stream.c
    Sends completed packets through a W5500 UDP socket

network.c
    Configures the transmitter's local network identity

w5500_port.c
    Connects the Wiznet driver to STM32 SPI and GPIO

debug_uart.c
    Provides firmware debug output

rtp_receiver.py
    Receives, validates, buffers, converts, and plays the stream
```

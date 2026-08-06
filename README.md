# Mono RTP over UDP Transmitter

A real-time embedded audio streaming project built with an STM32 Nucleo-F446RE, a PCM1808 audio ADC, and a W5500 Ethernet controller.

The STM32 captures digital audio from the PCM1808 over I2S using DMA, converts the incoming stereo sample frames to mono signed 16-bit PCM, places the samples into RTP packets, and sends them to a computer over UDP.

A Python receiver validates the RTP stream, converts the PCM byte order, buffers the packets, and plays the audio through the computer's output device.

## Project Goals

* Capture external audio on an STM32 using I2S and DMA
* Stream mono PCM audio across an Ethernet network
* Build RTP version 2 packets manually in embedded C
* Interface an STM32 with a W5500 over SPI
* Receive and play the stream with Python
* Analyze packet timing, sequence numbers, and timestamps in Wireshark
* Keep the firmware separated into clear hardware, audio, network, transport, and application modules

## Current Status

* [x] STM32 and W5500 SPI communication
* [x] Static IPv4 and UDP configuration
* [x] RTP version 2 packet generation
* [x] Sequence number and timestamp tracking
* [x] Signed 16-bit PCM serialization in network byte order
* [x] PCM1808 audio capture over I2S
* [x] Circular DMA audio input buffer
* [x] Half-buffer and full-buffer callback handling
* [x] Stereo input frame to mono 16-bit conversion
* [x] Transmission of 480 audio samples per RTP packet
* [x] Python RTP receiver and audio playback
* [x] RTP inspection in Wireshark
* [ ] Improve packet scheduling and reduce arrival jitter
* [ ] Add a timestamp-based jitter buffer to the receiver
* [ ] Add configurable network and audio settings
* [ ] Add stream discovery and receiver subscription

## Audio and Packet Format

```text
Transport:              RTP over UDP
Audio format:           Signed 16-bit linear PCM
Channels transmitted:   1 (mono)
Sample rate:             48,000 Hz
Samples per packet:      480
Audio per packet:        10 ms
RTP payload type:        96
RTP header size:         12 bytes
Audio payload size:      960 bytes
UDP payload size:        972 bytes
```

Each successfully transmitted packet increases:

```text
RTP sequence number: +1
RTP timestamp:       +480
```

## System Overview

```text
Analog audio input
        |
        v
PCM1808 audio ADC
        |
        | I2S stereo data
        v
STM32 I2S + circular DMA
        |
        | half-buffer callbacks
        v
Stereo frame to mono PCM conversion
        |
        | 480 signed 16-bit samples
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
Ethernet network
        |
        v
Python receiver
        |
        v
Computer audio output
```

## Repository Structure

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

docs/
├── architecture.md
└── Wiring.drawio
```

## Firmware Modules

* `main.c` — CubeMX-generated startup, clock configuration, and peripheral initialization
* `app.c` — Top-level initialization and processing of completed DMA audio buffers
* `audio.c` — Audio subsystem initialization
* `audio_input.c` — I2S DMA buffer, callbacks, and buffer-ready flags
* `audio_sine.c` — Earlier sine-wave and DAC test support
* `rtp.c` — RTP header construction, PCM serialization, sequence numbers, and timestamps
* `udp_stream.c` — W5500 UDP socket and destination configuration
* `network.c` — W5500 memory allocation and static network configuration
* `w5500_port.c` — STM32 HAL callbacks for W5500 SPI, chip select, and reset
* `debug_uart.c` — UART debug output
* `Receiver/rtp_receiver.py` — RTP validation, buffering, PCM conversion, and playback

## Current Data Path

The PCM1808 supplies stereo I2S frames to SPI2. DMA stores the incoming words in a circular buffer divided into two halves.

When either half is complete, a callback marks it ready. `App_Run()` selects the completed half, extracts the selected channel into a 480-sample mono buffer, and passes it to `RTP_SendAudio()`.

The RTP module creates a 12-byte header and serializes every signed 16-bit sample most-significant byte first. The completed 972-byte packet is then sent through the W5500 UDP socket.

## Network Configuration

The current firmware uses static network settings:

```text
W5500 IP address:       192.168.1.150
Subnet mask:            255.255.255.0
Gateway:                192.168.1.1
W5500 local UDP port:   5000
Receiver UDP port:      8080
Current receiver IP:    192.168.1.103
```

The transmitter address is configured in `Core/Src/network.c`.

The destination computer address is configured in `Core/Src/udp_stream.c`:

```c
static uint8_t target_ip[4] = {192, 168, 1, 103};
static uint16_t target_port = 8080;
```

Change `target_ip` to the IPv4 address of the computer running the Python receiver.

## Hardware

* STM32 Nucleo-F446RE
* PCM1808 audio ADC module
* W5500 Ethernet module
* Ethernet-connected computer
* Analog audio source
* UART connection for debug output

## Build and Run

### 1. Clone the Repository

```bash
git clone --recurse-submodules https://github.com/ntchaps/Mono-RTP-over-UDP-Transmitter.git
cd Mono-RTP-over-UDP-Transmitter
```

The `--recurse-submodules` option downloads the Wiznet ioLibrary submodule.

### 2. Configure the Firmware

1. Open the project in STM32CubeIDE.
2. Verify the W5500 settings in `Core/Src/network.c`.
3. Set the receiver computer's IPv4 address in `Core/Src/udp_stream.c`.
4. Confirm the PCM1808 and W5500 wiring.
5. Build and flash the firmware.

### 3. Run the Python Receiver

```bash
cd Receiver
python -m venv .venv
```

Activate the environment on Windows PowerShell:

```powershell
.venv\Scripts\Activate.ps1
```

Install the dependency and start the receiver:

```bash
python -m pip install -r requirements.txt
python rtp_receiver.py
```

The program listens on UDP port `8080` and plays the stream as 48 kHz mono signed 16-bit PCM.

See [`Receiver/README.md`](Receiver/README.md) for receiver details and troubleshooting.

## Wireshark Verification

Use this display filter:

```text
udp.port == 8080
```

If Wireshark displays the traffic only as UDP, select a packet and use:

```text
Analyze -> Decode As -> RTP
```

Expected values:

```text
RTP version:            2
Payload type:           96
Sequence increment:     1
Timestamp increment:    480
PCM payload size:       960 bytes
RTP packet size:        972 bytes
Target packet rate:     100 packets per second
Target packet interval: 10 ms
```

## Current Limitations

* The transmitted channel is selected by taking one 16-bit word from each incoming I2S frame
* The exact PCM1808 word alignment still needs final validation against the captured waveform
* Packet transmission is triggered from the main application loop after DMA callbacks rather than by a dedicated transmit scheduler
* UDP packets can arrive with timing variation even though each packet represents 10 ms of audio
* The receiver does not reorder packets or replace missing audio
* The receiver does not use RTP timestamps to control playback timing
* Network addresses and audio settings are currently fixed in source code

## Next Steps

* Validate PCM1808 sample alignment and channel selection
* Improve packet timing with a dedicated timer, DMA-driven pipeline, or double-buffered transmit stage
* Add receiver statistics for packet rate, loss, and arrival interval
* Add a proper jitter buffer with stable audio-block output
* Insert silence or concealment data for missing packets
* Add configurable destination, ports, sample rate, and output device
* Add simple transmitter discovery and receiver subscription

## Skills Demonstrated

* Embedded C
* Python
* STM32 HAL
* STM32CubeIDE and STM32CubeMX
* I2S audio capture
* DMA and interrupt callbacks
* SPI peripheral communication
* W5500 Ethernet control
* UDP socket programming
* RTP packet construction
* PCM audio representation and byte order
* UART debugging
* Wireshark packet analysis
* Real-time buffering and modular firmware design

## Documentation

See [`docs/architecture.md`](docs/architecture.md) for the module layout, ownership boundaries, and complete data flow.

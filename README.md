# Mono RTP over UDP Transmitter

An embedded audio networking project that streams mono PCM audio from an STM32 Nucleo-F446RE to a PC over Ethernet using RTP over UDP.

The STM32 generates a sine-wave test signal, converts the samples to signed 16-bit PCM, builds RTP packets, and sends them through a W5500 Ethernet controller. A Python receiver listens for the packets, validates the RTP header, converts the network-order PCM samples, and plays the stream through the computer's audio output.

## Project Goals

* Build a real-time mono audio transmitter on an STM32
* Implement RTP packetization over UDP
* Interface an STM32 with a W5500 Ethernet controller over SPI
* Stream and play audio on a PC
* Practice embedded C, networking, DMA, timers, and modular firmware design
* Verify packet timing and RTP fields using Wireshark
* Replace the generated test tone with external digital audio in a future version

## Current Features

* [x] Sine-wave test generation using a lookup table
* [x] Signed 16-bit mono PCM generation
* [x] 48 kHz audio stream
* [x] DAC test output using DMA and a hardware timer
* [x] SPI communication between the STM32 and W5500
* [x] Static MAC and IPv4 configuration
* [x] UDP socket initialization and packet transmission
* [x] RTP version 2 header generation
* [x] RTP sequence-number tracking
* [x] RTP timestamp tracking
* [x] Dynamic RTP payload type 96
* [x] Big-endian PCM serialization for network transmission
* [x] Fixed-rate transmission of 480 samples every 10 ms
* [x] Python receiver with UDP input and audio playback
* [x] RTP packet verification using Wireshark
* [x] UART debug output
* [x] Modular separation of application, audio, RTP, UDP, network, and hardware-interface code

## Current Audio Format

```text
Format:             Signed 16-bit linear PCM
Channels:           1 (mono)
Sample rate:        48,000 Hz
Samples per packet: 480
Packet interval:    10 ms
RTP payload type:   96
RTP header size:    12 bytes
Audio payload size: 960 bytes
UDP payload size:   972 bytes
```

The RTP timestamp increases by 480 for each successfully transmitted packet because each packet contains 480 audio samples.

The current sine source uses a 128-entry unsigned 12-bit lookup table. The network path centers each value around zero and scales it into signed 16-bit PCM. Advancing one table entry per output sample produces a 375 Hz test tone at a 48 kHz sample rate.

## System Overview

```text
STM32 sine LUT
      |
      v
Signed 16-bit PCM samples
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
Ethernet network
      |
      v
Python PC receiver
      |
      v
System audio output
```

## Repository Structure

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

Receiver/
├── README.md
├── requirements.txt
└── rtp_receiver.py

docs/
├── architecture.md
└── Wiring.drawio
```

## Firmware Modules

* `main.c` — STM32 startup and CubeMX-generated peripheral initialization
* `app.c` — Top-level initialization, audio packet scheduling, and application control
* `audio.c` — General audio initialization entry point
* `audio_sine.c` — Sine lookup table, DAC test output, and signed PCM sample generation
* `rtp.c` — RTP header construction, PCM serialization, sequence numbers, and timestamps
* `udp_stream.c` — W5500 UDP socket management and destination configuration
* `network.c` — W5500 MAC, IP address, subnet, and gateway configuration
* `w5500_port.c` — STM32 HAL interface for W5500 SPI, chip select, and reset
* `debug_uart.c` — UART debug output
* `Receiver/rtp_receiver.py` — RTP validation, PCM byte-order conversion, buffering, and audio playback

## RTP Packet Layout

Each UDP payload contains one RTP packet:

```text
Bytes 0-1       RTP version, flags, marker, and payload type
Bytes 2-3       Sequence number
Bytes 4-7       Timestamp
Bytes 8-11      SSRC
Bytes 12-971    480 signed 16-bit mono PCM samples
```

PCM samples are transmitted most-significant byte first. The PC receiver converts them from network byte order to the host byte order before playback.

## Network Configuration

The current implementation uses static network settings.

```text
W5500 local IP:       192.168.1.150
W5500 local UDP port: 5000
Receiver UDP port:    8080
```

The destination PC address is configured in `Core/Src/udp_stream.c`:

```c
static uint8_t target_ip[4] = {192, 168, 1, 103};
static uint16_t target_port = 8080;
```

Change `target_ip` so it matches the IPv4 address of the computer running the receiver. The STM32, W5500, and PC must be on compatible subnets.

## Hardware

* STM32 Nucleo-F446RE
* W5500 Ethernet module
* Ethernet-connected PC
* UART connection for debug output
* Optional oscilloscope or amplified speaker circuit for DAC testing

## Build and Run

### Firmware

1. Clone the repository and initialize the Wiznet submodule:

```bash
git clone --recurse-submodules https://github.com/ntchaps/Mono-RTP-over-UDP-Transmitter.git
```

2. Open the project in STM32CubeIDE.
3. Check the W5500 network configuration in `Core/Src/network.c`.
4. Set the destination PC address in `Core/Src/udp_stream.c`.
5. Build and flash the firmware to the Nucleo-F446RE.
6. Open the UART output if debug messages are needed.

### PC Receiver

From the repository root:

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

The receiver listens on all local interfaces at UDP port 8080 and plays the incoming stream as 48 kHz, mono, signed 16-bit PCM.

See [`Receiver/README.md`](Receiver/README.md) for receiver setup, command details, and troubleshooting.

## Wireshark Verification

Use this display filter:

```text
udp.port == 8080
```

If Wireshark identifies the packets only as UDP, select a packet and use:

```text
Analyze -> Decode As -> RTP
```

A correct stream should show approximately:

```text
Packet interval:      10 ms
Sequence increment:   1
Timestamp increment:  480
RTP payload type:     96
RTP payload length:   960 bytes
UDP payload length:   972 bytes
```

## Troubleshooting

### No packets arrive

* Confirm the destination IP in `udp_stream.c` matches the PC
* Confirm the PC firewall allows inbound UDP traffic on port 8080
* Confirm the Ethernet link is active
* Confirm the PC and W5500 are on compatible subnets
* Check the UART output for W5500 or send errors

### Wireshark shows UDP instead of RTP

Use **Analyze -> Decode As -> RTP** for UDP port 8080.

### Audio has clicks or buzzing

* Confirm both transmitter and receiver use 48,000 Hz
* Confirm the transmitter sends 480 samples every 10 ms
* Watch the receiver for sequence-gap or queue-full messages
* Disable audio enhancements on the selected Windows playback device
* Increase the receiver prebuffer if packet arrival jitter is causing underruns
* Confirm no second receiver process is already using UDP port 8080

### Socket bind error on Windows

Only one process can normally bind to UDP port 8080. Stop the existing receiver process or close the terminal that is still running it.

## Planned Improvements

* [ ] Replace the sine-wave source with external audio input
* [ ] Add I2S audio capture
* [ ] Add timer- or DMA-driven double buffering for tighter packet timing
* [ ] Add a receiver jitter buffer with timestamp-based playout
* [ ] Add stream discovery and receiver subscription
* [ ] Make destination address, ports, sample rate, and payload format configurable
* [ ] Support higher-resolution audio formats
* [ ] Update the architecture document to reflect the completed RTP receiver path

## Technologies and Skills

* Embedded C
* Python
* STM32 HAL
* STM32CubeIDE and STM32CubeMX
* SPI
* UART
* DAC and DMA
* Hardware timers
* UDP sockets
* RTP packet formatting
* PCM audio representation
* W5500 Ethernet controller
* Wireshark packet analysis
* Real-time audio buffering
* Modular firmware architecture

## Documentation

See [`docs/architecture.md`](docs/architecture.md) for the firmware architecture and module responsibilities.

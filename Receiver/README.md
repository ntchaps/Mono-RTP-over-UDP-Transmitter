# Python RTP Audio Receiver

This folder contains the PC receiver for the Mono RTP over UDP project.

The program listens for RTP packets from the STM32 transmitter, validates the RTP header, extracts the signed 16-bit PCM payload, converts the samples from network byte order, buffers several packets, and plays the stream through the computer's default audio output device.

## Audio Format

```text
Transport:           RTP over UDP
UDP port:            8080
RTP payload type:    96
Audio format:        Signed 16-bit PCM
Channels:            1 (mono)
Sample rate:         48,000 Hz
Samples per packet:  480
Packet duration:     10 ms
PCM payload size:    960 bytes
RTP packet size:     972 bytes
```

Each packet contains:

```text
12-byte RTP header
960-byte PCM audio payload
```

## Files

```text
Receiver/
├── README.md
├── requirements.txt
└── rtp_receiver.py
```

* `rtp_receiver.py` — Receives, validates, buffers, converts, and plays the RTP audio
* `requirements.txt` — Lists the required Python package

## Requirements

* Python 3
* `sounddevice`
* A working computer audio output device
* The STM32 configured to send packets to the computer's IPv4 address
* The transmitter and computer connected to compatible IPv4 networks

## Setup

Open a terminal in the repository root and enter:

```bash
cd Receiver
python -m venv .venv
```

Activate the virtual environment on Windows PowerShell:

```powershell
.venv\Scripts\Activate.ps1
```

Install the dependency:

```bash
python -m pip install -r requirements.txt
```

## Run the Receiver

```bash
python rtp_receiver.py
```

The program binds to:

```python
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
```

`0.0.0.0` allows the receiver to accept packets through any local IPv4 network interface.

The STM32 must send to the computer's actual IPv4 address, not to `0.0.0.0`.

On Windows, find the address with:

```powershell
ipconfig
```

Use the IPv4 address of the Ethernet adapter connected to the same network as the W5500.

Stop the receiver with:

```text
Ctrl+C
```

## How It Works

The receiver uses two execution paths:

1. A background network thread receives and validates RTP packets.
2. The main thread removes PCM blocks from a queue and writes them to the audio device.

```text
STM32 transmitter
       |
       | RTP over UDP
       v
UDP socket
       |
       v
RTP header validation
       |
       v
Sequence tracking
       |
       v
PCM payload extraction
       |
       v
Big-endian to host-endian conversion
       |
       v
Thread-safe audio queue
       |
       v
Startup prebuffer
       |
       v
sounddevice output stream
```

Separating reception from playback prevents every small packet-arrival variation from immediately blocking the audio output.

## RTP Validation

The receiver expects a standard 12-byte RTP header:

```text
Byte 0       RTP version and flags
Byte 1       Marker bit and payload type
Bytes 2-3    Sequence number
Bytes 4-7    Timestamp
Bytes 8-11   SSRC
Bytes 12+    PCM audio payload
```

It checks:

* The packet is at least 12 bytes long
* RTP version is 2
* Payload type is 96
* The PCM payload contains an even number of bytes

Packets that do not match the expected format are ignored.

## Sequence Numbers and Timestamps

The sequence number should increase by one for every packet.

```text
Packet 1 sequence: 100
Packet 2 sequence: 101
Packet 3 sequence: 102
```

A gap indicates a lost or out-of-order packet:

```text
Sequence gap: expected 101, received 102
```

The RTP timestamp should increase by 480 because each packet contains 480 audio samples:

```text
Packet 1 timestamp: 0
Packet 2 timestamp: 480
Packet 3 timestamp: 960
```

The current receiver prints sequence and timestamp information but does not reorder packets or schedule playback from the timestamps.

## PCM Byte Order

The STM32 transmits every signed 16-bit sample in big-endian network byte order.

```text
PCM value:      0x1234
Network bytes:  0x12 0x34
```

Most Windows computers are little-endian, so the receiver swaps the bytes before playback.

Without this conversion, the sample values are interpreted incorrectly and the stream sounds distorted or noisy.

## Buffering

The receiver places decoded PCM blocks into a thread-safe queue.

```text
Network thread -> audio queue -> playback thread
```

The current program waits for five packets before playback begins:

```text
5 packets × 10 ms = approximately 50 ms
```

This startup prebuffer provides limited protection against packet-arrival jitter. A larger prebuffer increases delay, while a smaller prebuffer makes playback more sensitive to timing variation.

The current design is a packet queue, not a complete jitter buffer. After playback begins, each packet is written as soon as it is removed from the queue.

## Audio Playback

The receiver opens a raw output stream with settings that match the transmitter:

```python
sd.RawOutputStream(samplerate=48000, channels=1, dtype="int16")
```

The transmitter and receiver must agree on:

```text
Sample rate:   48,000 Hz
Channels:      1
Sample type:   signed 16-bit PCM
Byte order:    converted to host order before playback
```

## Wireshark Verification

Use this display filter:

```text
udp.port == 8080
```

If Wireshark identifies the packets only as UDP, select a packet and use:

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
Target packet interval: 10 ms
Target packet rate:     100 packets per second
```

## Troubleshooting

### Port Already in Use

```text
OSError: [WinError 10048]
```

Another process is already using UDP port 8080. This is commonly another copy of the receiver.

Find the process on Windows with:

```powershell
netstat -ano | findstr :8080
```

Then stop that process or close the terminal running it.

### No Packets Are Received

Check that:

* The destination IP in `Core/Src/udp_stream.c` matches the computer
* The W5500 and computer are on compatible subnets
* The Ethernet link is active
* Windows Firewall allows inbound UDP traffic on port 8080
* The W5500 initializes successfully
* Wireshark shows packets on UDP port 8080

### Packets Arrive but No Audio Plays

Check that:

* The computer has a working default audio output device
* The device supports 48 kHz playback
* The receiver is not rejecting the RTP payload type
* Each packet contains a 960-byte audio payload
* The Python terminal is not reporting an exception

List available devices with:

```bash
python -c "import sounddevice as sd; print(sd.query_devices())"
```

### Audio Is Too Fast, Slow, or the Pitch Is Wrong

Confirm that both sides use 48,000 samples per second.

Playing 48 kHz samples at a different rate changes both speed and pitch.

### Audio Clicks, Buzzes, or Drops Out

Possible causes include:

* Packet loss
* Irregular packet arrival timing
* Queue underruns or overflows
* Incorrect PCM1808 sample alignment on the STM32
* Incorrect sample rate
* Incorrect PCM byte order
* Windows audio enhancements
* Excessive printing for every received packet
* Other applications interrupting audio playback

Check the terminal for:

```text
Sequence gap
Audio queue full; dropping packet
```

Also verify that the stream averages approximately 100 packets per second and that the RTP timestamp increases by 480 each packet.

### Reduce Receiver Overhead

The current script prints one detailed line for every packet. At approximately 100 packets per second, this can add unnecessary terminal and scheduling overhead.

For cleaner real-time playback, packet-by-packet printing can be disabled or replaced with a once-per-second statistics report.

## Current Limitations

* Fixed UDP port 8080
* Fixed 48 kHz sample rate
* Fixed mono signed 16-bit PCM format
* Fixed RTP payload type 96
* Uses the default audio output device
* Does not reorder packets
* Does not insert silence for missing packets
* Does not use RTP timestamps for playback scheduling
* Uses a basic FIFO queue instead of a timestamp-based jitter buffer
* Prints every received packet
* Does not automatically discover the transmitter

## Planned Improvements

* Add packet-rate, packet-loss, and jitter statistics
* Replace per-packet logging with periodic summaries
* Add timestamp-based ordering and playout
* Insert silence or concealment audio for missing packets
* Add a configurable jitter buffer
* Allow output-device selection
* Allow the UDP port, payload type, and sample rate to be configured
* Save optional WAV captures for debugging
* Add transmitter discovery and subscription

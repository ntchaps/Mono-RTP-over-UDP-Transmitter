# RTP Audio Receiver

This folder contains the Python receiver for the Mono RTP over UDP project.

The receiver listens for RTP packets sent by the STM32, extracts the PCM audio payload, converts the samples from network byte order, and plays the audio through the computer's default output device.

## Current Audio Format

```text
Transport:           RTP over UDP
UDP port:            8080
RTP payload type:    96
Audio format:        Signed 16-bit PCM
Channels:            1 (mono)
Sample rate:         48,000 Hz
Samples per packet:  480
Packet interval:     10 ms
PCM payload size:    960 bytes
RTP packet size:     972 bytes
```

Each packet contains:

```text
12-byte RTP header
960-byte PCM audio payload
```

The 960-byte payload represents 480 signed 16-bit samples:

```text
480 samples × 2 bytes per sample = 960 bytes
```

## Files

```text
Receiver/
├── README.md
├── requirements.txt
└── rtp_receiver.py
```

* `rtp_receiver.py` — Receives, validates, buffers, and plays the RTP audio
* `requirements.txt` — Lists the Python package required by the receiver

## Requirements

* Python 3
* `sounddevice`
* A working audio output device
* The STM32 transmitter configured to send packets to the PC's IPv4 address

Install the required package with:

```bash
python -m pip install -r requirements.txt
```

## Running the Receiver

Open a terminal inside the `Receiver` folder:

```bash
cd Receiver
```

Run the receiver:

```bash
python rtp_receiver.py
```

The program listens on UDP port 8080.

Stop it with:

```text
Ctrl+C
```

## How the Receiver Works

The receiver uses two main threads:

1. A network thread receives and processes RTP packets.
2. An audio thread writes the decoded PCM data to the output device.

A queue passes audio data between the two threads.

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
PCM payload extraction
       |
       v
Big-endian to host-endian conversion
       |
       v
Audio queue
       |
       v
sounddevice output stream
```

Separating packet reception from playback helps prevent network timing variations from directly interrupting the audio output.

## UDP Socket

The receiver binds to:

```python
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
```

Using `0.0.0.0` allows the program to receive packets through any IPv4 network interface on the PC.

The STM32 must send packets to the PC's actual IPv4 address, not to `0.0.0.0`.

On Windows, find the PC's IPv4 address with:

```powershell
ipconfig
```

Use the address of the Ethernet adapter connected to the W5500 network.

## RTP Header Processing

The receiver expects the standard 12-byte RTP header.

```text
Byte 0       RTP version and flags
Byte 1       Marker bit and payload type
Bytes 2-3    Sequence number
Bytes 4-7    Timestamp
Bytes 8-11   SSRC
Bytes 12+    PCM audio payload
```

The receiver checks:

* RTP version is 2
* Payload type is 96
* Packet length is at least 12 bytes
* PCM payload contains an even number of bytes

Packets that do not match the expected format are ignored.

## Sequence Numbers

The RTP sequence number increases by one for each packet.

The receiver compares the current sequence number with the expected value. This makes it possible to detect missing or out-of-order packets.

Example:

```text
Expected sequence: 120
Received sequence: 122
```

This indicates that packet 121 was lost or arrived out of order.

A missing packet currently creates a gap in the audio because the receiver does not yet insert replacement samples.

## RTP Timestamps

Each packet contains 480 samples, so the timestamp should increase by 480:

```text
Packet 1 timestamp: 0
Packet 2 timestamp: 480
Packet 3 timestamp: 960
Packet 4 timestamp: 1440
```

At 48,000 samples per second:

```text
480 / 48000 = 0.010 seconds
```

This means each RTP packet represents 10 ms of audio.

The current receiver parses the timestamp for debugging but does not yet use it to schedule playback or reorder packets.

## PCM Byte Order

The STM32 sends each signed 16-bit PCM sample in big-endian network byte order.

Example sample:

```text
PCM value: 0x1234
Network bytes: 0x12 0x34
```

Most Windows PCs use little-endian byte order, so the receiver swaps the two bytes before playback.

Without this conversion, the sample values would be interpreted incorrectly and the audio would sound distorted or noisy.

## Audio Queue

Received PCM payloads are placed into a thread-safe queue.

```text
Network thread -> audio queue -> playback thread
```

The receiver buffers several packets before playback begins. This creates a small amount of protection against UDP packet-arrival jitter.

For example, five 10 ms packets provide approximately:

```text
5 × 10 ms = 50 ms
```

of buffered audio.

A larger buffer can make playback more stable, but it also increases delay.

## Audio Playback

The receiver opens a mono signed 16-bit output stream at 48 kHz:

```python
sd.RawOutputStream(
    samplerate=48000,
    channels=1,
    dtype="int16"
)
```

The playback settings must match the transmitter.

If the STM32 sends 48 kHz audio but the receiver plays it at 8 kHz, the tone will play at the wrong speed and pitch.

## Wireshark Verification

Use this display filter:

```text
udp.port == 8080
```

If Wireshark displays the packets only as UDP:

```text
Analyze -> Decode As -> RTP
```

Expected packet values:

```text
RTP version:          2
Payload type:         96
Sequence increment:   1
Timestamp increment:  480
Packet interval:      approximately 10 ms
PCM payload size:     960 bytes
RTP packet size:      972 bytes
```

## Troubleshooting

### Port Already in Use

```text
OSError: [WinError 10048]
```

Another program is already using UDP port 8080.

This usually means another copy of the receiver is still running.

Find the process with:

```powershell
netstat -ano | findstr :8080
```

Then stop the existing process or close its terminal.

### No Packets Are Received

Check that:

* The destination IP in the STM32 firmware matches the PC
* The transmitter and PC are on compatible subnets
* The Ethernet connection is active
* Windows Firewall allows UDP port 8080
* The STM32 is successfully initializing the W5500
* Wireshark shows traffic on port 8080

### Packets Arrive but No Audio Plays

Check that:

* The computer has a working default output device
* The selected device supports 48 kHz playback
* The receiver is using mono signed 16-bit audio
* The RTP payload contains 960 audio bytes
* The receiver is not rejecting the payload type

List available audio devices with:

```bash
python -c "import sounddevice as sd; print(sd.query_devices())"
```

### Buzzing or Clicking

Possible causes include:

* Packet loss
* Inconsistent packet timing
* Receiver buffer underruns
* Incorrect sample rate
* Incorrect PCM byte order
* Windows audio enhancements
* Another application interrupting audio playback

Check the terminal for sequence-gap or queue-full messages.

Also verify that the transmitter sends:

```text
480 samples every 10 ms
```

and that the receiver plays the samples at:

```text
48,000 samples per second
```

## Current Limitations

* Only supports mono signed 16-bit PCM
* Uses a fixed 48 kHz sample rate
* Uses RTP payload type 96
* Does not reorder packets
* Does not replace missing packets
* Does not use RTP timestamps for playback timing
* Does not automatically discover the transmitter
* Uses the computer's default audio output device

## Planned Improvements

* [ ] Add timestamp-based packet ordering
* [ ] Add silence insertion for missing packets
* [ ] Add a proper jitter buffer
* [ ] Allow audio-device selection
* [ ] Allow sample rate and port configuration
* [ ] Display packet-loss statistics
* [ ] Support transmitter discovery and subscription

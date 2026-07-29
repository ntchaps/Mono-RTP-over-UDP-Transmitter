# Mono RTP over UDP Transmitter

An embedded networking project that uses an STM32 Nucleo-F446RE and a W5500 Ethernet controller to transmit mono audio over UDP.

The project currently generates a sine-wave test signal, outputs it through the STM32 DAC using DMA, and sends test packets over Ethernet. The next stage is to packetize audio using RTP and stream it to a PC receiver.

## Project Goals

* Build a real-time mono audio transmitter
* Use RTP over UDP for low-latency audio transport
* Interface an STM32 microcontroller with a W5500 Ethernet controller
* Practice embedded C, hardware peripherals, networking, and modular firmware design
* Capture and verify transmitted packets using Wireshark

## Current Features

* [x] Sine-wave generation using a lookup table
* [x] DAC output using DMA and a hardware timer
* [x] SPI communication between the STM32 and W5500
* [x] Static MAC and IPv4 configuration
* [x] UDP socket initialization and packet transmission
* [x] UART debugging
* [x] Modular separation of application, audio, network, UDP, and hardware-interface code

## In Progress

* [ ] Connect audio sample buffers to the network transmit path
* [ ] Send packets at a fixed audio rate
* [ ] Add RTP header generation
* [ ] Track RTP sequence numbers and timestamps
* [ ] Verify RTP packets in Wireshark
* [ ] Receive and play the stream on a PC
* [ ] Add external audio input
* [ ] Support 24-bit, 48 kHz mono audio

## Firmware Structure

```text
main.c
    STM32 startup and CubeMX-generated initialization

app.c
    Top-level application control

audio.c
    DAC, DMA, and sample timer control

audio_sine.c
    Sine-wave lookup-table test data

network.c
    W5500 local network configuration

udp_stream.c
    UDP socket management and packet transmission

w5500_port.c
    STM32 HAL interface for W5500 SPI, chip select, and reset

debug_uart.c
    UART debug output
```

A future `rtp_packet.c` module will build RTP headers and pass completed packets to `udp_stream.c`.

## Current Data Paths

Audio test output:

```text
audio_sine -> audio -> DAC/DMA -> analog output
```

UDP test transmission:

```text
app -> udp_stream -> W5500 -> Ethernet -> PC
```

Planned RTP audio transmission:

```text
audio samples -> RTP packet builder -> UDP stream -> W5500 -> PC
```

## Hardware

* STM32 Nucleo-F446RE
* W5500 Ethernet module
* Ethernet-connected PC for packet capture and testing
* UART connection for debug output

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
* RTP packet structure
* W5500 Ethernet controller
* Wireshark packet analysis
* Modular firmware design

## Documentation

See [`docs/architecture.md`](docs/architecture.md) for the firmware architecture, module responsibilities, and planned RTP data flow.

# Mono RTP over UDP Transmitter

This is an embedded networking project built around an **STM32 Nucleo-F446RE** and a **W5500 Ethernet module**. The goal is to transmit mono audio data over UDP and eventually format the stream as RTP audio.

Right now, the project uses a generated sine wave as a test signal. This makes it easier to debug the networking side before adding real audio input hardware. Once the packet timing and UDP transmission are working reliably, the next step is to move toward RTP headers and real audio capture from an external ADC.

## Why I Built This

I wanted a project that combines a few areas I have been trying to get stronger in:

* Embedded C
* STM32 peripheral setup
* SPI communication
* Ethernet and UDP networking
* Real-time packet timing
* Digital audio basics

Instead of only sending a basic UDP test packet, I wanted to build toward something closer to a real embedded audio streaming system.

## Hardware

Current hardware:

* STM32 Nucleo-F446RE
* W5500 Ethernet module
* Ethernet connection to a PC or local network

Planned hardware:

* PCM1808 or similar audio ADC for real mono audio input

## Tools

* STM32CubeIDE
* STM32 HAL
* Wiznet W5500 ioLibrary
* C
* Wireshark for packet inspection
* PC-side UDP/RTP receiver tools for testing

## Current Progress

* [x] Generate an internal sine wave test signal
* [x] Send basic UDP packets through the W5500
* [ ] Send fixed-size packets at a stable interval
* [ ] Add RTP header formatting
* [ ] Track RTP sequence numbers and timestamps
* [ ] Verify packet timing and structure in Wireshark
* [ ] Test receiving/playback on a PC
* [ ] Move toward 48 kHz mono audio
* [ ] Add real audio input using an external ADC

## System Overview

The STM32 generates or captures audio samples, places them into a packet buffer, and sends the packet through the W5500 over SPI. The W5500 handles the Ethernet side, while the STM32 handles the packet contents, timing, and eventually RTP formatting.

```text
Audio source
    |
    |  current: generated sine wave
    |  future: external audio ADC
    v
STM32 Nucleo-F446RE
    |
    |  packet buffer
    |  UDP/RTP formatting
    v
W5500 Ethernet Module
    |
    |  Ethernet / UDP
    v
PC receiver / Wireshark
```

## RTP Direction

The project currently starts with plain UDP because that is the simplest way to verify that the STM32 and W5500 are sending packets correctly.

The next major step is adding RTP-style packet headers, including:

* Sequence number
* Timestamp
* Payload type
* SSRC

Adding these fields will make the packets easier to inspect, sequence, and eventually play back as a real-time audio stream.

## Development Plan

The project is being built in stages:

* [x] Generate a known test signal on the STM32
* [x] Send basic UDP packets through the W5500
* [ ] Improve packet timing and consistency
* [ ] Add RTP header generation
* [ ] Verify packets with Wireshark
* [ ] Test PC-side receiving and playback
* [ ] Replace the test signal with real audio input

## What This Project Demonstrates

This project shows practical work with:

* Embedded firmware development in C
* STM32CubeIDE and HAL
* SPI-based communication with an external Ethernet controller
* UDP socket communication on embedded hardware
* Packet formatting and timing
* RTP audio streaming concepts
* Network debugging with Wireshark
* Building and testing an embedded system incrementally

## Status

This project is still in progress. The current focus is getting reliable UDP transmission working from the STM32 through the W5500. After that, the project will move toward RTP formatting and real audio input.

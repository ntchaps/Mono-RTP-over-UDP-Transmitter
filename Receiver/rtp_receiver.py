# 
# rtp_receiver.py
# 
#  Created on: Jul 31, 2026
#      Author: nickt
# 

import socket
import struct
import queue
import threading
from array import array
import sys
import sounddevice as sd

### CONSTANTS ###
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
SAMPLE_RATE = 8000
RTP_PAYLOAD_TYPE = 96
RTP_HEADER_SIZE = 12
PREBUFFER_PACKETS = 5

# converts packet from incoming Big-endian to Little-endian
def convert_network_pcm(payload):
    if len(payload) % 2 != 0:
        raise ValueError("Audio payload has an odd number of bytes")

    samples = array("h")
    samples.frombytes(payload)

    if sys.byteorder == "little":
        samples.byteswap()

    return samples.tobytes()

# Opens socket via vars above
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP port {UDP_PORT}")

audio_queue = queue.Queue(maxsize=20)
expected_sequence = None

# Opens output stream
stream = sd.RawOutputStream(SAMPLE_RATE, channels=1, dtype="int16")
stream.start()

def receive_audio():
    global expected_sequence

    # Packet receiving loop
    while True:
        # receive from port 8080
        packet, sender = sock.recvfrom(2048)

        # Error if packet length < 12
        if len(packet) < RTP_HEADER_SIZE:
            print("Packet is too short to be RTP")
            continue

        # Struct describes RTP packet.
        first_byte, second_byte, sequence, timestamp, ssrc = struct.unpack(
            "!BBHII",
            packet[:12]
        )

        version = first_byte >> 6
        payload_type = second_byte & 0x7F
        audio_payload = packet[RTP_HEADER_SIZE:]

        ### Error Messages
        if version != 2:
            print(f"Ignoring unsupported RTP version {version}")
            continue

        if payload_type != 96:
            print(f"Ignoring unexpected payload type {payload_type}")
            continue

        if expected_sequence is not None and sequence != expected_sequence:
            print(f"Sequence gap: expected {expected_sequence}, received {sequence}")

        expected_sequence = (sequence + 1) & 0xFFFF

        # RTP packet printouts
        print(
            f"Version={version}, "
            f"Payload type={payload_type}, "
            f"Sequence={sequence}, "
            f"Timestamp={timestamp}, "
            f"SSRC=0x{ssrc:08X}, " 
            f"Audio payload: {len(audio_payload)} bytes"
        )

        try:
            # Convert RTP samples from big to little endian
            pcm_audio = convert_network_pcm(audio_payload)
        except ValueError as error:
            print(f"Ignoring invalid payload: {error}")
            continue

        try:
            audio_queue.put(pcm_audio, timeout=0.05)
        except queue.Full:
            print("Audio queue full; dropping packet")

receiver_thread = threading.Thread(target=receive_audio, daemon=True)
receiver_thread.start()

stream = sd.RawOutputStream(samplerate=SAMPLE_RATE, channels=1, dtype="int16")
stream.start()

print(f"Listening on UDP port {UDP_PORT}")
print(f"Buffering {PREBUFFER_PACKETS} packets before playback")
print("Press Ctrl+C to stop")

try:
    buffered_packets = []

    for _ in range(PREBUFFER_PACKETS):
        buffered_packets.append(audio_queue.get())

    print("Playback started")

    for pcm_audio in buffered_packets:
        stream.write(pcm_audio)

    while True:
        pcm_audio = audio_queue.get()
        stream.write(pcm_audio)

except KeyboardInterrupt:
    print("\nStopping RTP receiver")

finally:
    stream.stop()
    stream.close()
    sock.close()
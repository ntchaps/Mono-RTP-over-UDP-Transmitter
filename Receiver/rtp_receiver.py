# 
# rtp_receiver.py
# 
#  Created on: Jul 31, 2026
#      Author: nickt
# 

import socket
import struct
from array import array
import sys
import sounddevice as sd

# converts packet from incoming Big-endian to Little-endian
def convert_network_pcm(payload):
    if len(payload) % 2 != 0:
        raise ValueError("Audio payload has an odd number of bytes")

    samples = array("h")
    samples.frombytes(payload)

    if sys.byteorder == "little":
        samples.byteswap()

    return samples.tobytes()

# IP packets are sent to (self)
UDP_IP = "0.0.0.0"

# port to read
UDP_PORT = 8080

# Opens socket via vars above
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP port {UDP_PORT}")

expected_sequence = None

# Opens output stream
stream = sd.RawOutputStream(samplerate=8000, channels=1, dtype="int16")
stream.start()

try:
    # Packet receiving loop
    while True:
        # receive from port 8080
        packet, sender = sock.recvfrom(2048)

        # Error if packet length < 12
        if len(packet) < 12:
            print("Packet is too short to be RTP")
            continue

        # Struct describes RTP packet.
        first_byte, second_byte, sequence, timestamp, ssrc = struct.unpack(
            "!BBHII",
            packet[:12]
        )


        version = first_byte >> 6
        payload_type = second_byte & 0x7F
        audio_payload = packet[12:]

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

        # Convert RTP samples from big to little endian
        pcm_audio = convert_network_pcm(audio_payload)

        # write the samples to the output audio stream
        stream.write(pcm_audio)
except KeyboardInterrupt:
    print("\nStopping RTP receiver")

finally:
    stream.stop()
    stream.close()
    sock.close()
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
import wave
import time

### CONSTANTS ###
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
SAMPLE_RATE = 48000
CHANNELS = 1
RTP_HEADER_SIZE = 12
PREBUFFER_PACKETS = 3
SAMPLE_WIDTH_BYTES = 2
WAV_FILENAME = "received_audio.wav"

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
sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 262144)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP port {UDP_PORT}")

audio_queue = queue.Queue(maxsize=100)
expected_sequence = None

received_packets = 0
sequence_gaps = 0
minimum_queue = PREBUFFER_PACKETS
maximum_queue = 0
last_packet_time = None
maximum_packet_interval = 0.0
last_packet_count = 0

wav_file = wave.open(WAV_FILENAME, "wb")
wav_file.setnchannels(CHANNELS)
wav_file.setsampwidth(SAMPLE_WIDTH_BYTES)
wav_file.setframerate(SAMPLE_RATE)

def receive_audio():
    global expected_sequence, received_packets, sequence_gaps
    global last_packet_time, maximum_packet_interval
    global running

    running = True

    # Packet receiving loop
    while running:
        try:
            # receive from port 8080
            packet, sender = sock.recvfrom(2048)

            packet_time = time.perf_counter()

            if last_packet_time is not None:
                packet_interval = packet_time - last_packet_time
                maximum_packet_interval = max(maximum_packet_interval, packet_interval)

            last_packet_time = packet_time

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
                sequence_gaps += 1

            expected_sequence = (sequence + 1) & 0xFFFF
            received_packets += 1

            try:
                # Convert RTP samples from big to little endian
                pcm_audio = convert_network_pcm(audio_payload)

                wav_file.writeframesraw(pcm_audio)
            except ValueError as error:
                print(f"Ignoring invalid payload: {error}")
                continue

            try:
                audio_queue.put_nowait(pcm_audio)
            except queue.Full:
                try:
                    audio_queue.get_nowait()
                except queue.Empty:
                    pass
            
                try:
                    audio_queue.put_nowait(pcm_audio)
                except queue.Full:
                    pass
        except OSError:
            break

receiver_thread = threading.Thread(target=receive_audio, daemon=True)
receiver_thread.start()

stream = sd.RawOutputStream(samplerate=SAMPLE_RATE, channels=1, dtype="int16")
stream.start()

print(f"Listening on UDP port {UDP_PORT}")
print(f"Buffering {PREBUFFER_PACKETS} packets before playback")
print("Press Ctrl+C to stop")

try:
    while audio_queue.qsize() < PREBUFFER_PACKETS:
        time.sleep(0.001)

    print("Playback started")

    last_status_time = time.monotonic()
    last_status_packet_count = received_packets

    while True:
        try:
            pcm_audio = audio_queue.get(timeout=0.050)
        except queue.Empty:
            print("\nPlayback starvation")
            continue

        stream.write(pcm_audio)

        queue_depth = audio_queue.qsize()
        minimum_queue = min(minimum_queue, queue_depth)
        maximum_queue = max(maximum_queue, queue_depth)

        current_time = time.monotonic()
        status_elapsed = current_time - last_status_time

        if status_elapsed >= 1.0:
            current_packet_count = received_packets
            packets_received_this_period = (
                current_packet_count - last_status_packet_count
            )
            packets_per_second = (
                packets_received_this_period / status_elapsed
            )

            print(
                f"\rPackets: {current_packet_count} | "
                f"Rate: {packets_per_second:.1f} packets/s | "
                f"Gaps: {sequence_gaps} | "
                f"Queue: {queue_depth} | "
                f"Range: {minimum_queue}-{maximum_queue} | "
                f"Max interval: "
                f"{maximum_packet_interval * 1000:.2f} ms",
                end="",
                flush=True
            )

            last_status_packet_count = current_packet_count
            last_status_time = current_time
            maximum_packet_interval = 0.0
            minimum_queue = queue_depth
            maximum_queue = queue_depth

except KeyboardInterrupt:
    print("\nStopping RTP receiver")

finally:
    running = False
    stream.stop()
    stream.close()
    wav_file.close()
    sock.close()

    print(f"WAV file saved: {WAV_FILENAME}")
    print(
        f"Format: {SAMPLE_RATE} Hz, "
        f"{CHANNELS} channel, "
        f"{SAMPLE_WIDTH_BYTES * 8}-bit PCM"
    )
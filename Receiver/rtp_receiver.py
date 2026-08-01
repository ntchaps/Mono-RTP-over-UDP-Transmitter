# 
# rtp_receiver.py
# 
#  Created on: Jul 31, 2026
#      Author: nickt
# 

import socket
import struct

UDP_IP = "0.0.0.0"
UDP_PORT = 8080

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP port {UDP_PORT}")

while True:
    packet, sender = sock.recvfrom(2048)

    print(f"Recevied {len(packet)} bytes from {sender}")
    print(packet[:12].hex(" "))

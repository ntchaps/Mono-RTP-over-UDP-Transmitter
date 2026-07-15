/*
 * network.c
 *
 *  Created on: Jul 17, 2026
 *      Author: nickt
 */

#include "network.h"
#include "w5500_port.h"
#include "debug_uart.h"

#include "wizchip_conf.h"
#include "socket.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define UDP_SOCKET     0
#define LOCAL_PORT     5000

// Set up target destination details
static uint8_t target_ip[4] = {192, 168, 1, 101};
static uint16_t target_port = 8080;
static uint8_t msg[] = "Hello World via UDP! Refactor Complete!"; 

void Network_Init(void)
{
    W5500_Port_Init();

    // Allocate 2KB buffer size for each of the 8 sockets
    uint8_t buf_size[] = {2, 2, 2, 2, 2, 2, 2, 2};
    wizchip_init(buf_size, buf_size); 
    // Define Network Parameters
    wiz_NetInfo net_info = {
        .mac = {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33},
        .ip  = {192, 168, 1, 150},
        .sn  = {255, 255, 255, 0},
        .gw  = {192, 168, 1, 1},
        .dhcp = NETINFO_STATIC
    };

    wizchip_setnetinfo(&net_info);    

    wiz_NetInfo check_info;
    wizchip_getnetinfo(&check_info);  

    Debug_Printf("W5500 IP: %d.%d.%d.%d\r\n",
            check_info.ip[0], check_info.ip[1],
            check_info.ip[2], check_info.ip[3]);  
}

void Network_SendTestPacket(void)
{
    uint8_t socket_status = getSn_SR(UDP_SOCKET);

    if (socket_status == SOCK_CLOSED)
    {
        socket(UDP_SOCKET, Sn_MR_UDP, LOCAL_PORT, 0);
    }

    if (getSn_SR(UDP_SOCKET) == SOCK_UDP)
    {
        sendto(UDP_SOCKET, msg, sizeof(msg) - 1, target_ip, target_port);
    }
}
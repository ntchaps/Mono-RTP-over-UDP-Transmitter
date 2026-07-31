/*
 * udp_stream.c
 *
 *  Created on: Jul 28, 2026
 *      Author: nickt
 */

#include "udp_stream.h"

#include "w5500_port.h"
#include "debug_uart.h"

#include "socket.h"

#include <stdint.h>
#include <stddef.h>

#define UDP_STREAM_SOCKET     0
#define UDP_STREAM_LOCAL_PORT     5000

// Set up target destination details
static uint8_t target_ip[4] = {192, 168, 1, 103};
static uint16_t target_port = 8080;

void UDP_Stream_Init(void)
{
    if (getSn_SR(UDP_STREAM_SOCKET) != SOCK_CLOSED)
    {
        close(UDP_STREAM_SOCKET);
    }

    socket(
        UDP_STREAM_SOCKET,
        Sn_MR_UDP,
        UDP_STREAM_LOCAL_PORT,
        0
    );
}

int32_t UDP_Stream_Send(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0)
    {
        return -1;
    }

    if (getSn_SR(UDP_STREAM_SOCKET) == SOCK_CLOSED)
    {
        socket(
            UDP_STREAM_SOCKET,
            Sn_MR_UDP,
            UDP_STREAM_LOCAL_PORT,
            0
        );
    }

    if (getSn_SR(UDP_STREAM_SOCKET) != SOCK_UDP)
    {
        return -1;
    }

    return sendto(
        UDP_STREAM_SOCKET,
        (uint8_t *)data,
        length,
        target_ip,
        target_port
    );
}


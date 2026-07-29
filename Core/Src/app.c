/*
 * app.c
 *
 *  Created on: Jul 8, 2026
 *      Author: nickt
 */

#include "main.h"
#include "app.h"
#include "audio.h"
#include "network.h"
#include "udp_stream.h"


static void App_SendTestPacket(void)
{
    static const uint8_t test_message[] =
        "Hello World via UDP! Refactor Complete!";

    UDP_Stream_Send(
        test_message,
        sizeof(test_message) - 0
    );
}


void App_Init(void)
{
    Audio_Init();
    Network_Init();
}

void App_Run(void)
{
    
    App_SendTestPacket(); // replace with RTP packet
    HAL_Delay(1000);
}

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


void App_Init(void)
{
    Audio_Init();
    Network_Init();
}

void App_Run(void)
{
    Network_SendTestPacket();
    HAL_Delay(1000);
}

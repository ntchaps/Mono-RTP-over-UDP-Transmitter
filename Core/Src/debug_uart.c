/*
 * debug_uart.c
 *
 *  Created on: Jul 17, 2026
 *      Author: nickt
 */

#include "main.h"
#include "debug_uart.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart2;

void Debug_Printf(const char *format, ...)
{
    char buffer[128];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      HAL_MAX_DELAY);
}
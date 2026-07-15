/*
 * w5500_port.c
 *
 *  Created on: Jul 17, 2026
 *      Author: nickt
 */
#include "main.h"
#include "w5500_port.h"
#include "wizchip_conf.h"

extern SPI_HandleTypeDef hspi1;

// Define CS pin control wrappers
void W5500_Select(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
}

// Defining CS pin control wrappers
void W5500_Unselect(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}

// SPI Read/Write Byte Wrappers
uint8_t W5500_ReadByte(void) {
	uint8_t rb;
	HAL_SPI_Receive(&hspi1, &rb, 1, HAL_MAX_DELAY);
	return rb;
}

void W5500_WriteByte(uint8_t wb) {
	HAL_SPI_Transmit(&hspi1, &wb, 1, HAL_MAX_DELAY);
}

void W5500_Port_Init(void)
{
        // Link SPI functions to WIZnet driver
        reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
        reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);  
}
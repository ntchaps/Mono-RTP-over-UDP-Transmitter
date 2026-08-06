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

#include <stdint.h>

void Network_Init(void)
{
    W5500_Port_Init();

    // PRINTOUT
    uint8_t version = getVERSIONR();
    Debug_Printf("W5500 VERSIONR = 0x%02X\r\n", version);
    
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
        check_info.ip[0], 
        check_info.ip[1],
        check_info.ip[2], 
        check_info.ip[3]
    );  
}


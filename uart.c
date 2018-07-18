#include "stc8f.h"
#include "intrins.h"
#include "uart.h"

extern bit busy;

void uart_init() {
    P_SW1 = 0x00;         //TXD/RXD引脚初始化 P3.1/P3.0
    SCON = 0x50;
    T2L = BRT;
    T2H = BRT >> 8;
    AUXR = 0x15;
    ES = 1;               //使能串口中断
}

void UartSend(char dat) {
    while (busy);
    busy = 1;
    SBUF = dat;
}




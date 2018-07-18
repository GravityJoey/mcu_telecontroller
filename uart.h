#ifndef UART_H
#define UART_H

#define FOSC  11059200UL
#define BRT   (65536 - FOSC / 9600 / 4)

void uart_init();

void UartSend(char dat);

#endif /* UART_H */


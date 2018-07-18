#include "stc8f.h"
#include "intrins.h"
#include "key_scan.h"
#include "led.h"
#include "uart.h"


extern char key_row;
extern char key_press;
extern char key_type;
extern char key_send_num;
extern bit mesh_state;
extern bit long_send_flg;
extern int time;
unsigned int press_time = 0;


void Delay10000us() {		//@11.0592MHz
    unsigned char i, j;

    _nop_();
    _nop_();
    i = 144;
    j = 157;
    do
    {
       while (--j);
    } while (--i);
}


void key_scan() {
    char key_value = 0;
    static char key_column = 0;

    P1 = P1 | 0xC3;
    P54 = 1;
    P3 = P3 & (~0xCC);

    Delay10000us();

    if(key_row != 0 && key_press == 0) {
        if(P10 == 0) key_column = 1;
        else if(P11 == 0) key_column = 2;
        else if(P16 == 0) key_column = 3;
        else if(P17 == 0) key_column = 4;
        else if(P54 == 0) key_column = 5;

        if(key_column != 0) {
            key_press = (key_row - 1) * 5 + key_column;
            long_send_flg = 1;

            if(mesh_state == 1)
                set_led_state(LED_ON);
            else
                set_led_state(LED_BLINK);

            press_time = time;
        }
    }

    if(key_press != 0) {
        switch(key_column) {
            case 1:
                key_value = P10;
            break;
            
            case 2:
                key_value = P11;
            break;
            
            case 3:
                key_value = P16;
            break;
            
            case 4:
                key_value = P17;
            break;
            
            case 5:
                key_value = P54;
            break;
            
            default:
            break;
        }

        if(key_value == 0) {
            if(time_exceed(press_time, 2000)) {
                key_type = KEY_PRESS_LONG;
                set_led_state(LED_BLINK);
            }
        }
        else {
            if(key_type == KEY_PRESS_LONG)
                key_type = KEY_PRESS_LONG_END;
            else
                key_type = KEY_PRESS_SHORT;

            set_led_state(LED_OFF);
        }

        if(key_type != KEY_PRESS) {
            key_send_num = key_press;
            if(key_type != KEY_PRESS_LONG) {
                key_press = 0;
                key_row = 0;
                key_column = 0;
            }
        }
    }
}


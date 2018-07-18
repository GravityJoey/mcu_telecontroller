#include "stc8f.h"
#include "intrins.h"
#include "mcu_telecontroller.h"
#include "led.h"
#include "key_scan.h"
#include "uart.h"

#define OUT_OF_MESH    0
#define IN_MESH        1

unsigned int time = 0;
unsigned int led_blink_time = 0;
bit busy = 0;
bit long_send_flg = 0;
bit int_flg = 0;
bit cmd_rece_flg = 0;
bit mesh_state = IN_MESH;
char led_blink_flg = 0;
char key_row = 0;
char key_press = 0;
char key_send_num = 0;
char key_type = 0;
char buffer[3];
char wptr = 0;
char mesh_cmd = 0;


struct led_state_t led_state;



void INT0_Isr() interrupt 0 using 1 {
    key_row = 1;
}
void INT1_Isr() interrupt 2 using 1 {
    key_row = 2;
}
void INT2_Isr() interrupt 10 using 1 {
    AUXINTIF = AUXINTIF & (~INT2IF);
    key_row = 3;
}
void INT3_Isr() interrupt 11 using 1 {
    AUXINTIF = AUXINTIF & (~INT3IF);
    key_row = 4;
}

void TM0_Isr() interrupt 1 using 1 {
    time++;                                //10ms
}

void UartIsr() interrupt 4 using 1 {
    if (TI) {
        TI = 0;
        busy = 0;
    }
    if (RI)
    {
        RI = 0;
        buffer[wptr++] = SBUF;
        if(wptr > 2) {
            wptr = 0;
            cmd_rece_flg = 1;
        }
    }
}

void Timer0_init() {
    AUXR &= 0x7F;		  //��ʱ��ʱ��12Tģʽ
    TMOD &= 0xF0;         //���ö�ʱ��ģʽ
    TL0 = 0x00;		      //���ö�ʱ��ֵ   10ms�ж�
    TH0 = 0xDC;		      //���ö�ʱ��ֵ   10ms�ж�
    TF0 = 0;		      //���TF0��־
    TR0 = 1;           //��ʱ��0��ʼ��ʱ
    ET0 = 1;              //ʹ�ܶ�ʱ���ж�
}

void INT_enable() {
    if(int_flg == 0) {
        IT0 = 1;                                    //ʹ��INT0�½����ж�
        EX0 = 1;                                    //ʹ��INT0�ж�
        IT1 = 1;                                    //ʹ��INT1�½����ж�
        EX1 = 1;                                    //ʹ��INT1�ж�
        INTCLKO = 0x30;                             //ʹ��INT2/INT3�ж� �½���
        int_flg = 1;
    }
}

void wakeup_8266() {
    P35 = 1;                                //����8266
}


void INT_disable() {
    if(int_flg) {
        wakeup_8266();
        EX0 = 0;                                //�ر�INT0�ж�
        EX1 = 0;                                //�ر�INT1�ж�
        INTCLKO = 0x00;                         //�ر�INT2/INT3�ж� �½���
        int_flg = 0;
    }
}

void into_sleep() {
    P1 = P1 & (~0xC3);
    P54 = 0;
    P3 = P3 | 0xCC;

    P35 = 0;                                    //����8266

    INT_enable();

    VOCTRL = 0x00;                              //����ģʽʱʹ���ڲ�SCCģ��,����Լ1.5uA
//    VOCTRL = 0x80;                              //����ģʽʱʹ���ⲿSCCģ��,����Լ0.15uA
    _nop_();
    _nop_();
//    PCON = IDL;                                 //MCU����IDLEģʽ
    PCON = PD;                                  //MCU�������ģʽ
    _nop_();
    _nop_();
}

void uart_run() {
    // send key num
    if(key_send_num != 0) {
        if(key_type == KEY_PRESS_LONG)
            key_send_num = key_send_num | 0x80;
        if(key_type == KEY_PRESS_LONG_END)
            key_send_num = key_send_num | 0xC0;

        if(key_type != KEY_PRESS_LONG || (key_type == KEY_PRESS_LONG && long_send_flg == 1)) {
//            if(mesh_state == IN_MESH) {
                UartSend(0x55);
                UartSend(0xAA);
                UartSend(key_send_num);
                long_send_flg = 0;
//            }
        }
        key_send_num = 0;
        if(key_type != KEY_PRESS_LONG)
            key_type = KEY_PRESS;
    }

    // receieve the data
    if(cmd_rece_flg) {
        cmd_rece_flg = 0;
        
        if(buffer[0] != 0x55) return;
        if(buffer[1] != 0x66) return;

//        UartSend(buffer[0]);
//        UartSend(buffer[1]);
//        UartSend(buffer[2]);

        mesh_cmd = buffer[2];

        switch(mesh_cmd) {
            case 0x00:
                mesh_state = OUT_OF_MESH;
            break;
            case 0x01:
                mesh_state = IN_MESH;
            break;
            case 0x02:
                mesh_state = OUT_OF_MESH;
                led_blink_flg = 1;
                led_blink_time = time;
                set_led_state(LED_BLINK);
            break;
            case 0x03:
                mesh_state = IN_MESH;
                led_blink_flg = 0;
                led_blink_time = 0;
                set_led_state(LED_OFF);
            break;

            default:
            break;
        }
    }
}

void main() {
    uart_init();
    Timer0_init();

    EA = 1;

    while(1) {
        INT_disable();            //���Ѻ� �ر��ⲿ�ж�

/*        if(mesh_state == OUT_OF_MESH) {
            led_blink_flg = 1;
            led_blink_time = time * 10;
            set_led_state(LED_BLINK);
        }
        else {
            led_blink_flg = 1;
            led_blink_time = 0;
            set_led_state(LED_OFF);
        }*/

        if(!led_blink_flg)
            key_scan();
        else {
            if(time_exceed(led_blink_time, 60000)) {
                led_blink_flg = 0;
                led_blink_time = 0;
                set_led_state(LED_OFF);
            }
        }
        uart_run();
        led_run();

        if(!busy && !key_press &&!led_blink_flg)
            into_sleep();
    }
}

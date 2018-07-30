#include "stc8f.h"
#include "intrins.h"
#include "mcu_telecontroller.h"
#include "led.h"
#include "key_scan.h"
#include "uart.h"

#define OUT_OF_MESH    0
#define IN_MESH        1

#define ALL_KEY_CHECK_MASK    0x3FFFF

unsigned int time = 0;
unsigned int led_blink_time = 0;
bit busy = 0;
bit long_send_flg = 0;
bit int_flg = 0;
bit cmd_rece_flg = 0;
bit mesh_state = IN_MESH;

char factory_test_flg = 0;
char fc_test_success = 0;
unsigned long fc_test_bit = 0;

char led_blink_flg = 0;
char key_row = 0;
char key_column = 0;
char key_press = 0;
char key_send_num = 0;
char key_type = 0;
unsigned char buffer[3];
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

void wait_400ms() {
    unsigned char i, j, k;

    i = 23;
    j = 113;
    k = 248;
    do {
        do {
            while (--k);
        } while (--j);
    } while (--i);
}

void Delay10ms()		//@11.0592MHz
{
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

void wakeup_8266() {
    P35 = 1;                   //����8266
    Delay10ms();
}

void INT_disable() {
    if(int_flg) {
        wakeup_8266();
        EX0 = 0;                  //�ر�INT0�ж�
        EX1 = 0;                  //�ر�INT1�ж�
        INTCLKO = 0x00;           //�ر�INT2/INT3�ж� �½���
        int_flg = 0;
    }
}

void into_sleep() {
    P1 = P1 & (~0xC3);
    P54 = 0;
    P3 = P3 | 0xCC;

    P35 = 0;                     //����8266

    INT_enable();

    VOCTRL = 0x00;               //����ģʽʱʹ���ڲ�SCCģ��,����Լ1.5uA
//    VOCTRL = 0x80;             //����ģʽʱʹ���ⲿSCCģ��,����Լ0.15uA
    _nop_();
    _nop_();
//    PCON = IDL;                //MCU����IDLEģʽ
    PCON = PD;                   //MCU�������ģʽ
    _nop_();
    _nop_();
}

void uart_send_key() {
    UartSend(0x5A);
    UartSend(0xA5);
    UartSend(key_send_num);
}

void uart_run() {
    //*********************// send key num
    if(key_send_num != 0) {
        if(key_type == KEY_PRESS_LONG)
            key_send_num = key_send_num | 0x80;
        if(key_type == KEY_PRESS_LONG_END)
            key_send_num = key_send_num | 0xC0;

        if(key_type != KEY_PRESS_LONG || (key_type == KEY_PRESS_LONG && long_send_flg == 1)) {
                uart_send_key();
                long_send_flg = 0;
        }
        key_send_num = 0;
        if(key_type != KEY_PRESS_LONG)
            key_type = KEY_PRESS;
    }

    //*******************// receieve the data
    if(cmd_rece_flg) {
        cmd_rece_flg = 0;
        
        if(buffer[0] != 0x5A) return;
        if(buffer[1] != 0xA5) return;

//        UartSend(buffer[0]);
//        UartSend(buffer[1]);
//        UartSend(buffer[2]);

        mesh_cmd = buffer[2];

        switch(mesh_cmd) {
//            case 0x00:
//                mesh_state = OUT_OF_MESH;
//            break;
//            case 0x01:
//                mesh_state = IN_MESH;
//            break;
            case 0x21:
                mesh_state = OUT_OF_MESH;
                led_blink_flg = 1;
                led_blink_time = time;
                set_led_state(LED_BLINK);
            break;
            case 0x22:
                if(mesh_state == OUT_OF_MESH)
                    set_led_state(LED_OFF);
                mesh_state = IN_MESH;
                led_blink_flg = 0;
            break;
            case 0x04:
                factory_test_flg = 1;
                fc_test_success = 0;
                fc_test_bit = 0;
                key_row = 0;
                key_column = 0;
                set_led_state(LED_BLINK_SLOWLY);
//                UartSend(0x66);
            break;

            default:
            break;
        }
    }
}

void factory_logic_test() {
//    static char key_send_num_pre = 0;
    if(!fc_test_success) {
        key_scan_logic_test();
//        if(key_send_num_pre != key_send_num) {
//            key_send_num_pre = key_send_num;
        fc_test_bit = fc_test_bit | (1 << (key_send_num - 1));
        fc_test_bit = fc_test_bit & ALL_KEY_CHECK_MASK;

        if(fc_test_bit == ALL_KEY_CHECK_MASK) {
            fc_test_success = 1;
            set_led_state(LED_ON);
        }
//        }
    }
}

void main() {
    uart_init();
    Timer0_init();

    EA = 1;

    while(1) {
        INT_disable();            //���Ѻ� �ر��ⲿ�ж�

        if(factory_test_flg) {
            factory_logic_test();
            led_run();
        }
        else {
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

            if(!busy && !key_press && !led_blink_flg) {
                wait_400ms();
                into_sleep();
            }
        }
    }
}


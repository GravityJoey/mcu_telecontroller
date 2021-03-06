#include "stc8f.h"
#include "intrins.h"
#include "mcu_telecontroller.h"
#include "led.h"
#include "key_scan.h"
#include "uart.h"

#define OUT_OF_MESH    0
#define IN_MESH        1

#define STATE_OUT_MESH  0x21
#define STATE_IN_MESH   0x22
#define FACTORY_TEST    0x23
#define STATE_NO_SLEEP  0x25

#define ALL_KEY_CHECK_MASK    0x3FFFF

unsigned long time = 0;
unsigned long led_blink_time = 0;
unsigned long led_blink_max_time = 0;
bit busy = 0;
bit long_send_flg = 0;
bit int_flg = 0;
bit cmd_rece_flg = 0;
bit mesh_state = IN_MESH;

char factory_test_flg = 0;
char fc_test_success = 0;
unsigned long fc_test_bit = 0;

char ota_flg = 0;
char led_blink_flg = 0;
char key_row = 0;
char key_column = 0;
char key_press = 0;
char key_send_num = 0;
char key_type = 0;
unsigned char buffer[3];
char wptr = 0;

struct led_state_t led_state;

void INT0_Isr() interrupt 0 using 0 {
}
void INT1_Isr() interrupt 2 using 0 {
}
void INT2_Isr() interrupt 10 using 0 {
    AUXINTIF = AUXINTIF & (~INT2IF);
}
void INT3_Isr() interrupt 11 using 0 {
    AUXINTIF = AUXINTIF & (~INT3IF);
}

void TM0_Isr() interrupt 1 using 1 {
    time++;                                //10ms
}

void UartIsr() interrupt 4 using 2 {
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

void Delay1000ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	i = 57;
	j = 27;
	k = 112;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void wakeup_8266() {
    P35 = 1;                   //唤醒8266
    Delay10ms();
}

void Timer0_init() {
    AUXR &= 0x7F;		  //定时器时钟12T模式
    TMOD &= 0xF0;         //设置定时器模式
    TL0 = 0x00;		      //设置定时初值   10ms中断
    TH0 = 0xDC;		      //设置定时初值   10ms中断
    TF0 = 0;		      //清除TF0标志
    TR0 = 1;           //定时器0开始计时
    ET0 = 1;              //使能定时器中断
}

void INT_enable() {
    if(int_flg == 0) {
        IT0 = 1;                                    //使能INT0下降沿中断
        EX0 = 1;                                    //使能INT0中断
        IT1 = 1;                                    //使能INT1下降沿中断
        EX1 = 1;                                    //使能INT1中断
        INTCLKO = 0x30;                             //使能INT2/INT3中断 下降沿
        int_flg = 1;
    }
}

void INT_disable() {
    if(int_flg) {
        wakeup_8266();
        EX0 = 0;                  //关闭INT0中断
        EX1 = 0;                  //关闭INT1中断
        INTCLKO = 0x00;           //关闭INT2/INT3中断 下降沿
        int_flg = 0;
    }
}

void into_sleep() {
    P1 &= (~0xC3);
    P54 = 0;
    P3 |= 0xCC;

    P35 = 0;                     //唤醒8266

    INT_enable();

    VOCTRL = 0x00;               //掉电模式时使用内部SCC模块,功耗约1.5uA
//    VOCTRL = 0x80;             //掉电模式时使用外部SCC模块,功耗约0.15uA
    _nop_();
    _nop_();
//    PCON = IDL;                //MCU进入IDLE模式
    PCON = PD;                   //MCU进入掉电模式
    _nop_();
    _nop_();
}

void uart_send_key(char data_t){
    UartSend(0x5A);
    UartSend(0xA5);
    UartSend(data_t);

}

void uart_recieve_cmd(){
    char mesh_cmd = 0;
    if(cmd_rece_flg) {
        cmd_rece_flg = 0;
        
        if(buffer[0] != 0x5A) return;
        if(buffer[1] != 0xA5) return;

//        UartSend(buffer[0]);
//        UartSend(buffer[1]);
//        UartSend(buffer[2]);

        mesh_cmd = buffer[2];

        switch(mesh_cmd) {
            case STATE_OUT_MESH:
                mesh_state = OUT_OF_MESH;
                led_blink_flg = 1;
                led_blink_time = time;
                led_blink_max_time = 60000;
                set_led_state(LED_BLINK);
            break;
            case STATE_IN_MESH:
                if(mesh_state == OUT_OF_MESH) {
                    set_led_state(LED_OFF);
                    Delay1000ms();
                }
                mesh_state = IN_MESH;
                if(ota_flg){
                    ota_flg = 0;
                    led_blink_time = time;
                    led_blink_max_time = 10000;
                }
                else{
                    led_blink_flg = 0;
                }
            break;
            case FACTORY_TEST:
                factory_test_flg = 1;
                led_blink_flg = 0;
                fc_test_success = 0;
                fc_test_bit = 0;
                key_row = 0;
                key_column = 0;
                key_press = 0;
                set_led_state(LED_BLINK_SLOWLY);
                Delay1000ms();
            break;
            case STATE_NO_SLEEP:
                ota_flg = 1;
                led_blink_flg = 1;
                led_blink_time = time;
                led_blink_max_time = 120000;
                set_led_state(LED_BLINK);
            break;

            default:
            break;
        }
    }
}

void uart_run() {
    //*********************// send key num
    if(key_send_num != 0) {
        if(key_type == KEY_PRESS_LONG)
            key_send_num = key_send_num | 0x80;
        if(key_type == KEY_PRESS_LONG_END)
            key_send_num = key_send_num | 0xC0;

        if(key_type != KEY_PRESS_LONG || (key_type == KEY_PRESS_LONG && long_send_flg == 1)) {
            uart_send_key(key_send_num);
            long_send_flg = 0;
        }
        key_send_num = 0;
        if(key_type != KEY_PRESS_LONG)
            key_type = KEY_PRESS;
    }

    //*******************// receieve the data
    uart_recieve_cmd();
}

void factory_logic_test() {
    if(!fc_test_success) {
        key_scan_logic_test();
        fc_test_bit = fc_test_bit | (1 << (key_send_num - 1));
        fc_test_bit = fc_test_bit & ALL_KEY_CHECK_MASK;

        if(fc_test_bit == ALL_KEY_CHECK_MASK) {
            fc_test_success = 1;
            set_led_state(LED_ON);
        }
    }
}

void main() {
    uart_init();
    Timer0_init();

    EA = 1;

    while(1) {
        INT_disable();            //唤醒后 关闭外部中断

        if(factory_test_flg) {
            factory_logic_test();
            led_run();
        }
        else {
            if(!led_blink_flg)
                key_scan();
            else {
                if(time_exceed(led_blink_time, led_blink_max_time)) {
                    led_blink_flg = 0;
                    led_blink_time = 0;
                    set_led_state(LED_OFF);
                }
            }
            uart_run();
            led_run();

            if(!busy && !key_press && !led_blink_flg &&!factory_test_flg) {
                into_sleep();
            }
        }
    }
}


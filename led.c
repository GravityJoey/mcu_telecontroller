#include "stc8f.h"
#include "intrins.h"
#include "led.h"
#include "uart.h"

extern unsigned int time;
extern struct led_state_t led_state;

char time_exceed(unsigned int time_pre, unsigned int cycle_ms) {
    return (char)((time - time_pre) > (cycle_ms/10));
}

void set_led_state(char state) {
    led_state.state = state;
}

void led_run() {
    static int blink_time = 0;

    if(led_state.state != led_state.pre_state) {
        led_state.pre_state = led_state.state;
        
        switch(led_state.state) {
            case LED_ON:
                P55 = 0;
            break;

            case LED_OFF:
                P55 = 1;
            break;

            case LED_BLINK:
                blink_time = time;
            break;

            default:
            break;
        }
    }

    if(led_state.state == LED_BLINK) {
        if(time_exceed(blink_time, 300)) {
            P55 = ~P55;
            blink_time = time;
        }
    }

}



















#include "stc8f.h"
#include "intrins.h"
#include "led.h"
#include "uart.h"

extern unsigned long time;
extern struct led_state_t led_state;

char time_exceed(unsigned long time_pre, unsigned long cycle_ms) {
    return (char)((time - time_pre) > (cycle_ms/10));
}

void set_led_state(char state) {
    led_state.state = state;
}

void led_run() {
    static long interval_time = 0;

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
            case LED_BLINK_SLOWLY:
                interval_time = time;
            break;

            default:
            break;
        }
    }

    if(led_state.state == LED_BLINK) {
        if(time_exceed(interval_time, 300)) {
            P55 = ~P55;
            interval_time = time;
        }
    }
    if(led_state.state == LED_BLINK_SLOWLY) {
        if(time_exceed(interval_time, 600)) {
            P55 = ~P55;
            interval_time = time;
        }
    }

}



















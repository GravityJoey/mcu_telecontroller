#ifndef LED_H
#define LED_H

#define LED_OFF                0
#define LED_ON                 1
#define LED_BLINK              2

struct led_state_t {
    char state;
    char pre_state;
};



void led_run();

char time_exceed(unsigned int time_pre, unsigned int cycle_ms);

void set_led_state(char state);







#endif /* LED_H */


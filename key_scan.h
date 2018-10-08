#ifndef KEY_SCAN_H
#define KEY_SCAN_H



enum{
    KEY_PRESS              = 0,
    KEY_PRESS_SHORT        = 1,
    KEY_PRESS_LONG         = 2,
    KEY_PRESS_LONG_END     = 3,
};

void Delay10000us();

void key_scan();

void key_scan_logic_test();





#endif /* KEY_SCAN_H */

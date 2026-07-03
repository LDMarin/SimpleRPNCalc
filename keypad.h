#ifndef KEYPAD_H
#define KEYPAD_H

#include "pico/stdlib.h"

#define NUM_ROWS 4
#define NUM_COLS 4
#define PIN_SHIFT_F 10
#define PIN_SHIFT_G 11
#define PIN_LED_F 27
#define PIN_LED_G 28

typedef enum
{
    SHIFT_NONE,
    SHIFT_F,
    SHIFT_G
} ShiftState;

typedef struct
{
    char code;
    ShiftState shift;
} KeyPress;

extern const uint ROW_PINS[NUM_ROWS];
extern const uint COL_PINS[NUM_COLS];
extern const char KEY_MAP[NUM_ROWS][NUM_COLS];

// Prototypes
void keypad_init(void);
KeyPress scan_keypad(void);
void update_shift_led(ShiftState state);

#endif // KEYPAD_H
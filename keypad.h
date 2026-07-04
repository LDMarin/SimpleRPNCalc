#ifndef KEYPAD_H
#define KEYPAD_H

#include "pico/stdlib.h"

#define NUM_ROWS 4
#define NUM_COLS 4
#define PIN_SHIFT_F 10
#define PIN_SHIFT_G 11
#define PIN_LED_F 27
#define PIN_LED_G 28

// Abstract enumeration completely decoupling coordinates from operational logic
typedef enum
{
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_PLUS,
    KEY_MINUS,
    KEY_MULT,
    KEY_DIV,
    KEY_DOT,
    KEY_ENTER,
    KEY_NONE
} KeyId;

typedef enum
{
    SHIFT_NONE,
    SHIFT_F,
    SHIFT_G
} ShiftState;

// Upgraded frame: id drives routing choices; symbol carries numeric strings
typedef struct
{
    KeyId id;
    char symbol;
    ShiftState shift;
} KeyPress;

extern const uint ROW_PINS[NUM_ROWS];
extern const uint COL_PINS[NUM_COLS];
extern const KeyId KEY_MAP_ID[NUM_ROWS][NUM_COLS];

// Prototypes
void keypad_init(void);
KeyPress scan_keypad(void);
void update_shift_led(ShiftState state);

#endif // KEYPAD_H
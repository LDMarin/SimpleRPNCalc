#include "keypad.h"

const uint ROW_PINS[NUM_ROWS] = {2, 3, 4, 5};
const uint COL_PINS[NUM_COLS] = {6, 7, 8, 9};

// Decoupled hardware table layout
const KeyId KEY_MAP_ID[NUM_ROWS][NUM_COLS] = {
    {KEY_0, KEY_1, KEY_4, KEY_7},
    {KEY_DOT, KEY_2, KEY_5, KEY_8},
    {KEY_ENTER, KEY_3, KEY_6, KEY_9},
    {KEY_PLUS, KEY_MINUS, KEY_MULT, KEY_DIV}};

// Translates a structured ID back into an entry character asset safely
static char get_symbol_from_id(KeyId id)
{
    switch (id)
    {
    case KEY_0:
        return '0';
    case KEY_1:
        return '1';
    case KEY_2:
        return '2';
    case KEY_3:
        return '3';
    case KEY_4:
        return '4';
    case KEY_5:
        return '5';
    case KEY_6:
        return '6';
    case KEY_7:
        return '7';
    case KEY_8:
        return '8';
    case KEY_9:
        return '9';
    case KEY_PLUS:
        return '+';
    case KEY_MINUS:
        return '-';
    case KEY_MULT:
        return '*';
    case KEY_DIV:
        return '/';
    case KEY_DOT:
        return '.';
    case KEY_ENTER:
        return 'E';
    default:
        return '\0';
    }
}

void keypad_init(void)
{
    gpio_init(PIN_LED_F);
    gpio_set_dir(PIN_LED_F, GPIO_OUT);
    gpio_put(PIN_LED_F, 1);

    gpio_init(PIN_LED_G);
    gpio_set_dir(PIN_LED_G, GPIO_OUT);
    gpio_put(PIN_LED_G, 1);

    for (int r = 0; r < NUM_ROWS; r++)
    {
        gpio_init(ROW_PINS[r]);
        gpio_set_dir(ROW_PINS[r], GPIO_OUT);
        gpio_put(ROW_PINS[r], 1);
    }

    for (int c = 0; c < NUM_COLS; c++)
    {
        gpio_init(COL_PINS[c]);
        gpio_set_dir(COL_PINS[c], GPIO_IN);
        gpio_pull_up(COL_PINS[c]);
    }

    gpio_init(PIN_SHIFT_F);
    gpio_set_dir(PIN_SHIFT_F, GPIO_IN);
    gpio_pull_up(PIN_SHIFT_F);

    gpio_init(PIN_SHIFT_G);
    gpio_set_dir(PIN_SHIFT_G, GPIO_IN);
    gpio_pull_up(PIN_SHIFT_G);
}

void update_shift_led(ShiftState state)
{
    if (state == SHIFT_F)
    {
        gpio_put(PIN_LED_F, 0);
        gpio_put(PIN_LED_G, 1);
    }
    else if (state == SHIFT_G)
    {
        gpio_put(PIN_LED_F, 1);
        gpio_put(PIN_LED_G, 0);
    }
    else
    {
        gpio_put(PIN_LED_F, 1);
        gpio_put(PIN_LED_G, 1);
    }
}

KeyPress scan_keypad(void)
{
    KeyPress result = {KEY_NONE, '\0', SHIFT_NONE};

    for (int r = 0; r < NUM_ROWS; r++)
    {
        gpio_put(ROW_PINS[r], 0);
        sleep_us(5);

        for (int c = 0; c < NUM_COLS; c++)
        {
            if (gpio_get(COL_PINS[c]) == 0)
            {
                if (gpio_get(PIN_SHIFT_F) == 0)
                    result.shift = SHIFT_F;
                else if (gpio_get(PIN_SHIFT_G) == 0)
                    result.shift = SHIFT_G;
                else
                    result.shift = SHIFT_NONE;

                result.id = KEY_MAP_ID[r][c];
                result.symbol = get_symbol_from_id(result.id);

                while (gpio_get(COL_PINS[c]) == 0)
                {
                    sleep_ms(10);
                }

                gpio_put(ROW_PINS[r], 1);
                return result;
            }
        }
        gpio_put(ROW_PINS[r], 1);
    }
    return result;
}
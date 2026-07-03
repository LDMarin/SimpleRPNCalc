#include "keypad.h"

const uint ROW_PINS[NUM_ROWS] = {2, 3, 4, 5};
const uint COL_PINS[NUM_COLS] = {6, 7, 8, 9};

const char KEY_MAP[NUM_ROWS][NUM_COLS] = {
    {'0', '1', '4', '7'},
    {'.', '2', '5', '8'},
    {'E', '3', '6', '9'},
    {'+', '-', '*', '/'}};

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
    KeyPress result = {'\0', SHIFT_NONE};

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

                result.code = KEY_MAP[r][c];

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
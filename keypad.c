/**
 * @file keypad.c
 * @brief Implementation of the 4x4 matrix keyboard scanner and shift modifier logic.
 *
 * This module manages GPIO configuration, hardware de-bouncing, active-low line scanning,
 * and stateful tracking for the gold [f] and blue [g] application modifier layers.
 */

#include "keypad.h"

/* --- Global Matrix Configuration Mapping Arrays --- */

/**
 * @brief Array of GPIO pin assignments designated as Row outputs.
 */
const uint ROW_PINS[NUM_ROWS] = {2, 3, 4, 5};

/**
 * @brief Array of GPIO pin assignments designated as Column inputs.
 */
const uint COL_PINS[NUM_COLS] = {6, 7, 8, 9};

/**
 * @brief Static 2D mapping grid correlating mechanical intersections to abstract functional KeyIds.
 *
 * Rows represent the activated driver pin; Columns represent the detected sensor pin.
 */
const KeyId KEY_MAP_ID[NUM_ROWS][NUM_COLS] = {
    {KEY_7, KEY_8, KEY_9, KEY_DIV},       // Row 0 Mapping
    {KEY_4, KEY_5, KEY_6, KEY_MULT},      // Row 1 Mapping
    {KEY_1, KEY_2, KEY_3, KEY_MINUS},     // Row 2 Mapping
    {KEY_0, KEY_DOT, KEY_ENTER, KEY_PLUS} // Row 3 Mapping
};

/**
 * @brief Mapping grid translating abstract KeyIds to their baseline ASCII characters.
 *
 * Indexed sequentially based on the underlying `KeyId` enum values.
 */
static const char KEY_CHAR_MAP[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '-', '*', '/', '.', '\n', '\0'};

/* --- Driver Function Implementations --- */

void keypad_init(void)
{
    // Configure all row pins as driven digital outputs, defaulted to HIGH
    for (int i = 0; i < NUM_ROWS; i++)
    {
        gpio_init(ROW_PINS[i]);
        gpio_set_dir(ROW_PINS[i], GPIO_OUT);
        gpio_put(ROW_PINS[i], 1);
    }

    // Configure all column pins as digital inputs backed by internal pull-up networks
    for (int j = 0; j < NUM_COLS; j++)
    {
        gpio_init(COL_PINS[j]);
        gpio_set_dir(COL_PINS[j], GPIO_IN);
        gpio_pull_up(COL_PINS[j]);
    }

    // Configure Modifier [f] and [g] pins as input rails with pull-ups
    gpio_init(PIN_SHIFT_F);
    gpio_set_dir(PIN_SHIFT_F, GPIO_IN);
    gpio_pull_up(PIN_SHIFT_F);

    gpio_init(PIN_SHIFT_G);
    gpio_set_dir(PIN_SHIFT_G, GPIO_IN);
    gpio_pull_up(PIN_SHIFT_G);

    // Configure Shift Mode visual status indicators as outputs, driven active-low (Off by default)
    gpio_init(PIN_LED_F);
    gpio_set_dir(PIN_LED_F, GPIO_OUT);
    gpio_put(PIN_LED_F, 1);

    gpio_init(PIN_LED_G);
    gpio_set_dir(PIN_LED_G, GPIO_OUT);
    gpio_put(PIN_LED_G, 1);
}

void update_shift_led(ShiftState state)
{
    switch (state)
    {
    case SHIFT_F:
        gpio_put(PIN_LED_F, 0); // Light up Gold Indicator
        gpio_put(PIN_LED_G, 1); // Darken Blue Indicator
        break;

    case SHIFT_G:
        gpio_put(PIN_LED_F, 1); // Darken Gold Indicator
        gpio_put(PIN_LED_G, 0); // Light up Blue Indicator
        break;

    case SHIFT_NONE:
    default:
        gpio_put(PIN_LED_F, 1); // Darken Gold Indicator
        gpio_put(PIN_LED_G, 1); // Darken Blue Indicator
        break;
    }
}

KeyPress scan_keypad(void)
{
    static ShiftState current_shift = SHIFT_NONE;
    KeyPress result = {KEY_NONE, '\0', SHIFT_NONE};

    // --- Phase 1: Evaluate Momentary Modifier State Changes ---
    // Physical buttons use active-low logic (0 = Pressed)
    if (gpio_get(PIN_SHIFT_F) == 0)
    {
        // Toggle or engage Gold layer; override conflicting blue state
        current_shift = (current_shift == SHIFT_F) ? SHIFT_NONE : SHIFT_F;
        update_shift_led(current_shift);
        sleep_ms(200); // Mechanical debounce window
        return result; // Return early with KEY_NONE
    }

    if (gpio_get(PIN_SHIFT_G) == 0)
    {
        // Toggle or engage Blue layer; override conflicting gold state
        current_shift = (current_shift == SHIFT_G) ? SHIFT_NONE : SHIFT_G;
        update_shift_led(current_shift);
        sleep_ms(200); // Mechanical debounce window
        return result; // Return early with KEY_NONE
    }

    // --- Phase 2: Sequential Matrix Scan Execution ---
    for (int r = 0; r < NUM_ROWS; r++)
    {
        // Pull current working row down to LOW to enable conduction loop
        gpio_put(ROW_PINS[r], 0);
        sleep_us(5); // Allow lines to electrically settle

        for (int c = 0; c < NUM_COLS; c++)
        {
            // Read column input line state
            if (gpio_get(COL_PINS[c]) == 0)
            {
                // Key actuation confirmed! Extract identifier profiles
                result.id = KEY_MAP_ID[r][c];
                result.symbol = KEY_CHAR_MAP[result.id];
                result.shift = current_shift;

                // Reset functional shift layers back to base baseline post-execution
                current_shift = SHIFT_NONE;
                update_shift_led(current_shift);

                // Hold execution thread until the physical key release is detected
                while (gpio_get(COL_PINS[c]) == 0)
                {
                    sleep_ms(10);
                }

                sleep_ms(50);             // Trailing debounce isolation pad
                gpio_put(ROW_PINS[r], 1); // Restore current row line to HIGH
                return result;
            }
        }

        // Restore current row line to HIGH before shifting to next iteration
        gpio_put(ROW_PINS[r], 1);
    }

    return result;
}
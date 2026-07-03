#include "pico/stdlib.h"
#include "rpn_stack.h"
#include "max7219.h"
#include "keypad.h"
#include "rpn_nvram.h"
#include <math.h>

int main(void)
{
    stdio_init_all();
    max7219_init();
    nvram_init();
    keypad_init();
    refresh_hardware_display();

    ShiftState active_shift = SHIFT_NONE;
    bool last_f = true;
    bool last_g = true;

    while (true)
    {
        bool curr_f = gpio_get(PIN_SHIFT_F);
        bool curr_g = gpio_get(PIN_SHIFT_G);

        if (last_f && !curr_f)
        {
            sleep_ms(5);
            active_shift = (active_shift == SHIFT_F) ? SHIFT_NONE : SHIFT_F;
            update_shift_led(active_shift);
        }
        if (last_g && !curr_g)
        {
            sleep_ms(5);
            active_shift = (active_shift == SHIFT_G) ? SHIFT_NONE : SHIFT_G;
            update_shift_led(active_shift);
        }
        last_f = curr_f;
        last_g = curr_g;

        KeyPress kp = scan_keypad();

        if (kp.id != KEY_NONE)
        {
            // If the unit was asleep, wake up the silicon and consume the keypress gracefully
            if (is_sleeping)
            {
                write_register_all(CMD_SHUTDOWN, 1);       // Wake up drivers
                max7219_set_brightness(current_intensity); // Re-establish light level
                refresh_hardware_display();                // Bring back the numbers

                is_sleeping = false;
                nvram_reset_timer(); // Reset our window

                // Wait for the user to lift their finger so it doesn't leak into calculations
                sleep_ms(200);
                continue;
            }
            nvram_reset_timer();

            // --- UPDATED: [f] HOLD DOWN + KEY_0 SHUTDOWN ---
            if (gpio_get(PIN_SHIFT_F) == 0 && kp.id == KEY_0)
            {
                nvram_force_sleep();
                active_shift = SHIFT_NONE; // Reset shift state cleanly
                update_shift_led(active_shift);
                continue;
            }

            // Live Momentary Brightness Override Engine
            if (gpio_get(PIN_SHIFT_F) == 0 && (kp.id == KEY_PLUS || kp.id == KEY_MINUS))
            {
                if (kp.id == KEY_PLUS)
                {
                    if (current_intensity < 15)
                    {
                        current_intensity++;
                        write_register_all(CMD_BRIGHTNESS, current_intensity);
                    }
                }
                else
                {
                    if (current_intensity > 0)
                    {
                        current_intensity--;
                        write_register_all(CMD_BRIGHTNESS, current_intensity);
                    }
                }
                active_shift = SHIFT_NONE;
                update_shift_led(active_shift);
                refresh_hardware_display();
                continue;
            }

            ShiftState current_execution_shift = active_shift;
            active_shift = SHIFT_NONE;
            update_shift_led(active_shift);

            if (math_error)
            {
                if (current_execution_shift == SHIFT_G && kp.id == KEY_ENTER)
                {
                    math_error = false;
                    input.length = 0;
                    input.buffer[0] = '\0';
                    input.has_decimal = false;
                    input.is_editing = false;
                    input.disable_lift = true;
                    refresh_hardware_display();
                }
                continue;
            }

            // --- GOLD [f] SHIFT RAIL ---
            if (current_execution_shift == SHIFT_F)
            {
                if (input.is_editing)
                {
                    input.is_editing = false;
                    input.length = 0;
                    input.has_decimal = false;
                }

                if (kp.id == KEY_ENTER)
                {
                    double temp = stack.x;
                    stack.x = stack.y;
                    stack.y = temp;
                }
                else if (kp.id == KEY_DOT)
                {
                    if (!input.disable_lift)
                        stack_push(stack.x);
                    stack.x = 3.141592653589793;
                    input.disable_lift = false;
                }
                else if (kp.id == KEY_0)
                {
                    rpn_roll_down();
                    lift_enabled = false;
                }
                else if (kp.id == KEY_1)
                {
                    stack.x = pow(10.0, stack.x);
                    lift_enabled = true;
                }
                else if (kp.id == KEY_2)
                {
                    stack.x = exp(stack.x);
                    lift_enabled = true;
                }
                else if (kp.id == KEY_3)
                {
                    stack.x = stack.x * stack.x;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_4)
                {
                    mode_degrees = !mode_degrees;
                    uint8_t flash_y[8];
                    uint8_t flash_x[8];
                    for (int i = 0; i < 8; i++)
                    {
                        flash_y[i] = CODE_B_BLANK;
                        flash_x[i] = CODE_B_BLANK;
                    }
                    flash_x[7] = mode_degrees ? 1 : 2;

                    for (int digit = 0; digit < 8; digit++)
                    {
                        write_register_dual(CMD_DIGIT0 + digit, flash_y[digit], flash_x[digit]);
                    }
                    sleep_ms(500);
                    lift_enabled = false;
                }
                else if (kp.id == KEY_5)
                {
                    rpn_integer_part();
                    lift_enabled = true;
                }
                else if (kp.id == KEY_6)
                { // --- STO ---
                    int idx = capture_two_digits();
                    if (idx >= 0 && idx <= 24)
                        rpn_store_register(idx);
                    else
                        math_error = true;

                    refresh_hardware_display();
                    continue;
                }
                else if (kp.id == KEY_7)
                {
                    double angle = stack.x;
                    if (mode_degrees)
                        angle *= DEG_TO_RAD;
                    stack.x = sin(angle);
                    lift_enabled = true;
                }
                else if (kp.id == KEY_8)
                {
                    double angle = stack.x;
                    if (mode_degrees)
                        angle *= DEG_TO_RAD;
                    stack.x = cos(angle);
                    lift_enabled = true;
                }
                else if (kp.id == KEY_9)
                {
                    double angle = stack.x;
                    if (mode_degrees)
                        angle *= DEG_TO_RAD;
                    stack.x = tan(angle);
                    lift_enabled = true;
                }
                else if (kp.id == KEY_PLUS)
                {
                    rpn_last_x();
                }
                else if (kp.id == KEY_MINUS)
                {
                    rpn_change_sign();
                }
                else if (kp.id == KEY_DIV)
                {
                    rpn_reciprocal();
                }
                else if (kp.id == KEY_MULT)
                {
                    rpn_power();
                }
            }
            // --- BLUE [g] SHIFT RAIL ---
            else if (current_execution_shift == SHIFT_G)
            {
                if (input.is_editing)
                {
                    input.is_editing = false;
                    input.length = 0;
                    input.has_decimal = false;
                }

                if (kp.id == KEY_ENTER)
                {
                    if (gpio_get(PIN_SHIFT_G) == 0)
                    {
                        // --- CLEAR STACK IMPLEMENTATION ---
                        stack.x = 0.0;
                        stack.y = 0.0;
                        stack.z = 0.0;
                        stack.t = 0.0;

                        math_error = false;
                        input.disable_lift = true;
                    }
                    else
                    {
                        // --- STANDARD CLx BEHAVIOR ---
                        if (math_error)
                            math_error = false;
                        else
                            stack.x = 0.0;
                        input.disable_lift = true;
                    }
                }
                else if (kp.id == KEY_DOT)
                {
                    rpn_factorial();
                }
                else if (kp.id == KEY_0)
                {
                    rpn_roll_up();
                    lift_enabled = false;
                }
                else if (kp.id == KEY_1)
                {
                    if (stack.x > 0.0)
                        stack.x = log10(stack.x);
                    else
                        math_error = true;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_2)
                {
                    if (stack.x > 0.0)
                        stack.x = log(stack.x);
                    else
                        math_error = true;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_3)
                {
                    if (stack.x >= 0.0)
                        stack.x = sqrt(stack.x);
                    else
                        math_error = true;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_4)
                {
                    rpn_toggle_coordinates();

                    uint8_t flash_y[8];
                    uint8_t flash_x[8];
                    for (int i = 0; i < 8; i++)
                    {
                        flash_y[i] = CODE_B_BLANK;
                        flash_x[i] = CODE_B_BLANK;
                    }

                    flash_x[7] = mode_polar ? CODE_B_P : CODE_B_BLANK;

                    for (int digit = 0; digit < 8; digit++)
                    {
                        write_register_dual(CMD_DIGIT0 + digit, flash_y[digit], flash_x[digit]);
                    }
                    sleep_ms(300);
                    lift_enabled = false;
                }
                else if (kp.id == KEY_5)
                {
                    rpn_fractional_part();
                    lift_enabled = true;
                }
                else if (kp.id == KEY_6)
                { // --- RCL ---
                    int idx = capture_two_digits();
                    if (idx >= 0 && idx <= 24)
                        rpn_recall_register(idx);
                    else
                        math_error = true;

                    refresh_hardware_display();
                    continue;
                }
                else if (kp.id == KEY_7)
                {
                    stack.x = asin(stack.x);
                    if (mode_degrees)
                        stack.x *= RAD_TO_DEG;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_8)
                {
                    stack.x = acos(stack.x);
                    if (mode_degrees)
                        stack.x *= RAD_TO_DEG;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_9)
                {
                    stack.x = atan(stack.x);
                    if (mode_degrees)
                        stack.x *= RAD_TO_DEG;
                    lift_enabled = true;
                }
                else if (kp.id == KEY_PLUS)
                {
                    rpn_view_stack();
                }
                else if (kp.id == KEY_MINUS)
                {
                    uint8_t flash_y[8] = {CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK,
                                          CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK};
                    uint8_t flash_x[8] = {CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK,
                                          CODE_B_BLANK, CODE_B_BLANK, CODE_B_BLANK, CODE_B_P};

                    for (int d = 0; d < 8; d++)
                    {
                        write_register_dual(CMD_DIGIT0 + d, flash_y[d], flash_x[d]);
                    }

                    KeyPress num_kp = {KEY_NONE, '\0', SHIFT_NONE};
                    while (num_kp.id == KEY_NONE)
                    {
                        num_kp = scan_keypad();
                        sleep_ms(10);
                    }

                    if (num_kp.id >= KEY_0 && num_kp.id <= KEY_9)
                    {
                        fix_digits = num_kp.symbol - '0';
                    }
                    lift_enabled = false;
                }
                else if (kp.id == KEY_DIV)
                {
                    rpn_percentage();
                }
                else if (kp.id == KEY_MULT)
                {
                    rpn_percent_difference();
                }
            }
            // --- STANDARD UN-SHIFTED KEYS ---
            else
            {
                if ((kp.id >= KEY_0 && kp.id <= KEY_9) || kp.id == KEY_DOT)
                {
                    input_add_char(kp.symbol); // Reused the clean ASCII character asset
                }
                else if (kp.id == KEY_ENTER)
                {
                    rpn_enter();
                }
                else if (kp.id == KEY_PLUS || kp.id == KEY_MINUS || kp.id == KEY_MULT || kp.id == KEY_DIV)
                {
                    rpn_execute_operator(kp.symbol);
                }
            }

            refresh_hardware_display();
        }
        nvram_check_timeout();
        sleep_ms(20);
    }
}
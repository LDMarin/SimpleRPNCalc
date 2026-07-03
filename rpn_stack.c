#include "rpn_stack.h"
#include "max7219.h"
#include "pico/stdlib.h"
#include "keypad.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

// Initialize state memory
RPN_Stack stack = {0.0, 0.0, 0.0, 0.0};
InputBuffer input = {"", 0, false, false, false};
uint8_t fix_digits = 4;
uint8_t current_intensity = 4;
bool lift_enabled = false;
bool math_error = false;
bool mode_degrees = true;
bool mode_polar = false;
double last_x = 0.0;
double registers[25] = {0.0};

void stack_push(double value)
{
    stack.t = stack.z;
    stack.z = stack.y;
    stack.y = stack.x;
    stack.x = value;
}

double stack_pop(void)
{
    double value = stack.x;
    stack.x = stack.y;
    stack.y = stack.z;
    stack.z = stack.t;
    return value;
}

void input_add_char(char c)
{
    if (!input.is_editing)
    {
        if (lift_enabled || !input.disable_lift)
        {
            stack.t = stack.z;
            stack.z = stack.y;
            stack.y = stack.x;
        }
        input.length = 0;
        input.buffer[0] = '\0';
        input.has_decimal = false;
        input.is_editing = true;
        lift_enabled = false;
    }

    if (input.length >= MAX_INPUT_LEN)
        return;

    if (c == '.')
    {
        if (input.has_decimal)
            return;
        input.has_decimal = true;
    }

    input.buffer[input.length++] = c;
    input.buffer[input.length] = '\0';
    stack.x = atof(input.buffer);
}

void rpn_enter(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
        stack_push(stack.x);
        input.disable_lift = true;
    }
    else
    {
        stack_push(stack.x);
        input.disable_lift = true;
    }
}

void rpn_execute_operator(char op)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    last_x = stack.x;

    double operand2 = stack_pop();
    double operand1 = stack_pop();
    double result = 0.0;

    switch (op)
    {
    case '+':
        result = operand1 + operand2;
        input.disable_lift = false;
        break;
    case '-':
        result = operand1 - operand2;
        input.disable_lift = false;
        break;
    case '*':
        result = operand1 * operand2;
        input.disable_lift = false;
        break;
    case '/':
        if (operand2 != 0.0)
        {
            result = operand1 / operand2;
            input.disable_lift = false;
            lift_enabled = true;
        }
        else
        {
            math_error = true;
            stack_push(operand1);
            stack_push(operand2);
            result = operand2;
        }
        break;
    default:
        return;
    }

    stack_push(result);
    lift_enabled = false;
}

void rpn_rect_to_polar(void)
{
    double x = stack.x;
    double y = stack.y;

    double r = sqrt(x * x + y * y);
    double theta = atan2(y, x); // Returns radians between -PI and +PI

    if (mode_degrees)
    {
        theta *= RAD_TO_DEG;
    }

    stack.x = r;     // X register gets magnitude
    stack.y = theta; // Y register gets angle
}

void rpn_polar_to_rect(void)
{
    double r = stack.x;
    double theta = stack.y;

    double theta_rad = theta;
    if (mode_degrees)
    {
        theta_rad *= DEG_TO_RAD;
    }

    double x = r * cos(theta_rad);
    double y = r * sin(theta_rad);

    stack.x = x; // X register gets horizontal component
    stack.y = y; // Y register gets vertical component
}

void rpn_toggle_coordinates(void)
{
    if (!mode_polar)
    {
        // --- Rectangular to Polar (Default Execution) ---
        double x = stack.x;
        double y = stack.y;

        double r = sqrt(x * x + y * y);
        double theta = atan2(y, x); // Radians (-PI to +PI)

        if (mode_degrees)
        {
            theta *= RAD_TO_DEG;
        }

        stack.x = r;
        stack.y = theta;
        mode_polar = true;
    }
    else
    {
        // --- Polar to Rectangular ---
        double r = stack.x;
        double theta = stack.y;

        double theta_rad = theta;
        if (mode_degrees)
        {
            theta_rad *= DEG_TO_RAD;
        }

        double x = r * cos(theta_rad);
        double y = r * sin(theta_rad);

        stack.x = x;
        stack.y = y;
        mode_polar = false;
    }
}

void rpn_roll_down(void)
{
    double temp = stack.x;
    stack.x = stack.y;
    stack.y = stack.z;
    stack.z = stack.t;
    stack.t = temp;

    lift_enabled = true;
}

void rpn_roll_up(void)
{
    double temp = stack.t;
    stack.t = stack.z;
    stack.z = stack.y;
    stack.y = stack.x;
    stack.x = temp;

    lift_enabled = true;
}

void rpn_integer_part(void)
{
    double int_part;
    modf(stack.x, &int_part);
    stack.x = int_part;
}

void rpn_fractional_part(void)
{
    double int_part;
    double frac_part = modf(stack.x, &int_part);
    stack.x = frac_part;
}

void rpn_factorial(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    double val = stack.x;

    // HP Convention: Factorial is defined for non-negative integers.
    // Check if it's negative or has a fractional part.
    double int_part;
    if (val < 0.0 || modf(val, &int_part) != 0.0)
    {
        math_error = true;
        return;
    }

    int n = (int)val;

    // Guard against overflow for an 8-digit/scientific display layout (e.g., n > 69)
    if (n > 69)
    {
        math_error = true;
        return;
    }

    double result = 1.0;
    for (int i = 1; i <= n; i++)
    {
        result *= i;
    }

    stack.x = result;
    lift_enabled = true;
}

void rpn_change_sign(void)
{
    if (input.is_editing)
    {
        // If the buffer already starts with a minus, strip it away
        if (input.buffer[0] == '-')
        {
            memmove(input.buffer, input.buffer + 1, input.length);
            input.length--;
        }
        else
        {
            // Otherwise, shift everything right by 1 and insert a minus sign
            if (input.length < MAX_INPUT_LEN)
            {
                memmove(input.buffer + 1, input.buffer, input.length + 1);
                input.buffer[0] = '-';
                input.length++;
            }
        }
        // Update the numerical value in X to match the edited buffer
        stack.x = atof(input.buffer);
    }
    else
    {
        // Standard stack manipulation mode
        stack.x = -stack.x;
        // Manual stack manipulation handles its own lift state per HP standards
    }
}

void rpn_last_x(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    // Lift the stack if lift is enabled before dropping the old operand back in
    if (lift_enabled || !input.disable_lift)
    {
        stack.t = stack.z;
        stack.z = stack.y;
        stack.y = stack.x;
    }

    stack.x = last_x;
    lift_enabled = true; // Allows the user to immediately type a new number over it
}

void rpn_view_stack(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    uint8_t flash_y[8];
    uint8_t flash_x[8];
    char main_buf[32];

    // Format X to absolute full double precision length
    snprintf(main_buf, sizeof(main_buf), "%.15lf", (double)stack.x); // Testing full double precision

    // Initialize display arrays to completely blank
    for (int i = 0; i < 8; i++)
    {
        flash_y[i] = CODE_B_BLANK;
        flash_x[i] = CODE_B_BLANK;
    }

    int len = strlen(main_buf);
    int buf_idx = 0;

    // --- POPULATE TOP ROW (Y-Hardware Display) ---
    // Safely pull exactly 8 visible digits out of the front of the string
    int digits_top = 0;
    int display_idx = 7; // Left-to-right alignment

    while (buf_idx < len && display_idx >= 0)
    {
        char c = main_buf[buf_idx];
        if (c == '-')
        {
            flash_y[display_idx--] = CODE_B_DASH;
        }
        else if (c >= '0' && c <= '9')
        {
            flash_y[display_idx] = c - '0';
            digits_top++;

            // Check if the very next character is our decimal point
            if (buf_idx + 1 < len && main_buf[buf_idx + 1] == '.')
            {
                flash_y[display_idx] |= DP_BIT; // Bind it to this active digit
                buf_idx++;                      // Skip past the decimal character in the buffer
            }
            display_idx--;
        }
        buf_idx++;
        if (digits_top == 8)
            break;
    }

    // --- POPULATE BOTTOM ROW (X-Hardware Display) ---
    // Pull the next 8 remaining digits sequentially out of the buffer
    int digits_bottom = 0;
    display_idx = 7;

    while (buf_idx < len && display_idx >= 0)
    {
        char c = main_buf[buf_idx];
        if (c >= '0' && c <= '9')
        {
            flash_x[display_idx] = c - '0';
            digits_bottom++;

            // Just in case a decimal point lands exactly on the split boundary
            if (buf_idx + 1 < len && main_buf[buf_idx + 1] == '.')
            {
                flash_x[display_idx] |= DP_BIT;
                buf_idx++;
            }
            display_idx--;
        }
        buf_idx++;
        if (digits_bottom == 8)
            break;
    }

    // Direct hardware register writes
    for (int digit = 0; digit < 8; digit++)
    {
        write_register_dual(CMD_DIGIT0 + digit, flash_y[digit], flash_x[digit]);
    }

    sleep_ms(2500);
    lift_enabled = true; // Corrected to enable lift based on bench tests!
}

void rpn_reciprocal(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    if (stack.x != 0.0)
    {
        stack.x = 1.0 / stack.x;
        lift_enabled = true;
    }
    else
    {
        math_error = true;
    }
}

void rpn_power(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    double x = stack_pop();
    double y = stack_pop();

    double int_part;

    // Safety verification check for invalid roots/powers
    if (y < 0.0 && modf(x, &int_part) != 0.0)
    {
        math_error = true;
        stack_push(y); // Restore stack state on error
        stack_push(x);
        return;
    }

    if (y == 0.0 && x <= 0.0)
    {
        math_error = true;
        stack_push(y);
        stack_push(x);
        return;
    }

    stack_push(pow(y, x));
    lift_enabled = true;
}

void rpn_percentage(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    // HP Convention: % computes (Y * X) / 100.
    // It replaces X with the result, but leaves Y intact!
    double x = stack.x;
    double y = stack.y;

    stack.x = (y * x) / 100.0;
    lift_enabled = true;
}

void rpn_percent_difference(void)
{
    if (input.is_editing)
    {
        input.is_editing = false;
        input.length = 0;
        input.has_decimal = false;
    }

    double x = stack_pop();
    double y = stack_pop();

    if (y == 0.0)
    {
        math_error = true;
        stack_push(y);
        stack_push(x);
        return;
    }

    double result = ((x - y) / y) * 100.0;
    stack_push(result);
    lift_enabled = true;
}

void rpn_store_register(int reg_idx)
{
    if (reg_idx >= 0 && reg_idx < 25)
    {
        registers[reg_idx] = stack.x; // Value stays in X, copies to register
        lift_enabled = true;
    }
    else
    {
        math_error = true;
    }
}

void rpn_recall_register(int reg_idx)
{
    if (reg_idx >= 0 && reg_idx < 25)
    {
        if (input.is_editing)
        {
            input.is_editing = false;
            input.length = 0;
            input.has_decimal = false;
        }

        // Lift the stack before bringing the value into X
        if (lift_enabled || !input.disable_lift)
        {
            stack.t = stack.z;
            stack.z = stack.y;
            stack.y = stack.x;
        }

        stack.x = registers[reg_idx];
        lift_enabled = true; // Allows typing over the recalled value immediately
    }
    else
    {
        math_error = true;
    }
}

int capture_two_digits(void)
{
    uint8_t flash_y[8];
    uint8_t flash_x[8];
    for (int i = 0; i < 8; i++)
    {
        flash_y[i] = CODE_B_BLANK;
        flash_x[i] = CODE_B_BLANK;
    }

    // Dual dash prompt " --" waiting for keyboard numbers
    flash_x[1] = CODE_B_DASH;
    flash_x[0] = CODE_B_DASH;
    for (int d = 0; d < 8; d++)
    {
        write_register_dual(CMD_DIGIT0 + d, flash_y[d], flash_x[d]);
    }

    // Capture first digit
    KeyPress d1 = {'\0', SHIFT_NONE};
    while (d1.code < '0' || d1.code > '9')
    {
        d1 = scan_keypad();
        sleep_ms(10);
    }
    flash_x[1] = d1.code - '0';
    for (int d = 0; d < 8; d++)
    {
        write_register_dual(CMD_DIGIT0 + d, flash_y[d], flash_x[d]);
    }
    sleep_ms(150);

    // Capture second digit
    KeyPress d2 = {'\0', SHIFT_NONE};
    while (d2.code < '0' || d2.code > '9')
    {
        d2 = scan_keypad();
        sleep_ms(10);
    }
    flash_x[0] = d2.code - '0';
    for (int d = 0; d < 8; d++)
    {
        write_register_dual(CMD_DIGIT0 + d, flash_y[d], flash_x[d]);
    }
    sleep_ms(150);

    return (d1.code - '0') * 10 + (d2.code - '0');
}
#include "max7219.h"
#include "rpn_stack.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

const uint8_t CMD_NOOP = 0;
const uint8_t CMD_DIGIT0 = 1;
const uint8_t CMD_DECODEMODE = 9;
const uint8_t CMD_BRIGHTNESS = 10;
const uint8_t CMD_SCANLIMIT = 11;
const uint8_t CMD_SHUTDOWN = 12;
const uint8_t CMD_DISPLAYTEST = 15;
extern uint8_t current_intensity;

static inline void cs_select(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

void write_register_all(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    cs_select();
    for (int i = 0; i < NUM_MODULES; i++)
    {
        spi_write_blocking(spi_default, buf, 2);
    }
    cs_deselect();
}

void write_register_dual(uint8_t reg, uint8_t data_y, uint8_t data_x)
{
    uint8_t buf[4] = {reg, data_y, reg, data_x};
    cs_select();
    spi_write_blocking(spi_default, buf, 4);
    cs_deselect();
}

void clear_display(void)
{
    for (int i = 0; i < 8; i++)
    {
        write_register_all(CMD_DIGIT0 + i, CODE_B_BLANK);
    }
}

void max7219_init(void)
{
    spi_init(spi_default, 5 * 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    write_register_all(CMD_SHUTDOWN, 0);
    write_register_all(CMD_DISPLAYTEST, 0);
    write_register_all(CMD_SCANLIMIT, 7);
    write_register_all(CMD_DECODEMODE, 255);
    write_register_all(CMD_SHUTDOWN, 1);
    write_register_all(CMD_BRIGHTNESS, current_intensity);
    clear_display();
}

void format_double_to_code_b(double value, uint8_t *digits_out)
{
    char str_buf[32];
    char fmt_buf[16];

    for (int i = 0; i < 8; i++)
        digits_out[i] = CODE_B_BLANK;

    double abs_val = fabs(value);
    double fix_threshold = pow(10.0, -(double)fix_digits);

    if ((abs_val > 0.0 && abs_val < fix_threshold) || abs_val >= 100000000.0)
    {
        snprintf(fmt_buf, sizeof(fmt_buf), "%%.%de", fix_digits);
        snprintf(str_buf, sizeof(str_buf), fmt_buf, value);

        char *e_ptr = strchr(str_buf, 'e');
        if (e_ptr)
        {
            *e_ptr = '\0';
            int exponent = atoi(e_ptr + 1);
            int len = strlen(str_buf);
            int str_idx = 0;
            int display_idx = 7;

            while (str_idx < len && display_idx >= 3)
            {
                char c = str_buf[str_idx];
                if (c == '-')
                {
                    digits_out[display_idx--] = CODE_B_DASH;
                }
                else if (c >= '0' && c <= '9')
                {
                    digits_out[display_idx] = c - '0';
                    if (str_idx + 1 < len && str_buf[str_idx + 1] == '.')
                    {
                        digits_out[display_idx] |= DP_BIT;
                        str_idx++;
                    }
                    display_idx--;
                }
                str_idx++;
            }

            if (exponent < 0)
            {
                digits_out[2] = CODE_B_DASH;
                exponent = -exponent;
            }
            else
            {
                digits_out[2] = CODE_B_BLANK;
            }
            digits_out[1] = (exponent / 10) % 10;
            digits_out[0] = exponent % 10;
            return;
        }
    }

    snprintf(fmt_buf, sizeof(fmt_buf), "%%.%df", fix_digits);
    snprintf(str_buf, sizeof(str_buf), fmt_buf, value);

    int len = strlen(str_buf);
    int str_idx = 0;
    int display_idx = 7;

    while (str_idx < len && display_idx >= 0)
    {
        char c = str_buf[str_idx];
        if (c == '-')
        {
            digits_out[display_idx--] = CODE_B_DASH;
        }
        else if (c >= '0' && c <= '9')
        {
            digits_out[display_idx] = c - '0';
            if (str_idx + 1 < len && str_buf[str_idx + 1] == '.')
            {
                digits_out[display_idx] |= DP_BIT;
                str_idx++;
            }
            display_idx--;
        }
        else if (c == '.')
        {
            digits_out[display_idx] = 0 | DP_BIT;
            display_idx--;
        }
        str_idx++;
    }
}

void format_string_to_code_b(const char *str, uint8_t *digits_out)
{
    for (int i = 0; i < 8; i++)
        digits_out[i] = CODE_B_BLANK;
    int len = strlen(str);
    int str_idx = 0;
    int display_idx = 7;

    while (str_idx < len && display_idx >= 0)
    {
        char c = str[str_idx];
        if (c == '-')
        {
            digits_out[display_idx--] = CODE_B_DASH;
        }
        else if (c >= '0' && c <= '9')
        {
            digits_out[display_idx] = c - '0';
            if (str_idx + 1 < len && str[str_idx + 1] == '.')
            {
                digits_out[display_idx] |= DP_BIT;
                str_idx++;
            }
            display_idx--;
        }
        else if (c == '.')
        {
            digits_out[display_idx] = 0 | DP_BIT;
            display_idx--;
        }
        str_idx++;
    }
}

void refresh_hardware_display(void)
{
    uint8_t digits_x[8];
    uint8_t digits_y[8];

    format_double_to_code_b(stack.y, digits_y);

    if (math_error)
    {
        for (int i = 0; i < 8; i++)
            digits_x[i] = CODE_B_DASH;
    }
    else if (input.is_editing)
    {
        format_string_to_code_b(input.buffer, digits_x);
    }
    else
    {
        format_double_to_code_b(stack.x, digits_x);
    }

    for (int digit = 0; digit < 8; digit++)
    {
        write_register_dual(CMD_DIGIT0 + digit, digits_y[digit], digits_x[digit]);
    }
}

void max7219_set_brightness(uint8_t intensity)
{
    if (intensity > 15)
        intensity = 15; // Limit to MAX7219 maximum intensity
    current_intensity = intensity;
    ;

    // CMD_INTENSITY (0x0A) is the register address for brightness on the MAX7219
    // Assuming you have a write function named write_register_dual or similar:
    write_register_dual(0x0A, current_intensity, current_intensity);
}
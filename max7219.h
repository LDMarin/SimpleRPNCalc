#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

#define CODE_B_BLANK 0x0F
#define CODE_B_DASH 0x0A
#define CODE_B_E 0x0B
#define CODE_B_H 0x0C
#define CODE_B_L 0x0D
#define CODE_B_P 0x0E
#define DP_BIT 0x80
#define NUM_MODULES 2

// Register Definitions
extern const uint8_t CMD_NOOP;
extern const uint8_t CMD_DIGIT0;
extern const uint8_t CMD_DECODEMODE;
extern const uint8_t CMD_BRIGHTNESS;
extern const uint8_t CMD_SCANLIMIT;
extern const uint8_t CMD_SHUTDOWN;
extern const uint8_t CMD_DISPLAYTEST;

// Prototypes
void max7219_init(void);
void write_register_all(uint8_t reg, uint8_t data);
void write_register_dual(uint8_t reg, uint8_t data_y, uint8_t data_x);
void clear_display(void);
void refresh_hardware_display(void);
void format_double_to_code_b(double value, uint8_t *digits_out);
void format_string_to_code_b(const char *str, uint8_t *digits_out);
void max7219_set_brightness(uint8_t intensity);

#endif // MAX7219_H
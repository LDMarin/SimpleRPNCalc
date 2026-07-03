#ifndef RPN_NVRAM_H
#define RPN_NVRAM_H

#include <stdint.h>
#include <stdbool.h>

// Save at the 1MB mark of the flash, well away from your application code
#define FLASH_TARGET_OFFSET (1024 * 1024 * 1)
#define NVRAM_MAGIC 0x424415C2 // Unique ID to verify data integrity

extern bool is_sleeping;

typedef struct
{
    uint32_t magic;       // Verification token
    double x, y, z, t;    // RPN Stack
    double registers[25]; // Storage registers 00-24
    uint8_t brightness;   // MAX7219 brightness setting
    uint8_t fix_digits;   // Display precision setting
    uint8_t reserved[3];  // Padding for 32-bit alignment
} CalculatorState;

void nvram_init(void);
void nvram_check_timeout(void);
void nvram_reset_timer(void);
void nvram_force_sleep(void);

#endif
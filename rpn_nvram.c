#include "rpn_nvram.h"
#include "rpn_stack.h"
#include "max7219.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>

#define I2C_PORT i2c0
#define PIN_SDA 0
#define PIN_SCL 1
#define EEPROM_ADDR 0x50
#define STATE_SIZE sizeof(CalculatorState)
#define PAGE_SIZE 16
#define I2C_TIMEOUT_US 25000 // 25ms safety timeout line protective guard

static uint32_t last_activity_time = 0;
static bool save_pending = false;
extern uint8_t current_intensity;

bool is_sleeping = false;

void nvram_reset_timer(void)
{
    last_activity_time = to_ms_since_boot(get_absolute_time());
    save_pending = true;
}

// Internal helper to perform the actual EEPROM write sequence
static void execute_eeprom_write(void)
{
    CalculatorState current_state;
    current_state.magic = NVRAM_MAGIC;
    current_state.x = stack.x;
    current_state.y = stack.y;
    current_state.z = stack.z;
    current_state.t = stack.t;

    for (int i = 0; i < 25; i++)
    {
        current_state.registers[i] = registers[i];
    }
    current_state.brightness = current_intensity;
    current_state.fix_digits = fix_digits;

    uint8_t write_buf[STATE_SIZE];
    memcpy(write_buf, &current_state, STATE_SIZE);

    uint16_t bytes_written = 0;

    while (bytes_written < STATE_SIZE)
    {
        uint8_t chunk_size = PAGE_SIZE;
        if (bytes_written + chunk_size > STATE_SIZE)
        {
            chunk_size = STATE_SIZE - bytes_written;
        }

        uint8_t tx_frame[PAGE_SIZE + 1];
        tx_frame[0] = (uint8_t)(bytes_written & 0xFF);
        memcpy(&tx_frame[1], &write_buf[bytes_written], chunk_size);

        // Safe write sequence with a hard microsecond timeout threshold
        i2c_write_timeout_us(I2C_PORT, EEPROM_ADDR, tx_frame, chunk_size + 1, false, I2C_TIMEOUT_US);

        // Essential physical delay allowing the 24LC16B internal write machine to reset
        sleep_ms(5);

        bytes_written += chunk_size;
    }
}

void nvram_force_sleep(void)
{
    // Silicon Shutdown: blank display drivers before handling memory transmission
    write_register_all(CMD_SHUTDOWN, 0);
    sleep_ms(5);

    // Clear trackers first to protect state
    save_pending = false;
    is_sleeping = true;

    // Commit current state safely to EEPROM
    execute_eeprom_write();
}

void nvram_init(void)
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    sleep_ms(10);

    CalculatorState loaded_state;
    uint8_t start_addr = 0x00;

    // Protected read command sequence
    i2c_write_timeout_us(I2C_PORT, EEPROM_ADDR, &start_addr, 1, true, I2C_TIMEOUT_US);
    int bytes_read = i2c_read_timeout_us(I2C_PORT, EEPROM_ADDR, (uint8_t *)&loaded_state, STATE_SIZE, false, I2C_TIMEOUT_US);

    if (bytes_read == STATE_SIZE && loaded_state.magic == NVRAM_MAGIC)
    {
        stack.x = loaded_state.x;
        stack.y = loaded_state.y;
        stack.z = loaded_state.z;
        stack.t = loaded_state.t;

        for (int i = 0; i < 25; i++)
        {
            registers[i] = loaded_state.registers[i];
        }
        current_intensity = loaded_state.brightness;
        max7219_set_brightness(current_intensity);

        fix_digits = loaded_state.fix_digits;
    }

    last_activity_time = to_ms_since_boot(get_absolute_time());
    save_pending = false;
    is_sleeping = false;
}

void nvram_check_timeout(void)
{
    if (is_sleeping)
        return;

    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed_time = current_time - last_activity_time;

    // Background Auto-Save sequence
    if (save_pending && elapsed_time >= 5000)
    {
        save_pending = false; // Reset BEFORE calling write to break any execution overlap
        execute_eeprom_write();
    }

    // Automated deep sleep timeout sequence: Blank display after 60 seconds
    if (elapsed_time >= 60000)
    {
        nvram_force_sleep();
    }
}
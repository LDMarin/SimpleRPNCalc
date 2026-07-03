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
#define I2C_TIMEOUT_US 25000

static uint32_t last_activity_time = 0;
static bool save_pending = false;
extern uint8_t current_intensity;

bool is_sleeping = false;

// OPTIMIZATION: Keep a lightweight RAM cache of the last written physical EEPROM state
static CalculatorState last_saved_cache;

void nvram_reset_timer(void)
{
    last_activity_time = to_ms_since_boot(get_absolute_time());
    save_pending = true;
}

// Internal helper to pack variables into a target structure
static void pack_current_state(CalculatorState *dest)
{
    dest->magic = NVRAM_MAGIC;
    dest->x = stack.x;
    dest->y = stack.y;
    dest->z = stack.z;
    dest->t = stack.t;

    for (int i = 0; i < 25; i++)
    {
        dest->registers[i] = registers[i];
    }
    dest->brightness = current_intensity;
    dest->fix_digits = fix_digits;

    // Explicitly zero the reserved padding to keep memcmp predictable
    dest->reserved[0] = 0;
    dest->reserved[1] = 0;
    dest->reserved[2] = 0;
}

static void execute_eeprom_write(void)
{
    CalculatorState current_state;
    pack_current_state(&current_state);

    // OPTIMIZATION: If current state matches our last write exactly, skip the I2C bus write entirely!
    if (memcmp(&current_state, &last_saved_cache, STATE_SIZE) == 0)
    {
        return;
    }

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

        i2c_write_timeout_us(I2C_PORT, EEPROM_ADDR, tx_frame, chunk_size + 1, false, I2C_TIMEOUT_US);
        sleep_ms(5);

        bytes_written += chunk_size;
    }

    // OPTIMIZATION: Store the state into cache now that it's successfully on the wire
    memcpy(&last_saved_cache, &current_state, STATE_SIZE);
}

void nvram_force_sleep(void)
{
    write_register_all(CMD_SHUTDOWN, 0);
    sleep_ms(5);

    save_pending = false;
    is_sleeping = true;

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

        // OPTIMIZATION: Seed our cache on startup so we know exactly what is in the hardware chip
        memcpy(&last_saved_cache, &loaded_state, STATE_SIZE);
    }
    else
    {
        // If blank/corrupted EEPROM, zero the tracking cache out safely
        memset(&last_saved_cache, 0, STATE_SIZE);
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

    if (save_pending && elapsed_time >= 5000)
    {
        save_pending = false;
        execute_eeprom_write();
    }

    if (elapsed_time >= 60000)
    {
        nvram_force_sleep();
    }
}
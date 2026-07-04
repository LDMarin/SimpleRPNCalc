/**
 * @file keypad.h
 * @brief Driver matrix scanning logic and key translation mapping for the RPN Calculator.
 *
 * Handles the hardware tracking of a 4x4 matrix keyboard, operational status shifts ([f] and [g]),
 * and translation of raw hardware signals into clean application-level data structures.
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include "pico/stdlib.h"

/* --- Hardware Matrix Constants & Pin Mapping --- */
#define NUM_ROWS 4     ///< Total number of row output lines in the physical matrix
#define NUM_COLS 4     ///< Total number of column input lines in the physical matrix
#define PIN_SHIFT_F 10 ///< GPIO input pin mapped to the gold modifier functional switch [f]
#define PIN_SHIFT_G 11 ///< GPIO input pin mapped to the blue modifier functional switch [g]
#define PIN_LED_F 27   ///< GPIO output pin controlling the gold [f] shift status LED indicator
#define PIN_LED_G 28   ///< GPIO output pin controlling the blue [g] shift status LED indicator

/**
 * @enum KeyId
 * @brief Abstract identifiers that completely decouple mechanical switch coordinates
 *        from downstream application and core operational logic.
 */
typedef enum
{
    KEY_0,     ///< Numeric '0' / Roll down key
    KEY_1,     ///< Numeric '1' / 10^x function key
    KEY_2,     ///< Numeric '2' / e^x function key
    KEY_3,     ///< Numeric '3' / x^2 function key
    KEY_4,     ///< Numeric '4' / DEG-RAD switch key
    KEY_5,     ///< Numeric '5' / Integer part key
    KEY_6,     ///< Numeric '6' / Store register key
    KEY_7,     ///< Numeric '7' / Sine function key
    KEY_8,     ///< Numeric '8' / Cosine function key
    KEY_9,     ///< Numeric '9' / Tangent function key
    KEY_PLUS,  ///< Mathematical addition key
    KEY_MINUS, ///< Mathematical subtraction key
    KEY_MULT,  ///< Mathematical multiplication key
    KEY_DIV,   ///< Mathematical division key
    KEY_DOT,   ///< Decimal point identifier key
    KEY_ENTER, ///< Core stack operations entry key
    KEY_NONE   ///< Sentinel value representing no active keypad actuation detected
} KeyId;

/**
 * @enum ShiftState
 * @brief Identifies the active hardware function modification layer currently selected by the user.
 */
typedef enum
{
    SHIFT_NONE, ///< Default baseline keyboard layer
    SHIFT_F,    ///< Gold modifier [f] layer engaged
    SHIFT_G     ///< Blue modifier [g] layer engaged
} ShiftState;

/**
 * @struct KeyPress
 * @brief Composite frame encapsulating an atomic user transaction context.
 *
 * Driven by an abstract functional identifier, carrying safe display symbol strings
 * alongside immediate momentary or latched shift state rules.
 */
typedef struct
{
    KeyId id;         ///< Unique structural key identifier mapped from current matrix location
    char symbol;      ///< Safe ASCII character asset representing the button's baseline character
    ShiftState shift; ///< Capture status of the modifier rails at the exact moment of engagement
} KeyPress;

/* --- Cross-Module Interlinking Global Arrays --- */
extern const uint ROW_PINS[NUM_ROWS];              ///< Physical GPIO configuration mappings for row outputs
extern const uint COL_PINS[NUM_COLS];              ///< Physical GPIO configuration mappings for column inputs
extern const KeyId KEY_MAP_ID[NUM_ROWS][NUM_COLS]; ///< Static hardware matrix cross-reference table

/* --- Functional Drivers & Component Prototypes --- */

/**
 * @brief Initializes all underlying matrix GPIO channels, directions, and pull-up configuration registers.
 *
 * Sets Row lines as outputs initialized to high, configures columns and functional modifiers
 * as standard inputs backed by internal pull-up network routing, and turns off active shift indicators.
 */
void keypad_init(void);

/**
 * @brief Conducts a full hardware-level sequential row-by-column scan cycle of the matrix.
 *
 * Evaluates raw mechanical pin switches, filters operational shift flags on matching triggers,
 * manages transactional contact bounce delays, and returns parsed abstract key codes.
 *
 * @return KeyPress A data packet describing the key event. Returns KEY_NONE if no key is pressed.
 */
KeyPress scan_keypad(void);

/**
 * @brief Synchronizes physical functional LED hardware indicator status with the running calculation shift mode.
 *
 * Maps logical enum configurations directly into pin voltage drives (active-low) to update indicator bulbs.
 *
 * @param state The active target configuration shift layer to represent visually on the panel.
 */
void update_shift_led(ShiftState state);

#endif // KEYPAD_H
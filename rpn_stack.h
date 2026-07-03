#ifndef RPN_STACK_H
#define RPN_STACK_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_INPUT_LEN 8
#define M_PI 3.14159265358979323846
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

// Classic 4-level RPN Stack
typedef struct
{
    double x;
    double y;
    double z;
    double t;
} RPN_Stack;

// State tracking for the input buffer
typedef struct
{
    char buffer[MAX_INPUT_LEN + 1];
    int length;
    bool has_decimal;
    bool is_editing;
    bool disable_lift;
} InputBuffer;

// Shared Global Variables (Declared extern for cross-module linking)
extern RPN_Stack stack;
extern InputBuffer input;
extern uint8_t fix_digits;
extern uint8_t current_intensity;
extern bool lift_enabled;
extern bool math_error;
extern bool mode_degrees;
extern bool mode_polar;
extern double last_x;
extern double registers[25];

// Function Prototypes
void input_add_char(char c);
void rpn_enter(void);
void rpn_execute_operator(char op);
int capture_two_digits(void);
void rpn_store_register(int reg_idx);
void rpn_recall_register(int reg_idx);

// Stack Manipulation Utilities
void stack_push(double value);
double stack_pop(void);
void rpn_roll_down(void);
void rpn_roll_up(void);
void rpn_last_x(void);
void rpn_view_stack(void);

// Math Utilities
void rpn_integer_part(void);
void rpn_fractional_part(void);
void rpn_rect_to_polar(void);
void rpn_polar_to_rect(void);
void rpn_toggle_coordinates(void);

// Math Functions
void rpn_factorial(void);
void rpn_change_sign(void);
void rpn_reciprocal(void);
void rpn_power(void);
void rpn_percentage(void);
void rpn_percent_difference(void);

#endif // RPN_STACK_H
# SimpleRPNCalc: An Embedded RP-Notation Scientific Instrument Core

A high-performance hardware realization of a classic 4-level Reverse Polish Notation (RPN) scientific calculator architecture implemented on the Raspberry Pi Pico microcontroller using the native C/C++ SDK.

The system operates over a custom multi-layered $4\times4$ mechanical switch input matrix and controls a dual 8-digit MAX7219 common-cathode LED display arrangement via high-speed SPI serial communication lines. User data structures and persistent configurations are tracked over a dedicated external serial I2C EEPROM.

---

## Technical Specifications & Topology Overview

                    +----------------------------+
                    |   Raspberry Pi Pico Core   |
                    |      (RP2040 Silicon)      |
                    +----------------------------+
                     /            |             \
      High-Speed SPI/             |GPIO Matrix   \Dedicated Inputs & I2C Bus
                   v              v               v
        +-------------------+   +-------------+  +------------------+
        | Dual MAX7219      |   | 4x4 Keypad  |  | [f] & [g] Keys   |
        | Display Matrix    |   | Switch Array|  | 24LC16B EEPROM   |
        +-------------------+   +-------------+  +------------------+

- **Processor Core:** RP2040 Dual ARM Cortex-M0+ processing at baseline clock frequencies.
- **Operating Display Topology:** Cascaded 16-digit LED layout (Dual 8-Digit matrices representing the internal mathematical Workspace parameters $Y$ and $X$).
- **Memory Management:** Non-blocking external Non-Volatile Virtual Memory Tracking System mapping a 244-byte state structure directly into Block 0 of a Microchip 24LC16B Serial EEPROM over a fast-mode $400\text{ kHz}$ I2C rail.
- **Power Optimization Framework:** Built-in 60-second automatic firmware suspension system coupled with an instantaneous physical key hardware sequence interrupt bypass macro.

---

## Theory of Operation

### 1. Register Architecture and Stack Principles

The system replicates the structural execution behavior of legacy laboratory instruments (e.g., the HP-42S architecture). The fundamental stack is structured as a 4-level hierarchy composed of registers $X$, $Y$, $Z$, and $T$:
$$\text{Stack Structure: } [T] \longrightarrow [Z] \longrightarrow [Y] \longrightarrow [X]$$

- **Register $X$:** Represents the active primary input buffer workspace and directly maps to the lower physical display panel.
- **Register $Y$:** Holds the immediate secondary operant variable, mapped directly onto the upper physical display panel.
- **Registers $Z$ & $T$:** Provide deep automated nested storage capabilities for calculation sequences, minimizing the need for manual memory interaction during chain operations.

### 2. Intelligent Auto-Save & Bus-Hardened Execution Model

To ensure absolute user data survival while protecting the calculation thread from physical bus lockups, state tracking uses a layered dual-timer system:

1. **Inactivity Detection:** A hardware countdown tracking module counts inactive time starting from boot up or since the last valid keypress transaction.
2. **Background Auto-Save Fallback:** If modifications are made to stack or register states, a 5-second baseline stability window runs. Once this short threshold passes, the system slices the 244-byte memory structure into safe 16-byte chunks and streams them out via non-blocking, microsecond-bounded I2C transfers to the 24LC16B. This ensures calculations survive unexpected power failure without triggering memory writes on every single digit entry.
3. **Silicon Shutdown State:** At the 60-second absolute marking point or upon immediate manual force sequence, the software drops the MAX7219 out of active scanning using command register parameters (`CMD_SHUTDOWN = 0`), writes a complete localized backup trace to the EEPROM, and asserts a low-current sleep state flag (`is_sleeping = true`).
4. **Wakeup Ingestion Logic:** The first incoming keypad matrix transaction caught by scanning algorithms pulls the device out of low-current hibernation. It clears the sleep state, restores visual lighting parameters, refreshes the screens, and safely returns control to the main menu without dropping calculations or modifying memory states.

---

## Hardware Interface Pin Configuration

| Pico Hardware Pin | Pin Functional Allocation         | Schematic Component Destination    |
| :---------------- | :-------------------------------- | :--------------------------------- |
| **GPIO 0**        | I2C0 Serial Data Line (SDA)       | 24LC16B Pin 5 (SDA)                |
| **GPIO 1**        | I2C0 Serial Clock Line (SCL)      | 24LC16B Pin 6 (SCL)                |
| **GPIO 2 - 5**    | Matrix Row Output Lines           | Keypad Row 0 - Row 3               |
| **GPIO 6 - 9**    | Matrix Column Pull-Up Inputs      | Keypad Col 0 - Col 3               |
| **GPIO 10**       | Momentary Active Low Input        | Gold Functional Modifier `[f]`     |
| **GPIO 11**       | Momentary Active Low Input        | Blue Functional Modifier `[g]`     |
| **GPIO 16**       | Default SPI Data TX Line          | Dual MAX7219 Master Data In (DIN)  |
| **GPIO 17**       | Default SPI Chip Select (CS)      | Dual MAX7219 Chip Select (CS)      |
| **GPIO 18**       | Default SPI Clock Line (SCK)      | Dual MAX7219 Clock Interface (CLK) |
| **GPIO 27**       | Dedicated LED Status Drive Output | Gold Shift Mode Indicator          |
| **GPIO 28**       | Dedicated LED Status Drive Output | Blue Shift Mode Indicator          |

---

## Keypad Assignments

| Physical Key | Normal (Unshifted) |                  `[f]` Shifted                   |                    `[g]` Shifted                    |      `[f]` or `[g]` Hold (Combo)       |
| :----------: | :----------------: | :----------------------------------------------: | :-------------------------------------------------: | :------------------------------------: |
|   **`0`**    |        `0`         |           Roll Down (`rpn_roll_down`)            |               Roll Up (`rpn_roll_up`)               | `[f]` Soft Sleep (`nvram_force_sleep`) |
|   **`1`**    |        `1`         |               $10^x$ (`rpn_pow10`)               |             $\text{LOG}$ (`rpn_log10`)              |                                        |
|   **`2`**    |        `2`         |                $e^x$ (`rpn_exp`)                 |               $\text{LN}$ (`rpn_ln`)                |                                        |
|   **`3`**    |        `3`         |                  $x^2$ (`x*x`)                   |               $\sqrt{x}$ (`rpn_sqrt`)               |                                        |
|   **`4`**    |        `4`         | $\text{DEG} \leftrightarrow \text{RAD}$ (Toggle) | $\text{POLAR} \leftrightarrow \text{RECT}$ (Toggle) |                                        |
|   **`5`**    |        `5`         |               Integer Part (`INT`)               |              Fractional Part (`FRAC`)               |                                        |
|   **`6`**    |        `6`         |       $\text{STO}$ (`rpn_store_register`)        |        $\text{RCL}$ (`rpn_recall_register`)         |                                        |
|   **`7`**    |        `7`         |             $\text{SIN}$ (`rpn_sin`)             |             $\text{ASIN}$ (`rpn_asin`)              |                                        |
|   **`8`**    |        `8`         |             $\text{COS}$ (`rpn_cos`)             |             $\text{ACOS}$ (`rpn_acos`)              |                                        |
|   **`9`**    |        `9`         |             $\text{TAN}$ (`rpn_tan`)             |             $\text{ATAN}$ (`rpn_atan`)              |                                        |
|   **`+`**    |        $+$         |           $\text{LSTx}$ (`rpn_last_x`)           |                    $\text{VIEW}$                    |          `[f]` Brightness Up           |
|   **`-`**    |        $-$         |               Change Sign ($+/-$)                |            $\text{FIX}$ (Precision Menu)            |         `[f]` Brightness Down          |
|   **`*`**    |        $*$         |               $y^x$ (`rpn_power`)                |      $\%\text{CHG}$ (`rpn_percent_difference`)      |                                        |
|   **`/`**    |        $/$         |             $1/x$ (`rpn_reciprocal`)             |               $\%$ (`rpn_percentage`)               |                                        |
|   **`.`**    |        `.`         |                 $\pi$ ($M\_PI$)                  |               $x!$ (`rpn_factorial`)                |                                        |
|   **`E`**    |   $\text{ENTER}$   |       $x\leftrightarrow y$ (`rpn_swap_xy`)       |             $\text{CLx}$ (Clear Reg X)              |      `[g]` Hold: Clear Full Stack      |

## Built-In Shortcut Features

- **Instant Soft Power Off:** Press and hold down the physical `[f]` key while tapping `0`. This forces immediate serial writing to the 24LC16B, blanks the display panels, and sets the system into low-current hibernation.
- **Momentary Brightness Overrides:** Press and hold down the physical `[f]` key while repeatedly hitting `+` or `-` to adjust the LED display brightness dynamically (0 to 15) without losing your calculations.
- **Full Hierarchy Clear Stack:** Press and hold down the physical `[g]` modifier key while pressing `ENTER` (`E`). This drops all 4 levels of the operational structure ($X, Y, Z, T$) down to zero and resets all active mathematical overflow or error states.

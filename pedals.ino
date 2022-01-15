/*
    Copyright (C) 2021  Majidzadeh (hashpragmaonce@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define FLOAT_TYPE float

// ---- CONFIG ---- //

// Pins

const int PEDAL_INPUTS[] = { A0, A1, A2 };

const int LED_INDICATOR = LED_BUILTIN; // Built-in LED (13)

// Testing Options (Disable for actual use, 0: disabled, 1: enabled)

#define PRINT_FINAL_INPUT 0
#define PRINT_SENSOR_VALUES 0
#define PRINT_SENSOR_VALUES_START_AND_STOP 0

// Settings

const int PEDALS_COUNT = 1;

// Minimum accepted travel distance for calibration.
const int PEDAL_INPUTS_MINIMUM_DISTANCE = 50;
// Minimum duration to wait for calibration.
const int CALIBRATION_MINIMUM_DURATION_IN_SECONDS = 10;

// Enable PRINT_SENSOR_VALUES and read the sensor values in different pedal positions (through serial monitoring)
// to set the deadzones properly.
const int PEDALS_INNER_DEADZONE = 5;
const int PEDALS_OUTER_DEADZONE = 20;

// In case more specific deadzone settings are needed, change these for each individual pedal:
const int PEDAL_INNER_DEADZONES[] = { PEDALS_INNER_DEADZONE, PEDALS_INNER_DEADZONE, PEDALS_INNER_DEADZONE };
const int PEDAL_OUTER_DEADZONES[] = { PEDALS_OUTER_DEADZONE, PEDALS_OUTER_DEADZONE, PEDALS_OUTER_DEADZONE };

// Curve for each pedal. Examples:
// Linear: return x;
// Power of 2: return x * x;
// Power of 3: return x * x * x;
// Power of 4: x2 = x * x; return x2 * x2;
// Reversed Power of 2: x2 = 1 - x; return 1 - (x2 * x2);
// Reversed Power of 3: x2 = 1 - x; return 1 - (x2 * x2 * x2);
// Reversed Power of 4: x2 = 1 - x; x2 = x2 * x2; return 1 - (x2 * x2);
FLOAT_TYPE INTERPOLATE(const int& index, const FLOAT_TYPE& x)
{
    FLOAT_TYPE x2;
    switch (index)
    {
        case 0: return x; // pedal 0: x (linear)
        case 1: x2 = x * x; return x2 * x2; // pedal 1: x^4 (suitable for brake pedal with a progressive spring)
        case 2: return x; // pedal 2: x (linear)
        default: return x;
    }
}

// Interface

#include "Joystick.h"

//Joystick_ Joystick(); // Simple form
Joystick_ Joystick(
    JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
    0, 0, // buttonCount, hatSwitchCount
    true, true, true, // X, Y, Z (Used for pedals 0, 1, and 2)
    false, false, false, // Rx, Ry, Yz
    false,  // Rudder
    false, // Throttle
    false,  // Accelerator
    false,  // Brake
    false // Steering
);

const int16_t MAX_JOYSTICK_VALUE = 1000;

void INIT()
{
    Joystick.begin(false);
    Joystick.setXAxisRange(0, MAX_JOYSTICK_VALUE);
    Joystick.setYAxisRange(0, MAX_JOYSTICK_VALUE);
    Joystick.setZAxisRange(0, MAX_JOYSTICK_VALUE);
}

void SET(const int& index, const FLOAT_TYPE& x)
{
    switch (index)
    {
        case 0:
            Joystick.setXAxis((int16_t)(MAX_JOYSTICK_VALUE * x));
            break;
        case 1:
            Joystick.setYAxis((int16_t)(MAX_JOYSTICK_VALUE * x));
            break;
        case 2:
            Joystick.setZAxis((int16_t)(MAX_JOYSTICK_VALUE * x));
            break;
    }
#if PRINT_FINAL_INPUT
    Serial.print("Pedal No. ");
    Serial.print(index);
    Serial.print(": ");
    Serial.println(x);
#endif
}

void UPDATE()
{
    Joystick.sendState();
}

// ---- LOGIC ---- //

// 65535 - (seconds * 16000000 / prescaler)
// 1 sec with prescaler=1024: 65535 - (1 * 16000000 / 1024) = 49910
const int timer_preload = 49910;

int * initial_values;
int * actual_start_values;
int * actual_stop_values;
int * start_values;
int * stop_values;
int * current_values;
int calibrate_mode_timer = CALIBRATION_MINIMUM_DURATION_IN_SECONDS;

void setup()
{
    Serial.begin(9600);

    pinMode(LED_INDICATOR, OUTPUT);
    digitalWrite(LED_INDICATOR, LOW);

    initial_values = new int[PEDALS_COUNT];
    actual_start_values = new int[PEDALS_COUNT];
    actual_stop_values = new int[PEDALS_COUNT];
    start_values = new int[PEDALS_COUNT];
    stop_values = new int[PEDALS_COUNT];
    current_values = new int[PEDALS_COUNT];
    for (int i = 0; i < PEDALS_COUNT; i++)
    {
        initial_values[i] = analogRead(PEDAL_INPUTS[i]);
        actual_start_values[i] = initial_values[i];
        actual_stop_values[i] = initial_values[i];
        start_values[i] = initial_values[i];
        stop_values[i] = initial_values[i];
    }

    // timer setup
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = timer_preload;
    TCCR1B |= (1 << CS10)|(1 << CS12);
    TIMSK1 |= (1 << TOIE1);
    interrupts();

    INIT();
    for (int i = 0; i < PEDALS_COUNT; i++)
        SET(i, 0);
    UPDATE();
}

ISR(TIMER1_OVF_vect)
{
    TCNT1 = timer_preload;
    if (calibrate_mode_timer != 0)
    {
        calibrate_mode_timer--;
        digitalWrite(LED_INDICATOR, digitalRead(LED_INDICATOR) ^ 1);
    }
    else
    {
        digitalWrite(LED_INDICATOR, 0);
    }
}

int distance(const int& a, const int& b)
{
    int d = b - a;
    if (d < 0)
        return -d;
    return d;
}

FLOAT_TYPE normalize(const FLOAT_TYPE& x)
{
    if (x < 0)
        return 0;
    if (x > 1)
        return 1;
    return x;
}

void loop()
{
    for (int i = 0; i < PEDALS_COUNT; i++)
    {
        current_values[i] = analogRead(PEDAL_INPUTS[i]);

        if (calibrate_mode_timer != 0)
        {
            int d1 = distance(current_values[i], actual_start_values[i]);
            int d2 = distance(current_values[i], actual_stop_values[i]);
            int d = distance(actual_start_values[i], actual_stop_values[i]);
            if (d1 > d || d2 > d) // out of range => calibrate
            {
                if (d2 <= d1)
                    actual_stop_values[i] = current_values[i];
                else
                    actual_start_values[i] = current_values[i];
                calibrate_mode_timer = CALIBRATION_MINIMUM_DURATION_IN_SECONDS;
            }
            d1 = distance(initial_values[i], actual_start_values[i]);
            d2 = distance(initial_values[i], actual_stop_values[i]);
            if (d2 < d1) // stop closer to initial than start => swap
            {
                int temp = actual_start_values[i];
                actual_start_values[i] = actual_stop_values[i];
                actual_stop_values[i] = temp;
            }
            d = distance(actual_start_values[i], actual_stop_values[i]);
            if (d < PEDAL_INPUTS_MINIMUM_DISTANCE)
                calibrate_mode_timer = CALIBRATION_MINIMUM_DURATION_IN_SECONDS;
            if (d > PEDAL_INNER_DEADZONES[i] + PEDAL_OUTER_DEADZONES[i])
            {
                if (actual_stop_values[i] > actual_start_values[i])
                {
                    start_values[i] = actual_start_values[i] + PEDAL_INNER_DEADZONES[i];
                    stop_values[i] = actual_stop_values[i] - PEDAL_OUTER_DEADZONES[i];
                }
                else
                {
                    start_values[i] = actual_start_values[i] - PEDAL_INNER_DEADZONES[i];
                    stop_values[i] = actual_stop_values[i] + PEDAL_OUTER_DEADZONES[i];
                }
            }
            else
            {
                start_values[i] = actual_start_values[i];
                stop_values[i] = actual_stop_values[i];
            }
            if (d == 0)
                continue;
        }

#if PRINT_SENSOR_VALUES
        Serial.print("Pedal No. ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(current_values[i]);
  #if PRINT_SENSOR_VALUES_START_AND_STOP
        Serial.print("             Start:");
        Serial.println(start_values[i]);
        Serial.print("             Stop:");
        Serial.println(stop_values[i]);
  #endif
#endif
        FLOAT_TYPE x = (FLOAT_TYPE)(current_values[i] - start_values[i])
                       / (FLOAT_TYPE)(stop_values[i] - start_values[i]);
        x = normalize(x);
        x = INTERPOLATE(i, x);
        SET(i, x);
    }
    UPDATE();
}

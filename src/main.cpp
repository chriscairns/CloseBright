/*

This code controls the brightness of an LED attached to pin 9 of an arduino Uno
based on the readings of a Maxbotix MB1200 ultrasonic rangefinder attached to pin A0

*/

#include <Arduino.h>

const int DISTANCE_SENSOR_PIN = A0; // Pin for distance sensor
const int LED_PIN = 9;              // Pin for FET which drives LEDs
const int NUM_SAMPLES = 10;         // Number of samples to use for smoothing - how best to approach smoothing?

const uint16_t icr = 0xffff;              // 16-bit pwm resolution

// Array to store the sensor readings
int sensor_values[NUM_SAMPLES];

// Array to store 256 gamma corrected 16-bit brightness values
// Should this be stored in PROGMEM? (is that also referred to as ROM?) Why?
// Ultimately I'll make my own LUT once we have a good prototype.

const uint16_t gamma_correction_LUT [] = {
0, 0, 0, 0, 1, 1, 2, 3, 4, 6, 8, 10, 13, 16, 19, 24,
28, 33, 39, 46, 53, 60, 69, 78, 88, 98, 110, 122, 135, 149, 164, 179,
196, 214, 232, 252, 273, 295, 317, 341, 366, 393, 420, 449, 478, 510, 542, 575,
610, 647, 684, 723, 764, 806, 849, 894, 940, 988, 1037, 1088, 1140, 1194, 1250, 1307,
1366, 1427, 1489, 1553, 1619, 1686, 1756, 1827, 1900, 1975, 2051, 2130, 2210, 2293, 2377, 2463,
2552, 2642, 2734, 2829, 2925, 3024, 3124, 3227, 3332, 3439, 3548, 3660, 3774, 3890, 4008, 4128,
4251, 4376, 4504, 4634, 4766, 4901, 5038, 5177, 5319, 5464, 5611, 5760, 5912, 6067, 6224, 6384,
6546, 6711, 6879, 7049, 7222, 7397, 7576, 7757, 7941, 8128, 8317, 8509, 8704, 8902, 9103, 9307,
9514, 9723, 9936, 10151, 10370, 10591, 10816, 11043, 11274, 11507, 11744, 11984, 12227, 12473, 12722, 12975,
13230, 13489, 13751, 14017, 14285, 14557, 14833, 15111, 15393, 15678, 15967, 16259, 16554, 16853, 17155, 17461,
17770, 18083, 18399, 18719, 19042, 19369, 19700, 20034, 20372, 20713, 21058, 21407, 21759, 22115, 22475, 22838,
23206, 23577, 23952, 24330, 24713, 25099, 25489, 25884, 26282, 26683, 27089, 27499, 27913, 28330, 28752, 29178,
29608, 30041, 30479, 30921, 31367, 31818, 32272, 32730, 33193, 33660, 34131, 34606, 35085, 35569, 36057, 36549,
37046, 37547, 38052, 38561, 39075, 39593, 40116, 40643, 41175, 41711, 42251, 42796, 43346, 43899, 44458, 45021,
45588, 46161, 46737, 47319, 47905, 48495, 49091, 49691, 50295, 50905, 51519, 52138, 52761, 53390, 54023, 54661,
55303, 55951, 56604, 57261, 57923, 58590, 59262, 59939, 60621, 61308, 62000, 62697, 63399, 64106, 64818, 65535};

//** FUNCTION DECLARATIONS **\\

void setupPWM16()
{

  DDRB |= _BV(PB1) | _BV(PB2);                  // Set pins 9 and 10 as outputs
  TCCR1A = _BV(COM1A1) | _BV(COM1B1)            // Non-Inv PWM
           | _BV(WGM11);                        // Mode 14: Fast PWM, TOP=ICR1
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Prescaler 1
  ICR1 = icr;                                   // TOP counter value (Relieving OCR1A*)
}

// 16-bit version of analogWrite(). Only for pins 9 & 10
void analogWrite16(uint8_t pin, uint16_t val)
{
  switch (pin)
  {
  case 9:
    OCR1A = val;
    break;
  case 10:
    OCR1B = val;
    break;
  }
}

// sycophancy

template <typename T>
void print(T arg)
{
  Serial.print(arg);
  Serial.print(' ');
}

template <typename T, typename... Args>
void print(T head, Args... tail)
{
  print(head);
  print(tail...);
}

template <typename T, typename... Args>
void println(T head, Args... tail)
{
  print(head, tail...);
  Serial.println();
}

//***********************************************************************\\


void setup() {
Serial.begin(9600);
setupPWM16(); // setup pwm on pins 9 and 10

}

//***********************************************************************\\


void loop() {
Serial.println("*"); // loop begins

// Read distance sensor value
int distance = analogRead(DISTANCE_SENSOR_PIN);

// print the value to the serial monitor
// Serial.println(distance); 

// Add the new distance reading to the array
for (int i = NUM_SAMPLES - 1; i > 0; i--)
{
  sensor_values[i] = sensor_values[i - 1];
}
sensor_values[0] = distance;

// Calculate the average of the sensor values
int sum = 0;
for (int i = 0; i < NUM_SAMPLES; i++)
{
  sum += sensor_values[i];
}
int avg = sum / NUM_SAMPLES;

// constrain the rolling average to determine distance when lightbox is at full brightness
int constrained_avg = constrain(avg, 100, 240);

// Map the average distance to an 8-bit brightness value for the LED
int eight_bit_brightness = map(constrained_avg, 100, 240, 255, 0);

// Map the 8-bit brightness value to a 16-bit gamma corrected brightness value from LUT

uint16_t sixteen_bit_gamma_corrected_brightness = gamma_correction_LUT[eight_bit_brightness];

analogWrite16(9, sixteen_bit_gamma_corrected_brightness); // pwm the leds via the FET

// obsequiousness

println("avg = ", avg, "\t", "constr_avg =", constrained_avg, "\t", "8_bit_value = ", eight_bit_brightness, "\t", "16_bit_gc_value =", sixteen_bit_gamma_corrected_brightness);

// Delay for a short time
delay(1);
}

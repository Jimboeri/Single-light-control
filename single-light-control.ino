/*
 * This sketch will control a single LED
 * It will turn on when motion is detected. The Passive Infrared Indicator is a HC-SR501 with the output connected to Pin 8
 */

/*
 * Define libraries used here
 */
#include <EEPROM.h>

/*
 * Pre-processor definitions
 */

#define VERSION_STRING "Single Light Control V1.0"
#define PIR_INPUT 8
#define LED 9
#define LDR A0
#define POT A1

#define LED_OFF 0
#define LED_RAMP_UP 1
#define LED_ON 2
#define LED_RAMP_DOWN 3

#define PIR_MOTION 1
#define PIR_STILL 0

#define MIN_LDR 100   // This is the minimum value the LRD will register

/*
 * Define variables
 */
int motion = 0;           // holds the status of the PIR output
int led_status = LED_RAMP_UP;
int ramp_up = 2500;       // milliseconds needed to bring LED up to full brightness
int ramp_down = 5000;
int led_on_time = 30000;   // milliseconds the light will stay on before checking the PIR again
long var1 = 0;
int brightness;
long temp1;
int sensorValue = 0;        // value read from the light dependent resistor
int potValue = 0;           // value from the potentiometer
int potMapped = 0;          // this will store the adjusted potentiometer value so it can be compared to the LDR
int outputValue = 0;        // value output to the PWM (analog out)
int report_delay = 5000;    // millisec between reporting on sensor value

unsigned long start_delay = 10000L;  // The PIR takes about 1 minute to settle down
unsigned long next_check = 0; // This is the next time the PIR will be checked
unsigned long start = 0;
unsigned long current = 0;
unsigned long report_sensor = 0;
unsigned long t1 = 0L;
int t2 = 0;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(VERSION_STRING);
  pinMode(PIR_INPUT, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  next_check = millis() + start_delay + led_on_time;
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // read from EEPROM

  EEPROM.get(1, t1);      // start delay parameter
  if (t1 < 4000000000)
    start_delay = t1;
  else {
    EEPROM.put(1, start_delay);
    Serial.print("Initial setup of ");
  }
  Serial.print("Start delay is ");
  Serial.print(start_delay);
  Serial.println(" millseconds");

  EEPROM.get(5, t2);      // ramp up parameter
  if (t2 < 30000000 && t2 > 0)
    ramp_up = t2;
  else {
    EEPROM.put(5, ramp_up);
    Serial.print("Initial setup of ");
  }
  Serial.print("Ramp up time is ");
  Serial.print(ramp_up);
  Serial.println(" millseconds");

  EEPROM.get(7, t2);      // ramp down parameter
  if (t2 < 30000000 && t2 > 0)
    ramp_down = t2;
  else {
    EEPROM.put(7, ramp_down);
    Serial.print("Initial setup of ");
  }
  Serial.print("Ramp down time is ");
  Serial.print(ramp_down);
  Serial.println(" millseconds");

  EEPROM.get(9, t2);      // led_on_time parameter
  if (t2 < 30000000 && t2 > 0)
    led_on_time = t2;
  else {
    EEPROM.put(9, led_on_time);
    Serial.print("Initial setup of ");
  }
  Serial.print("Time light is to stay on is ");
  Serial.print(led_on_time);
  Serial.println(" milliseconds");

}

void loop() {
  // put your main code here, to run repeatedly:
  motion = digitalRead(PIR_INPUT);
  current = millis();
  sensorValue = analogRead(LDR);
  potValue = analogRead(POT);
  potMapped = map(potValue, 0, 1023, MIN_LDR, 1023);
  serialEvent();            // call the function
  if (stringComplete)       // keep all serial processing in one place
    process_serial();

  // output sensor value to serial channel
  if (report_sensor < current) {
    report_sensor = current + report_delay;
    Serial.print("Raw ");
    Serial.print(sensorValue);
    Serial.print (" Pot value ");
    Serial.print(potValue);
    Serial.print (" Pot adjusted ");
    Serial.print(potMapped);
    Serial.println();
  }

  switch (led_status) {
    case LED_RAMP_UP:
      if (current > (start + ramp_up)) {      // this is the end of the ramp up, ensure the LED is on and change status
        digitalWrite(LED, HIGH);
        led_status = LED_ON;
        Serial.println("Ramp up complete");
        break;
      }
      var1 = (current - start) * 255L;        // brightness increases during ramp up
      brightness = int(var1 / ramp_up);
      analogWrite(LED, brightness);
      delay(1);
      break;
    case LED_RAMP_DOWN:
      if (motion == PIR_MOTION) {
        led_status = LED_RAMP_UP;
        Serial.println("Motion detected - turning on");
      }
      if (current > (start + ramp_down)) {
        digitalWrite(LED, LOW);
        led_status = LED_OFF;
        Serial.println("Ramp down complete");
        break;
      }
      var1 = int(current - start);
      temp1 = var1 * 255L;
      brightness = 255 - (temp1 / ramp_down);
      analogWrite(LED, brightness);
      delay(1);
      break;
    case LED_ON:
      if (motion == PIR_MOTION)
      {
        next_check = current + led_on_time;    // if the PIR detects motion reset the next check time
        break;
      }
      else
      {
        if (next_check < current)              // if no motion has been detected by the next check, start the ramp down
        {
          led_status = LED_RAMP_DOWN;
          start = millis();
          Serial.println("NO Motion detected - turning off");
        }
      }     // end of LED_ON section

    case LED_OFF:
      if (motion == PIR_MOTION)                 // motion detected
      {
        Serial.println("Motion detected - check for light level");
        if (sensorValue < potMapped) {     // we only turn on if the ambient light is below this value
          next_check = millis() + led_on_time + ramp_up;
          led_status = LED_RAMP_UP;
          start = millis();
          Serial.println("Ambient light level low enough - turning on");
        }
      }   // end of LED_OFF section
  }       // end of case statement
}         // end of the main loop

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void process_serial() {
  Serial.println(inputString);
  // clear the string:
  inputString = "";
  stringComplete = false;

}

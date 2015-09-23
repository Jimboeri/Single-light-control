/*
 * This sketch will control a single LED
 * It will turn on when motion is detected. The Passive Infrared Indicator is a HC-SR501 with the output connected to Pin 8
 */

/*
 * Define libraries used here
 */

/*
 * Pre-processor definitions
 */

#define VERSION_STRING "Single Light Control V1.0"
#define PIR_INPUT 8
#define LED 9
#define LDR A0

#define LED_OFF 0
#define LED_RAMP_UP 1
#define LED_ON 2
#define LED_RAMP_DOWN 3

#define PIR_MOTION 1
#define PIR_STILL 0

/*
 * Define variables
 */
int motion = 0;           // holds the status of the PIR output
int led_status = LED_RAMP_UP;
int ramp_up = 1000;       // milliseconds needed to bring LED up to full brightness
int ramp_down = 5000;
int led_on_time = 5000;   // milliseconds the light will stay on before checking the PIR again
long var1 = 0;
int brightness;
long temp1;
int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output to the PWM (analog out)

unsigned long start_delay = 1000;  // The PIR takes about 1 minute to settle down
unsigned long next_check = 0; // This is the next time the PIR will be checked
unsigned long start = 0;
unsigned long current = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(VERSION_STRING);
  pinMode(PIR_INPUT, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  next_check = millis() + start_delay + led_on_time;
}

void loop() {
  // put your main code here, to run repeatedly:
  motion = digitalRead(PIR_INPUT);
  current = millis();
  sensorValue = analogRead(LDR);
  // map it to the range of the analog out:
  outputValue = map(sensorValue, 0, 1023, 0, 255);
  // change the analog out value:
  //Serial.println(outputValue);
  //delay(100);
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
        next_check = millis() + led_on_time;    // if the PIR detects motion reset the next check time
        break;
      }
      else
      {
        if (next_check < millis())              // if no motion has been detected by the next check, start the ramp down
        {
          led_status = LED_RAMP_DOWN;
          start = millis();
          Serial.println("NO Motion detected - turning off");
        }
      }     // end of LED_ON section

    case LED_OFF:
      if (motion == PIR_MOTION)                 // motion detected
      {
        next_check = millis() + led_on_time + ramp_up;
        led_status = LED_RAMP_UP;
        start = millis();
        Serial.println("Motion detected - turning on");
      }   // end of LED_OFF section
  }       // end of case statement
}         // end of the main loop

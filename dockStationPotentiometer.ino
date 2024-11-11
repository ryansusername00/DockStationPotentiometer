const int pwmPin = 2;           // PWM input pin from docking station (use a digital pin with interrupt capability
const int fanPwmPin = 5;        // Pin connected to the fan's PWM control pin
const int potPin = A0;          // Pin connected to the potentiometer
volatile unsigned long highTime;  // Time the signal is high
volatile unsigned long lowTime;   // Time the signal is low
volatile bool readingPWM = false; // To track whether we are reading PWM
const int minThreshold = 10;     // Minimum threshold for valid duty cycle percentage
const int debounceCount = 5;     // Number of stable readings to debounce the signal

int stablePWMInput = 0;         // Debounced PWM input from docking station
int stableCounter = 0;          // Counter for stable readings

void setup() {
  // Set the PWM frequency on D5 to ~31.4 kHz
  TCCR0B = TCCR0B & 0b11111000 | 0x01;  // Set Timer0 prescaler to 1, increasing frequency on pins 5 and 6 to avoid motor noise
  
  Serial.begin(9600);  // Start serial communication
  pinMode(pwmPin, INPUT);         // Set the PWM input pin as an input
  pinMode(fanPwmPin, OUTPUT);     // Set the PWM fan pin as output
  pinMode(potPin, INPUT);         // Set the potentiometer pin as input
  delay(1000);                    // 1 second delay to allow everything to power up properly
  attachInterrupt(digitalPinToInterrupt(pwmPin), pwmISR, CHANGE); // Attach an interrupt on PWM pin
}

void loop() {
  // Read the potentiometer value (0 to 1023)
  int potValue = analogRead(potPin);

  // Reverse the potentiometer value mapping to match its physical behavior (Reversed pot?)
  int minFanSpeed = map(potValue, 1023, 0, 75, 255);

  int pwmValue = minFanSpeed;     // Set a default value based on potentiometer
  int pwmFromDocking = 0;         // Variable for docking station PWM

  if (readingPWM) {
    // Calculate period (total time of one cycle)
    unsigned long period = highTime + lowTime;

    // Calculate duty cycle as a percentage
    float dutyCycle = (100.0 * highTime) / period;

    // Ignore very small duty cycles to avoid fluttering
    if (dutyCycle > minThreshold) {
      pwmFromDocking = map(dutyCycle, 0, 100, 0, 255);
    }

    // Debounce the PWM input by checking if it's stable for debounceCount cycles
    if (pwmFromDocking == stablePWMInput) {
      stableCounter++;  // Increment stable counter if input is the same
    } else {
      stableCounter = 0;  // Reset if input changes
      stablePWMInput = pwmFromDocking;  // Update to the new stable input
    }

    // Only update the fan speed if the PWM input is stable for debounceCount cycles
    if (stableCounter >= debounceCount) {
      pwmValue = max(stablePWMInput, minFanSpeed);
    }

    // Clear reading flag
    readingPWM = false;
  }

  // Write the final PWM signal to the fan
  analogWrite(fanPwmPin, pwmValue);

  // Commenting out the serial output
  /*
  Serial.print("PWM Input from Docking: ");
  Serial.print(stablePWMInput);
  Serial.print(", PWM Output to Fan: ");
  Serial.println(pwmValue);
  */

  delay(500); // Slow down the loop to avoid spamming the serial monitor
}

void pwmISR() {
  static unsigned long lastChangeTime = 0;
  unsigned long currentTime = micros(); 
  unsigned long elapsedTime = currentTime - lastChangeTime;

  if (digitalRead(pwmPin) == HIGH) {
    lowTime = elapsedTime; 
  } else {
    highTime = elapsedTime; 
  }

  lastChangeTime = currentTime;

  // Set the flag that we are ready to process the PWM data
  if (highTime > 0 && lowTime > 0) {
    readingPWM = true;
  }
}

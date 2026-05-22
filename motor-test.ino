// ================================================================
// ELEGOO SMART CAR V4.0 / V1.1 SHIELD TEST CODE
// ================================================================

// 1. PIN DEFINITIONS (From Manual Page 6)
const int PIN_PWMA = 5;   // Left Motor Speed
const int PIN_AIN1 = 7;   // Left Motor Direction
const int PIN_PWMB = 6;   // Right Motor Speed
const int PIN_BIN1 = 8;   // Right Motor Direction
const int PIN_STBY = 3;   // THE "WAKE UP" PIN

void setup() {
  // 2. CONFIGURE PINS
  pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // 3. WAKE UP THE DRIVER (CRITICAL STEP!)
  // If this is LOW, motors are disconnected.
  digitalWrite(PIN_STBY, HIGH); 
  
  // Optional: Safety delay
  delay(1000);
}

void loop() {
  // --- TEST: MOVE FORWARD ---
  
  // Left Motor Forward
  digitalWrite(PIN_AIN1, HIGH);  // Direction: High = Forward (usually)
  analogWrite(PIN_PWMA, 150);    // Speed (0-255)

  // Right Motor Forward
  digitalWrite(PIN_BIN1, HIGH);  // Direction: High = Forward
  analogWrite(PIN_PWMB, 150);    // Speed (0-255)

  delay(1000); // Drive for 1 second

  // --- TEST: STOP ---
  analogWrite(PIN_PWMA, 0);
  analogWrite(PIN_PWMB, 0);
  
  delay(1000); // Wait 1 second
  
  // --- TEST: REVERSE ---
  // To reverse on this specific shield, you just FLIP the Direction Pin
  digitalWrite(PIN_AIN1, LOW); 
  analogWrite(PIN_PWMA, 150);
  
  digitalWrite(PIN_BIN1, LOW); 
  analogWrite(PIN_PWMB, 150);
  
  delay(1000);
  
  // Stop again
  analogWrite(PIN_PWMA, 0);
  analogWrite(PIN_PWMB, 0);
  delay(1000);
}

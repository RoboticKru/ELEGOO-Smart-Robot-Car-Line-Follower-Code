#include <Wire.h>

// --- 1. HARDWARE PIN DEFINITIONS ---
const int PIN_PWMA = 5;   
const int PIN_AIN1 = 7;   
const int PIN_PWMB = 6;   
const int PIN_BIN1 = 8;   
const int PIN_STBY = 3;   

// Sensors
const int pinL = A2;
const int pinC = A1;
const int pinR = A0;

const int PIN_LED = 13; 
const int MPU_ADDR = 0x68;

// --- 2. TUNING PARAMETERS ---
// Speed
const int MAX_SPEED = 180;     
const int BASE_SPEED = 60;     // Safe, consistent speed
const int TURN_SPEED = 90;     // RESTORED: Fast sweep speed

// PID (Restored Aggressive Tuning for Curves)
const float Kp = 75.0;         // Snappy steering
const float Kd = 50.0;         // Strong dampening

// Rescue Settings
const int SWEEP_ANGLE = 135;       
const int GAP_MOMENTUM_TIME = 150; 

// --- 3. GLOBAL VARIABLES ---
int minVal[3] = {1023, 1023, 1023}; 
int maxVal[3] = {0, 0, 0};          
int threshold[3] = {500, 500, 500}; 

int16_t AcZ;         
int16_t GyZ;
float yawRate = 0;   
float yawAngle = 0;  
float gyroOffset = 0;
unsigned long prevTime = 0;

int lastError = 0;
unsigned long lossStartTime = 0;
bool isLifted = false;
unsigned long landTime = 0;

void setup() {
  pinMode(PIN_PWMA, OUTPUT); pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT); pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);
  pinMode(pinL, INPUT); pinMode(pinC, INPUT); pinMode(pinR, INPUT);
  pinMode(PIN_LED, OUTPUT);
  
  digitalWrite(PIN_STBY, HIGH);
  Serial.begin(9600);
  Wire.begin();
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0); 
  Wire.endTransmission(true);

  // Default Gyro Range
  
  flashLED(2, 500);      
  calibrateGyro();       
  autoCalibrateSensors();
  flashLED(4, 200);      
}

void loop() {
  updateMPU();
  
  // --- SAFETY CHECKS ---
  if (checkLift()) {
    stopMotors();
    isLifted = true;
    lossStartTime = 0; 
    return;
  }
  if (isLifted) {
    if (landTime == 0) landTime = millis();
    if (millis() - landTime < 1000) { stopMotors(); return; }
    isLifted = false; landTime = 0;
    yawAngle = 0;
  }
  
  // --- SENSORS ---
  bool L = (analogRead(pinL) > threshold[0]);
  bool C = (analogRead(pinC) > threshold[1]);
  bool R = (analogRead(pinR) > threshold[2]);
  
  bool lineFound = (L || C || R);
  
  // --- MAIN DRIVING LOGIC (V1 PID) ---
  if (lineFound) {
    lossStartTime = 0;
    yawAngle = 0; 
    
    // ERROR CALCULATION (Restored V1 Logic)
    // Left Sensor = Positive Error
    // Right Sensor = Negative Error
    int error = 0;
    if (!L && C && !R) error = 0;       
    else if (!L && C && R) error = -2;  
    else if (!L && !C && R) error = -3; 
    else if (L && C && !R) error = 2;   
    else if (L && !C && !R) error = 3;  
    
    else if (L && C && R) {
       if (lastError > 0) error = 3;
       else if (lastError < 0) error = -3;
       else error = 0;
    }

    // Stability Lock (Anti-Pivot)
    if (error == 0 && abs(yawRate) > 200) {
       setMotors(-40, -40); 
       delay(20);
       lastError = 0;
       return;
    }

    // PID Correction
    float P = error;
    float D = error - lastError;
    float correction = (Kp * P) + (Kd * D);
    lastError = error;
    
    // Motor Mixing
    int mLeft = BASE_SPEED + correction;
    int mRight = BASE_SPEED - correction;
    setMotors(mLeft, mRight);
  } 
  else {
    // --- LINE LOST ---
    if (lossStartTime == 0) lossStartTime = millis();
    if (millis() - lossStartTime < GAP_MOMENTUM_TIME) return;
    
    stopMotors();
    performHunterSeeker();
  }
}

// ================================================================
//   RESCUE: ORIGINAL V1 SWEEP
// ================================================================
void performHunterSeeker() {
  updateMPU(); 
  yawAngle = 0; 
  
  // Determine Direction (Left is Positive in V1 Logic)
  int turnDir = (lastError >= 0) ? 1 : -1; 
  
  // SWEEP 1: Look in Likely Direction
  if (turnToAngle(SWEEP_ANGLE * turnDir)) return;
  stopMotors(); delay(50);
  
  // SWEEP 2: Look Opposite Direction
  if (turnToAngle(SWEEP_ANGLE * -turnDir)) return;
  stopMotors(); delay(50);

  // SWEEP 3: Re-center
  if (turnToAngle(0)) return;

  // FAILSAFE: Move Blindly
  unsigned long blindTimer = millis();
  while(millis() - blindTimer < 300) {
    if (checkLift()) return;
    if (checkForLineQuick()) return;
    setMotors(BASE_SPEED, BASE_SPEED);
  }
  stopMotors();
}

// Helper: Turns robot until angle reached OR line found
bool turnToAngle(int target) {
  unsigned long timeout = millis();
  
  while (true) {
    updateMPU();
    
    if (checkLift()) { stopMotors(); isLifted = true; return true; }
    
    // "Touch & Go" Check
    if (checkForLineQuick()) {
      stopMotors(); delay(50); return true;
    }

    if (target > yawAngle) {
      if (yawAngle >= target) break; 
      setMotors(-TURN_SPEED, TURN_SPEED); // Left
    } else {
      if (yawAngle <= target) break; 
      setMotors(TURN_SPEED, -TURN_SPEED); // Right
    }
    
    if (millis() - timeout > 2000) break;
  }
  return false;
}

bool checkForLineQuick() {
  if (analogRead(pinC) > threshold[1]) return true;
  if (analogRead(pinL) > threshold[0]) return true;
  if (analogRead(pinR) > threshold[2]) return true;
  return false;
}

// ================================================================
//   HELPERS
// ================================================================

void calibrateGyro() {
  long sum = 0;
  for(int i=0; i<500; i++) {
    updateMPU();
    sum += GyZ;
    delay(1);
  }
  gyroOffset = sum / 500.0;
}

bool checkLift() {
  static unsigned long liftTimer = 0;
  // Threshold 2000: Robust against vibration
  if (AcZ < 2000) { 
    if (liftTimer == 0) liftTimer = millis();
    // MUST be tilted for 200ms continuous to trigger stop
    if (millis() - liftTimer > 200) return true; 
  } else {
    liftTimer = 0; 
  }
  return false;
}

void autoCalibrateSensors() {
  unsigned long start = millis();
  while(millis() - start < 300) { setMotors(-70, 70); recordMinMax(); }
  start = millis();
  while(millis() - start < 600) { setMotors(70, -70); recordMinMax(); }
  start = millis();
  while(millis() - start < 300) { setMotors(-70, 70); recordMinMax(); }
  stopMotors();
  
  for(int i=0; i<3; i++) {
    threshold[i] = (minVal[i] + maxVal[i]) / 2;
    if (maxVal[i] - minVal[i] < 100) threshold[i] = 500; 
  }
}

void recordMinMax() {
  int vals[3] = {analogRead(pinL), analogRead(pinC), analogRead(pinR)};
  for(int i=0; i<3; i++) {
    if (vals[i] > maxVal[i]) maxVal[i] = vals[i];
    if (vals[i] < minVal[i]) minVal[i] = vals[i];
  }
}

void updateMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true); 
  
  int16_t AcX = Wire.read()<<8|Wire.read();
  int16_t AcY = Wire.read()<<8|Wire.read();
  AcZ = Wire.read()<<8|Wire.read(); 
  int16_t temp = Wire.read()<<8|Wire.read();
  int16_t GyX = Wire.read()<<8|Wire.read();
  int16_t GyY = Wire.read()<<8|Wire.read();
  GyZ = Wire.read()<<8|Wire.read(); 
  
  unsigned long currTime = millis();
  float dt = (currTime - prevTime) / 1000.0;
  prevTime = currTime;
  
  if (dt < 1.0) {
    yawRate = (GyZ - gyroOffset) / 131.0; 
    yawAngle += yawRate * dt;
  }
}

void setMotors(int left, int right) {
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);
  if (left > MAX_SPEED) left = MAX_SPEED;
  if (right > MAX_SPEED) right = MAX_SPEED;
  if (left < -MAX_SPEED) left = -MAX_SPEED;
  if (right < -MAX_SPEED) right = -MAX_SPEED;

  if (left >= 0) {
    digitalWrite(PIN_AIN1, HIGH); analogWrite(PIN_PWMA, left);
  } else {
    digitalWrite(PIN_AIN1, LOW); analogWrite(PIN_PWMA, abs(left));
  }
  
  if (right >= 0) {
    digitalWrite(PIN_BIN1, HIGH); analogWrite(PIN_PWMB, right);
  } else {
    digitalWrite(PIN_BIN1, LOW); analogWrite(PIN_PWMB, abs(right));
  }
}

void stopMotors() {
  setMotors(0, 0);
}

void flashLED(int times, int delayTime) {
  for(int i=0; i<times; i++) {
    digitalWrite(PIN_LED, HIGH); delay(delayTime);
    digitalWrite(PIN_LED, LOW); delay(delayTime);
  }
}

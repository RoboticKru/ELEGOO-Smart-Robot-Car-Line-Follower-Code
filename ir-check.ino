// Line Tracking Sensor Test
// Left: A2, Center: A1, Right: A0

const int pinLeft = A2;
const int pinCenter = A1;
const int pinRight = A0;

void setup() {
  // Start communication with the computer
  Serial.begin(9600);
  
  // Set the pins to Input mode
  pinMode(pinLeft, INPUT);
  pinMode(pinCenter, INPUT);
  pinMode(pinRight, INPUT);
}

void loop() {
  // Read the sensors
  int leftValue = analogRead(pinLeft);
  int centerValue = analogRead(pinCenter);
  int rightValue = analogRead(pinRight);

  // Print the values to the Serial Monitor
  Serial.print("Left: ");
  Serial.print(leftValue);
  Serial.print(" | Center: ");
  Serial.print(centerValue);
  Serial.print(" | Right: ");
  Serial.println(rightValue);

  // Small pause so the text doesn't fly by too fast
  delay(500);
}

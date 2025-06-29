#include <Arduino.h>

// Motor pins (adjust these to your wiring)
const int motorPin1 = 3;   // Left motor forward (PWM)
const int motorPin2 = 6;   // Left motor backward (PWM)
const int motorPin3 = 5;   // Right motor forward (PWM)
const int motorPin4 = 4;   // Right motor backward (PWM)
const int enablePin1 = 9;  // Left motor speed control
const int enablePin2 = 10; // Right motor speed control

// Emergency stop button pin
const int stopPin = 2;

// Movement parameters
int motorSpeed = 200;  // Default speed (0-255)100
int distance = 0;
int adjustedDistance = 0;
int angle = 0;
int cars = 0;

// Timing and state control
unsigned long previousMillis = 0;
const long interval = 100;  // Control interval in ms
volatile bool stopFlag = false;

// Movement states
enum State { IDLE, MOVING_FORWARD, MOVING_BACKWARD, TRACE, TURNING_LEFT, TURNING_RIGHT };
State currentState = IDLE;

// Exponential growth constant
const float exp_grow = 0.001;
float growthFactor = 0.0;
// Interrupt handler for emergency stop
void stopInterrupt() {
  stopFlag = true;
}

// Exponential growth function for distance adjustment
float expGrowth(unsigned long time) {
  return exp(exp_grow * time);
}

void setup() {
  Serial.begin(9600);
  
  // Setup motor control pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(enablePin1, OUTPUT);
  pinMode(enablePin2, OUTPUT);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  // Set initial motor speeds
  analogWrite(enablePin1, motorSpeed);
  analogWrite(enablePin2, motorSpeed-10);
  
  // Setup emergency stop
  //pinMode(stopPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(stopPin), stopInterrupt, FALLING);
  
  Serial.println("Robot ready. Waiting for commands (F50/B30/L90/R45/S)");
}

void handleLeftTurn() {
  // Turn left (left motors backward, right motors forward)
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);

  angle--;
  
  if (angle <= 0) {
    stop();
    currentState = IDLE;
    Serial.println("Left turn completed");
  }
}

void handleRightTurn() {
  // Turn right (left motors forward, right motors backward)
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);

  angle--;
  
  if (angle <= 0) {
    stop();
    currentState = IDLE;
    Serial.println("Right turn completed");
  }
}

void stop() {
  // Stop all motors
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  
  currentState = IDLE;
}

void loop() {
  // Check for emergency stop
  if (stopFlag) {
    stop();
    stopFlag = false;
  }

  // Handle movement based on current state
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    switch (currentState) {
      case MOVING_FORWARD:
        handleForwardMovement();
        break;
      case MOVING_BACKWARD:
        handleBackwardMovement();
        break;
      case TURNING_LEFT:
        handleLeftTurn();
        break;
      case TURNING_RIGHT:
        handleRightTurn();
        break;
      case TRACE:
        handleTrace();
        break;  
      case IDLE:
        // No movement
        break;
    }
  }

  // Process incoming serial commands
  if (Serial.available() > 0) {
    String incomingCommand = Serial.readStringUntil('\n');
    incomingCommand.trim();
    processCommand(incomingCommand);
  }
}

void processCommand(String command) {
  if (command.length() == 0) return;

  char cmdType = command.charAt(0);
  int value = command.substring(1).toInt();

  switch (cmdType) {
    case 'F': // Forward
      distance = value;
      growthFactor = expGrowth(millis());
      adjustedDistance = int((distance) +(1 - growthFactor));
      adjustedDistance = min(adjustedDistance, 150);
      currentState = MOVING_FORWARD;
      Serial.print("Moving forward ");
      Serial.print(distance);
      Serial.println(" units");
      break;
      
    case 'B': // Backward
      distance = value;
      growthFactor = expGrowth(millis());
      adjustedDistance =  int((distance/6) +(1 - growthFactor));
      adjustedDistance = min(adjustedDistance, 150);
      currentState = MOVING_BACKWARD;
      Serial.print("Moving backward ");
      Serial.print(distance);
      Serial.println(" units");
      break;

      case 'T': // Backward
      cars = value;
      currentState = TRACE;
      Serial.print("Tracing ");
      Serial.print(cars);
      Serial.println(" Car slots");
      break;
      
    case 'L': // Left turn
      angle = value;
      currentState = TURNING_LEFT;
      Serial.print("Turning left ");
      Serial.print(angle);
      Serial.println(" degrees");
      break;
      
    case 'R': // Right turn
      angle = value;
      currentState = TURNING_RIGHT;
      Serial.print("Turning right ");
      Serial.print(angle);
      Serial.println(" degrees");
      break;
      
    case 'S': // Stop
      stop();
      Serial.println("Stopping immediately");
      break;
      
    default:
      Serial.println("Unknown command: " + command);
      break;
  }
}

void handleForwardMovement() {

  // Move forward
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);

  adjustedDistance--;
  
  if (adjustedDistance <= 0) {
    stop();
    currentState = IDLE;
    Serial.println("Forward movement completed");
  }
}

void handleBackwardMovement() {

  // Move backward
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);

  adjustedDistance-- ;
  
  if (adjustedDistance <= 0) {
    stop();
    currentState = IDLE;
    Serial.println("Backward movement completed");
  }
}

void handleTrace() {
 for (int i = 0; i< cars; i++)
{
  // Move forward
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
  delay(20);
  // Turn 90 left
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(4);
  // Move forward
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
  delay(10);
  // Turn 90 left
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(4);
  // Move forward
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
  delay(20);
  // Turn 90 left
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(4);
  // Move forward
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
  delay(20);

  Serial.println("Tracing completed"); 
}}


#include <Arduino.h>

// Motor pins (adjust these to your wiring)
const int motorPin1 = 3;   // Left motor forward (PWM)
const int motorPin2 = 6;   // Left motor backward (PWM)
const int motorPin3 = 5;   // Right motor forward (PWM)
const int motorPin4 = 4;   // Right motor backward (PWM)
const int enablePin1 = 9;  // Left motor speed control
const int enablePin2 = 10; // Right motor speed control

// Movement parameters
int motorSpeed = 144;      // Default speed (0-255)
int distance = 0;
int angle = 0;
int cars = 0;

// Timing and state control
unsigned long previousMillis = 0;
unsigned long movementStartTime = 0;
unsigned long movementDuration = 0;
const long interval = 100;  // Control interval in ms

// Movement states
enum State { IDLE, MOVING_FORWARD, MOVING_BACKWARD, TRACE, TURNING_LEFT, TURNING_RIGHT };
State currentState = IDLE;

void setup() {
  Serial.begin(9600);
  
  // Setup motor control pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(enablePin1, OUTPUT);
  pinMode(enablePin2, OUTPUT);
  
  // Initialize all motors to stop
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  
  // Set initial motor speeds
  analogWrite(enablePin1, motorSpeed);
  analogWrite(enablePin2, motorSpeed-10);
  
  Serial.println("Robot ready. Waiting for commands (F50/B30/L90/R45/T3/S)");
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

void handleForwardMovement() {
  // Move forward
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);

  // Check if movement duration has elapsed
  if (millis() - movementStartTime >= movementDuration) {
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

  // Check if movement duration has elapsed
  if (millis() - movementStartTime >= movementDuration) {
    stop();
    currentState = IDLE;
    Serial.println("Backward movement completed");
  }
}

void handleTrace() {
  static int completedSlots = 0;
  static unsigned long lastStepTime = 0;
  static int traceState = 0; // 0=forward, 1=turn, 2=short forward, 3=turn
  
  if (completedSlots >= cars) {
    stop();
    currentState = IDLE;
    completedSlots = 0;
    Serial.println("Tracing completed");
    return;
  }

  unsigned long currentTime = millis();
  
  switch(traceState) {
    case 0: // Initial forward
      if (currentTime - lastStepTime < 2000) {
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        digitalWrite(motorPin3, LOW);
        digitalWrite(motorPin4, HIGH);
      } else {
        traceState = 1;
        lastStepTime = currentTime;
      }
      break;
      
    case 1: // Turn left
      if (currentTime - lastStepTime < 400) {
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        digitalWrite(motorPin3, HIGH);
        digitalWrite(motorPin4, LOW);
      } else {
        traceState = 2;
        lastStepTime = currentTime;
      }
      break;
      
    case 2: // Short forward
      if (currentTime - lastStepTime < 1000) {
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        digitalWrite(motorPin3, LOW);
        digitalWrite(motorPin4, HIGH);
      } else {
        traceState = 3;
        lastStepTime = currentTime;
      }
      break;
      
    case 3: // Turn left again
      if (currentTime - lastStepTime < 400) {
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        digitalWrite(motorPin3, HIGH);
        digitalWrite(motorPin4, LOW);
      } else {
        traceState = 0;
        lastStepTime = currentTime;
        completedSlots++;
        Serial.print("Completed slot ");
        Serial.println(completedSlots);
      }
      break;
  }
}

void loop() {
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
      movementDuration = distance * 100; // 100ms per unit
      movementStartTime = millis();
      currentState = MOVING_FORWARD;
      Serial.print("Moving forward ");
      Serial.print(distance);
      Serial.println(" units");
      break;
      
    case 'B': // Backward
      distance = value;
      movementDuration = distance * 100; // 100ms per unit
      movementStartTime = millis();
      currentState = MOVING_BACKWARD;
      Serial.print("Moving backward ");
      Serial.print(distance);
      Serial.println(" units");
      break;

    case 'T': // Trace parking slots
      cars = value;
      currentState = TRACE;
      Serial.print("Tracing ");
      Serial.print(cars);
      Serial.println(" car slots");
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

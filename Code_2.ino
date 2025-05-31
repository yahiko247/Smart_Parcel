#include <Keypad.h>
#include <Servo.h>

// --- Ultrasonic Sensor Configuration ---
#define trigPin A0 // Ultrasonic sensor trigger pin (Analog pin used as Digital)
#define echoPin A1 // Ultrasonic sensor echo pin (Analog pin used as Digital)

const int THRESHOLD_DISTANCE_CM = 50; // Distance in cm to consider an object "present"
const unsigned long ULTRASONIC_TIMEOUT_US = 20000; // Timeout for pulseIn (microseconds)

long duration;
int distance;
bool objectWasPresent = false; // State to track if an object was previously detected

// Timing for ultrasonic sensor readings (2 minutes delay)
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL_MS = 500; // change this to serial reading interval

// --- Keypad Configuration ---
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; // Connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// --- Servo Configuration ---
Servo servoLock;   // Servo object for the lock mechanism
Servo servoMoney;  // Servo object for the money dispenser

const int LOCK_SERVO_PIN = 10;  // Digital pin for the lock servo
const int MONEY_SERVO_PIN = 11; // Digital pin for the money dispenser servo

// Define clear angles for servo movements
const int LOCK_CLOSED_ANGLE = 0;   // Angle for lock closed
const int LOCK_OPEN_ANGLE = 180;   // Angle for lock open
const int MONEY_CLOSED_ANGLE = 0;  // Angle for money dispenser closed/rest
const int MONEY_OPEN_ANGLE = 90;   // Angle for money dispenser open/dispensing

// --- Password Configuration ---
const String correctPassword = "1234"; // Password for the lock
String enteredPassword = "";          // Stores user's keypad input

// --- Function Prototypes ---
int readUltrasonicDistance();
void handleMoneyDispenseLogic(int currentDistance);
void processKeypadInput(char key);

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging

  // Set up ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Attach servos to their respective pins
  servoLock.attach(LOCK_SERVO_PIN);
  servoMoney.attach(MONEY_SERVO_PIN);

  // Set initial positions for both servos (e.g., closed/at rest)
  servoLock.write(LOCK_CLOSED_ANGLE);
  servoMoney.write(MONEY_CLOSED_ANGLE);

  Serial.println("System Ready.");
  Serial.println("---------------------------");
  Serial.println("Enter password to unlock:");
  Serial.println("Press '*' to clear entered password.");
  Serial.println("Press '#' to confirm password.");
  Serial.println("Money will dispense when an object is removed from sensor range.");
  Serial.println("---------------------------");

  // Initialize lastSensorReadTime to current millis() so the first reading happens immediately
  lastSensorReadTime = millis();
}

void loop() {
  // Check if it's time to read the ultrasonic sensor
  if (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL_MS) {
    // Update the last read time
    lastSensorReadTime = millis();

    // Read distance from ultrasonic sensor
    distance = readUltrasonicDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Handle logic for money dispensing based on sensor reading
    handleMoneyDispenseLogic(distance);
  }

  // Process any keypad input (this will still be responsive)
  char key = customKeypad.getKey();
  if (key) {
    processKeypadInput(key);
  }

  // No general delay here, allowing other tasks (like keypad) to run continuously
}

int readUltrasonicDistance() {
  // Clear the trigPin by setting it LOW for 2 microseconds
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH for 10 microseconds to send a sound pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  // Includes a timeout to prevent infinite wait if no echo is received
  duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT_US);

  if (duration == 0) {
    // If pulseIn timed out, it means no echo was received within the timeout period
    return -1; // Indicate an invalid reading
  }

  // Calculate distance in centimeters (speed of sound ~0.034 cm/microsecond)
  return duration * 0.034 / 2;
}

void handleMoneyDispenseLogic(int currentDistance) {
  // Ignore readings that indicate a timeout or extremely high distance
  if (currentDistance == -1 || currentDistance > 400) { // Max practical range for many HC-SR04 is ~400cm
    // Optionally, handle this error or just ignore it for now
    return;
  }

  bool currentObjectPresent = (currentDistance < THRESHOLD_DISTANCE_CM);

  // Check if an object was present and has now been removed
  if (objectWasPresent && !currentObjectPresent) {
    Serial.println("--- OBJECT REMOVED! Dispensing money... ---");
    servoMoney.write(MONEY_OPEN_ANGLE); // Move servo to dispense position
    delay(1500);                        // Wait for dispensing action
    servoMoney.write(MONEY_CLOSED_ANGLE); // Return servo to original position
    Serial.println("Money dispensed. Servo returned to home.");
  }
  // Optional: Add a message if an object just became present
  else if (!objectWasPresent && currentObjectPresent) {
    Serial.println("--- Object detected. ---");
  }

  // Update the 'objectWasPresent' state for the next loop iteration
  objectWasPresent = currentObjectPresent;
}

void processKeypadInput(char key) {
  Serial.print("Key pressed: ");
  Serial.println(key);

  if (key == '#') { // Confirm password
    if (enteredPassword == correctPassword) {
      Serial.println("Password correct! Opening lock...");
      servoLock.write(LOCK_OPEN_ANGLE); // Rotate lock servo to open position
      delay(3000);                      // Keep it open for 3 seconds
      servoLock.write(LOCK_CLOSED_ANGLE); // Return lock servo to closed position
      Serial.println("Lock closed.");
    } else {
      Serial.println("Password invalid!");
    }
    enteredPassword = ""; // Clear password input after attempt
    Serial.println("Enter new password (if needed):");
  } else if (key == '*') { // Clear entered password
    enteredPassword = "";
    Serial.println("Entered password cleared.");
    Serial.println("Enter password:");
  }
  // The 'D' key for money dispense has been removed as it's now sensor-controlled.
  else {
    // Build the entered password string for numeric/letter keys
    enteredPassword += key;
  }
}

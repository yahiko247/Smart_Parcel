#include <Keypad.h>
#include <Servo.h>

// --- Ultrasonic Sensor Configuration ---
#define trigPin A0
#define echoPin A1

const int THRESHOLD_DISTANCE_CM = 50;
const unsigned long ULTRASONIC_TIMEOUT_US = 20000;

long duration;
int distance;
bool objectWasPresent = false;

unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL_MS = 500;

// --- Keypad Configuration ---
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// --- Servo Configuration ---
Servo servoLock;
Servo servoMoney;

const int LOCK_SERVO_PIN = 10;
const int MONEY_SERVO_PIN = 11;

const int LOCK_CLOSED_ANGLE = 0;
const int LOCK_OPEN_ANGLE = 180;
const int MONEY_CLOSED_ANGLE = 0;
const int MONEY_OPEN_ANGLE = 90;

// --- Password Configuration ---
bool moneyPossiblyNotTaken = false;
const String correctPassword = "1234";
String enteredPassword = "";

// --- Lock Control Timer ---
bool lockIsOpen = false;
unsigned long lockOpenedTime = 0;

// --- Function Prototypes ---
int readUltrasonicDistance();
void handleMoneyDispenseLogic(int currentDistance);
void processKeypadInput(char key);

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  servoLock.attach(LOCK_SERVO_PIN);
  servoMoney.attach(MONEY_SERVO_PIN);

  servoLock.write(LOCK_CLOSED_ANGLE);
  servoMoney.write(MONEY_CLOSED_ANGLE);

  Serial.println("System Ready.");
  Serial.println("---------------------------");
  Serial.println("Enter password to unlock:");
  Serial.println("Press '*' to clear entered password.");
  Serial.println("Press '#' to confirm password.");
  Serial.println("Press 'D' to manually close the compartment.");
  Serial.println("Money will dispense when an object is removed from sensor range.");
  Serial.println("---------------------------");

  lastSensorReadTime = millis();
}

void loop() {
  // Ultrasonic sensor reading
  if (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadTime = millis();
    distance = readUltrasonicDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    handleMoneyDispenseLogic(distance);
  }

  // Keypad input
  char key = customKeypad.getKey();
  if (key) {
    processKeypadInput(key);
  }

  // Auto-close lock after 1 minute
  if (lockIsOpen && millis() - lockOpenedTime >= 60000) {
    Serial.println("Auto-closing lock after 1 minute...");
    servoLock.write(LOCK_CLOSED_ANGLE);
    lockIsOpen = false;
  }
}

int readUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

void handleMoneyDispenseLogic(int currentDistance) {
  if (currentDistance == -1 || currentDistance > 400) return;

  bool currentObjectPresent = (currentDistance < THRESHOLD_DISTANCE_CM);

  if (currentObjectPresent && !objectWasPresent) {
    Serial.println("--- Object detected! Dispensing money... ---");
    servoMoney.write(MONEY_OPEN_ANGLE);
    delay(1500);
    servoMoney.write(MONEY_CLOSED_ANGLE);
    Serial.println("Money dispensed. Servo returned to home.");

    int recheckDistance = readUltrasonicDistance();
    if (recheckDistance != -1 && recheckDistance < THRESHOLD_DISTANCE_CM) {
      moneyPossiblyNotTaken = true;
      Serial.println("Money possibly not taken. Enter secondary password (4321) to re-dispense.");
    }
  } else if (!currentObjectPresent && objectWasPresent) {
    Serial.println("--- Object removed. ---");
    moneyPossiblyNotTaken = false;
  }

  objectWasPresent = currentObjectPresent;
}

void processKeypadInput(char key) {
  Serial.print("Key pressed: ");
  Serial.println(key);

  if (key == '#') {
    if (enteredPassword == correctPassword) {
      Serial.println("Password correct! Opening lock...");
      servoLock.write(LOCK_OPEN_ANGLE);
      lockIsOpen = true;
      lockOpenedTime = millis();
    } 
    else if (enteredPassword == "4321" && moneyPossiblyNotTaken && objectWasPresent) {
      Serial.println("Secondary password correct! Re-dispensing money...");
      servoMoney.write(MONEY_OPEN_ANGLE);
      delay(1500);
      servoMoney.write(MONEY_CLOSED_ANGLE);
      Serial.println("Money re-dispensed. Servo returned to home.");
      moneyPossiblyNotTaken = false;
    } 
    else {
      Serial.println("Password invalid!");
    }

    enteredPassword = "";
    Serial.println("Enter new password (if needed):");
  } 
  else if (key == '*') {
    enteredPassword = "";
    Serial.println("Entered password cleared.");
    Serial.println("Enter password:");
  } 
  else if (key == 'D') {
    if (lockIsOpen) {
      Serial.println("Manual close triggered.");
      servoLock.write(LOCK_CLOSED_ANGLE);
      lockIsOpen = false;
    } else {
      Serial.println("Lock is already closed.");
    }
  } 
  else {
    enteredPassword += key;
  }
}

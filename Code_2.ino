#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust LCD address if needed

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; // Connect keypad ROW0, ROW1, ROW2, ROW3 to these Arduino pins
byte colPins[COLS] = {5, 4, 3, 2}; // Connect keypad COL0, COL1, COL2, COL3 to these Arduino pins

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Servo setup
Servo parcelServo;    // Controls parcel compartment lock
Servo paymentServo;   // Controls payment compartment lock

const int parcelServoPin = 10;
const int paymentServoPin = 11;

// Sensors pins - using digital input for simulation purposes in Tinkercad
const int parcelSensorPin = 12;   // Sensor detecting parcel placed
const int cashSensorPin = 13;     // Sensor detecting cash taken

// Password setup
const String correctPassword = "1234";
String passwordInput = "";
bool parcelCompOpen = false;
bool paymentCompOpen = false;

// Timing variables
unsigned long sensorDetectedTime = 0;
bool parcelDetected = false;
bool cashDetected = false;
bool parcelClosing = false;
bool paymentClosing = false;

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  
  parcelServo.attach(parcelServoPin);
  paymentServo.attach(paymentServoPin);
  
  pinMode(parcelSensorPin, INPUT);
  pinMode(cashSensorPin, INPUT);
  
  // Initially both compartments locked
  lockParcelCompartment();
  lockPaymentCompartment();
  
  lcd.setCursor(0, 0);
  lcd.print("Smart Parcel Box");
  lcd.setCursor(0, 1);
  lcd.print("Enter Password:");
}

void loop() {
  char key = keypad.getKey();

  if (!parcelCompOpen) {
    // Wait for password input if parcel compartment is closed
    if (key) {
      if (key == '#') {
        // Check password when # pressed
        if (passwordInput == correctPassword) {
          lcd.clear();
          lcd.print("Password Correct");
          delay(1000);
          openParcelCompartment();
          lcd.clear();
          lcd.print("Place Parcel...");
          passwordInput = "";
        } else {
          lcd.clear();
          lcd.print("Wrong Password!");
          delay(1000);
          lcd.clear();
          lcd.print("Enter Password:");
          passwordInput = "";
        }
      } else if (key == '*') {
        // Clear input
        passwordInput = "";
        lcd.clear();
        lcd.print("Enter Password:");
      } else {
        // Add key to input if less than 4 digits
        if (passwordInput.length() < 4) {
          passwordInput += key;
          lcd.setCursor(passwordInput.length()-1, 1);
          lcd.print("*"); // Mask input
        }
      }
    }
  }
  
  // Parcel compartment is open - monitor parcel placement sensor
  if (parcelCompOpen && !parcelDetected) {
    int parcelSensorVal = digitalRead(parcelSensorPin);
    if (parcelSensorVal == HIGH) { // Detected parcel placement
      parcelDetected = true;
      sensorDetectedTime = millis();
      lcd.clear();
      lcd.print("Parcel Detected");
    }
  }
  
  if (parcelDetected && parcelCompOpen && !parcelClosing) {
    // After 5 seconds delay close parcel compartment and open payment compartment
    if (millis() - sensorDetectedTime >= 5000) {
      closeParcelCompartment();
      openPaymentCompartment();
      parcelClosing = true;
      parcelDetected = false;
      lcd.clear();
      lcd.print("Payment Open");
      lcd.setCursor(0,1);
      lcd.print("Collect Cash");
    }
  }
  
  // Payment compartment is open - monitor cash taken sensor
  if (paymentCompOpen && !cashDetected) {
    int cashSensorVal = digitalRead(cashSensorPin);
    if (cashSensorVal == HIGH) { // Cash taken detected
      cashDetected = true;
      sensorDetectedTime = millis();
      lcd.clear();
      lcd.print("Cash Taken");
    }
  }
  
  if (cashDetected && paymentCompOpen && !paymentClosing) {
    // After 5 seconds delay close payment compartment and reset
    if (millis() - sensorDetectedTime >= 5000) {
      closePaymentCompartment();
      paymentClosing = true;
      cashDetected = false;
      lcd.clear();
      lcd.print("Locker Ready");
      delay(2000);
      lcd.clear();
      lcd.print("Enter Password:");
      parcelClosing = false;
      paymentClosing = false;
      parcelCompOpen = false;
      paymentCompOpen = false;
    }
  }
}

// Servo control functions
void openParcelCompartment() {
  parcelServo.write(90); // Adjust position for open - e.g. 90 degrees
  parcelCompOpen = true;
}

void closeParcelCompartment() {
  parcelServo.write(0);  // Locked position - e.g. 0 degrees
  parcelCompOpen = false;
}

void openPaymentCompartment() {
  paymentServo.write(90);// Adjust position for open - e.g. 90 degrees
  paymentCompOpen = true;
}

void closePaymentCompartment() {
  paymentServo.write(0); // Locked position - e.g. 0 degrees
  paymentCompOpen = false;
}


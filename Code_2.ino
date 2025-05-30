#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>

// LCD setup
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; // Connect keypad ROW0-3
byte colPins[COLS] = {A3, A2, A1, A0}; // Use analog pins for COL0-3 to avoid conflict with LCD

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo setup
Servo parcelServo;
Servo paymentServo;

const int parcelServoPin = 10;
const int paymentServoPin = A5;  // Use A5 to avoid LCD & keypad conflict

// Sensors
const int parcelSensorPin = A4;
const int cashSensorPin = 13;

// Password setup
const String correctPassword = "1234";
String passwordInput = "";
bool parcelCompOpen = false;
bool paymentCompOpen = false;

unsigned long sensorDetectedTime = 0;
bool parcelDetected = false;
bool cashDetected = false;
bool parcelClosing = false;
bool paymentClosing = false;

void setup() {
  Serial.begin(9600);
  
  parcelServo.attach(parcelServoPin);
  paymentServo.attach(paymentServoPin);
  
  pinMode(parcelSensorPin, INPUT);
  pinMode(cashSensorPin, INPUT);
  
 closeParcelCompartment();
closePaymentCompartment();

  
  lcd.begin(16, 2);  // initialize the LCD
  lcd.setCursor(0, 0);
  lcd.print("Smart Parcel Box");
  lcd.setCursor(0, 1);
  lcd.print("Enter Password:");
}

void loop() {
  char key = keypad.getKey();

  if (!parcelCompOpen) {
    if (key) {
      if (key == '#') {
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
        passwordInput = "";
        lcd.clear();
        lcd.print("Enter Password:");
      } else {
        if (passwordInput.length() < 4) {
          passwordInput += key;
          lcd.setCursor(passwordInput.length()-1, 1);
          lcd.print("*");
        }
      }
    }
  }

  if (parcelCompOpen && !parcelDetected) {
    if (digitalRead(parcelSensorPin) == HIGH) {
      parcelDetected = true;
      sensorDetectedTime = millis();
      lcd.clear();
      lcd.print("Parcel Detected");
    }
  }

  if (parcelDetected && parcelCompOpen && !parcelClosing) {
    if (millis() - sensorDetectedTime >= 5000) {
      closeParcelCompartment();
      openPaymentCompartment();
      parcelClosing = true;
      parcelDetected = false;
      lcd.clear();
      lcd.print("Payment Open");
      lcd.setCursor(0, 1);
      lcd.print("Collect Cash");
    }
  }

  if (paymentCompOpen && !cashDetected) {
    if (digitalRead(cashSensorPin) == HIGH) {
      cashDetected = true;
      sensorDetectedTime = millis();
      lcd.clear();
      lcd.print("Cash Taken");
    }
  }

  if (cashDetected && paymentCompOpen && !paymentClosing) {
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
  parcelServo.write(90);
  parcelCompOpen = true;
}

void closeParcelCompartment() {
  parcelServo.write(0);
  parcelCompOpen = false;
}

void openPaymentCompartment() {
  paymentServo.write(90);
  paymentCompOpen = true;
}

void closePaymentCompartment() {
  paymentServo.write(0);
  paymentCompOpen = false;
}

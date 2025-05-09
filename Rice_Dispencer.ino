#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <HX711.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Load Cell
#define DT 3
#define SCK 2
HX711 scale;
float calibration_factor = 100; // <- Adjust after calibration

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {8, 9, 10, 11};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo, LEDs, Buzzer
Servo dispenserServo;
#define SERVO_PIN 12
#define GREEN_LED 13
#define RED_LED A0
#define BUZZER_PIN A1

// Variables
String inputWeight = "";
float targetWeight = 0.0;
bool dispensing = false;
bool transactionComplete = false;

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Rice Dispenser");
  delay(2000);
  lcd.clear();
  lcd.print("Enter kilos:");

  dispenserServo.attach(SERVO_PIN);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (transactionComplete && key == '*') {
      // Reset system
      lcd.clear();
      lcd.print("Enter kilos:");
      inputWeight = "";
      transactionComplete = false;
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      return;
    }

    if (!dispensing && !transactionComplete) {
      if (key >= '0' && key <= '9') {
        inputWeight += key;
        lcd.setCursor(0, 1);
        lcd.print(inputWeight + " kg     ");
      } else if (key == '#') {
        dispenserServo.write(90);
        targetWeight = inputWeight.toFloat(); // Convert input to float
        dispenserServo.write(0);
        inputWeight = "";
        

        lcd.clear();
        lcd.print("Target: ");
        lcd.print(targetWeight, 3);
        lcd.print(" kg");
        delay(1000);

        startDispensing(targetWeight); // Start motor and dispensing
      } else if (key == '*') {
        // Clear input if no transaction yet
        inputWeight = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
    }
  }
}

void startDispensing(float kilos) {
  dispensing = true;
  float targetGrams = kilos * calibration_factor;

  lcd.clear();
  lcd.print("Dispensing...");
  

  while (dispensing) {
    float current = scale.get_units(); // Read raw data
    float currentKg = current / calibration_factor;

    lcd.setCursor(0, 1);
    lcd.print("Now: ");
    lcd.print(currentKg, 3);
    lcd.print(" kg");

    if (current >= targetGrams - 5) { // Stop within ~5g margin
      dispenserServo.write(0); // Stop motor
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);

      lcd.clear();
      lcd.print("Done!");
      tone(BUZZER_PIN, 1000, 500); // 500ms beep

      delay(2000);
      lcd.clear();
      lcd.print("Press * to reset");

      scale.tare(); // Reset scale
      dispensing = false;
      transactionComplete = true;
    } else {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    }

    delay(200);
  }
}

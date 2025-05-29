#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <Keypad.h>
#include <Servo.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Load Cell
#define DT 3
#define SCK 2
HX711 scale;
float calibration_factor = 102655.00;

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
#define GREEN_LED A0
#define RED_LED 13
#define BUZZER_PIN A1

// Variables
String inputAmount = "";
float pricePerKg = 55.0;
float targetWeight = 0.0;
bool dispensing = false;
bool transactionComplete = false;
float currentWeight = 0.0;
unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Rice Dispenser");
  delay(2000);
  lcd.clear();
  lcd.print("Enter Pesos:");

  dispenserServo.attach(SERVO_PIN);
  dispenserServo.write(90);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  char key = keypad.getKey();

  if (key == '*') {
    resetSystem();
    return;
  }

  if (dispensing) {
    monitorWeight();
    return;
  }

  if (transactionComplete) {
    if (key && key >= '0' && key <= '9') {
      inputAmount = "";
      targetWeight = 0.0;
      transactionComplete = false;
      lcd.clear();
      lcd.print("Enter Pesos:");
      lcd.setCursor(0, 1);
      inputAmount += key;
      lcd.print(inputAmount + " PHP    ");
    }
    return;
  }

  if (key) {
    if (!dispensing && !transactionComplete) {
      if (key >= '0' && key <= '9') {
        if (inputAmount.length() < 4) {
          inputAmount += key;
          lcd.setCursor(0, 1);
          lcd.print(inputAmount + " PHP    ");
        }
      } else if (key == '#') {
        if (inputAmount.length() > 0) {
          float amount = inputAmount.toFloat();
          if (amount > 0) {
            targetWeight = amount / pricePerKg;
            lcd.clear();
            lcd.print("Dispensing...");
            lcd.setCursor(0, 1);
            lcd.print(targetWeight, 2);
            lcd.print(" kg rice");
            dispensing = true;
            scale.tare();
            dispenserServo.write(0);
            digitalWrite(GREEN_LED, HIGH);
            digitalWrite(RED_LED, LOW);
            delay(1500);
            lcd.clear();
            lcd.print("Dispensing...");
          }
        }
      }
    }
  }
}

void monitorWeight() {
  currentWeight = scale.get_units();
  if (millis() - lastUpdateTime > 250) {
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 2);
    lcd.print("/");
    lcd.print(targetWeight, 2);
    lcd.print(" kg   ");
    lastUpdateTime = millis();
  }

  if (currentWeight >= targetWeight) {
    dispenserServo.write(90);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER_PIN, 1000, 1000);

    lcd.clear();
    lcd.print("Complete!");
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 2);
    lcd.print(" kg dispensed");

    dispensing = false;
    transactionComplete = true;

    // Send formatted data
    Serial.print("KG:");
    Serial.print(currentWeight, 2);
    Serial.print(",PESO:");
    Serial.println(inputAmount);

    delay(2000);
    lcd.clear();
    lcd.print("Transaction Done");
    lcd.setCursor(0, 1);
    lcd.print("Enter Peso:");

  }
}

void resetSystem() {
  dispenserServo.write(90);
  dispensing = false;
  transactionComplete = false;
  inputAmount = "";
  targetWeight = 0.0;

  lcd.clear();
  lcd.print("System Reset");
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER_PIN, 2000, 500);
  delay(1000);
  digitalWrite(RED_LED, LOW);
  scale.tare();

  lcd.clear();
  lcd.print("Enter Pesos:");
  lcd.setCursor(0, 1);

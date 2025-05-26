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
float calibration_factor = 102655.00; // Adjust this to your calibrated value

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
String inputWeight = "";
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
  lcd.print("Enter kilos:");

  dispenserServo.attach(SERVO_PIN);
  dispenserServo.write(90); // Initial position (stopped)
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  char key = keypad.getKey();

  // Reset system on '*' key
  if (key == '*') {
    resetSystem();
    return;
  }

  // Handle dispensing state
  if (dispensing) {
    monitorWeight();
    return;
  }

  // Handle transaction complete state
  if (transactionComplete) {
    // Wait for user to start a new transaction by entering a new amount
    if (key && key >= '0' && key <= '9') {
      // Prepare for new transaction
      inputWeight = "";
      targetWeight = 0.0;
      transactionComplete = false;
      lcd.clear();
      lcd.print("Enter kilos:");
      lcd.setCursor(0, 1);
      inputWeight += key;
      lcd.print(inputWeight + " kg     ");
    }
    return;
  }

  if (key) {
    if (!dispensing && !transactionComplete) {
      if (key >= '0' && key <= '9') {
        if (inputWeight.length() < 4) { // Limit input length
          inputWeight += key;
          lcd.setCursor(0, 1);
          lcd.print(inputWeight + " kg     ");
        }
      } else if (key == '#') {
        // Start dispensing
        if (inputWeight.length() > 0) {
          targetWeight = inputWeight.toFloat();
          if (targetWeight > 0) {
            dispensing = true;
            scale.tare();  // Reset scale to zero
            lcd.clear();
            lcd.print("Dispensing...");
            dispenserServo.write(0);  // Start motor (adjust value as needed)
            digitalWrite(GREEN_LED, HIGH);
            digitalWrite(RED_LED, LOW);
          }
        }
      }
    }
  }
}

void monitorWeight() {
  // Read weight from scale
  currentWeight = scale.get_units();
  
  // Update display every 250ms to avoid flicker
  if (millis() - lastUpdateTime > 250) {
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 2);
    lcd.print("/");
    lcd.print(targetWeight, 2);
    lcd.print(" kg   ");
    lastUpdateTime = millis();
  }

  // Check if target weight reached
  if (currentWeight >= targetWeight) {
    // Stop dispensing
    dispenserServo.write(90);  // Stop motor
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    
    // Sound alert
    tone(BUZZER_PIN, 1000, 1000);
    
    lcd.clear();
    lcd.print("Complete!");
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 2);
    lcd.print(" kg dispensed");
    
    dispensing = false;
    transactionComplete = true;
    
    // Send the weight to ESP32 for recording the sale
    Serial.println(currentWeight);
    
    delay(2000);
    lcd.clear();
    lcd.print("Press * to reset");
    lcd.setCursor(0, 1);
    lcd.print("Transaction complete");
  }
}

// Fixed reset system function
void resetSystem() {
  // Stop all operations
  dispenserServo.write(90);  // Stop motor
  
  // Reset all state variables
  dispensing = false;
  transactionComplete = false;
  inputWeight = "";
  targetWeight = 0.0;
  
  // Reset display
  lcd.clear();
  lcd.print("System Reset");
  
  // Visual feedback
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER_PIN, 2000, 500);
  delay(1000);
  digitalWrite(RED_LED, LOW);
  
  // Reset scale
  scale.tare();
  
  // Ready for new input
  lcd.clear();
  lcd.print("Enter kilos:");
  lcd.setCursor(0, 1);
}

/*
 * FSMB Recruitment 2026 - Phase 1
 * Embedded System Engineer
 * Voltage Protector Device
 *
 *  Latching faults, button menu, EEPROM, serial commands
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ---------- Pin definitions ----------
#define PIN_VOLTAGE   A0
#define PIN_CURRENT   A1
#define PIN_BTN_UP    2
#define PIN_BTN_DOWN  3
#define PIN_BTN_SEL   4
#define PIN_RELAY     7

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Calibration ----------
const float V_MIN_REAL = 0.0;
const float V_MAX_REAL = 300.0;
const float I_MIN_REAL = 0.0;
const float I_MAX_REAL = 15.0;

// ---------- Thresholds (loaded from EEPROM) ----------
float V_OVER  = 250.0;
float V_UNDER = 180.0;
float I_OVER  = 10.0;

// ---------- EEPROM addresses ----------
#define EE_MAGIC   0
#define EE_VOVER   4
#define EE_VUNDER  8
#define EE_IOVER   12
#define EE_MAGIC_VALUE 0xA5

// ---------- State machine ----------
enum State { NORMAL, FAULT_LATCHED, MENU };
State state = NORMAL;

// ---------- Fault reason ----------
enum FaultType { NONE, OVER_V, UNDER_V, OVER_I };
FaultType lastFault = NONE;

// ---------- Menu ----------
int menuIndex = 0;          // 0=VMAX, 1=VMIN, 2=IMAX
float menuValue = 0;

// ---------- Button debounce ----------
unsigned long lastBtnTime = 0;
const unsigned long DEBOUNCE_MS = 200;

bool buttonPressed(uint8_t pin) {
  if (digitalRead(pin) == LOW && (millis() - lastBtnTime) > DEBOUNCE_MS) {
    lastBtnTime = millis();
    return true;
  }
  return false;
}

// ---------- EEPROM ----------
void saveThresholds() {
  EEPROM.put(EE_MAGIC, (byte)EE_MAGIC_VALUE);
  EEPROM.put(EE_VOVER, V_OVER);
  EEPROM.put(EE_VUNDER, V_UNDER);
  EEPROM.put(EE_IOVER, I_OVER);
}

void loadThresholds() {
  byte magic = EEPROM.read(EE_MAGIC);
  if (magic == EE_MAGIC_VALUE) {
    EEPROM.get(EE_VOVER, V_OVER);
    EEPROM.get(EE_VUNDER, V_UNDER);
    EEPROM.get(EE_IOVER, I_OVER);
  } else {
    saveThresholds();
  }
}

// ---------- ADC helpers ----------
int readADC(uint8_t pin) {
  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 8;
}

float adcToVoltage(int adc) {
  return V_MIN_REAL + (adc / 1023.0) * (V_MAX_REAL - V_MIN_REAL);
}

float adcToCurrent(int adc) {
  return I_MIN_REAL + (adc / 1023.0) * (I_MAX_REAL - I_MIN_REAL);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_SEL, INPUT_PULLUP);

  digitalWrite(PIN_RELAY, HIGH);

  Wire.begin();
  lcd.init();
  lcd.backlight();

  loadThresholds();

  lcd.setCursor(0, 0);
  lcd.print("Voltage Protector");
  lcd.setCursor(0, 1);
  lcd.print("V2 Ready");
  delay(1500);
  lcd.clear();

  Serial.println("Voltage Protector V2 initialized.");
  Serial.print("Thresholds: VMAX=");
  Serial.print(V_OVER);
  Serial.print(" VMIN=");
  Serial.print(V_UNDER);
  Serial.print(" IMAX=");
  Serial.println(I_OVER);
  Serial.println("Commands: GET_STATUS | SET_VMAX n | SET_VMIN n | SET_IMAX n | RESET");
}

// ---------- Serial command handler ----------
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd == "GET_STATUS") {
    Serial.print("STATUS: V=");
    Serial.print(adcToVoltage(readADC(PIN_VOLTAGE)), 1);
    Serial.print("V I=");
    Serial.print(adcToCurrent(readADC(PIN_CURRENT)), 2);
    Serial.print("A Relay=");
    Serial.print(state == NORMAL ? "ON" : "OFF");
    Serial.print(" VMAX=");
    Serial.print(V_OVER);
    Serial.print(" VMIN=");
    Serial.print(V_UNDER);
    Serial.print(" IMAX=");
    Serial.println(I_OVER);
  }
  else if (cmd.startsWith("SET_VMAX ")) {
    V_OVER = cmd.substring(9).toFloat();
    saveThresholds();
    Serial.print("OK: VMAX="); Serial.println(V_OVER);
  }
  else if (cmd.startsWith("SET_VMIN ")) {
    V_UNDER = cmd.substring(9).toFloat();
    saveThresholds();
    Serial.print("OK: VMIN="); Serial.println(V_UNDER);
  }
  else if (cmd.startsWith("SET_IMAX ")) {
    I_OVER = cmd.substring(9).toFloat();
    saveThresholds();
    Serial.print("OK: IMAX="); Serial.println(I_OVER);
  }
  else if (cmd == "RESET") {
    if (state == FAULT_LATCHED) {
      state = NORMAL;
      lastFault = NONE;
      digitalWrite(PIN_RELAY, HIGH);
      Serial.println("OK: Fault cleared, relay closed");
    } else {
      Serial.println("OK: No fault to clear");
    }
  }
  else {
    Serial.println("ERR: Unknown command");
  }
}

// ---------- Menu handler ----------
void handleMenu() {
  if (buttonPressed(PIN_BTN_SEL)) {
    menuIndex++;
    if (menuIndex > 2) {
      saveThresholds();
      state = NORMAL;
      lastFault = NONE;
      digitalWrite(PIN_RELAY, HIGH);
      lcd.clear();
      Serial.println("Menu saved. Relay ON.");
      return;
    }
    menuValue = (menuIndex == 0) ? V_OVER : (menuIndex == 1) ? V_UNDER : I_OVER;
  }

  if (buttonPressed(PIN_BTN_UP))   menuValue += 1.0;
  if (buttonPressed(PIN_BTN_DOWN)) menuValue -= 1.0;

  if (menuIndex == 0) V_OVER  = menuValue;
  if (menuIndex == 1) V_UNDER = menuValue;
  if (menuIndex == 2) I_OVER  = menuValue;

  lcd.setCursor(0, 0);
  const char* label = (menuIndex == 0) ? "Set VMAX:" : (menuIndex == 1) ? "Set VMIN:" : "Set IMAX:";
  lcd.print(label);
  lcd.print("       ");

  lcd.setCursor(0, 1);
  lcd.print(menuValue, 1);
  lcd.print("    ");
}

// ---------- Main loop ----------
void loop() {
  handleSerial();

  // Enter menu if SELECT pressed in NORMAL state
  if (state == NORMAL && buttonPressed(PIN_BTN_SEL)) {
    state = MENU;
    menuIndex = 0;
    menuValue = V_OVER;
    lcd.clear();
    return;
  }

  if (state == MENU) {
    handleMenu();
    return;
  }

  int vAdc = readADC(PIN_VOLTAGE);
  int iAdc = readADC(PIN_CURRENT);
  float voltage = adcToVoltage(vAdc);
  float current = adcToCurrent(iAdc);

  // ---- Fault detection ----
  FaultType fault = NONE;
  if (voltage > V_OVER)        fault = OVER_V;
  else if (voltage < V_UNDER)  fault = UNDER_V;
  else if (current > I_OVER)   fault = OVER_I;

  if (fault != NONE && state == NORMAL) {
    state = FAULT_LATCHED;
    lastFault = fault;
    digitalWrite(PIN_RELAY, LOW);
    Serial.print("FAULT LATCHED: ");
    if (fault == OVER_V)  Serial.println("OVER-VOLTAGE");
    if (fault == UNDER_V) Serial.println("UNDER-VOLTAGE");
    if (fault == OVER_I)  Serial.println("OVER-CURRENT");
    Serial.println("Send RESET or press SELECT to clear.");
  }

  // ---- LCD display ----
  if (state == FAULT_LATCHED) {
    lcd.setCursor(0, 0);
    lcd.print("!! FAULT !!     ");
    lcd.setCursor(0, 1);
    if (lastFault == OVER_V)  lcd.print("OVER-VOLTAGE   ");
    if (lastFault == UNDER_V) lcd.print("UNDER-VOLTAGE  ");
    if (lastFault == OVER_I)  lcd.print("OVER-CURRENT   ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("V:");
    lcd.print(voltage, 1);
    lcd.print("V I:");
    lcd.print(current, 2);
    lcd.print("A ");

    lcd.setCursor(0, 1);
    lcd.print("Relay:ON  OK   ");
  }

  // ---- Periodic serial status ----
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 1000) {
    lastStatus = millis();
    Serial.print("V=");
    Serial.print(voltage, 1);
    Serial.print("V I=");
    Serial.print(current, 2);
    Serial.print("A Relay=");
    Serial.print(state == NORMAL ? "ON" : "OFF");
    Serial.print(" State=");
    Serial.println(state == NORMAL ? "NORMAL" : "FAULT_LATCHED");
  }

  delay(100);
}

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 10
#define BUTTON1_PIN 2  // V's birthday (May 30) - label "V"
#define BUTTON2_PIN 3  // B's birthday - label "B"
#define BUTTON3_PIN 4  // Christmas (Dec 24) - label "*"

 // DIN  → D11 (MOSI pin - hardware SPI)
 // CS   → D10 (can be any digital pin)
 // CLK  → D13 (SCK pin - hardware SPI)

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
RTC_DS3231 rtc;

char displayBuffer[20];
int displayMode = 0;  // 0 = time, 1 = birthday1, 2 = birthday2, 3 = christmas
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool lastButton1State = HIGH;
bool lastButton2State = HIGH;
bool lastButton3State = HIGH;
int lastDisplayedMinute = -1;  // Track last displayed minute for throttling

// Auto-return to clock timing
unsigned long countdownStartTime = 0;
const unsigned long countdownTimeout = 30000;  // 30 seconds

// Display intensity (0-15)
const int clockIntensity = 0;
const int countdownIntensity = 3;

// Icon animation
const unsigned long iconInterval = 3000;  // 3 seconds
unsigned long lastIconSwitch = 0;
int iconIndex = 0;
const char birthdayIcons[] = {'@', '~', '#'};   // cake, present, heart
const char christmasIcons[] = {'^', '*', '~'};   // tree, star, present

// Custom V character (maps to 'V')
const uint8_t vChar[] = {
  5,
  B00001111,
  B00110000,
  B01000000,
  B00110000,
  B00001111,
};

// Custom B character (maps to 'B')
const uint8_t bChar[] = {
  5,
  B01111111,
  B01001001,
  B01001001,
  B01001001,
  B00110110,
};

// Custom cake character (maps to '@')
const uint8_t cakeChar[] = {
  8,
  B01110000,
  B01111101,
  B01110000,
  B01111101,
  B01110000,
  B01111101,
  B01110000,
  B00000000,
};

// Heart character (maps to '#')
const uint8_t heartChar[] = {
  8,
  B00001100,
  B00010010,
  B00100010,
  B01000100,
  B00100010,
  B00010010,
  B00001100,
  B00000000,
};

// Custom present character (maps to '~')
const uint8_t presentChar[] = {
  8,
  B00000000,
  B01111100,
  B01010101,
  B01111110,
  B01010101,
  B01111100,
  B00000000,
  B00000000,
};

// Tree character (maps to '^')
const uint8_t treeChar[] = {
  8,
  B00100000,
  B00110100,
  B00111110,
  B01111111,
  B00111110,
  B00110100,
  B00100000,
  B00000000,
};

// Star character (maps to '&')
const uint8_t starChar[] = {
  8,
  B00001000,
  B00101010,
  B00011100,
  B01111111,
  B00011100,
  B00101010,
  B00001000,
  B00000000
};

void setup() {
  Serial.begin(9600);
  
  // Initialize buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  
  // Initialize display
  myDisplay.begin();
  myDisplay.setIntensity(clockIntensity);
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.displayClear();

  // Add custom characters
  myDisplay.addChar('~', presentChar);
  myDisplay.addChar('^', treeChar);
  myDisplay.addChar('*', starChar);
  myDisplay.addChar('V', vChar);
  myDisplay.addChar('B', bChar);
  myDisplay.addChar('@', cakeChar);
  myDisplay.addChar('#', heartChar);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    myDisplay.print("NO RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  myDisplay.print("Ready");
  delay(1000);
}

void loop() {
  DateTime now = rtc.now();
  
  // Read button states
  bool button1State = digitalRead(BUTTON1_PIN);
  bool button2State = digitalRead(BUTTON2_PIN);
  bool button3State = digitalRead(BUTTON3_PIN);
  
  // Button 1 handling (May 30 - "V")
  if (button1State == LOW && lastButton1State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (displayMode == 1) {
        displayMode = 0;  // Back to time
        myDisplay.setIntensity(clockIntensity);
        lastDisplayedMinute = -1;  // Force time update
      } else {
        displayMode = 1;  // Show birthday countdown
        myDisplay.setIntensity(countdownIntensity);
        iconIndex = 0;
        lastIconSwitch = millis();
        countdownStartTime = millis();
      }
      lastDebounceTime = millis();
    }
  }
  lastButton1State = button1State;

  // Button 2 handling (July 28 - "B")
  if (button2State == LOW && lastButton2State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (displayMode == 2) {
        displayMode = 0;  // Back to time
        myDisplay.setIntensity(clockIntensity);
        lastDisplayedMinute = -1;  // Force time update
      } else {
        displayMode = 2;  // Show July 28 countdown
        myDisplay.setIntensity(countdownIntensity);
        iconIndex = 0;
        lastIconSwitch = millis();
        countdownStartTime = millis();
      }
      lastDebounceTime = millis();
    }
  }
  lastButton2State = button2State;

  // Button 3 handling (Christmas - "*")
  if (button3State == LOW && lastButton3State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (displayMode == 3) {
        displayMode = 0;  // Back to time
        myDisplay.setIntensity(clockIntensity);
        lastDisplayedMinute = -1;  // Force time update
      } else {
        displayMode = 3;  // Show Christmas countdown
        myDisplay.setIntensity(countdownIntensity);
        iconIndex = 0;
        lastIconSwitch = millis();
        countdownStartTime = millis();
      }
      lastDebounceTime = millis();
    }
  }
  lastButton3State = button3State;

  // Auto-return to clock after 30 seconds
  if (displayMode != 0 && (millis() - countdownStartTime >= countdownTimeout)) {
    displayMode = 0;
    myDisplay.setIntensity(clockIntensity);
    lastDisplayedMinute = -1;  // Force time update
  }
  
  // Rotate icons every 3 seconds in countdown modes
  if (displayMode != 0 && (millis() - lastIconSwitch >= iconInterval)) {
    iconIndex = (iconIndex + 1) % 3;
    lastIconSwitch = millis();
  }

  // Display based on current mode
  switch (displayMode) {
    case 0:  // Time mode - only update when minute changes
      if (now.minute() != lastDisplayedMinute) {
        sprintf(displayBuffer, "%02d:%02d", now.hour(), now.minute());
        myDisplay.print(displayBuffer);
        lastDisplayedMinute = now.minute();
      }
      break;
      
    case 1:  // V's birthday (May 30)
      displayBirthdayCountdown(now, 5, 30, "V", birthdayIcons[iconIndex]);
      break;

    case 2:  // B's birthday (July 28)
      displayBirthdayCountdown(now, 7, 28, "B", birthdayIcons[iconIndex]);
      break;

    case 3:  // Christmas (Dec 24)
      displayChristmasCountdown(now, 12, 24, christmasIcons[iconIndex]);
      break;
  }
  
  delay(100);
}

void displayChristmasCountdown(DateTime now, int month, int day, char icon) {
  int daysUntil = calculateDaysUntil(now, month, day);
  sprintf(displayBuffer, "%c  %3d", icon, daysUntil);
  myDisplay.print(displayBuffer);
}

void displayBirthdayCountdown(DateTime now, int month, int day, const char* name, char icon) {
  int daysUntil = calculateDaysUntil(now, month, day);
  sprintf(displayBuffer, "%s%c%3d", name, icon, daysUntil);
  myDisplay.print(displayBuffer);
}

int calculateDaysUntil(DateTime now, int targetMonth, int targetDay) {
  // Normalize current date to midnight for accurate day counting
  DateTime todayMidnight(now.year(), now.month(), now.day(), 0, 0, 0);

  // Create target date for this year (already at midnight)
  DateTime targetDate(now.year(), targetMonth, targetDay, 0, 0, 0);

  // If date has passed this year, calculate for next year
  if (todayMidnight.unixtime() > targetDate.unixtime()) {
    targetDate = DateTime(now.year() + 1, targetMonth, targetDay, 0, 0, 0);
  }

  // Calculate difference in seconds and convert to days
  long secondsUntil = targetDate.unixtime() - todayMidnight.unixtime();
  int daysUntil = secondsUntil / 86400;

  return daysUntil;
}
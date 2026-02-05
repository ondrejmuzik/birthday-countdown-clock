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

// Animation timing
unsigned long lastIconSwap = 0;
int currentIcon = 0;
const int NUM_ICONS = 3;
const char iconChars[] = {'~', '`', '#'};  // cake, present, heart

// Custom present character (maps to '~')
const uint8_t presentChar[] = {
  7,
  B00000000,
  B01111100,
  B01010101,
  B01111110,
  B01010101,
  B01111100,
  B00000000,
};

// Custom cake character (maps to '`')
const uint8_t cakeChar[] = {
  7,
  B01110000,
  B01111101,
  B01110000,
  B01111101,
  B01110000,
  B01111101,
  B01110000
};

// Heart character (maps to '#')
const uint8_t heartChar[] = {
  7,
  B00001100,
  B00010010,
  B00100010,
  B01000100,
  B00100010,
  B00010010,
  B00001100,
};

void setup() {
  Serial.begin(9600);
  
  // Initialize buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  
  // Initialize display
  myDisplay.begin();
  myDisplay.setIntensity(0);  // Start with time brightness
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.displayClear();

  // Add custom characters
  myDisplay.addChar('~', cakeChar);    // cake
  myDisplay.addChar('`', presentChar); // present
  myDisplay.addChar('#', heartChar);   // heart
  
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
        myDisplay.setIntensity(0);
      } else {
        displayMode = 1;  // Show birthday countdown
        myDisplay.setIntensity(4);
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
        myDisplay.setIntensity(0);
      } else {
        displayMode = 2;  // Show July 28 countdown
        myDisplay.setIntensity(4);
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
        myDisplay.setIntensity(0);
      } else {
        displayMode = 3;  // Show Christmas countdown
        myDisplay.setIntensity(4);
      }
      lastDebounceTime = millis();
    }
  }
  lastButton3State = button3State;
  
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
      displayBirthdayCountdown(now, 5, 30, "V");
      break;

    case 2:  // B's birthday (July 28)
      displayBirthdayCountdown(now, 7, 28, "B");
      break;
      
    case 3:  // Christmas (Dec 24) - label "*"
      displayCountdown(now, 12, 24, "*");
      break;
  }
  
  delay(100);
}

void displayCountdown(DateTime now, int month, int day, const char* label) {
  int daysUntil = calculateDaysUntil(now, month, day);
  sprintf(displayBuffer, "%s %d", label, daysUntil);
  myDisplay.print(displayBuffer);
}

void displayBirthdayCountdown(DateTime now, int month, int day, const char* name) {
  int daysUntil = calculateDaysUntil(now, month, day);

  // Cycle through icons every 2 seconds
  if (millis() - lastIconSwap >= 2000) {
    currentIcon = (currentIcon + 1) % NUM_ICONS;
    lastIconSwap = millis();
  }

  char icon = iconChars[currentIcon];
  sprintf(displayBuffer, "%c%s %d", icon, name, daysUntil);
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
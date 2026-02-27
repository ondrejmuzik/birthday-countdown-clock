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

// Auto-return to clock timing
unsigned long countdownStartTime = 0;
const unsigned long countdownTimeout = 30000;  // 30 seconds

// Display intensity (0-15)
int clockIntensity = 6;
int countdownIntensity = 9;

// DST offset (0 = off, 1 = +1 hour)
int dstOffset = 0;

// Simultaneous button press tracking
unsigned long simPressStartTime = 0;
bool simPressHandled = false;

// Icon animation
const unsigned long iconInterval = 3000;  // 3 seconds
unsigned long lastIconSwitch = 0;
int iconIndex = 0;
const char birthdayIcons[] = {'@', '~', '#'};   // cake, present, heart
const char christmasIcons[] = {'^', '*', '~'};   // tree, star, present

// Dot display state
bool dotDisplayActive = false;
unsigned long dotDisplayStartTime = 0;
int dotFullScreens = 0;     // number of full 256-LED pages
int dotRemainder = 0;       // leftover dots after full pages

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

// Cake character (maps to '@')
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

// Present character (maps to '~')
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

// Star character (maps to '*')
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

// Forward declarations
int calculateDaysUntil(DateTime now, int targetMonth, int targetDay);

void handleSimultaneousPress(int combo) {
  if (combo == 12) {
    clockIntensity = max(0, clockIntensity - 3);
    countdownIntensity = max(0, countdownIntensity - 3);
  } else if (combo == 23) {
    clockIntensity = min(15, clockIntensity + 3);
    countdownIntensity = min(15, countdownIntensity + 3);
  } else if (combo == 13) {
    dstOffset = dstOffset == 0 ? 1 : 0;
    lastDisplayedMinute = -1;  // force clock redraw
  }
  myDisplay.setIntensity(displayMode == 0 ? clockIntensity : countdownIntensity);
}

void handleButtonPress(int buttonMode, int targetMonth, int targetDay) {
  if (displayMode == buttonMode && dotDisplayActive) {
    // 3rd press: dot display -> back to clock
    dotDisplayActive = false;
    displayMode = 0;
    myDisplay.displayClear();
    myDisplay.setIntensity(clockIntensity);
    lastDisplayedMinute = -1;
  } else if (displayMode == buttonMode && !dotDisplayActive) {
    // 2nd press: text countdown -> dot display
    dotDisplayActive = true;
    int daysUntil = calculateDaysUntil(rtc.now(), targetMonth, targetDay);
    dotFullScreens = daysUntil / 256;
    dotRemainder = daysUntil % 256;
    dotDisplayStartTime = millis();
    countdownStartTime = millis();  // reset auto-return timer
    myDisplay.displayClear();
  } else {
    // 1st press: any state -> text countdown
    displayMode = buttonMode;
    dotDisplayActive = false;
    myDisplay.setIntensity(countdownIntensity);
    iconIndex = 0;
    lastIconSwitch = millis();
    countdownStartTime = millis();
  }
}

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
  
  // Simultaneous press detection (1 second hold required)
  bool combo12 = (button1State == LOW && button2State == LOW && button3State == HIGH);
  bool combo23 = (button1State == HIGH && button2State == LOW && button3State == LOW);
  bool combo13 = (button1State == LOW && button2State == HIGH && button3State == LOW);
  bool anyCombo = combo12 || combo23 || combo13;

  if (anyCombo) {
    if (simPressStartTime == 0) simPressStartTime = millis();
    if (!simPressHandled && (millis() - simPressStartTime >= 1000)) {
      handleSimultaneousPress(combo12 ? 12 : (combo23 ? 23 : 13));
      simPressHandled = true;
    }
  } else {
    simPressStartTime = 0;
    simPressHandled = false;
  }

  // Button 1 handling (May 30 - "V")
  if (!anyCombo && button1State == LOW && lastButton1State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      handleButtonPress(1, 5, 30);
      lastDebounceTime = millis();
    }
  }
  lastButton1State = button1State;

  // Button 2 handling (July 28 - "B")
  if (!anyCombo && button2State == LOW && lastButton2State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      handleButtonPress(2, 7, 28);
      lastDebounceTime = millis();
    }
  }
  lastButton2State = button2State;

  // Button 3 handling (Christmas - "*")
  if (!anyCombo && button3State == LOW && lastButton3State == HIGH) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      handleButtonPress(3, 12, 24);
      lastDebounceTime = millis();
    }
  }
  lastButton3State = button3State;

  // Auto-return to clock after 30 seconds
  if (displayMode != 0 && (millis() - countdownStartTime >= countdownTimeout)) {
    displayMode = 0;
    dotDisplayActive = false;
    myDisplay.displayClear();
    myDisplay.setIntensity(clockIntensity);
    lastDisplayedMinute = -1;  // Force time update
  }
  
  // Icon rotation reserved for future imminent-date feature

  // Display based on current mode
  switch (displayMode) {
    case 0:  // Time mode - only update when minute changes
      if (now.minute() != lastDisplayedMinute) {
        int displayHour = (now.hour() + dstOffset) % 24;
        sprintf(displayBuffer, "%02d:%02d", displayHour, now.minute());
        myDisplay.print(displayBuffer);
        lastDisplayedMinute = now.minute();
      }
      break;

    case 1:  // V's birthday (May 30)
    case 2:  // B's birthday (July 28)
    case 3:  // Christmas (Dec 24)
      if (dotDisplayActive) {
        handleDotDisplay();
      } else if (displayMode == 1) {
        displayBirthdayCountdown(now, 5, 30, "V", '@');  // cake
      } else if (displayMode == 2) {
        displayBirthdayCountdown(now, 7, 28, "B", '@');  // cake
      } else {
        displayChristmasCountdown(now, 12, 24, '^');  // tree
      }
      break;
  }
  
  delay(100);
}

void displayDots(int numDots) {
  MD_MAX72XX *mx = myDisplay.getGraphicObject();
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // buffer changes
  mx->clear();

  // Fill left-to-right, bottom-to-top (row 7 first, then row 6, etc.)
  // Each row has 32 columns; dot index = row * 32 + column_position
  for (int c = 0; c < 32; c++) {
    int col = 31 - c;  // col 0 is rightmost in FC16_HW, so invert
    uint8_t colValue = 0;
    for (int row = 0; row < 8; row++) {
      if (row * 32 + c < numDots) {
        colValue |= (1 << (7 - row));
      }
    }
    mx->setColumn(col, colValue);
  }

  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);  // flush to display
}

void handleDotDisplay() {
  const unsigned long phaseDuration = 3000;  // 3s per phase

  if (dotFullScreens == 0) {
    // No overflow — just show dots as static display
    displayDots(dotRemainder);
    myDisplay.setIntensity(countdownIntensity);
    return;
  }

  // If remainder is 0 (exact multiple of 256), treat as one fewer full screen
  // and show 256 dots in the remainder phase instead of a blank screen
  int effectiveFullScreens = dotRemainder == 0 ? dotFullScreens - 1 : dotFullScreens;
  int effectiveRemainder = dotRemainder == 0 ? 256 : dotRemainder;

  // Cycle: [full screen, 3s] x N ... [remainder, 3s]
  int totalPhases = effectiveFullScreens + 1;  // +1 for remainder phase
  unsigned long cycleDuration = (unsigned long)totalPhases * phaseDuration;
  unsigned long elapsed = (millis() - dotDisplayStartTime) % cycleDuration;
  int currentPhase = elapsed / phaseDuration;

  if (currentPhase < effectiveFullScreens) {
    displayDots(256);
  } else {
    displayDots(effectiveRemainder);
  }
  myDisplay.setIntensity(countdownIntensity);
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
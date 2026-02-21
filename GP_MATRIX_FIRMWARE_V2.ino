// RP2040_Icon_Display.ino
#include <Adafruit_NeoPixel.h>
#include "icons.h"

// Configuration
#define LED_PIN        29
#define NUM_LEDS       64  // 8x8 = 64
#define LED_TYPE       NEO_GRB + NEO_KHZ800

// Create NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, LED_TYPE);

// Serial command buffer
String inputString = "";
bool stringComplete = false;

// Current mode state
uint8_t currentMode = 0;
uint8_t currentBrightness = 50;
uint8_t currentState = 0;
uint8_t currentCategory = 1;
uint8_t currentIntensity = 1;

// For mode 1 (Illumination) RGB values
uint8_t currentR = 255;
uint8_t currentG = 255;
uint8_t currentB = 255;

// For mode 3 (Beacon) timing
unsigned long lastBeaconToggle = 0;
bool beaconColorState = false;
uint8_t beaconBaseTime = 500; // Default 500ms
uint8_t beaconMultiplier = 1;  // Default multiplier (now 1-255)

// Flag to indicate first command received
bool firstCommandReceived = false;

// Helper function to set a single LED with RGB values
void setPixelColor(int index, uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(index, strip.Color(r, g, b));
}

// Display icon from 3D array [y][x][rgb] with serpentine layout
void displayIcon(const uint8_t icon[8][8][3]) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      int flippedY = 7 - y;  // Flip Y-axis
      
      // If row is even, go left to right; if odd, go right to left
      int actualX;
      if (flippedY % 2 == 0) {
        actualX = x;  // Even row: left to right
      } else {
        actualX = 7 - x;  // Odd row: right to left
      }
      
      int index = flippedY * 8 + actualX;
      uint8_t r = icon[y][x][0];
      uint8_t g = icon[y][x][1];
      uint8_t b = icon[y][x][2];
      setPixelColor(index, r, g, b);
    }
  }
  strip.show();
}

// Display icon by index
void displayIconByIndex(int index) {
  if (index >= 0 && index < NUM_ICONS) {
    displayIcon((const uint8_t(*)[8][3])icon_table[index]);
    Serial.print("OK:DISPLAYED_ICON_");
    Serial.println(index);
  }
}

// RGB Test Pattern
void runRGBTest() {
  Serial.println("\n=== RUNNING RGB TEST ===");
  
  // Test Red
  Serial.println("Testing RED - all LEDs red");
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 255, 0, 0);
  }
  strip.show();
  delay(1000);
  
  // Test Green
  Serial.println("Testing GREEN - all LEDs green");
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 0, 255, 0);
  }
  strip.show();
  delay(1000);
  
  // Test Blue
  Serial.println("Testing BLUE - all LEDs blue");
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 0, 0, 255);
  }
  strip.show();
  delay(1000);
  
  Serial.println("RGB TEST COMPLETE");
}

// Standby mode - very dark red square
void setStandbyMode() {
  Serial.println("Entering STANDBY mode - dark red square");
  
  // Very dark red (barely visible)
  uint8_t darkRed = 2;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, darkRed, 0, 0);
  }
  strip.show();
}

// Mode 0: Turn off all LEDs
void modeOff() {
  strip.clear();
  strip.show();
  Serial.println("OK:MODE_OFF");
}

// Mode 1: Illumination - all LEDs at specified RGB color
void modeIllumination(uint8_t brightnessPercent, uint8_t r, uint8_t g, uint8_t b) {
  uint8_t brightness = map(brightnessPercent, 1, 100, 0, 255);
  brightness = constrain(brightness, 0, 255);
  
  currentR = r;
  currentG = g;
  currentB = b;
  
  uint8_t scaledR = map(r, 0, 255, 0, brightness);
  uint8_t scaledG = map(g, 0, 255, 0, brightness);
  uint8_t scaledB = map(b, 0, 255, 0, brightness);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, scaledR, scaledG, scaledB);
  }
  strip.show();
  Serial.print("OK:ILLUMINATION_R");
  Serial.print(r);
  Serial.print("_G");
  Serial.print(g);
  Serial.print("_B");
  Serial.print(b);
  Serial.print("_BR");
  Serial.println(brightnessPercent);
}

// Mode 2: State display using icons and progress bar
void modeState(uint8_t state, uint8_t category, uint8_t intensity) {
  intensity = constrain(intensity, 1, 100); // Intensity is percentage 1-100
  
  if (state == 0) { // All good - use icon_02_ok (green square)
    displayIconByIndex(2); // icon_02_ok
    Serial.println("OK:STATE_GOOD_ICON");
  }
  else if (state == 2) { // Major Issue - use icon_00_error (red X)
    displayIconByIndex(0); // icon_00_error
    Serial.println("OK:STATE_ERROR_ICON");
  }
  else { // state == 1 (Okay) - progress bar on bottom three rows
    strip.clear();
    
    // Define colors based on category for top rows
    uint8_t topColor[3];
    
    if (category == 1) { // Battery - Yellow
      topColor[0] = 255;
      topColor[1] = 255;
      topColor[2] = 0;
    } else { // Navigation - Cyan
      topColor[0] = 0;
      topColor[1] = 255;
      topColor[2] = 255;
    }
    
    // Top 3 rows (rows 0-2): Always lit with category color
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 8; x++) {
        int flippedY = 7 - y;  // Flip Y-axis to match serpentine layout
        
        // Apply serpentine row reversal
        int actualX;
        if (flippedY % 2 == 0) {
          actualX = x;  // Even row: left to right
        } else {
          actualX = 7 - x;  // Odd row: right to left
        }
        
        int index = flippedY * 8 + actualX;
        setPixelColor(index, topColor[0], topColor[1], topColor[2]);
      }
    }
    
    // Middle 2 rows (rows 3-4): Black (separator)
    // Already cleared by strip.clear(), so nothing to do
    
    // Bottom 3 rows (rows 5-7): Purple progress bar
    // Fill columns from left to right across all 3 rows
    int totalColumns = 8; // 8 columns
    int columnsToLight = map(intensity, 1, 100, 1, totalColumns);
    
    for (int x = 0; x < columnsToLight; x++) {
      for (int y = 5; y <= 7; y++) {
        int flippedY = 7 - y;  // Flip Y-axis to match serpentine layout
        
        // Apply serpentine row reversal
        int actualX;
        if (flippedY % 2 == 0) {
          actualX = x;  // Even row: left to right
        } else {
          actualX = 7 - x;  // Odd row: right to left (reverses column order)
        }
        
        int index = flippedY * 8 + actualX;
        setPixelColor(index, 128, 0, 128); // Purple
      }
    }
    
    strip.show();
    Serial.print("OK:STATE_OKAY_C");
    Serial.print(category);
    Serial.print("_P");
    Serial.println(intensity);
  }
}

// Mode 3: Beacon - strobing red/blue with adjustable timing
void modeBeacon(uint8_t brightnessPercent, uint8_t baseTime, uint8_t multiplier) {
  uint8_t brightness = map(brightnessPercent, 1, 100, 0, 255);
  brightness = constrain(brightness, 0, 255);
  
  // Calculate strobe interval - multiplier now supports full 1-255 range
  unsigned long strobeInterval = (unsigned long)baseTime * multiplier;
  if (strobeInterval < 10) strobeInterval = 10; // Minimum 10ms for visibility
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastBeaconToggle >= strobeInterval) {
    lastBeaconToggle = currentMillis;
    beaconColorState = !beaconColorState;
    
    if (beaconColorState) {
      // Red
      for (int i = 0; i < NUM_LEDS; i++) {
        setPixelColor(i, brightness, 0, 0);
      }
    } else {
      // Blue
      for (int i = 0; i < NUM_LEDS; i++) {
        setPixelColor(i, 0, 0, brightness);
      }
    }
    strip.show();
  }
}

// Parse the command
void parseCommand(String cmd) {
  cmd.trim();
  Serial.print("DEBUG: Received raw string: '");
  Serial.print(cmd);
  Serial.println("'");
  
  if (cmd.length() == 0) {
    Serial.println("DEBUG: Empty command");
    return;
  }
  
  // Check for special text commands
  String upperCmd = cmd;
  upperCmd.toUpperCase();
  
  if (upperCmd == "HELP") {
    printHelp();
    return;
  }
  else if (upperCmd == "RGBTEST") {
    runRGBTest();
    setStandbyMode();
    return;
  }
  
  // Parse comma-separated values
  int commaIndex1 = cmd.indexOf(',');
  int commaIndex2 = cmd.indexOf(',', commaIndex1 + 1);
  int commaIndex3 = cmd.indexOf(',', commaIndex2 + 1);
  int commaIndex4 = cmd.indexOf(',', commaIndex3 + 1);
  
  if (commaIndex1 > 0 && commaIndex2 > 0 && commaIndex3 > 0 && commaIndex4 > 0) {
    String modeStr = cmd.substring(0, commaIndex1);
    String brightnessStr = cmd.substring(commaIndex1 + 1, commaIndex2);
    String byte3Str = cmd.substring(commaIndex2 + 1, commaIndex3);
    String byte4Str = cmd.substring(commaIndex3 + 1, commaIndex4);
    String byte5Str = cmd.substring(commaIndex4 + 1);
    
    modeStr.trim();
    brightnessStr.trim();
    byte3Str.trim();
    byte4Str.trim();
    byte5Str.trim();
    
    int mode = modeStr.toInt();
    int brightness = brightnessStr.toInt();
    int byte3 = byte3Str.toInt();
    int byte4 = byte4Str.toInt();
    int byte5 = byte5Str.toInt();
    
    Serial.print("DEBUG: Parsed - mode:");
    Serial.print(mode);
    Serial.print(" brightness:");
    Serial.print(brightness);
    Serial.print(" byte3:");
    Serial.print(byte3);
    Serial.print(" byte4:");
    Serial.print(byte4);
    Serial.print(" byte5:");
    Serial.println(byte5);
    
    bool valid = false;
    
    switch(mode) {
      case 0:
        valid = true;
        break;
        
      case 1:
        if (brightness >= 0 && brightness <= 100 &&
            byte3 >= 0 && byte3 <= 255 &&
            byte4 >= 0 && byte4 <= 255 &&
            byte5 >= 0 && byte5 <= 255) {
          valid = true;
        }
        break;
        
      case 2:
        if (brightness >= 0 && brightness <= 100 &&
            byte3 >= 0 && byte3 <= 2 &&
            byte4 >= 1 && byte4 <= 2 &&
            byte5 >= 1 && byte5 <= 100) {
          valid = true;
        }
        break;
        
      case 3:
        if (brightness >= 0 && brightness <= 100 &&
            byte3 >= 0 && byte3 <= 255 &&
            byte4 >= 1 && byte4 <= 255) { // Multiplier now 1-255
          valid = true;
        }
        break;
    }
    
    if (valid) {
      Serial.println("DEBUG: Values valid, executing...");
      
      currentMode = mode;
      currentBrightness = brightness;
      
      if (!firstCommandReceived) {
        firstCommandReceived = true;
        Serial.println("First command received - exiting standby mode");
      }
      
      uint8_t globalBrightness = map(brightness, 0, 100, 0, 255);
      strip.setBrightness(globalBrightness);
      
      switch(mode) {
        case 0:
          modeOff();
          break;
          
        case 1:
          modeIllumination(brightness, byte3, byte4, byte5);
          break;
          
        case 2:
          modeState(byte3, byte4, byte5);
          break;
          
        case 3:
          beaconBaseTime = byte3;
          beaconMultiplier = byte4;
          Serial.print("OK:BEACON_MODE_B");
          Serial.print(brightness);
          Serial.print("_T");
          Serial.print(byte3);
          Serial.print("_M");
          Serial.println(byte4);
          break;
      }
    } else {
      Serial.println("DEBUG: Values out of range for the selected mode!");
      printHelp();
    }
  } else {
    Serial.println("DEBUG: Could not find 4 commas in the string");
    Serial.println("ERR: Invalid format. Use: mode,brightness,byte3,byte4,byte5");
  }
}

void printHelp() {
  Serial.println("\n=== AVAILABLE COMMANDS ===");
  Serial.println("Format: mode,brightness,byte3,byte4,byte5");
  
  Serial.println("\nMode 0 (OFF):");
  Serial.println("  0,0,0,0,0 - Turn all LEDs off");
  
  Serial.println("\nMode 1 (ILLUMINATION):");
  Serial.println("  mode=1, brightness=0-100(%), byte3=R(0-255), byte4=G(0-255), byte5=B(0-255)");
  Serial.println("  Example: 1,50,255,0,0 - Red at 50%");
  
  Serial.println("\nMode 2 (STATE):");
  Serial.println("  mode=2, brightness=0-100(%), byte3=state(0-2), byte4=category(1-2), byte5=intensity(1-100)");
  Serial.println("  state: 0=Good (green square icon), 1=Okay (progress bar), 2=Issue (red X icon)");
  Serial.println("  category: 1=Battery (yellow top), 2=Navigation (cyan top)");
  Serial.println("  Layout for state 1:");
  Serial.println("    - Rows 0-2: Solid yellow/cyan");
  Serial.println("    - Rows 3-4: Black (separator)");
  Serial.println("    - Rows 5-7: Purple progress bar (fills left to right in columns)");
  Serial.println("  intensity: 1-100 (percentage of bottom 3 rows filled with purple)");
  Serial.println("  Examples:");
  Serial.println("    2,50,0,1,1 - Green square icon");
  Serial.println("    2,50,1,1,30 - Battery, bottom 3 rows 30% purple (2-3 columns)");
  Serial.println("    2,50,1,2,75 - Navigation, bottom 3 rows 75% purple (6 columns)");
  Serial.println("    2,50,1,1,100 - Battery, full purple progress bar (8 columns)");
  Serial.println("    2,50,2,1,1 - Red X icon");
  
  Serial.println("\nMode 3 (BEACON):");
  Serial.println("  mode=3, brightness=0-100(%), byte3=baseTime(0-255ms), byte4=multiplier(1-255), byte5=ignored");
  Serial.println("  Strobe interval = baseTime × multiplier milliseconds");
  Serial.println("  Examples:");
  Serial.println("    3,80,100,2,0   - Strobe every 200ms");
  Serial.println("    3,50,100,10,0  - Strobe every 1 second (1000ms)");
  Serial.println("    3,50,250,20,0  - Strobe every 5 seconds (5000ms)");
  Serial.println("    3,90,10,50,0   - Strobe every 500ms");
  Serial.println("    3,50,255,255,0 - Strobe every 65 seconds (max interval)");
  
  Serial.println("\nSpecial commands:");
  Serial.println("  RGBTEST - Run RGB color test");
  Serial.println("  HELP - Show this help");
}

void setup() {
  Serial.begin(115200);
  
  strip.begin();
  strip.show();
  strip.setBrightness(50);
  
  Serial.println("\n=================================");
  Serial.println("RP2040 Matrix Display Ready");
  Serial.println("=================================");
  
  // Quick power-on test
  Serial.println("Power-on test...");
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 100, 0, 0);
  }
  strip.show();
  delay(500);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 0, 100, 0);
  }
  strip.show();
  delay(500);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 0, 0, 100);
  }
  strip.show();
  delay(500);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, 100, 100, 100);
  }
  strip.show();
  delay(500);

  setStandbyMode();
  
  Serial.println("System in STANDBY mode");
  Serial.println("Type HELP for commands");
}

void loop() {
  if (stringComplete) {
    parseCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
  
  if (firstCommandReceived && currentMode == 3) {
    modeBeacon(currentBrightness, beaconBaseTime, beaconMultiplier);
  }
  
  delay(10);
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
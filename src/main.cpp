/*********************************************************************************************************
 * LILYGO T-Display-S3 HC-SR04 Ultrasonic Distance Sensor Project
 *
 * Description:
 *   This code reads distance data from a HC-SR04 ultrasonic distance sensor and displays it on the
 *   built-in screen of the LilyGO T-Display-S3 using the TFT_eSPI library. The distance is displayed
 *   numerically in millimeters (mm) and as a visual meter in centimeters (0-100cm) with a colour gradient
 *   (red at 0cm to green at 100cm). The code uses state machine logic to avoid blocking delays.
 *
 * Key Features:
 *   - Displays measured distance in millimeters (mm) for higher precision
 *   - Visual meter shows distance in centimeters (0-100cm)
 *   - Smooth updates using a sptite to prevent flickering
 *   - Non-blocking state machine implementation
 *
 * How It Works:
 *   1. Sensor Reading: Triggers the HC-SR04 and measures echo pulse duration
 *   2. Distance Calculation: Converts pulse duration to distance in cm, then to mm for display
 *   3. Display: Shows measured distance numerically in mm and as a visual level meter (0 to 100cm)
 *   4. State Machine: Manages timing of sensor readings and display updates (4Hz refresh rate)
 *
 * Pin Connections:
 *   - HC-SR04 Trig  -> GPIO1 (output)
 *   - HC-SR04 Echo  -> GPIO2 (input)
 *   - HC-SR04 GND   -> GND
 *   - HC-SR04 VCC   -> 5V
 *   - LCD Backlight -> GPIO15
 *
 * Notes:
 *   - Keep sensor perpendicular to measured surface for accurate readings
 *   - Minimum measurable distance is 2cm (20mm)
 *   - Visual meter shows 0-100cm range while numeric display shows actual measurement (20mm-4000mm)
 *   - For best results, avoid measuring soft/uneven surfaces and in areas with high noise
 *   - The TFT_eSPI library is configured for LilyGO T-Display-S3
 *
 * HC-SR04 Specifications:
 *   - Measurement Range: ~2cm to ~400cm (20mm to 4000mm)
 *   - Resolution: 0.3cm (3mm)
 *   - Accuracy: ±3mm
 *   - Operating Voltage: 5V DC
 *   - Trigger Pulse Duration: 10µs
 *   - Interface: Trigger/Echo
 * 
 **********************************************************************************************************/

/*************************************************************
******************* INCLUDES & DEFINITIONS *******************
**************************************************************/

#include <Arduino.h>
#include <TFT_eSPI.h>

// TFT_eSPI
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite meterFillSprite = TFT_eSprite(&tft);

// HC-SR04 Pins
#define TRIGGER_PIN 1 // digital pin connected to Trig (GPIO1)
#define ECHO_PIN 2    // digital pin connected to Echo (GPIO2)

// Display parameters
#define LEVEL_METER_X 50       // x position
#define LEVEL_METER_Y 75       // y position
#define LEVEL_METER_WIDTH 40   // width of bar
#define LEVEL_METER_HEIGHT 220 // height of bar
#define MIN_DISTANCE_CM 0      // minimum distance to display
#define MAX_DISTANCE_CM 100    // maximum distance to display

// State Machine States
enum class State {
  READ_SENSOR,    // state for reading sensor data
  UPDATE_DISPLAY, // state for updating the display
  WAIT            // state for waiting between updates
};

// Global variables
State currentState = State::READ_SENSOR;  // initial state
unsigned long previousMillis = 0;         // for non-blocking timing
const unsigned long updateInterval = 250; // update every 250ms (4Hz refresh)
long duration = 0;                        // pulse duration in microseconds
float distance_cm = 0;                    // distance in centimeters
float prev_distance_cm = -1;              // previous distance value


/*************************************************************
********************** HELPER FUNCTIONS **********************
**************************************************************/

// Function to create gradient colours
uint16_t getGradientColour(float distance) {
  // Normalize distance to 0.0-1.0 range
  float ratio = constrain(distance / MAX_DISTANCE_CM, 0.0, 1.0);
  
  // Calculate colour components (red to green gradient)
  uint8_t red = 255 * (1.0 - ratio); // lower distances towards red
  uint8_t green = 255 * ratio;       // higher distances towards green
  uint8_t blue = 0;
  
  // Convert to 16-bit colour (RGB565)
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

// Function to draw the static screen elements
void drawStaticScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Draw the title
  tft.setCursor(0, 0);
  tft.println("----------------------------");
  tft.println(" HC-SR04 Distance Sensor");
  tft.println("----------------------------");
  tft.println("Distance:");

  // Draw 2 meter borders to make it thicker
  tft.drawRect(LEVEL_METER_X, LEVEL_METER_Y, LEVEL_METER_WIDTH, LEVEL_METER_HEIGHT, TFT_DARKGREY); // inner
  tft.drawRect(LEVEL_METER_X - 1, LEVEL_METER_Y - 1, LEVEL_METER_WIDTH + 2, LEVEL_METER_HEIGHT + 2, TFT_DARKGREY); // outer

  // Draw distance markers (every 10cm)
  for (int cm = MIN_DISTANCE_CM; cm <= MAX_DISTANCE_CM; cm += 10) {
    int y_pos = map(cm, MIN_DISTANCE_CM, MAX_DISTANCE_CM, LEVEL_METER_Y + LEVEL_METER_HEIGHT, LEVEL_METER_Y);
    
    // Draw the horizontal marker line to the right of the bar
    tft.drawFastHLine(LEVEL_METER_X + LEVEL_METER_WIDTH, y_pos, 10, TFT_DARKGREY); 
    
    // Set the position for the text label to the right of the marker line
    tft.setCursor(LEVEL_METER_X + LEVEL_METER_WIDTH + 15, y_pos - 8);
    tft.print(cm);
    
    // Only add "cm" to 0 and 100 labels
    if (cm == MIN_DISTANCE_CM || cm == MAX_DISTANCE_CM) {
      tft.print("cm");
    }
  }

  // Initialize meter fill sprite with 1px buffer inside border
  meterFillSprite.createSprite(LEVEL_METER_WIDTH - 2, LEVEL_METER_HEIGHT - 2);
  meterFillSprite.fillSprite(TFT_BLACK);
  meterFillSprite.pushSprite(LEVEL_METER_X + 1, LEVEL_METER_Y + 1);
}

// Function to update distance display (in mm)
void updateDistanceDisplay() {
  // Convert cm to mm (1cm = 10mm)
  float distance_mm = distance_cm * 10;
  
  // Update measured value
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(60, 48);
  tft.fillRect(60, 48, 80, 15, TFT_BLACK);
  tft.print(distance_mm, 0); // 0 decimal places for mm
  tft.println(" mm");
  
  // Rest of the function remains the same
  float meter_distance_cm = constrain(distance_cm, MIN_DISTANCE_CM, MAX_DISTANCE_CM);
  
  if (abs(meter_distance_cm - prev_distance_cm) > 1.0) {
    // Calculate fill height accounting for 1px buffer at bottom
    int fillHeight = map(meter_distance_cm, MIN_DISTANCE_CM, MAX_DISTANCE_CM, 0, LEVEL_METER_HEIGHT - 2);
    
    // Clear sprite
    meterFillSprite.fillSprite(TFT_BLACK);
    
    // Draw gradient fill (red at bottom, green at top)
    for (int y = 0; y < fillHeight; y++) {
      // Calculate current distance position (0 at bottom, 100 at top)
      float current_dist = map(y, 0, LEVEL_METER_HEIGHT - 2, MIN_DISTANCE_CM, MAX_DISTANCE_CM);
      
      // Get gradient colour for this position
      uint16_t colour = getGradientColour(current_dist);
      
      // Draw horizontal line (starting from bottom)
      meterFillSprite.drawFastHLine(0, (LEVEL_METER_HEIGHT - 2) - y - 1, LEVEL_METER_WIDTH - 2, colour);
    }
    
    // Push sprite to display
    meterFillSprite.pushSprite(LEVEL_METER_X + 1, LEVEL_METER_Y + 1);
    
    prev_distance_cm = meter_distance_cm;
  }
}

// Function to read distance from sensor
void readDistance() {
  // Trigger the sensor with clean pulse
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Read the echo pulse duration
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance (cm)
  distance_cm = (duration / 2) * 0.0343; // speed of sound 343 m/s (0.0343 cm/µs)
  
  // Handle cases where sensor reads beyond its effective range
  if (distance_cm > 400) { // sensor max range is ~400cm
    distance_cm = 400;
  }
  else if (distance_cm < 2) { // sensor min range is ~2cm
    distance_cm = 0;
  }
}


/*************************************************************
*********************** MAIN FUNCTIONS ***********************
**************************************************************/

// SETUP
void setup() {
  // Initialize the TFT display
  tft.init();
  tft.setRotation(0);                     // adjust rotation (0 & 2 portrait | 1 & 3 landscape)
  tft.fillScreen(TFT_BLACK);              // clear screen
  tft.setTextFont(2);                     // set the font
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // set text colour

  tft.println("Initialising...\n");
  delay(1000);
  
  // Set button pin with pullup
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Draw the initial static screen
  drawStaticScreen();
}

// MAIN LOOP
void loop() {
  unsigned long currentMillis = millis(); // get the current millis time

  // State Machine Logic
  switch (currentState) {
    case State::READ_SENSOR: {
        // Read sensor data
        readDistance();
        currentState = State::UPDATE_DISPLAY;
        break;
      }
      
    case State::UPDATE_DISPLAY: {
        // Update display
        updateDistanceDisplay();
        currentState = State::WAIT;
        previousMillis = currentMillis;
        break;
      }
      
    case State::WAIT:
      // Wait for the specified interval before next reading
      if (currentMillis - previousMillis >= updateInterval) {
        currentState = State::READ_SENSOR;
      }
      break;

    default:
      currentState = State::READ_SENSOR;
      break;
  }
}

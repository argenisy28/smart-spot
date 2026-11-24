/*
 * ============================================================
 * Smart Spot - Parking Spot Detection System [PROTOTYPE]
 * ============================================================
 *
 * Controller:
 *   ESP32-S3 DevKitC-1
 *
 * Parking Spot Hardware:
 *   - HC-SR04 Ultrasonic Sensor
 *   - Green LED
 *   - Red LED
 *
 * Display:
 *   - 0.96-inch I2C OLED
 *   - SSD1306
 *   - 128 x 64 pixels
 *
 * Parking Space Available:
 *   Green LED = ON
 *   Red LED   = OFF
 *
 * Parking Space Occupied:
 *   Green LED = OFF
 *   Red LED   = ON
 *
 * Features:
 *   - Sequential ultrasonic measurements
 *   - Reduced ultrasonic sensor interference
 *   - Two consecutive matching readings confirm FREE, OCCUPIED, or ERROR
 *   - ERROR turns both LEDs off when echoes are repeatedly missed
 *   - State-change-only Serial output
 *   - OLED parking availability display
 *
 * ============================================================
 */

// ============================================================
// Library Includes
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// Parking Spot Structure
// ============================================================

enum class ParkingState : uint8_t
{
    FREE,
    OCCUPIED,
    ERROR
};

struct ParkingSpot
{
    int id;

    int trigPin;
    int echoPin;

    int greenLedPin;
    int redLedPin;

    // Previous classified reading, including a missed echo (ERROR).
    ParkingState previousState;

    // Confirmed state used by the LEDs, Serial output, and OLED.
    ParkingState spotStatus;

    // True only after two consecutive classified readings agree.
    bool initialized;

    // False at startup; the first reading
    // establishes previousState without confirming a status.
    bool hasPreviousState;
};

// ============================================================
// Parking Spot Definitions
// ============================================================

ParkingSpot parkingSpots[] =
{
    /*
     * Parking Spot 1
     *
     * TRIG  -> GPIO 5
     * ECHO  -> GPIO 4
     * GREEN -> GPIO 6
     * RED   -> GPIO 7
     */
    {1, 5, 4, 6, 7, ParkingState::FREE, ParkingState::FREE, false, false},

    /*
     * Parking Spot 2
     *
     * TRIG  -> GPIO 9
     * ECHO  -> GPIO 8
     * GREEN -> GPIO 10
     * RED   -> GPIO 11
     */
    {2, 9, 8, 10, 11, ParkingState::FREE, ParkingState::FREE, false, false}
};

// Automatically calculate the number of parking spots.
const int NUMBER_OF_SPOTS =
    sizeof(parkingSpots) / sizeof(parkingSpots[0]);

// ============================================================
// Detection Configuration
// ============================================================

// Toy-car prototype threshold used internally to detect occupancy.
// Distance is not stored or reported; outputs show parking status only.
const float DETECTION_DISTANCE_CM = 20.0;

// Maximum amount of time to wait for an ultrasonic echo.
const unsigned long ECHO_TIMEOUT_US = 30000;

// Delay between different HC-SR04 sensors.
const unsigned long SENSOR_SEQUENCE_DELAY_MS = 60;

// ============================================================
// OLED Display Configuration
// ============================================================

// I2C GPIO pins.
const int OLED_SDA_PIN = 12;
const int OLED_SCL_PIN = 13;

// OLED resolution.
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

// Common SSD1306 I2C address.
const int OLED_ADDRESS = 0x3C;

// Keep two readable rows per page; additional configured spots rotate.
const int SPOTS_PER_PAGE = 2;
const int DISPLAY_PAGE_COUNT =
    (NUMBER_OF_SPOTS + SPOTS_PER_PAGE - 1) / SPOTS_PER_PAGE;
const unsigned long DISPLAY_PAGE_INTERVAL_MS = 3000;
int displayPage = 0;
unsigned long lastDisplayPageChangeMs = 0;

// Display failure must not prevent sensor or LED operation.
// Set true only after the OLED responds and its buffer initializes.
bool oledReady = false;

// Create the OLED display object.
// -1 indicates that no separate reset GPIO is required.
Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// ============================================================
// Function Prototypes
// ============================================================

float measureDistance(ParkingSpot &spot);
void processParkingSpot(ParkingSpot &spot);
void updateSpotStatus(ParkingSpot &spot, ParkingState currentState);
void updateLEDs(ParkingSpot &spot);
void updateDisplay();

// ============================================================
// System Initialization
// ============================================================

void setup()
{
    // Start serial communication.
    Serial.begin(115200);


    // --------------------------------------------------------
    // Initialize I2C and OLED Display
    // --------------------------------------------------------

    if (Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN))
    {
        // Probe the address: display.begin() alone does not reliably
        // detect an absent OLED. An acknowledgement is not a full
        // display health test, but detects a disconnected device.
        Wire.beginTransmission(OLED_ADDRESS);
        if (Wire.endTransmission() == 0)
        {
            // I2C already uses our custom pins; do not begin it again.
            oledReady = display.begin(
                SSD1306_SWITCHCAPVCC, OLED_ADDRESS, true, false
            );
        }
    }

    if (!oledReady)
    {
        // Log once and continue to initialize the parking hardware.
        Serial.println(
            "ERROR: OLED unavailable. Continuing without display."
        );
    }
    else
    {
        // Only access the display buffer after successful initialization.
        display.clearDisplay();

        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);

        display.setCursor(25, 25);
        display.println("Initializing...");

        display.display();
    }

    // --------------------------------------------------------
    // Initialize Parking Spot Hardware
    // --------------------------------------------------------

    for (int i = 0; i < NUMBER_OF_SPOTS; i++)
    {
        // Configure HC-SR04 GPIOs.
        pinMode(parkingSpots[i].trigPin, OUTPUT);
        pinMode(parkingSpots[i].echoPin, INPUT);

        // Configure LED GPIOs.
        pinMode(parkingSpots[i].greenLedPin, OUTPUT);
        pinMode(parkingSpots[i].redLedPin, OUTPUT);

        // Ensure TRIG begins LOW.
        digitalWrite(parkingSpots[i].trigPin, LOW);

        // No state is confirmed at startup. Keep both LEDs off
        // until two consecutive classified readings agree.
        digitalWrite(parkingSpots[i].greenLedPin, LOW);
        digitalWrite(parkingSpots[i].redLedPin, LOW);
    }


    // --------------------------------------------------------
    // Serial Startup Information
    // --------------------------------------------------------

    Serial.println();
    Serial.println("====================================");
    Serial.println("Smart Parking Detection System");
    Serial.println("System Started");

    // Show CHECKING until each spot has a confirmed status.
    updateDisplay();
    lastDisplayPageChangeMs = millis();
    Serial.println("====================================");
}


// ============================================================
// Main Program Loop
// ============================================================

void loop()
{
    /*
     * Process each parking spot sequentially.
     *
     * Only one HC-SR04 is triggered at a time to reduce
     * ultrasonic interference between nearby sensors.
     */
    for (int i = 0; i < NUMBER_OF_SPOTS; i++)
    {
        processParkingSpot(parkingSpots[i]);

        // Allow ultrasonic signals from the previous
        // measurement to dissipate.
        delay(SENSOR_SEQUENCE_DELAY_MS);

        // Rotate even when occupancy is unchanged, without extra delays.
        const unsigned long now = millis();
        if (oledReady && DISPLAY_PAGE_COUNT > 1 &&
            now - lastDisplayPageChangeMs >= DISPLAY_PAGE_INTERVAL_MS)
        {
            displayPage = (displayPage + 1) % DISPLAY_PAGE_COUNT;
            lastDisplayPageChangeMs = now;
            updateDisplay();
        }
    }
}


// ============================================================
// Parking Spot Processing
// ============================================================

void processParkingSpot(ParkingSpot &spot)
{
    float distance = measureDistance(spot);

    // Classify this measurement without changing the confirmed status.
    // currentState is temporary; previousState and spotStatus persist
    // in each spot's structure between normal passes through loop().
    // A missed echo participates in the same two-reading confirmation.
    const ParkingState currentState = (distance < 0)
        ? ParkingState::ERROR
        : ((distance <= DETECTION_DISTANCE_CM)
            ? ParkingState::OCCUPIED : ParkingState::FREE);

    if (!spot.hasPreviousState)
    {
        // First reading at startup:
        // establish a baseline and wait for the next normal scan.
        spot.previousState = currentState;
        spot.hasPreviousState = true;
        return;
    }

    if (spot.previousState != currentState)
    {
        // Readings disagree: remember the new reading, but keep the
        // confirmed spotStatus and its outputs unchanged.
        spot.previousState = currentState;
    }
    else if (!spot.initialized || spot.spotStatus != currentState)
    {
        // Readings agree: publish the first confirmed status or a change.
        // No update is needed when the confirmed status already matches.
        updateSpotStatus(spot, currentState);
    }
}


// ============================================================
// Confirmed Parking Spot Status Update
// ============================================================

void updateSpotStatus(ParkingSpot &spot, ParkingState currentState)
{
    // Called only after two consecutive classified readings agree.
    spot.spotStatus = currentState;
    spot.initialized = true;

    updateLEDs(spot);

    Serial.print("Parking Spot ");
    Serial.print(spot.id);
    Serial.print(": ");

    if (spot.spotStatus == ParkingState::ERROR)
    {
        Serial.println("ERROR");
    }
    else if (spot.spotStatus == ParkingState::OCCUPIED)
    {
        Serial.println("OCCUPIED");
    }
    else
    {
        Serial.println("FREE");
    }

    // Refresh only for initial confirmation or a confirmed status change.
    updateDisplay();
}

// ============================================================
// Ultrasonic Distance Measurement
// ============================================================

float measureDistance(ParkingSpot &spot)
{
    // Ensure TRIG begins LOW.
    digitalWrite(spot.trigPin, LOW);
    delayMicroseconds(2);


    // --------------------------------------------------------
    // Generate HC-SR04 Trigger Pulse
    // --------------------------------------------------------

    digitalWrite(spot.trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(spot.trigPin, LOW);


    // --------------------------------------------------------
    // Measure ECHO Pulse
    // --------------------------------------------------------

    unsigned long echoDuration = pulseIn(
        spot.echoPin,
        HIGH,
        ECHO_TIMEOUT_US
    );


    // --------------------------------------------------------
    // Validate ECHO
    // --------------------------------------------------------

    if (echoDuration == 0)
    {
        return -1.0;
    }


    // --------------------------------------------------------
    // Calculate Distance
    // --------------------------------------------------------

    /*
     * Speed of sound:
     * approximately 0.0343 cm/us
     *
     * The ultrasonic signal travels:
     *
     * Sensor -> Object -> Sensor
     *
     * Therefore, the total distance is divided by two.
     */
    float distance =
        (echoDuration * 0.0343) / 2.0;

    return distance;
}


// ============================================================
// Parking Spot LED Control
// ============================================================

void updateLEDs(ParkingSpot &spot)
{
    if (spot.spotStatus == ParkingState::ERROR)
    {
        // Occupancy is unavailable while echoes are missing.
        digitalWrite(spot.greenLedPin, LOW);
        digitalWrite(spot.redLedPin, LOW);
    }
    else if (spot.spotStatus == ParkingState::OCCUPIED)
    {
        // Parking space occupied.
        digitalWrite(spot.greenLedPin, LOW);
        digitalWrite(spot.redLedPin, HIGH);
    }
    else
    {
        // Parking space available.
        digitalWrite(spot.greenLedPin, HIGH);
        digitalWrite(spot.redLedPin, LOW);
    }
}


// ============================================================
// OLED Parking Availability Display
// ============================================================

void updateDisplay()
{
    // All callers can request a refresh safely when the OLED is disabled.
    // Sensor confirmation, LEDs, and Serial output remain independent.
    if (!oledReady)
    {
        return;
    }

    // Clear the previous frame.
    display.clearDisplay();

    // Configure standard text appearance.
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);


    // --------------------------------------------------------
    // Header
    // --------------------------------------------------------

    display.setCursor(4, 2);
    display.println("Parking Availability");

    // Horizontal separator.
    display.drawLine(
        0,
        13,
        SCREEN_WIDTH - 1,
        13,
        SSD1306_WHITE
    );


    // --------------------------------------------------------
    // Configured Parking Spots on the Current Page
    // --------------------------------------------------------

    const int firstSpot = displayPage * SPOTS_PER_PAGE;
    for (int i = firstSpot;
         i < NUMBER_OF_SPOTS && i < firstSpot + SPOTS_PER_PAGE;
         i++)
    {
        const ParkingSpot &spot = parkingSpots[i];
        display.setCursor(5, 24 + (i - firstSpot) * 18);
        display.print("Spot ");
        display.print(spot.id);
        display.print(": ");

        if (!spot.initialized)
        {
            display.print("CHECKING");
        }
        else if (spot.spotStatus == ParkingState::ERROR)
        {
            display.print("ERROR");
        }
        else if (spot.spotStatus == ParkingState::OCCUPIED)
        {
            display.print("OCCUPIED");
        }
        else
        {
            display.print("FREE");
        }
    }

    if (DISPLAY_PAGE_COUNT > 1)
    {
        display.setCursor(5, 56);
        display.print("Page ");
        display.print(displayPage + 1);
        display.print("/");
        display.print(DISPLAY_PAGE_COUNT);
    }


    // --------------------------------------------------------
    // Send Frame to OLED
    // --------------------------------------------------------

    display.display();
}

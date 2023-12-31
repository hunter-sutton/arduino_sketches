/**
 *
 * Adapted from Adafruit BLE Code
 *
**/

#include <bluefruit.h>
#include <ble_gap.h>
#include <math.h>

// Uart over BLE service
BLEUart bleuart;

// Function prototypes for packetparser.cpp
uint8_t readPacket(BLEUart *ble_uart, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t *data, const uint32_t numBytes);

// Packet buffer
extern uint8_t packetbuffer[];

// Pins
const int M1_1 = 7;
const int M1_2 = 11;
const int M2_1 = 31;
const int M2_2 = 30;

// Current Device Mode
enum DeviceMode {
    DPAD_CONTROL = 0,
    GRADIENT_ASCENT = 1
};

DeviceMode device_mode = DPAD_CONTROL;

void stopMotors() {
    digitalWrite(M1_1, LOW);
    digitalWrite(M1_2, LOW);
    digitalWrite(M2_1, LOW);
    digitalWrite(M2_2, LOW);
}

void goForward() {
    digitalWrite(M1_1, HIGH);
    digitalWrite(M1_2, LOW);
    digitalWrite(M2_1, HIGH);
    digitalWrite(M2_2, LOW);
}

void goBackward() {
    digitalWrite(M1_1, LOW);
    digitalWrite(M1_2, HIGH);
    digitalWrite(M2_1, LOW);
    digitalWrite(M2_2, HIGH);
}

void turnLeft() {
    digitalWrite(M1_1, LOW);
    digitalWrite(M1_2, HIGH);
    digitalWrite(M2_1, HIGH);
    digitalWrite(M2_2, LOW);
}

void turnRight() {
    digitalWrite(M1_1, HIGH);
    digitalWrite(M1_2, LOW);
    digitalWrite(M2_1, LOW);
    digitalWrite(M2_2, HIGH);
}

void setup(void) {
    Serial.begin(115200);
    while (!Serial) delay(10);  // for nrf52840 with native usb

    Serial.println(F("Robo Car"));
    Serial.println(F("--------"));

    Bluefruit.begin();
    Bluefruit.setTxPower(4);  // Check bluefruit.h for supported values

    // Configure and start the BLE Uart service
    bleuart.begin();

    // Set up and start advertising
    startAdv();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(M1_1, OUTPUT);
    pinMode(M1_2, OUTPUT);
    pinMode(M2_1, OUTPUT);
    pinMode(M2_2, OUTPUT);
    stopMotors();
}

void startAdv(void) {
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include the BLE UART (AKA 'NUS') 128-bit UUID
    Bluefruit.Advertising.addService(bleuart);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
    * - Enable auto advertising if disconnected
    * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
    * - Timeout for fast mode is 30 seconds
    * - Start(timeout) with timeout = 0 will advertise forever (until connected)
    */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);  // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);    // number of seconds in fast mode
    Bluefruit.Advertising.start(0);              // 0 = Don't stop advertising after n seconds
}

// constantly wait for a new packet
void loop(void) {
    // Wait for new data to arrive
    uint8_t len = readPacket(&bleuart, 500);
    if (len == 0) return;

    // Got a packet!
    printHex(packetbuffer, len);

    // Buttons
    if (packetbuffer[1] == 'B') {
        uint8_t buttnum = packetbuffer[2] - '0';
        boolean pressed = packetbuffer[3] - '0';
        Serial.print("Button ");
        Serial.print(buttnum);

        if (pressed && buttnum == 1) {
            device_mode = (device_mode == DPAD_CONTROL) ? GRADIENT_ASCENT : DPAD_CONTROL;
            String mode_name = (device_mode == DPAD_CONTROL) ? "D-Pad Control" : "Gradient Ascent";
            Serial.println("Switched to " + mode_name + "mode");

            // reset motors
            stopMotors();

            return;  // skip the button processing steps
        }

        if (device_mode == DPAD_CONTROL) {
            if (pressed) {
                Serial.println(" pressed");

                if (buttnum == 5) { // forward

                    Serial.println("Forward");
                    goForward();
                    digitalWrite(LED_BUILTIN, HIGH);

                } else if (buttnum == 7) { // left

                    Serial.println("Left");    
                    turnLeft();
                    digitalWrite(LED_BUILTIN, HIGH);

                } else if (buttnum == 8) { // right

                    Serial.println("Right");    
                    turnRight();
                    digitalWrite(LED_BUILTIN, HIGH);

                } else if (buttnum == 6) { // reverse

                    Serial.println("Reverse");    
                    goBackward();
                    digitalWrite(LED_BUILTIN, HIGH);

                }
            } else { // reset (undo action of button press)

                Serial.println(" released");
                stopMotors();
                digitalWrite(LED_BUILTIN, LOW);

            }
        } else if (device_mode == GRADIENT_ASCENT) { // NOT WORKING YET
            // gradient ascent
            Serial.println("Gradient ascent mode");

            bool connected = Bluefruit.connected();

            if (connected) {
                Serial.println("Connected");

                uint16_t connHandle = Bluefruit.connHandle();

                int8_t rssi = Bluefruit.Connection(connHandle)->getRssi();
                Serial.print("Rssi: ");
                Serial.println(rssi);
            }

            Serial.println("Got here");
        }
    }
}

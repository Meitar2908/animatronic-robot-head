/*
 * Animatronic Robot Head
 * Remote Controller
 *
 * Reads joystick and antenna control inputs and sends commands
 * wirelessly to the robot head using ESP-NOW.
 */

#include <esp_now.h>
#include <WiFi.h>

#define EYE_X_PIN   32
#define EYE_Y_PIN   33
#define TEETH_Y_PIN 35
#define ANTENNA_PIN 39 

// Variables for Interrupt-based CCPM reading
volatile unsigned long pulseStart = 0;
volatile uint16_t sharedPulseWidth = 1500;

// Interrupt Service Routine (ISR)
void IRAM_ATTR handleAntennaPulse() {
  if (digitalRead(ANTENNA_PIN) == HIGH) {
    pulseStart = micros();
  } else {
    uint16_t duration = micros() - pulseStart;
    if (duration >= 900 && duration <= 2100) {
      sharedPulseWidth = duration;
    }
  }
}

float smoothTeeth = 0;

typedef struct {
  uint16_t eyeX;
  uint16_t eyeY;
  uint16_t teethY;
  uint16_t antenna;
} ControlPacket;

ControlPacket packet;
uint8_t receiverMac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

void setup() {
  Serial.begin(115200);
  pinMode(ANTENNA_PIN, INPUT);
  
  // Attach interrupt to the Antenna Pin
  attachInterrupt(digitalPinToInterrupt(ANTENNA_PIN), handleAntennaPulse, CHANGE);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  packet.eyeX = analogRead(EYE_X_PIN);
  packet.eyeY = analogRead(EYE_Y_PIN);

  // Teeth smoothing logic [cite: 11]
  int rawTeeth = analogRead(TEETH_Y_PIN);
  smoothTeeth = smoothTeeth * 0.7f + rawTeeth * 0.3f;
  if (smoothTeeth > 2048) smoothTeeth = 2048; // [cite: 12]
  packet.teethY = (uint16_t)smoothTeeth;

  // The interrupt updates this value automatically in the background
  packet.antenna = sharedPulseWidth;

  esp_now_send(receiverMac, (uint8_t*)&packet, sizeof(packet));
  delay(20); 
}

/*
 * Animatronic Robot Head
 * Head Controller
 *
 * Receives wireless control commands from the handheld ESP32 remote
 * using ESP-NOW and controls the head's servo mechanisms through a
 * PCA9685 PWM controller.
 */

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ESP32Servo.h>


// PCA9685
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Servo channels
#define UPPER_TEETH_1 0
#define UPPER_TEETH_2 1
#define LOWER_TEETH_1 2
#define LOWER_TEETH_2 3

#define EYE_BALL_SERVO 4
#define EYE_LID_SERVO  5

#define ANTENNA_SERVO  6

// Servo ranges
#define EYEBALL_SERVO_MIN 100
#define EYEBALL_SERVO_MAX 500
#define EYELID_SERVO_MIN 200
#define EYELID_SERVO_MAX 500

#define TEETH_MIN 200
#define TEETH_MAX 500

// Antenna PWM
#define ANT_OFF   100
#define ANT_ON    500

typedef struct {
  uint16_t eyeX;
  uint16_t eyeY;
  uint16_t teethY;
  uint16_t antenna; // 16-bit CCPM value
} ControlPacket;

ControlPacket packet;   // GLOBAL PACKET


// Map ESP32 ADC → PCA9685
int map12bit(int x, int outMin, int outMax) {
  return map(x, 0, 4095, outMin, outMax);
}

// ===== ESP-NOW RECEIVE CALLBACK =====
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(packet)) {
    memcpy(&packet, data, sizeof(packet));
  }

  Serial.print("eyeX=");
  Serial.print(packet.eyeX);
  Serial.print(" eyeY=");
  Serial.print(packet.eyeY);
  Serial.print(" teethY=");
  Serial.print(packet.teethY);
  Serial.print(" antenna=");
  Serial.println(packet.antenna);
}

void setup() {
  Serial.begin(115200);

  // ESP-NOW init
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  // PCA9685 Init
  Wire.begin(21, 22);    // ESP32 defaults
  pwm.begin();
  pwm.setPWMFreq(50);    // 50 Hz servos

}

void loop() {

  // === EYES ===
  int eyeBallPWM = map12bit(packet.eyeX, EYEBALL_SERVO_MIN, EYEBALL_SERVO_MAX);
  int eyeLidPWM  = map12bit(packet.eyeY, EYELID_SERVO_MIN, EYELID_SERVO_MAX);

  pwm.setPWM(EYE_BALL_SERVO, 0, eyeBallPWM);
  pwm.setPWM(EYE_LID_SERVO,  0, eyeLidPWM);

  // === TEETH (INVERTED MECHANISM) ===
  int upperTeethPWM = map(packet.teethY, 0,4095, TEETH_MIN, TEETH_MAX);
  int lowerTeethPWM = map(packet.teethY, 0,4095, TEETH_MAX, TEETH_MIN);

  pwm.setPWM(UPPER_TEETH_1, 0, lowerTeethPWM);
  pwm.setPWM(UPPER_TEETH_2, 0, upperTeethPWM);
  pwm.setPWM(LOWER_TEETH_1, 0, upperTeethPWM);
  pwm.setPWM(LOWER_TEETH_2, 0, lowerTeethPWM);

  // === ANTENNA USING PCA9685 CHANNEL 6 ===
  uint16_t pulse = packet.antenna;

  // Validate CCPM pulse
  if (pulse < 1000 || pulse > 2200) {
      pulse = 1500; // safe stop
  }

  // Convert pulse width (1000–2000 µs) → PCA9685 units
  int antPwm = map(pulse, 1000, 2000, ANT_OFF, ANT_ON);

  // Drive continuous-rotation servo via PCA9685
  pwm.setPWM(ANTENNA_SERVO, 0, antPwm);

  delay(20); // 50 Hz
}

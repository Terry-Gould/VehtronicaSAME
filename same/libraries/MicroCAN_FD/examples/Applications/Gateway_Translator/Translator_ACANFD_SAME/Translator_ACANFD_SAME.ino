// One-way CAN gateway translator using raw ACANFD_SAME frames.
//
// This worked example receives selected source messages on CAN0 and transmits
// translated destination messages on CAN1. It keeps all message and signal
// handling in this sketch, so the byte packing is visible.
//
// The example data translates selected 350Z engine/dash values into RX-8
// cluster messages. The same structure can be reused for other vehicles,
// modules, bench rigs or industrial CAN networks.

#define CAN0_MESSAGE_RAM_SIZE (4352)
#define CAN1_MESSAGE_RAM_SIZE (4352)

#include <Arduino.h>
#include <ACANFD_SAME.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

static uint16_t gEngineRpm = 0;
static int16_t gCoolantTempC = 0;
static bool gEngineLight = false;
static bool gCruiseLight = false;
static bool gSetLight = false;

static uint32_t gTime5msGroup = 0;
static uint32_t gTime12msGroup = 0;
static uint32_t gTime35msGroup = 0;
static uint32_t gTime75msGroup = 0;

static uint16_t rx8ScaledRpm(uint16_t rpm) {
  return (uint32_t)rpm * 382U / 100U;
}

static uint8_t rx8CoolantByte(int16_t tempC) {
  return (uint8_t)(tempC + 40);
}

static uint8_t rx8GaugeCoolantByte(int16_t tempC) {
  if (tempC < -12 || tempC > 139) return 0xFF;
  return rx8CoolantByte(tempC);
}

static bool sendCan1Frame(uint32_t id, uint8_t len, const uint8_t* data) {
  CANFDMessage frame;
  frame.id = id;
  frame.ext = false;
  frame.type = CANFDMessage::CAN_DATA;
  frame.len = len;
  for (uint8_t i = 0; i < len; i++) {
    frame.data[i] = data[i];
  }
  return can1.sendFrame(frame) == 0;
}

static void send201() {
  const uint16_t rpm = rx8ScaledRpm(gEngineRpm);
  const uint16_t vehicleSpeed = 10000; // 0 km/h encoded as (km/h * 100) + 10000.
  const uint8_t data[8] = {
    (uint8_t)((rpm >> 8) & 0xFF),
    (uint8_t)(rpm & 0xFF),
    0xFF,
    0xFF,
    (uint8_t)((vehicleSpeed >> 8) & 0xFF),
    (uint8_t)(vehicleSpeed & 0xFF),
    0xFF,
    0xFF
  };
  sendCan1Frame(0x201, 8, data);
}

static void send203() {
  const uint8_t data[7] = { 19, 19, 19, 24, 0xAF, 2, 19 };
  sendCan1Frame(0x203, 7, data);
}

static void send215() {
  const uint8_t data[8] = { 0x02, 45, 0x02, 45, 0x02, 42, 0x00, 0x80 };
  sendCan1Frame(0x215, 8, data);
}

static void send231() {
  const uint8_t data[5] = { 0x0F, 0x00, 0xFF, 0xFF, 0x00 }; // Neutral/not-in-gear default.
  sendCan1Frame(0x231, 5, data);
}

static void send240() {
  const uint8_t data[8] = {
    4,
    0x00,
    191,
    rx8CoolantByte(gCoolantTempC),
    128,
    0x87,
    0x00,
    0x00
  };
  sendCan1Frame(0x240, 8, data);
}

static void send250() {
  const uint8_t data[8] = { 0x00, 0x00, 128, 0x00, 0x00, 0x00, 0x00, 0x04 };
  sendCan1Frame(0x250, 8, data);
}

static void send420() {
  const uint8_t rx8EngineLightByte = gEngineLight ? 0x40 : 0x00;
  const uint8_t data[7] = {
    rx8GaugeCoolantByte(gCoolantTempC),
    0x00,
    0x00,
    0x00,
    0x01,
    rx8EngineLightByte,
    0x00
  };
  sendCan1Frame(0x420, 7, data);
}

static void send620() {
  const uint8_t data[7] = { 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x04 };
  sendCan1Frame(0x620, 7, data);
}

static void send630() {
  const uint8_t data[8] = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A, 0x6A };
  sendCan1Frame(0x630, 8, data);
}

static void send650() {
  uint8_t cruiseByte = 0x00;
  if (gCruiseLight) cruiseByte |= 0x40; // RX-8 Main = 350Z Cruise.
  if (gSetLight) cruiseByte |= 0x80;    // RX-8 CruiseMain = 350Z Set.
  sendCan1Frame(0x650, 1, &cruiseByte);
}

static void decode350ZEngine23D(const CANFDMessage& frame) {
  if (frame.len < 8) return;
  const uint16_t rawRpm = ((uint16_t)frame.data[4] << 8) | frame.data[3];
  gEngineRpm = (uint16_t)(((uint32_t)rawRpm * 32U + 5U) / 10U);
  gCoolantTempC = (int16_t)frame.data[7] - 50;
}

static void decode350ZDashLights(const CANFDMessage& frame) {
  if (frame.len < 4) return;

  // ID 0x233 byte 3 stores these dash lights as individual bits.
  gSetLight = (frame.data[3] & (1U << 0)) != 0;
  gCruiseLight = (frame.data[3] & (1U << 1)) != 0;
  gEngineLight = (frame.data[3] & (1U << 3)) != 0;
}

static void receive350ZFrames() {
  CANFDMessage frame;

  while (can0.receiveFD0(frame) || can0.receiveFD1(frame)) {
    switch (frame.id) {
      case 0x23D: decode350ZEngine23D(frame); break;
      case 0x233: decode350ZDashLights(frame); break;
      default: break;
    }
  }
}

static void transmitRx8Frames() {
  const uint32_t now = millis();

  if ((uint32_t)(now - gTime5msGroup) >= 5) {
    gTime5msGroup = now;
    send215();
  }

  if ((uint32_t)(now - gTime12msGroup) >= 12) {
    gTime12msGroup = now;
    send231();
    send203();
    send201();
  }

  if ((uint32_t)(now - gTime35msGroup) >= 35) {
    gTime35msGroup = now;
    send650();
  }

  if ((uint32_t)(now - gTime75msGroup) >= 75) {
    gTime75msGroup = now;
    send630();
    send620();
    send420();
    send250();
    send240();
  }
}

static void beginTransceivers() {
#if defined(PIN_CAN0_STANDBY)
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);
#endif
#if defined(PIN_CAN1_STANDBY)
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, LOW);
#endif
}

void setup() {
  beginTransceivers();

  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }
  Serial.println("CAN0 to CAN1 raw ACANFD_SAME translator");

  ACANFD_SAME_Settings can0Settings(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  ACANFD_SAME_Settings can1Settings(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  can0Settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  can1Settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  can0Settings.mHardwareRxFIFO0Size = 64;
  can0Settings.mDriverReceiveFIFO0Size = 64;
  can0Settings.mHardwareRxFIFO1Size = 8;
  can0Settings.mDriverReceiveFIFO1Size = 8;
  can0Settings.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::REJECT;
  can0Settings.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::REJECT;

  ACANFD_SAME::StandardFilters can0Filters;
  can0Filters.addClassic(0x23D, 0x7FF, ACANFD_SAME_FilterAction::FIFO0);
  can0Filters.addClassic(0x233, 0x7FF, ACANFD_SAME_FilterAction::FIFO0);

  const uint32_t can0Error = can0.beginFD(can0Settings, can0Filters);
  const uint32_t can1Error = can1.beginFD(can1Settings);

  Serial.print("can0 begin status 0x");
  Serial.println(can0Error, HEX);
  Serial.print("can1 begin status 0x");
  Serial.println(can1Error, HEX);
}

void loop() {
  receive350ZFrames();
  transmitRx8Frames();
}


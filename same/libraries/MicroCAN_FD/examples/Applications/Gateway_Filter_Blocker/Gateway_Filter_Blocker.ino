// Bidirectional CAN gateway with one edited signal.
//
// CAN1 -> CAN0:
//   All frames are forwarded unchanged.
//
// CAN0 -> CAN1:
//   All frames are forwarded, but standard ID 0x201 has its vehicle speed
//   signal capped at 80 km/h before it is sent to CAN1.
//
// Vehicle speed is ID 0x201, bytes 4 and 5, big-endian, with:
//   physical km/h = (raw * 0.01) - 100
//   raw = (physical km/h + 100) / 0.01
//
// Therefore 80 km/h is raw 18000, or 0x4650.

#define CAN0_MESSAGE_RAM_SIZE (4352)
#define CAN1_MESSAGE_RAM_SIZE (4352)

#include <Arduino.h>
#include <ACANFD_SAME.h>

// Required only when TinyUSB is selected, so the external TinyUSB library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

static const uint32_t CAN_BIT_RATE = 500UL * 1000UL;
static const uint32_t SPEED_MESSAGE_ID = 0x201;
static const uint16_t MAX_SPEED_RAW = 18000; // 80 km/h encoded as (80 + 100) / 0.01.

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

static bool isSpeedFrame(const CANFDMessage& frame) {
  return !frame.ext &&
         frame.id == SPEED_MESSAGE_ID &&
         frame.type != CANFDMessage::CAN_REMOTE &&
         frame.len >= 6;
}

static void capVehicleSpeed(CANFDMessage& frame) {
  uint16_t speedRaw = ((uint16_t)frame.data[4] << 8) | frame.data[5];

  if (speedRaw > MAX_SPEED_RAW) {
    speedRaw = MAX_SPEED_RAW;
    frame.data[4] = (uint8_t)(speedRaw >> 8);
    frame.data[5] = (uint8_t)(speedRaw & 0xFF);
  }
}


static void forwardCAN1ToCAN0() {
  CANFDMessage frame;

  while (can1.receiveFD0(frame)) {
    can0.sendFrame(frame);
  }

  while (can1.receiveFD1(frame)) {
    can0.sendFrame(frame);
  }
}

static void forwardCAN0ToCAN1Unchanged() {
  CANFDMessage frame;

  while (can0.receiveFD1(frame)) {
    can1.sendFrame(frame);
  }
}

static void forwardCAN0ToCAN1WithSpeedCap() {
  CANFDMessage frame;

  while (can0.receiveFD0(frame)) {
    if (isSpeedFrame(frame)) {
      capVehicleSpeed(frame);
    }

    can1.sendFrame(frame);
  }
}

void setup() {
  beginTransceivers();

  ACANFD_SAME_Settings can0Settings(ACANFD_SAME_Settings::CLOCK_48MHz, CAN_BIT_RATE, DataBitRateFactor::x1);
  can0Settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  can0Settings.mHardwareRxFIFO0Size = 64;
  can0Settings.mDriverReceiveFIFO0Size = 64;
  can0Settings.mHardwareRxFIFO1Size = 64;
  can0Settings.mDriverReceiveFIFO1Size = 64;
  can0Settings.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::FIFO1;
  can0Settings.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::FIFO1;

  ACANFD_SAME::StandardFilters can0Filters;
  can0Filters.addClassic(SPEED_MESSAGE_ID, 0x7FF, ACANFD_SAME_FilterAction::FIFO0);

  ACANFD_SAME_Settings can1Settings(ACANFD_SAME_Settings::CLOCK_48MHz, CAN_BIT_RATE, DataBitRateFactor::x1);
  can1Settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  can1Settings.mHardwareRxFIFO0Size = 64;
  can1Settings.mDriverReceiveFIFO0Size = 64;
  can1Settings.mHardwareRxFIFO1Size = 64;
  can1Settings.mDriverReceiveFIFO1Size = 64;
  can1Settings.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::FIFO0;
  can1Settings.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::FIFO0;

  can0.beginFD(can0Settings, can0Filters);
  can1.beginFD(can1Settings);
}

void loop() {
  forwardCAN1ToCAN0();
  forwardCAN0ToCAN1Unchanged();
  forwardCAN0ToCAN1WithSpeedCap();
}
// One-way CAN gateway translator using CANMessageSignal definitions.
//
// This worked example receives selected source messages on CAN0 and transmits
// translated destination messages on CAN1. The message and signal definitions
// are kept in separate header files so the main sketch can focus on the
// translation logic.
//
// The example data translates selected 350Z engine/dash values into RX-8
// cluster messages. The same structure can be reused for other vehicles,
// modules, bench rigs or industrial CAN networks.

#define CAN0_MESSAGE_RAM_SIZE (4352)
#define CAN1_MESSAGE_RAM_SIZE (4352)

#include <Arduino.h>
#include <ACANFD_SAME.h>
#include <CANMessageSignal.h>
#include "350Z_Signals.h"
#include "RX8_Signals.h"

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

using namespace CANMessageSignal;

CanChannel rx8Can1(can1); // We only do this for CAN1 because the sketch only sends on CAN1. CAN0 is read directly from ACANFD_SAME and passed to the decoder functions.

static void apply350ZToRx8Signals() {
  rpm.setSignalValue(z350Rpm.signalValue());
  speedo.setSignalValue(0);
  rawECT.setSignalValue(z350CoolantTemp.signalValue());
  gaugeECT.setSignalValue(z350CoolantTemp.signalValue());

  oilGauge.setSignalValue("Ok");
  celOn.setSignalValue(z350EngineLight.signalValue());
  celFlashing.setSignalValue(0);
  coolantLight.setSignalValue(0);
  batteryLight.setSignalValue(0);
  oilLight.setSignalValue(0);

  cruiseLight.setSignalValue(z350CruiseLight.signalValue());   // RX-8 Main = 350Z Cruise.
  cruiseMainLight.setSignalValue(z350SetLight.signalValue());  // RX-8 CruiseMain = 350Z Set.

  steeringLight.setSignalValue(0);
  absLight.setSignalValue(0);
  brakeWarnLight.setSignalValue(0);
  DscOffLightn.setSignalValue(1);
  tcLight.setSignalValue(0);
  tcFlashingLight.setSignalValue(0);
  dscTcEnable.setSignalValue(1);
}

static void decode350ZFrame(const CANFDMessage& frame) {
  bool decoded = false;
  decoded |= z350Engine23D.decode(frame);
  decoded |= z350DashLights233.decode(frame);

  if (decoded) {
    apply350ZToRx8Signals();
  }
}

static void receive350ZFrames() {
  CANFDMessage frame;

  while (can0.receiveFD0(frame) || can0.receiveFD1(frame)) {
    decode350ZFrame(frame);
  }
}

static void transmitRx8Frames() {
  rx8Can1.sendIfDue(engine215, 5);
  rx8Can1.sendIfDue(engine231, 12);
  rx8Can1.sendIfDue(engine203, 12);
  rx8Can1.sendIfDue(engine201, 12);
  rx8Can1.sendIfDue(engine650, 35);
  rx8Can1.sendIfDue(engine630, 75);
  rx8Can1.sendIfDue(engine620, 75);
  rx8Can1.sendIfDue(engine420, 75);
  rx8Can1.sendIfDue(engine250, 75);
  rx8Can1.sendIfDue(engine240, 75);
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
  Serial.println("CAN0 to CAN1 CANMessageSignal translator");

  bool signalsOk = true;
  signalsOk &= set350ZSignals();
  signalsOk &= setDeviceSignals();
  apply350ZToRx8Signals();

  Serial.print("signal setup ");
  Serial.println(signalsOk ? "ok" : "failed");

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

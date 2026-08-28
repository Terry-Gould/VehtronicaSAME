/*
  MicroCAN-FD CANMessageSignal Template

  This is a minimal starting point for using the CANMessageSignal to be able to define CAN Bus signals and messages
  similar to the DBC format and transmit them on the CAN Bus.

  The idea is that you define one or more signals per message. You start by building your CAN bus message,
  which consists of the ID, friendly name, DLC, default fill value, frame format, and other metadata.
  You then create one or more signals, which consist of items such as signal name, start bit, bit length,
  scaling, offset, and unit. Each signal is then attached to a message.

  Install the library from Library Manager before compiling this sketch.

  If not available from the library manager, please install it manually by downloading it from here:
  https://github.com/Terry-Gould/CANMessageSignal

  CANMessageSignal provides message/signal helper classes. It does not replace the CAN driver,
  so practical MicroCAN-FD projects will normally always require ACANFD_SAME.

  For more detailed explanations, see the examples included in the library.

  If you upload this to MicroCAN-FD and connect CAN0 to a Mazda RX-8 S1 instrument cluster, the RPM gauge will show 3000 RPM.
*/
#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (0)

#include <ACANFD_SAME.h>
#include <CANMessageSignal.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

using namespace CANMessageSignal;

CanMessage engine201({ .name = "Engine201",
                       .idType = STANDARD,
                       .id = 0x201,
                       .dlc = 8,
                       .defaultFill = 0xFF,
                       .comment = "From ECM, contains RPM, Vehicle Speed and APP" });

CanByteSignal rpm({ .name = "EngineRPM",
                    .dataType = UNSIGNED,
                    .startByte = 0,
                    .byteLength = 2,
                    .endianness = BIG_ENDIAN,
                    .factor = 0.26,
                    .offset = 0.0,
                    .unit = "rpm",
                    .comment = "Engine speed",
                    .min = 0,
                    .max = 10000,
                    .defaultValue = 0,
                    .overrideCapable = true });

CanChannel microCan0(can0);

void setup() {
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);

  engine201.addSignal(rpm);

  ACANFD_SAME_Settings settings(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  can0.beginFD(settings);

  microCan0.setBusType(CAN_CLASSIC);
}

void loop() {
  rpm.setSignalValue(3000);
  microCan0.sendIfDue(engine201, 50);
}
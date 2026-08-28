/*
  MicroCAN-FD CAN Signal Studio Interface Template

  This is the bare minimum required to get the MicroCAN-FD to connect with 
  and appear in the CAN Signal Studio web app.

  Upload this to the MicroCAN-FD, connect it via USB, open the following link,
  click 'Add Device' then 'Connect' and follow the web browser prompts. 
  You will then see a signal appear in the CAN Bus Control tab and
  a parameter appear in the Device Control tab.  

  https://cansignalstudio.terrygouldengineer.com/

  Install the library from Library Manager before compiling this sketch.

  If not available from the library manager, please install it manually by downloading it from here:
  https://github.com/Terry-Gould/CANSignalStudioInterface

  CANSignalStudioInterface provides the web interface. CANMessageSignal provides message/signal helper classes.
  It does not replace the CAN driver, so you still need to include ACANFD_SAME.

  This is the bare minimum, for more detail and proper documentation,
  see the library examples and the README in the repo linked above.
*/
#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (0)

#include <ACANFD_SAME.h>
#include <CANMessageSignal.h>
#include <CANSignalStudioInterface.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

using namespace CANMessageSignal;

CANSignalStudioInterface webapp;
WebAppChannel microCan0(can0, webapp, "MicroCAN CAN0");

EnumMap vinStatusMap({ { 0, "USB" },
                       { 1, "VIN" } });

DeviceParameter vinStatus({ .name = "VIN Status",
                            .access = READ_ONLY,
                            .dataType = UNSIGNED,
                            .unit = "",
                            .group = "Power",
                            .min = 0.0,
                            .max = 1.0,
                            .defaultValue = 0.0,
                            .enumMap = &vinStatusMap });

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

void setup() {
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);
  pinMode(VIN_STATUS, INPUT);

  Serial.begin(115200);
  webapp.attachSerial(Serial);
  webapp.setDeviceInfo("MicroCAN-FD WebApp Library", "X.X.X", "dev", "00001");

  microCan0.setBusType(CAN_CLASSIC);

  engine201.addSignal(rpm);
  webapp.registerMessage(microCan0, engine201);
  webapp.registerParameter(vinStatus);

  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  settings0.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  (void)webapp.beginChannel(microCan0, settings0);
}

void loop() {
  webapp.service();
  vinStatus.setValue(digitalRead(VIN_STATUS) == HIGH ? 1.0 : 0.0);
  rpm.setSignalValue(3000);
  webapp.sendIfDue(microCan0, engine201, 50);
}

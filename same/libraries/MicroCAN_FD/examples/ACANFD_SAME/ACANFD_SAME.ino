/*
  MicroCAN-FD ACANFD_SAME Template

  This is a minimal starting point for using the CAN FD controllers on
  the MicroCAN-FD. The CAN transceiver standby pins are defined by the
  MicroCAN-FD board support package.

  Nearly all applications for the MicroCAN-FD will require the ACANFD_SAME library,
  this is the main library that allows you to use the two CAN Bus controllers. 

  Install it from Library Manager before compiling this sketch.

  If not available from the library manager, please install it manually by downloading it from here:
  https://github.com/Terry-Gould/ACANFD_SAME

  For more detailed explanations, see the examples included in the library.

*/

#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (0)

#include <ACANFD_SAME.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

void printFrameData(const CANFDMessage& frame) {
  Serial.print(" data=");

  for (uint8_t i = 0; i < frame.len; i++) {
    if (frame.data[i] < 0x10) {
      Serial.print('0');
    }

    Serial.print(frame.data[i], HEX);

    if (i + 1 < frame.len) {
      Serial.print(' ');
    }
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);

  while (!Serial && millis() < 1500) {
    delay(10);
  }

  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);

  ACANFD_SAME_Settings settings(ACANFD_SAME_Settings::CLOCK_48MHz,
                                500UL * 1000UL,
                                DataBitRateFactor::x4);

  settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  const uint32_t can0Error = can0.beginFD(settings);

  Serial.println("MicroCAN-FD ACANFD_SAME template");
  Serial.print("CAN0 begin status: 0x");
  Serial.println(can0Error, HEX);
}

static const uint32_t PERIOD = 1000;  // How often to send the CAN frame in milliseconds
static uint32_t sendTime = PERIOD;    // To keep track of the next scheduled time to send.
static uint32_t sentCount = 0;        // To count how many messages have been sent.

void loop() {
  CANFDMessage frame;

  if (can0.receiveFD0(frame)) {
    Serial.print("CAN0 RX id=0x");
    Serial.print(frame.id, HEX);
    Serial.print(" len=");
    Serial.println(frame.len);
    printFrameData(frame);
  }

  if (millis() >= sendTime) {
    sendTime += PERIOD;

    CANFDMessage frame;                   // This can be named anything you like, you may need multiple names when sending and managing multiple frames.
    frame.id = 0x201;                     // For a standard 11-bit ID, valid range is: 0x000 to 0x7FF, For an extended 29-bit ID, valid range is: 0x00000000 to 0x1FFFFFFF.
    frame.ext = false;                    // This line is optional here because the default is normally false, but setting it explicitly makes the example clearer.
    frame.type = CANFDMessage::CAN_DATA;  // For CAN 2.0 options are CAN_REMOTE, CAN_DATA
    frame.len = 8;                        // How many bytes you are sending, 0-8 for CAN 2.0
    frame.data[0] = 0x11;
    frame.data[1] = 0x22;
    frame.data[2] = 0x33;
    frame.data[3] = 0x44;
    frame.data[4] = 0x55;
    frame.data[5] = 0x66;
    frame.data[6] = 0x77;
    frame.data[7] = 0x88;

    const uint32_t sendStatus = can0.tryToSendReturnStatusFD(frame);

    // This just prints to tell us if the .tryToSendReturnStatusFD was ok or if there was an error.
    if (sendStatus == 0) {
      sentCount += 1;
      Serial.print("Sent ");
      Serial.println(sentCount);
    } else {
      Serial.print("Sent error 0x");
      Serial.println(sendStatus, HEX);
    }
  }
}

/* 
  MicroCAN-FD dual-channel external-loopback power and traffic stress test.

  Goal: keep both CAN FD controllers busy without intentionally blocking their
  FIFOs, run a free-looping DMA SRAM copy in the background, and spend the CPU
  time that remains on high-throughput FPU/integer/SRAM work.

  The goal is simply to measure the maximum amount of power in order to measure
  maximum current draw of the MicroCAN-FD, this sketch serves no other purpose.
*/

#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (1728)

#include <ACANFD_SAME.h>
#include <Adafruit_ZeroDMA.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

static const uint32_t NOMINAL_BIT_RATE = 1000UL * 1000UL;
static const DataBitRateFactor DATA_BIT_RATE_FACTOR = DataBitRateFactor::x8;
static const uint16_t MAX_SOFTWARE_PENDING = 768;
static const uint8_t CAN_SEND_BURST_PER_CHANNEL = 96;
static const uint8_t FRAME_POOL_SIZE = 255;
static const uint16_t CORE_LOAD_ITERATIONS = 192;
static const uint8_t TX_INDEX_FIFO = 0;
static const uint8_t CAN0_PAYLOAD_MARKER = 0xC0;
static const uint8_t CAN1_PAYLOAD_MARKER = 0xC1;

static Adafruit_ZeroDMA gDma;
static DmacDescriptor *gDmaDescriptor = nullptr;
static uint32_t gDmaSource[1024] __attribute__((aligned(16)));
static uint32_t gDmaDestination[1024] __attribute__((aligned(16)));
static CANFDMessage gFramePool0[FRAME_POOL_SIZE];
static CANFDMessage gFramePool1[FRAME_POOL_SIZE];

static volatile float gFloatLoad = 0.125f;
static volatile int64_t gMacLoad = 0;
static volatile uint32_t gPowerSink = 0;
static volatile uint32_t gScratch[512];
static uint8_t gUsbBulkBlock[64];

static uint32_t gSequence0 = 0;
static uint32_t gSequence1 = 0;
static uint8_t gPoolIndex0 = 0;
static uint8_t gPoolIndex1 = 0;
static uint32_t gSent0 = 0;
static uint32_t gSent1 = 0;
static uint32_t gReceived0 = 0;
static uint32_t gReceived1 = 0;
static uint32_t gMismatch0 = 0;
static uint32_t gMismatch1 = 0;
static uint32_t gSendErrors0 = 0;
static uint32_t gSendErrors1 = 0;
static uint16_t gPending0 = 0;
static uint16_t gPending1 = 0;
static uint32_t gLastReportMs = 0;

static void enableExternalTransceivers() {
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);
  digitalWrite(PIN_CAN1_STANDBY, LOW);
}

static void startDmaMemoryHammer() {
  for (uint16_t i = 0; i < 1024; i++) {
    gDmaSource[i] = 0xA5A50000UL ^ (uint32_t(i) * 0x10204081UL);
    gDmaDestination[i] = 0x5A5A0000UL ^ (uint32_t(i) * 0x01020408UL);
  }

  gDma.setAction(DMA_TRIGGER_ACTON_TRANSACTION);
  if (gDma.allocate() != DMA_STATUS_OK) {
    return;
  }
  gDma.setPriority(DMA_PRIORITY_3);
  gDma.loop(true);
  gDmaDescriptor = gDma.addDescriptor(gDmaSource,
                                      gDmaDestination,
                                      1024,
                                      DMA_BEAT_SIZE_WORD,
                                      true,
                                      true);
  if (gDmaDescriptor != nullptr) {
    gDma.startJob();
    gDma.trigger();
  }
}

static void enableInternalPeripheralLoad() {
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_TC0 | MCLK_APBAMASK_TC1;
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_TCC0 | MCLK_APBBMASK_TCC1 | MCLK_APBBMASK_TC2 | MCLK_APBBMASK_TC3;
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC2 | MCLK_APBCMASK_TCC3 | MCLK_APBCMASK_TC4 | MCLK_APBCMASK_TC5;
  MCLK->APBDMASK.reg |= MCLK_APBDMASK_TCC4;

  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TCC1_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TCC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TCC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TCC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC1_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC2_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;
  GCLK->PCHCTRL[TC5_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | GCLK_PCHCTRL_CHEN;

  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
  TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
  TCC2->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
  TCC3->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
  TCC4->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
  TC0->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC1->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC2->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
}

static void configureCANSettings(ACANFD_SAME_Settings &settings) {
  settings.mModuleMode = ACANFD_SAME_Settings::EXTERNAL_LOOP_BACK;
  settings.mDriverReceiveFIFO0Size = 512;
  settings.mDriverReceiveFIFO1Size = 0;
  settings.mHardwareRxFIFO0Size = 64;
  settings.mHardwareRxFIFO1Size = 0;
  settings.mHardwareRxFIFO0Payload = ACANFD_SAME_Settings::PAYLOAD_64_BYTES;
  settings.mHardwareTransmitTxFIFOSize = 32;
  settings.mHardwareDedicacedTxBufferCount = 0;
  settings.mHardwareTransmitBufferPayload = ACANFD_SAME_Settings::PAYLOAD_64_BYTES;
  settings.mDriverTransmitFIFOSize = 128;
  settings.mEnableRetransmission = true;
}

static void initFramePool(CANFDMessage *pool, const uint8_t marker, const uint16_t baseId) {
  for (uint8_t frameIndex = 0; frameIndex < FRAME_POOL_SIZE; frameIndex++) {
    CANFDMessage &frame = pool[frameIndex];
    frame.idx = TX_INDEX_FIFO;
    frame.ext = false;
    frame.id = baseId + frameIndex;
    frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
    frame.len = 64;
    frame.data[0] = marker;
    for (uint8_t i = 1; i < frame.len; i++) {
      frame.data[i] = ((i + frameIndex) & 1) ? 0x55 : 0xAA;
    }
  }
}

static CANFDMessage nextFrame(CANFDMessage *pool, uint8_t &poolIndex, uint32_t &sequence) {
  CANFDMessage frame = pool[poolIndex];
  poolIndex += 1;
  if (poolIndex >= FRAME_POOL_SIZE) {
    poolIndex = 0;
  }
  sequence += 1;
  frame.data[1] = uint8_t(sequence);
  frame.data[2] = uint8_t(sequence >> 8);
  frame.data[3] = uint8_t(sequence >> 16);
  frame.data[4] = uint8_t(sequence >> 24);
  frame.data[5] = uint8_t(TC0->COUNT16.COUNT.reg);
  frame.data[6] = uint8_t(TC1->COUNT16.COUNT.reg);
  frame.data[7] = uint8_t(TCC0->COUNT.reg);
  return frame;
}

static bool validStressFrame(const CANFDMessage &actual, const uint8_t marker) {
  return (actual.type == CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH) && (actual.len == 64) && (actual.data[0] == marker);
}

static void drainChannel(ACANFD_SAME &can, uint16_t &pending, uint32_t &received, uint32_t &mismatches, const uint8_t marker) {
  CANFDMessage actual;
  while (can.receiveFD0(actual)) {
    if ((pending == 0) || !validStressFrame(actual, marker)) {
      mismatches += 1;
    } else {
      pending -= 1;
    }
    received += 1;
  }
}

static void pumpCanChannel(ACANFD_SAME &can,
                           uint16_t &pending,
                           CANFDMessage *pool,
                           uint8_t &poolIndex,
                           uint32_t &sequence,
                           uint32_t &sent,
                           uint32_t &sendErrors) {
  for (uint8_t i = 0; i < CAN_SEND_BURST_PER_CHANNEL; i++) {
    if ((pending >= MAX_SOFTWARE_PENDING) || !can.sendBufferNotFullForIndex(TX_INDEX_FIFO)) {
      return;
    }
    CANFDMessage frame = nextFrame(pool, poolIndex, sequence);
    const uint32_t sendStatus = can.tryToSendReturnStatusFD(frame);
    if (sendStatus == 0) {
      pending += 1;
      sent += 1;
    } else {
      sendErrors += 1;
      return;
    }
  }
}

static void saturateUsbCdcWithoutBlocking() {
  if (!Serial) {
    return;
  }
  const int writable = Serial.availableForWrite();
  if (writable < int(sizeof(gUsbBulkBlock))) {
    return;
  }
  for (uint8_t i = 0; i < sizeof(gUsbBulkBlock); i++) {
    gUsbBulkBlock[i] = uint8_t(gPowerSink >> ((i & 3) * 8));
  }
  Serial.write(gUsbBulkBlock, sizeof(gUsbBulkBlock));
}

static __attribute__((optimize("O3"))) void burnCoreSlice() {
  float a = gFloatLoad + 1.00024414f;
  float b = 0.99975586f;
  float c = 0.50048828f;
  float d = 1.49951172f;
  float e = 0.25097656f;
  float f = 1.75048828f;
  float g = 0.87524414f;
  float h = 1.12597656f;
  int32_t x = int32_t(gPowerSink | 1U);
  int32_t y = int32_t((gPowerSink << 1) | 0x13579BDFUL);
  int32_t z = int32_t((gPowerSink >> 1) | 0x2468ACE1UL);
  int32_t w = int32_t((gPowerSink << 3) | 0x10203041UL);
  int64_t acc = gMacLoad;
  int64_t acc2 = gMacLoad ^ 0x55AA55AA33CC33CCLL;

  for (uint16_t i = 0; i < CORE_LOAD_ITERATIONS; i++) {
    a = (a * 1.00012207f) + b;
    b = (b * 0.99987793f) - c;
    c = (c * 1.00024414f) + d;
    d = (d * 0.99975586f) - a;
    e = (e * 1.00048828f) + f;
    f = (f * 0.99951172f) - g;
    g = (g * 1.00097656f) + h;
    h = (h * 0.99902344f) - e;
    a += e * 0.03125f;
    c -= g * 0.015625f;
    acc += int64_t(x) * int64_t(y);
    acc2 += int64_t(z) * int64_t(w);
    x = (x << 5) ^ (x >> 3) ^ int32_t(acc);
    y = (y << 7) ^ (y >> 5) ^ int32_t(acc >> 32);
    z = (z << 9) ^ (z >> 2) ^ int32_t(acc2);
    w = (w << 3) ^ (w >> 7) ^ int32_t(acc2 >> 32);
    gScratch[i & 511] = uint32_t(x) ^ uint32_t(y) ^ uint32_t(z) ^ uint32_t(w) ^ uint32_t(TC0->COUNT16.COUNT.reg);
    gDmaSource[(i * 7) & 1023] ^= uint32_t(acc) + uint32_t(x);
    gDmaSource[(i * 13) & 1023] ^= uint32_t(acc2) + uint32_t(z);
    __asm__ __volatile__ ("" : "+w" (a), "+w" (b), "+w" (c), "+w" (d), "+w" (e), "+w" (f), "+w" (g), "+w" (h), "+r" (x), "+r" (y), "+r" (z), "+r" (w), "+r" (acc), "+r" (acc2) :: "memory");
  }

  const uint16_t scratchIndex = uint16_t((uint32_t(acc ^ acc2) ^ gScratch[(acc >> 8) & 511]) & 0x1FF);
  gPowerSink ^= uint32_t(digitalRead(VIN_STATUS)) + uint32_t(acc) + gScratch[scratchIndex];
  gFloatLoad = a + b + c + d + e + f + g + h;
  gMacLoad = acc ^ acc2;
}

static void reportOncePerSecond() {
  const uint32_t now = millis();
  if ((now - gLastReportMs) < 1000) {
    return;
  }
  gLastReportMs = now;
  Serial.print("CAN0 sent=");
  Serial.print(gSent0);
  Serial.print(" rx=");
  Serial.print(gReceived0);
  Serial.print(" pend=");
  Serial.print(gPending0);
  Serial.print(" rxPeak=");
  Serial.print(can0.driverReceiveFIFO0PeakCount());
  Serial.print(" txPeak=");
  Serial.print(can0.transmitFIFOPeakCount());
  Serial.print(" err=");
  Serial.print(gSendErrors0);
  Serial.print(" mismatch=");
  Serial.print(gMismatch0);
  Serial.print(" | CAN1 sent=");
  Serial.print(gSent1);
  Serial.print(" rx=");
  Serial.print(gReceived1);
  Serial.print(" pend=");
  Serial.print(gPending1);
  Serial.print(" rxPeak=");
  Serial.print(can1.driverReceiveFIFO0PeakCount());
  Serial.print(" txPeak=");
  Serial.print(can1.transmitFIFOPeakCount());
  Serial.print(" err=");
  Serial.print(gSendErrors1);
  Serial.print(" mismatch=");
  Serial.print(gMismatch1);
  Serial.print(" dma=");
  Serial.println(gDma.isActive() ? "on" : "off");
}

void setup() {
  Serial.begin(115200);
  const uint32_t serialStart = millis();
  while (!Serial && ((millis() - serialStart) < 1500)) {
    delay(10);
  }

  enableExternalTransceivers();
  pinMode(VIN_STATUS, INPUT);
  enableInternalPeripheralLoad();
  startDmaMemoryHammer();
  initFramePool(gFramePool0, CAN0_PAYLOAD_MARKER, 0x100);
  initFramePool(gFramePool1, CAN1_PAYLOAD_MARKER, 0x200);

  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz, NOMINAL_BIT_RATE, DATA_BIT_RATE_FACTOR);
  ACANFD_SAME_Settings settings1(ACANFD_SAME_Settings::CLOCK_48MHz, NOMINAL_BIT_RATE, DATA_BIT_RATE_FACTOR);
  configureCANSettings(settings0);
  configureCANSettings(settings1);

  const uint32_t error0 = can0.beginFD(settings0);
  const uint32_t error1 = can1.beginFD(settings1);

  Serial.println("MicroCAN-FD dual CAN FD external-loopback stress test");
  Serial.print("CAN0 begin=0x");
  Serial.print(error0, HEX);
  Serial.print(" ramWords=");
  Serial.print(can0.messageRamRequiredMinimumSize());
  Serial.print(" arb=");
  Serial.print(settings0.actualArbitrationBitRate());
  Serial.print(" data=");
  Serial.println(settings0.actualDataBitRate());
  Serial.print("CAN1 begin=0x");
  Serial.print(error1, HEX);
  Serial.print(" ramWords=");
  Serial.print(can1.messageRamRequiredMinimumSize());
  Serial.print(" arb=");
  Serial.print(settings1.actualArbitrationBitRate());
  Serial.print(" data=");
  Serial.println(settings1.actualDataBitRate());
}

void loop() {
  for (uint8_t i = 0; i < 64; i++) {
    drainChannel(can0, gPending0, gReceived0, gMismatch0, CAN0_PAYLOAD_MARKER);
    drainChannel(can1, gPending1, gReceived1, gMismatch1, CAN1_PAYLOAD_MARKER);
    pumpCanChannel(can0, gPending0, gFramePool0, gPoolIndex0, gSequence0, gSent0, gSendErrors0);
    pumpCanChannel(can1, gPending1, gFramePool1, gPoolIndex1, gSequence1, gSent1, gSendErrors1);
    saturateUsbCdcWithoutBlocking();
    burnCoreSlice();
  }

  reportOncePerSecond();
}

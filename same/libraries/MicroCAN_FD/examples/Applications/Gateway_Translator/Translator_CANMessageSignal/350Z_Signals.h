#pragma once

#include "CANMessageSignal.h"

using namespace CANMessageSignal;


// ----------------------------------------

CanMessage z350EngineStatus1F9({ .name = "350Z_EngineStatus_1F9",
                                 .idType = STANDARD,
                                 .id = 0x1F9,
                                 .dlc = 2,
                                 .defaultFill = 0x00,
                                 .comment = "350Z engine run state. Byte 0 upper nibble." });

EnumMap z350EngineStatusMap({
  { 0x0, "Off" },
  { 0x1, "NotRunningFault" },
  { 0x2, "Running" },
  { 0x3, "TurningOver" },
  { 0xE, "RunningFault" },
});

CanSignal z350EngineStatus({ .name = "EngineStatus",
                             .dataType = UNSIGNED,
                             .startBit = 4,
                             .bitLength = 4,
                             .endianness = LITTLE_ENDIAN,
                             .factor = 1,
                             .offset = 0,
                             .unit = "state",
                             .comment = "Byte 0 upper nibble engine state.",
                             .min = 0,
                             .max = 15,
                             .defaultValue = 0,
                             .signalRole = NORMAL_SIGNAL,
                             .multiplexor = nullptr,
                             .multiplexValue = 0,
                             .enumMap = &z350EngineStatusMap,
                             .overrideCapable = true });

// ----------------------------------------

CanMessage z350Accelerator231({ .name = "350Z_Accelerator_231",
                                .idType = STANDARD,
                                .id = 0x231,
                                .dlc = 8,
                                .defaultFill = 0x00,
                                .comment = "350Z accelerator pedal position. Byte 2 is 0.5 percent increments." });

CanByteSignal z350AcceleratorPosition({ .name = "AcceleratorPosition",
                                        .dataType = UNSIGNED,
                                        .startByte = 2,
                                        .byteLength = 1,
                                        .endianness = LITTLE_ENDIAN,
                                        .factor = 0.5,
                                        .offset = 0,
                                        .unit = "%",
                                        .comment = "Accelerator pedal position. Raw range 0x00 to 0xC8 gives 0 to 100 percent in 0.5 percent steps.",
                                        .min = 0,
                                        .max = 100,
                                        .defaultValue = 0,
                                        .overrideCapable = true });

// ----------------------------------------

CanMessage z350Engine23D({ .name = "350Z_Engine_23D",
                           .idType = STANDARD,
                           .id = 0x23D,
                           .dlc = 8,
                           .defaultFill = 0x00,
                           .comment = "350Z engine data. Contains accelerator raw, RPM and coolant temperature." });

CanByteSignal z350AcceleratorRaw({ .name = "AcceleratorPositionRaw",
                                   .dataType = UNSIGNED,
                                   .startByte = 1,
                                   .byteLength = 1,
                                   .endianness = LITTLE_ENDIAN,
                                   .factor = 1,
                                   .offset = 0,
                                   .unit = "raw",
                                   .comment = "Accelerator pedal raw value, directly proportional to accelerator position.",
                                   .min = 0,
                                   .max = 255,
                                   .defaultValue = 0,
                                   .overrideCapable = true });

CanByteSignal z350Rpm({ .name = "EngineRPM",
                        .dataType = UNSIGNED,
                        .startByte = 3,
                        .byteLength = 2,
                        .endianness = LITTLE_ENDIAN,
                        .factor = 3.2,
                        .offset = 0,
                        .unit = "rpm",
                        .comment = "Engine speed from bytes 3 and 4. Byte 4 is the high byte, byte 3 is the low byte, then raw is scaled by 3.2.",
                        .min = 0,
                        .max = 10000,
                        .defaultValue = 0,
                        .overrideCapable = true });

CanByteSignal z350CoolantTemp({ .name = "CoolantTemp",
                                .dataType = UNSIGNED,
                                .startByte = 7,
                                .byteLength = 1,
                                .endianness = LITTLE_ENDIAN,
                                .factor = 1,
                                .offset = -50,
                                .unit = "C",
                                .comment = "Coolant temperature from 0x23D byte 7.",
                                .min = -40,
                                .max = 205,
                                .defaultValue = 0,
                                .overrideCapable = true });

// ----------------------------------------

CanMessage z350DashLights233({ .name = "350Z_DashLights_233",
                               .idType = STANDARD,
                               .id = 0x233,
                               .dlc = 5,
                               .defaultFill = 0x00,
                               .comment = "350Z dash lights. Byte 3 low nibble contains set, cruise and engine lights." });

CanByteBitSignal z350SetLight({ .name = "SetLight",
                                .byte = 3,
                                .bit = 0,
                                .comment = "SET light. ID 0x233 byte 3 bit 0.",
                                .defaultValue = 0,
                                .signalRole = NORMAL_SIGNAL,
                                .multiplexor = nullptr,
                                .multiplexValue = 0,
                                .enumMap = nullptr,
                                .inverted = false,
                                .overrideCapable = true });

CanByteBitSignal z350CruiseLight({ .name = "CruiseLight",
                                   .byte = 3,
                                   .bit = 1,
                                   .comment = "CRUISE light. ID 0x233 byte 3 bit 1.",
                                   .defaultValue = 0,
                                   .signalRole = NORMAL_SIGNAL,
                                   .multiplexor = nullptr,
                                   .multiplexValue = 0,
                                   .enumMap = nullptr,
                                   .inverted = false,
                                   .overrideCapable = true });

CanByteBitSignal z350EngineLight({ .name = "EngineLight",
                                   .byte = 3,
                                   .bit = 3,
                                   .comment = "Engine warning light. ID 0x233 byte 3 bit 3.",
                                   .defaultValue = 0,
                                   .signalRole = NORMAL_SIGNAL,
                                   .multiplexor = nullptr,
                                   .multiplexValue = 0,
                                   .enumMap = nullptr,
                                   .inverted = false,
                                   .overrideCapable = true });

// ----------------------------------------

bool set350ZSignals() {
  bool ok = true;

  ok &= z350EngineStatus1F9.addSignal(z350EngineStatus);

  ok &= z350Accelerator231.addSignal(z350AcceleratorPosition);

  ok &= z350Engine23D.addSignal(z350AcceleratorRaw);
  ok &= z350Engine23D.addSignal(z350Rpm);
  ok &= z350Engine23D.addSignal(z350CoolantTemp);

  ok &= z350DashLights233.addSignal(z350SetLight);
  ok &= z350DashLights233.addSignal(z350CruiseLight);
  ok &= z350DashLights233.addSignal(z350EngineLight);

  return ok;
}

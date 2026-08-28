# Vehtronica SAM-BA USB CDC Bootloader

This bootloader is derived from the Microsoft/Adafruit UF2 SAMD bootloader, but
the Vehtronica configuration is intentionally reduced to the flow used by
MicroCAN-FD.

## Supported Runtime Interfaces

The current Vehtronica build supports:

- USB CDC SAM-BA monitor for `bossac` uploads
- 1200 bps touch reset into bootloader mode
- Double-tap reset entry
- OpenOCD/SWD programming of the bootloader image

The current Vehtronica build does not expose:

- UF2 drag-and-drop updates
- USB mass storage boot drive
- WebUSB bootloader interface
- HID bootloader interface
- UART bootloader programming

The source tree still contains inherited UF2, MSC, HID, WebUSB, and UART code
from the upstream bootloader. These paths are disabled by the configuration in
`inc/uf2.h` unless explicitly re-enabled.

## Build Outputs

For each board, the build produces:

- `bootloader-<board>-<version>.bin`
  Raw bootloader image linked for flash address `0x00000000`. Use this with
  SWD/OpenOCD or another external programmer.

- `bootloader-<board>-<version>.elf`
  ELF image with symbols for debugging, inspection, and map analysis.

- `bootloader-<board>-<version>.map`
  Linker map for reviewing section layout and size.

- `update-bootloader-<board>-<version>.bin`
  Application-space bootloader updater image linked for the sketch area after
  the bootloader.

- `update-bootloader-<board>-<version>.elf`
  ELF form of the updater image.

- `update-bootloader-<board>-<version>.map`
  Linker map for the updater image.

The build intentionally does not produce `.uf2` or `.ino` updater files.

## Programming The Bootloader With OpenOCD

The Arduino BSP provides OpenOCD programmer entries through `same/programmers.txt`.
In Arduino IDE:

1. Select the correct Vehtronica board.
2. Select `CMSIS-DAP / Atmel-ICE / DAPLink over OpenOCD` or `J-Link over OpenOCD`.
3. Run `Tools > Burn Bootloader`.

The board entry supplies the bootloader binary path. The selected programmer
supplies the OpenOCD interface script.

## Updating The Bootloader Over USB CDC

If a working Vehtronica bootloader is already installed, the updater `.bin` can
be uploaded with Arduino CLI using the normal board upload recipe:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" upload `
  --fqbn VehtronicaSAME:same:vehtronica_microcan_fd `
  --port COM13 `
  --input-file "path\to\update-bootloader-MicroCAN_FD_v1_0_0.bin" `
  --verbose
```

The board upload recipe writes application images at offset `0x4000`, so the
updater image must be the `update-bootloader-*.bin`, not the raw bootloader
`bootloader-*.bin`.

## Configuration

Runtime feature selection is controlled in `inc/uf2.h`. Board identity,
USB VID/PID, and hardware definitions are in `boards/<board>/board_config.h`.

The bootloader validates an application before jumping to it. The current checks
reject blank vectors, require the initial stack pointer to be inside real RAM,
require the reset vector Thumb bit to be set, and require the masked reset
handler address to be inside application flash.

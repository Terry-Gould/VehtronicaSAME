# VehtronicaSAME

Arduino board support package for Vehtronica SAME51/SAME54 based boards.

The first supported public release target is the Vehtronica MicroCAN-FD, a
dual-channel CAN/CAN FD development, test, and analysis board based on the
Microchip SAME51.

## Supported Boards

- Vehtronica MicroCAN-FD

## Installation

This package is intended to be installed through Arduino IDE Boards Manager.

1. Open Arduino IDE.
2. Open **File > Preferences**.
3. Add this URL to **Additional boards manager URLs**:

   ```text
   https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
   ```

4. Open **Tools > Board > Boards Manager**.
5. Search for `Vehtronica`.
6. Install **Vehtronica SAMD/E Boards**.

The GitHub repository and release assets must be public before Arduino IDE can
fetch the package index and archives without authentication.

The Boards Manager package installs the board support package and the required
tooling for Windows, macOS, and Linux. Tool versions are pinned by the package
index so installs are repeatable across machines.

## If The Package Does Not Appear

Check the following:

- The Additional boards manager URL must be entered exactly as shown above.
- If Boards Manager was already open, close and reopen it after adding the URL.
- Use the Boards Manager search term `Vehtronica`, not `MicroCAN`.
- Make sure the computer has internet access to GitHub release downloads.
- If testing before public release, remember that Arduino IDE cannot use GitHub
  authentication for private package indexes or private release assets.

You can also test the package index with Arduino CLI:

```text
arduino-cli core update-index --additional-urls https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
arduino-cli core search Vehtronica --additional-urls https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
```

If the package appears in search but installation fails, remove any partial
installation and try again:

```text
arduino-cli core uninstall VehtronicaSAME:same
arduino-cli core install VehtronicaSAME:same --additional-urls https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
```

## Board Selection

After installation, select:

```text
Tools > Board > Vehtronica SAMD/E Boards > Vehtronica MicroCAN-FD
```

The default options are recommended for normal use:

- USB Stack: Arduino
- CPU Speed: 120 MHz
- Cache: Enabled
- Optimize: Small (-Os)
- Max QSPI: 50 MHz
- Debug: Off

## USB Stack Options

The default USB stack is the Arduino USB stack.

TinyUSB is available as an optional advanced stack. It requires the external
**Adafruit TinyUSB Library** from Arduino Library Manager. Sketches that use
TinyUSB-specific APIs should include:

```cpp
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif
```

This keeps the BSP small and lets users receive TinyUSB updates through Arduino
Library Manager.

## Uploading

Normal sketch upload uses BOSSA over the MicroCAN-FD USB serial bootloader.

If the sketch upload port is lost after a bad sketch or incorrect board
selection, enter bootloader mode by double-tapping reset. On enclosed boards
without a reset button, this may require briefly shorting the internal reset pad
to ground twice in quick succession.

## Burn Bootloader

Bootloader programming is available through Arduino IDE's **Burn Bootloader**
flow using OpenOCD and a supported SWD programmer.

The packaged bootloader binary is:

```text
same/bootloaders/microcan_fd/bootloader-MicroCAN_FD_v1_0_0.bin
```

The bootloader source is kept in the public repository under `bootloader/`, but
it is not installed by Boards Manager.

## Libraries

MicroCAN-FD applications normally use these separately installed Arduino
libraries:

- ACANFD_SAME
- CANMessageSignal
- CANSignalStudioInterface

They are intentionally not bundled into this BSP so users and developers do not
end up with duplicate installed library copies.

## Attribution

This BSP is derived from Adafruit's Arduino SAMD/SAME board support package and
uses the Microsoft/Adafruit UF2 SAMD bootloader as the bootloader source base.

Third-party licenses and notices are retained in the relevant source folders.
See [LICENSE.md](LICENSE.md) for the top-level license summary.

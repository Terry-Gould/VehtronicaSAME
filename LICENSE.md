# License and Third-Party Notices

This repository contains a board support package derived from existing open
source Arduino, Adafruit, Microsoft, ARM, and Microchip/Atmel components.

The files in this repository are not all under a single license. Individual
source files and folders retain their original copyright notices and license
headers.

Key license areas:

- `same/` contains Arduino/Adafruit SAMD/SAME board support package code. Many
  source files in this tree are licensed under the GNU Lesser General Public
  License version 2.1 or later, as stated in their file headers.
- `bootloader/` is derived from the Microsoft/Adafruit UF2 SAMD bootloader and
  includes its own license and third-party notices in that folder.
- `tools/` contains third-party toolchain, CMSIS, device pack, BOSSA, and
  related files. These retain the licenses and notices supplied by their
  upstream projects.

Vehtronica-specific additions and modifications are provided under the same
license terms as the files they modify unless a file states otherwise.

Before redistributing modified versions, review the license headers and notices
in the relevant folders.

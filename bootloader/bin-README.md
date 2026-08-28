# Vehtronica Bootloader Binary Package

This directory is used by the bootloader build packaging targets.

Vehtronica bootloader packages contain raw binary and ELF outputs only. They do
not contain UF2 or Arduino sketch updater files.

Use:

- `bootloader-*.bin` for SWD/OpenOCD programming at flash address `0x00000000`
- `bootloader-*.elf` for debug and symbol inspection
- `update-bootloader-*.bin` for USB CDC/bossac update through the Arduino upload
  recipe
- `update-bootloader-*.elf` for debug and symbol inspection of the updater image

The updater `.bin` is an application-space image. Upload it using the board's
normal Arduino upload flow so it is written at the sketch offset configured in
`boards.txt`.

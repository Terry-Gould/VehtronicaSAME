# Boards Manager Release Process

This folder contains maintainer tooling for generating the Arduino Boards
Manager package index and release archives for VehtronicaSAME.

The release strategy is:

- `same/` is packaged as the Arduino platform archive.
- Required tools are declared as Vehtronica-owned tool dependencies.
- Native executable tools use one archive per supported host, following the
  upstream Arduino/Adafruit host mappings for the tested tool versions.
- `bootloader/` remains in the public repository as source code, but is not
  installed by Boards Manager.

## Generate Release Files

Run from the repository root:

```powershell
.\extras\release\create-board-manager-package.ps1 -Version 1.0.0
```

This creates:

- `package_vehtronica_same_index.json`
- `dist/VehtronicaSAME-same-1.0.0.zip`
- mirrored tool archives under `dist/tool-archives/`

The release assets are not stored in Git. They are generated into `dist/` and
uploaded to the GitHub release.

## Tool Versions

Version `1.0.0` uses the tested Arduino/Adafruit tool versions:

- `arm-none-eabi-gcc` `9-2019q4`
- `bossac` `1.8.0-48-gb176eee`
- `CMSIS` `5.4.0`
- `CMSIS-Atmel` `1.2.2`
- `openocd` `0.11.0-arduino2`

The script reads the installed Arduino and Adafruit package indexes to preserve
the upstream host mappings, archive file names, file sizes, and checksums. It
then points those entries at the Vehtronica GitHub release asset URLs.

The current upstream host mappings support:

- Windows 32/64-bit via the Windows tool flavour.
- macOS Intel via the x86_64 macOS tool flavour.
- macOS Apple Silicon via Arduino's compatible macOS x86_64 fallback.
- Linux x86, x86_64, ARM, and ARM64 where upstream tool flavours exist.

## Publish Version 1.0.0

After reviewing the generated files:

```powershell
git add README.md LICENSE.md .gitignore package_vehtronica_same_index.json extras/release same/platform.txt
git commit -m "Add Boards Manager release packaging"
git tag -a 1.0.0 -m "VehtronicaSAME 1.0.0"
git push origin main
git push origin 1.0.0
gh release create 1.0.0 dist\VehtronicaSAME-same-1.0.0.zip dist\tool-archives\* --title "VehtronicaSAME 1.0.0" --notes "Initial public MicroCAN-FD BSP release."
```

If the tag name changes, regenerate the package index so the release URLs inside
`package_vehtronica_same_index.json` match the actual GitHub release path.

## Fresh Install Test

After publishing the release assets, test a fresh install:

```powershell
arduino-cli core update-index --additional-urls https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
arduino-cli core install vehtronica:same --additional-urls https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
arduino-cli board listall Vehtronica
```

Then compile at least one MicroCAN-FD sketch with the Arduino USB stack and one
with TinyUSB.

## User Installation URL

Users add this URL to Arduino IDE's Additional Boards Manager URLs:

```text
https://raw.githubusercontent.com/Terry-Gould/VehtronicaSAME/main/package_vehtronica_same_index.json
```

The repository and release assets must be public before normal users can install
the BSP without GitHub authentication.

/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define ARDUINO_MAIN
#include "Arduino.h"

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

#if defined(USE_TINYUSB)
struct Adafruit_USBD_Device;

extern Adafruit_USBD_Device TinyUSBDevice;
extern "C" uint8_t const *tud_descriptor_configuration_cb(uint8_t index);
extern "C" void tinyusb_set_manufacturer_descriptor(Adafruit_USBD_Device *dev, const char *s) asm("_ZN20Adafruit_USBD_Device25setManufacturerDescriptorEPKc");
extern "C" void tinyusb_set_product_descriptor(Adafruit_USBD_Device *dev, const char *s) asm("_ZN20Adafruit_USBD_Device20setProductDescriptorEPKc");
extern "C" uint8_t tinyusb_add_string_descriptor(Adafruit_USBD_Device *dev, const char *s) asm("_ZN20Adafruit_USBD_Device19addStringDescriptorEPKc");

static void initTinyUSBDescriptors() {
  tinyusb_set_manufacturer_descriptor(&TinyUSBDevice, USB_MANUFACTURER);
  tinyusb_set_product_descriptor(&TinyUSBDevice, USB_PRODUCT);
}

static void updateTinyUSBCDCInterfaceDescriptor() {
  uint8_t const strid = tinyusb_add_string_descriptor(&TinyUSBDevice, USB_PRODUCT);
  uint8_t *config = (uint8_t *)tud_descriptor_configuration_cb(0);

  if (!config || !strid) {
    return;
  }

  uint16_t const total_length = config[2] | ((uint16_t)config[3] << 8);
  for (uint16_t offset = 0; offset + 8 < total_length;) {
    uint8_t const length = config[offset];
    if (!length) {
      break;
    }

    uint8_t const descriptor_type = config[offset + 1];
    uint8_t const interface_class = config[offset + 5];
    if (descriptor_type == 0x04 && interface_class == 0x02) {
      config[offset + 8] = strid;
    }

    offset += length;
  }
}
#endif

// Initialize C library
extern "C" void __libc_init_array(void);

/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
  init();

  __libc_init_array();

  initVariant();

  delay(1);

#if defined(USE_TINYUSB)
  initTinyUSBDescriptors();
  TinyUSB_Device_Init(0);
  updateTinyUSBCDCInterfaceDescriptor();
#elif defined(USBCON)
  USBDevice.init();
  USBDevice.attach();
#endif

  setup();

  for (;;)
  {
    loop();
    yield(); // yield run usb background task

    if (serialEventRun) serialEventRun();
  }

  return 0;
}

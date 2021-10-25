#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_URL    0b10
#define NVRAM7_24V    0b100
#define NVRAM7_UPDATE 0b1000

struct {
  uint8_t proc;
  uint16_t pwm;
  uint8_t nvram7;
  uint32_t rate;
  uint8_t comset;
  uint8_t change;
  uint32_t crc32;
} nvram __attribute__ ((packed));

uint32_t calculateCRC32(const uint8_t *data, size_t length);
void load_nvram() {
  File fp;
  uint8_t * str = (uint8_t *)&nvram;
  ESP.rtcUserMemoryRead(0, (uint32_t*) &nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    memset(&nvram, 0, sizeof(nvram));

    if (SPIFFS.begin()) {
      fp = SPIFFS.open("/nvram.save", "r");
      if (fp) {
        fp.read(str, sizeof(nvram));
        fp.close();
      }
    }
  }
}
void save_nvram() {
  File fp;
  uint8_t * str = (uint8_t *)&nvram;
  if (nvram.change == 0) return;
  nvram.change = 0;
  nvram.crc32 = calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32));
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/nvram.save", "w");
    if (fp) {
      fp.write(str, sizeof(nvram));
      fp.close();
    }
  }
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
#endif //__NVRAM__

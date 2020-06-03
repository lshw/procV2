#ifndef __NVRAM__
#define __NVRAM__

//地址7
#define PROC 0

#define NVRAM7_URL    0b10
#define NVRAM7_24V    0b100
#define NVRAM7_UPDATE 0b1000

struct {
  uint32_t crc32;
  uint8_t proc;
  uint16_t pwm;
  uint8_t nvram7;
  byte data[508];
} nvram;

uint32_t calculateCRC32(const uint8_t *data, size_t length);
void load_nvram() {
  ESP.rtcUserMemoryRead(0, (uint32_t*) &nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram.data[0], sizeof(nvram.data))) {
    memset(&nvram.data[0], 0, sizeof(nvram.data));
  }
}
void save_nvram() {
  nvram.crc32 = calculateCRC32((uint8_t*) &nvram.data[0], sizeof(nvram.data));
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
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

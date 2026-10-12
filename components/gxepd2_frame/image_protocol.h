#pragma once
#include <cstddef>
#include <cstdint>
namespace esphome::gxepd2_frame::image_protocol {
constexpr size_t HEADER_SIZE = 16;
constexpr size_t PAYLOAD_SIZE = 48000;
constexpr size_t STRIP_SIZE = 1600;
inline uint16_t read16(const uint8_t *p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
inline uint32_t read32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline bool valid_header(const uint8_t *p) {
  return p[0]=='E' && p[1]=='P' && p[2]=='D' && p[3]=='1' && read16(p+4)==800 &&
         read16(p+6)==480 && read32(p+8)==PAYLOAD_SIZE;
}
// IEEE CRC-32, same result as Python zlib.crc32. Start 0xffffffff, final XOR 0xffffffff.
inline uint32_t crc_update(uint32_t crc, const uint8_t *data, size_t size) {
  for (size_t i=0; i<size; i++) {
    crc ^= data[i];
    for (uint8_t bit=0; bit<8; bit++) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0U);
  }
  return crc;
}
}  // namespace esphome::gxepd2_frame::image_protocol

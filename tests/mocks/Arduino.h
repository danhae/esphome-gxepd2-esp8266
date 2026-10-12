#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
inline uint32_t fake_now=0;
inline unsigned fake_free_heap=42000;
inline uint32_t millis() { return fake_now; }
inline uint32_t micros() { return fake_now*1000; }
inline void delay(unsigned ms) { fake_now+=ms; }
inline void yield() {}
constexpr int LOW=0;
inline int digitalRead(int) { return LOW; }
#define F(x) x
struct FakeESP { unsigned getFreeHeap() const { return fake_free_heap; } unsigned getMaxFreeBlockSize() const { return fake_free_heap; } };
inline FakeESP ESP;

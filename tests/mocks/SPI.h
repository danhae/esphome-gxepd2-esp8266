#pragma once
#include "Arduino.h"
struct FakeSPI { void begin() {} }; inline FakeSPI SPI;

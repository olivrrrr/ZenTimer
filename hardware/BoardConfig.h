#pragma once
#include "../ScreenGeometry.h"
namespace BoardConfig {
constexpr uint8_t LcdDin = 10, LcdClk = 8, LcdCs = 9, LcdDc = 7, LcdReset = 3, LcdBacklight = 6;
// Confirmed by the user's combined hardware test (opposite to the old plan).
constexpr uint8_t TouchIrq = 0, TouchReset = 1;
constexpr auto Rotation = ScreenGeometry::Rotation::Clockwise;
}

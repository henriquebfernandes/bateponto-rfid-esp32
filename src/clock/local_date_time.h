#pragma once

#include <Arduino.h>

String formatLocalDateTime(int64_t timestampMs, const char* format = "%Y-%m-%dT%H:%M:%S");
bool parseLocalDateTime(const String& value, int64_t& timestampMs);

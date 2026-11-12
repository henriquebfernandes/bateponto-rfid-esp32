#pragma once

#include <Arduino.h>

bool beginRfidReader();
bool readRfidCard(String& uid);

#include "local_date_time.h"

#include <time.h>

String formatLocalDateTime(int64_t timestampMs, const char* format) {
  const time_t seconds = timestampMs / 1000;
  struct tm localTime;
  char text[32];
  if (!localtime_r(&seconds, &localTime) ||
      strftime(text, sizeof(text), format, &localTime) == 0) {
    return "";
  }
  return text;
}

bool parseLocalDateTime(const String& value, int64_t& timestampMs) {
  // aceita o formato do navegador com segundos opcionais
  if (value.length() != 16 && value.length() != 19) {
    return false;
  }
  const String dateTime = value.length() == 16 ? value + ":00" : value;
  struct tm localTime = {};
  const char* end = strptime(dateTime.c_str(), "%Y-%m-%dT%H:%M:%S", &localTime);
  if (!end || *end != '\0') {
    return false;
  }

  // rejeita datas que o relógio normaliza, como 31 de fevereiro
  localTime.tm_isdst = -1;
  const time_t seconds = mktime(&localTime);
  const int64_t candidate = static_cast<int64_t>(seconds) * 1000;
  if (formatLocalDateTime(candidate) != dateTime) {
    return false;
  }
  timestampMs = candidate;
  return true;
}

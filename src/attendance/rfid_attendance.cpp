#include "rfid_attendance.h"

#include <sys/time.h>

#include "attendance_storage.h"
#include "config.h"
#include "../cards/card_storage.h"
#include "../people/people_storage.h"

bool registerRfidAttendance(const String& uid) {
  AttendanceRecord record;
  const StorageResult owner = findCardOwner(uid, record.personId);
  if (owner != StorageResult::ok) {
    Serial.println(owner == StorageResult::notFound ? "Cartão sem vínculo."
                                                    : "Falha ao consultar o cartão.");
    return false;
  }

  Person person;
  if (findPerson(record.personId, person) != StorageResult::ok) {
    Serial.println("Falha ao consultar a pessoa vinculada ao cartão.");
    return false;
  }

  timeval now;
  gettimeofday(&now, nullptr);
  record.timestampMs = static_cast<int64_t>(now.tv_sec) * 1000 + now.tv_usec / 1000;
  record.source = AttendanceSource::rfid;

  // aplica o intervalo por pessoa e alterna pelo registro mais recente
  AttendanceRecord latest;
  bool inCooldown = false;
  const int64_t cutoffMs = record.timestampMs - ATTENDANCE_COOLDOWN_MS;
  const bool complete = visitAttendance([&](const AttendanceRecord& stored) {
    if (stored.personId != record.personId) {
      return;
    }
    if (stored.timestampMs > cutoffMs && stored.timestampMs <= record.timestampMs) {
      inCooldown = true;
    }
    if (latest.id.isEmpty() || isMoreRecent(stored, latest)) {
      latest = stored;
    }
  });
  if (!complete) {
    Serial.println("Falha ao consultar os registros de ponto.");
    return false;
  }
  if (inCooldown) {
    Serial.println("Aguarde o intervalo mínimo entre registros.");
    return false;
  }

  if (!latest.id.isEmpty() && latest.type == AttendanceType::entry) {
    record.type = AttendanceType::exit;
  }
  if (!createAttendance(record)) {
    Serial.println("Falha ao salvar o registro de ponto.");
    return false;
  }
  Serial.println(record.type == AttendanceType::entry ? "Entrada registrada." : "Saída registrada.");
  return true;
}

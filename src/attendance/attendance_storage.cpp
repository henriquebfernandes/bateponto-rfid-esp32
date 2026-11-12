#include "attendance_storage.h"

#include <esp_random.h>
#include <sys/time.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../people/people_storage.h"

namespace {
constexpr char RECORD_DIRECTORY[] = "/records";
constexpr char STAGING_PATH[] = "/records/.pending";
constexpr size_t RECORD_ID_LENGTH = 24;

bool isRecordId(const String& id) {
  return isHexId(id.c_str(), RECORD_ID_LENGTH, "0123456789abcdef");
}

String recordPath(const String& id) {
  return String(RECORD_DIRECTORY) + '/' + id;
}

bool saveAttendance(const String& id, const AttendanceRecord& record) {
  // salva a pessoa, o tipo, a data em milissegundos e a fonte em linhas separadas
  char text[48];
  snprintf(text, sizeof(text), "%s\n%c\n%lld\n%c", record.personId.c_str(),
           record.type == AttendanceType::entry ? 'E' : 'S',
           static_cast<long long>(record.timestampMs),
           record.source == AttendanceSource::manual ? 'M' : 'R');
  return writeStoredText(recordPath(id), STAGING_PATH, text);
}
}

bool beginAttendanceStorage() {
  if (!ensureStorageDirectory(RECORD_DIRECTORY)) {
    Serial.println("Falha ao preparar o armazenamento de registros.");
    return false;
  }
  return true;
}

bool isMoreRecent(const AttendanceRecord& first, const AttendanceRecord& second) {
  if (first.timestampMs != second.timestampMs) {
    return first.timestampMs > second.timestampMs;
  }
  return first.id.compareTo(second.id) > 0;
}

StorageResult findAttendance(const String& id, AttendanceRecord& record) {
  if (!isRecordId(id)) {
    return StorageResult::notFound;
  }

  char text[48];
  const StorageResult result = readStoredText(recordPath(id), text, sizeof(text));
  if (result != StorageResult::ok) {
    return result;
  }

  // verifica os campos antes de carregar o registro
  if (strlen(text) < PERSON_ID_LENGTH + 6 || text[PERSON_ID_LENGTH] != '\n' ||
      text[PERSON_ID_LENGTH + 2] != '\n') {
    return StorageResult::error;
  }
  text[PERSON_ID_LENGTH] = '\0';
  const char type = text[PERSON_ID_LENGTH + 1];
  const char* timestampText = text + PERSON_ID_LENGTH + 3;
  char* end;
  errno = 0;
  const long long timestamp = strtoll(timestampText, &end, 10);
  if (!isPersonId(text) || (type != 'E' && type != 'S') ||
      end == timestampText || errno == ERANGE || end[0] != '\n' ||
      (end[1] != 'M' && end[1] != 'R') || end[2] != '\0') {
    return StorageResult::error;
  }

  record.id = id;
  record.personId = text;
  record.timestampMs = timestamp;
  record.type = type == 'E' ? AttendanceType::entry : AttendanceType::exit;
  record.source = end[1] == 'M' ? AttendanceSource::manual : AttendanceSource::rfid;
  return StorageResult::ok;
}

bool visitAttendance(const std::function<void(const AttendanceRecord&)>& visit) {
  return visitStoredFiles(RECORD_DIRECTORY, [&](const char* id) {
    AttendanceRecord record;
    if (findAttendance(id, record) != StorageResult::ok) {
      return false;
    }
    visit(record);
    return true;
  });
}

bool createAttendance(const AttendanceRecord& record) {
  // usa a hora de criação para ordenar registros com a mesma data e hora
  timeval now;
  gettimeofday(&now, nullptr);
  const uint64_t createdAtUs = static_cast<uint64_t>(now.tv_sec) * 1000000 + now.tv_usec;
  char id[RECORD_ID_LENGTH + 1];
  AttendanceRecord existing;
  StorageResult result;
  do {
    snprintf(id, sizeof(id), "%016llx%08lx", static_cast<unsigned long long>(createdAtUs),
             static_cast<unsigned long>(esp_random()));
    result = findAttendance(id, existing);
  } while (result == StorageResult::ok);

  return result == StorageResult::notFound && saveAttendance(id, record);
}

bool updateAttendance(const AttendanceRecord& record) {
  // conserva a fonte original ao atualizar os campos editáveis
  AttendanceRecord updated;
  if (findAttendance(record.id, updated) != StorageResult::ok) {
    return false;
  }
  updated.personId = record.personId;
  updated.timestampMs = record.timestampMs;
  updated.type = record.type;
  return saveAttendance(updated.id, updated);
}

StorageResult removeAttendance(const String& id) {
  if (!isRecordId(id)) {
    return StorageResult::notFound;
  }
  return removeStoredFile(recordPath(id));
}

bool removePersonAttendance(const String& personId) {
  return removeMatchingFiles(RECORD_DIRECTORY, [&](const char* id, bool& match) {
    AttendanceRecord record;
    if (findAttendance(id, record) != StorageResult::ok) {
      return false;
    }
    match = record.personId == personId;
    return true;
  });
}

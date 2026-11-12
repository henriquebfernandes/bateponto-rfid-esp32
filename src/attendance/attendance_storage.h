#pragma once

#include "../storage/storage_files.h"

enum class AttendanceType {
  entry,
  exit
};

enum class AttendanceSource {
  manual,
  rfid
};

struct AttendanceRecord {
  String id;
  String personId;
  int64_t timestampMs = 0;
  AttendanceType type = AttendanceType::entry;
  AttendanceSource source = AttendanceSource::manual;
};

bool beginAttendanceStorage();
bool isMoreRecent(const AttendanceRecord& first, const AttendanceRecord& second);
bool visitAttendance(const std::function<void(const AttendanceRecord&)>& visit);
StorageResult findAttendance(const String& id, AttendanceRecord& record);
bool createAttendance(const AttendanceRecord& record);
bool updateAttendance(const AttendanceRecord& record);
StorageResult removeAttendance(const String& id);
bool removePersonAttendance(const String& personId);

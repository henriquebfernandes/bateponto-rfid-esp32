#pragma once

#include <Arduino.h>
#include <functional>

constexpr char STORAGE_MOUNT_PATH[] = "/littlefs";

enum class StorageResult {
  ok,
  notFound,
  error
};

bool ensureStorageDirectory(const char* path);
bool isHexId(const char* id, size_t length, const char* digits);
StorageResult readStoredText(const String& path, char* text, size_t capacity);
bool writeStoredText(const String& path, const char* stagingPath, const String& text);
StorageResult removeStoredFile(const String& path);
bool visitStoredFiles(const char* directory, const std::function<bool(const char*)>& visit);
bool removeMatchingFiles(const char* directory,
                         const std::function<bool(const char*, bool&)>& matches);

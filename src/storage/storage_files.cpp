#include "storage_files.h"

#include <LittleFS.h>
#include <dirent.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

bool ensureStorageDirectory(const char* path) {
  return LittleFS.exists(path) || LittleFS.mkdir(path);
}

bool isHexId(const char* id, size_t length, const char* digits) {
  return strlen(id) == length && strspn(id, digits) == length;
}

StorageResult readStoredText(const String& path, char* text, size_t capacity) {
  const String fullPath = String(STORAGE_MOUNT_PATH) + path;
  FILE* file = fopen(fullPath.c_str(), "rb");
  if (!file) {
    return errno == ENOENT ? StorageResult::notFound : StorageResult::error;
  }

  // reserva o terminador e rejeita arquivos que excedem o tamanho esperado
  const size_t length = fread(text, 1, capacity, file);
  const bool valid = length > 0 && length < capacity && feof(file) && !ferror(file) &&
                     memchr(text, '\0', length) == nullptr;
  const bool closed = fclose(file) == 0;
  if (!valid || !closed) {
    return StorageResult::error;
  }
  text[length] = '\0';
  return StorageResult::ok;
}

bool writeStoredText(const String& path, const char* stagingPath, const String& text) {
  // publica o arquivo somente depois de concluir a gravação temporária
  const String fullPath = String(STORAGE_MOUNT_PATH) + stagingPath;
  FILE* file = fopen(fullPath.c_str(), "wb");
  if (!file) {
    return false;
  }
  const bool written = fwrite(text.c_str(), 1, text.length(), file) == text.length();
  const bool closed = fclose(file) == 0;
  if (written && closed && LittleFS.rename(stagingPath, path.c_str())) {
    return true;
  }
  LittleFS.remove(stagingPath);
  return false;
}

StorageResult removeStoredFile(const String& path) {
  const String fullPath = String(STORAGE_MOUNT_PATH) + path;
  if (unlink(fullPath.c_str()) == 0) {
    return StorageResult::ok;
  }
  return errno == ENOENT ? StorageResult::notFound : StorageResult::error;
}

bool visitStoredFiles(const char* directory, const std::function<bool(const char*)>& visit) {
  const String fullPath = String(STORAGE_MOUNT_PATH) + directory;
  DIR* entries = opendir(fullPath.c_str());
  if (!entries) {
    return false;
  }

  bool complete = true;
  while (complete) {
    errno = 0;
    const dirent* entry = readdir(entries);
    if (!entry) {
      complete = errno == 0;
      break;
    }
    if (entry->d_name[0] != '.' && entry->d_type != DT_DIR) {
      complete = visit(entry->d_name);
    }
  }
  closedir(entries);
  return complete;
}

bool removeMatchingFiles(const char* directory,
                         const std::function<bool(const char*, bool&)>& matches) {
  // coleta lotes pequenos para excluir depois de fechar o diretório
  constexpr size_t BATCH_SIZE = 16;
  String ids[BATCH_SIZE];
  size_t count;
  do {
    count = 0;
    const bool complete = visitStoredFiles(directory, [&](const char* id) {
      if (count == BATCH_SIZE) {
        return true;
      }
      bool match = false;
      if (!matches(id, match)) {
        return false;
      }
      if (match) {
        ids[count++] = id;
      }
      return true;
    });
    if (!complete) {
      return false;
    }
    for (size_t index = 0; index < count; ++index) {
      if (removeStoredFile(String(directory) + '/' + ids[index]) != StorageResult::ok) {
        return false;
      }
    }
  } while (count == BATCH_SIZE);
  return true;
}

#include "people_storage.h"

#include <LittleFS.h>
#include <dirent.h>
#include <esp_partition.h>
#include <esp_random.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>

#include "../attendance/attendance_storage.h"
#include "../cards/card_storage.h"

namespace {
constexpr char PARTITION_LABEL[] = "spiffs";
constexpr char PEOPLE_DIRECTORY[] = "/people";
constexpr char TEMP_PATH[] = "/people/.pending";

bool isPartitionBlank(const esp_partition_t* partition) {
  // procura dados gravados em toda a partição
  uint8_t block[256];
  for (size_t offset = 0; offset < partition->size; offset += sizeof(block)) {
    const size_t length = std::min<size_t>(sizeof(block), partition->size - offset);
    if (esp_partition_read(partition, offset, block, length) != ESP_OK ||
        !std::all_of(block, block + length, [](uint8_t byte) { return byte == 0xFF; })) {
      return false;
    }
  }
  return true;
}

bool validName(const String& name) {
  if (name.isEmpty() || name.length() > PERSON_NAME_MAX_BYTES) {
    return false;
  }
  for (size_t i = 0; i < name.length(); ++i) {
    const unsigned char character = name[i];
    if (character < ' ' || character == 0x7F) {
      return false;
    }
  }
  return true;
}

PersonResult savePerson(const String& path, String& name) {
  name.trim();
  if (!validName(name)) {
    return PersonResult::invalidName;
  }

  return writeStoredText(path, TEMP_PATH, name) ? PersonResult::saved : PersonResult::storageError;
}
}

bool beginPeopleStorage() {
  // formata apenas quando a montagem falha e a partição está vazia
  if (!LittleFS.begin(false, STORAGE_MOUNT_PATH, 10, PARTITION_LABEL)) {
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, PARTITION_LABEL);
    if (!partition) {
      Serial.println("Partição de dados 'spiffs' não encontrada.");
      return false;
    }
    if (!isPartitionBlank(partition)) {
      Serial.println("Falha ao montar o LittleFS. A partição contém dados ou não pôde ser lida.");
      return false;
    }
    if (!LittleFS.format() || !LittleFS.begin(false, STORAGE_MOUNT_PATH, 10, PARTITION_LABEL)) {
      Serial.println("Falha ao inicializar o LittleFS na partição vazia.");
      return false;
    }
  }

  // prepara o diretório dos cadastros após montar o LittleFS
  if (!ensureStorageDirectory(PEOPLE_DIRECTORY)) {
    Serial.println("Falha ao criar o diretório de pessoas.");
    return false;
  }
  Serial.println("LittleFS pronto.");
  return true;
}

bool isPersonId(const char* id) {
  return isHexId(id, PERSON_ID_LENGTH, "0123456789abcdef");
}

StorageResult findPerson(const String& id, Person& person) {
  if (!isPersonId(id.c_str())) {
    return StorageResult::notFound;
  }

  char name[PERSON_NAME_MAX_BYTES + 1];
  const String path = String(PEOPLE_DIRECTORY) + '/' + id;
  const StorageResult result = readStoredText(path, name, sizeof(name));
  if (result != StorageResult::ok) {
    return result;
  }
  if (!validName(name)) {
    return StorageResult::error;
  }
  person = {id, name};
  return StorageResult::ok;
}

PeopleListResult visitPeople(const std::function<void(const Person&)>& visit) {
  const String directoryPath = String(STORAGE_MOUNT_PATH) + PEOPLE_DIRECTORY;
  DIR* directory = opendir(directoryPath.c_str());
  if (!directory) {
    return PeopleListResult::unavailable;
  }

  // percorre o diretório mesmo quando um arquivo de cadastro não abre
  PeopleListResult result = PeopleListResult::complete;
  while (true) {
    errno = 0;
    const dirent* entry = readdir(directory);
    if (!entry) {
      if (errno != 0) {
        result = PeopleListResult::incomplete;
      }
      break;
    }
    if (entry->d_type == DT_DIR || !isPersonId(entry->d_name)) {
      continue;
    }

    // lê um cadastro por vez e segue para o próximo se houver falha
    Person person;
    if (findPerson(entry->d_name, person) != StorageResult::ok) {
      result = PeopleListResult::incomplete;
      continue;
    }
    visit(person);
  }
  closedir(directory);
  return result;
}

PersonResult createPerson(String name) {
  // gera um identificador que ainda não está em uso
  char id[PERSON_ID_LENGTH + 1];
  String path;
  do {
    const unsigned long high = esp_random();
    const unsigned long low = esp_random();
    snprintf(id, sizeof(id), "%08lx%08lx", high, low);
    path = String(PEOPLE_DIRECTORY) + '/' + id;
  } while (LittleFS.exists(path.c_str()));

  return savePerson(path, name);
}

PersonResult renamePerson(const String& id, String name) {
  // localiza o cadastro pelo identificador antes de atualizar o nome
  if (!isPersonId(id.c_str())) {
    return PersonResult::notFound;
  }
  const String path = String(PEOPLE_DIRECTORY) + '/' + id;
  if (!LittleFS.exists(path.c_str())) {
    return PersonResult::notFound;
  }
  return savePerson(path, name);
}

StorageResult deletePerson(const String& id) {
  Person person;
  const StorageResult result = findPerson(id, person);
  if (result != StorageResult::ok) {
    return result;
  }

  // mantém a pessoa até concluir a remoção de todos os dados associados
  if (!removePersonCards(id) || !removePersonAttendance(id)) {
    return StorageResult::error;
  }
  return removeStoredFile(String(PEOPLE_DIRECTORY) + '/' + id);
}

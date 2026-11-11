#include "people_storage.h"

#include <LittleFS.h>
#include <dirent.h>
#include <esp_partition.h>
#include <esp_random.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace {
constexpr char MOUNT_PATH[] = "/littlefs";
constexpr char PARTITION_LABEL[] = "spiffs";
constexpr char PEOPLE_DIRECTORY[] = "/people";
constexpr char TEMP_PATH[] = "/people/.pending";
constexpr size_t ID_LENGTH = 16;

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

bool validId(const char* id) {
  // impede que o identificador altere o caminho do arquivo
  return strlen(id) == ID_LENGTH &&
         strspn(id, "0123456789abcdef") == ID_LENGTH;
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

  // usa um arquivo temporário para preservar o cadastro se a gravação falhar
  const String stagingPath = String(MOUNT_PATH) + TEMP_PATH;
  FILE* file = fopen(stagingPath.c_str(), "wb");
  if (!file) {
    return PersonResult::storageError;
  }
  const bool written = fwrite(name.c_str(), 1, name.length(), file) == name.length();
  const bool closed = fclose(file) == 0;
  if (!written || !closed || !LittleFS.rename(TEMP_PATH, path.c_str())) {
    LittleFS.remove(TEMP_PATH);
    return PersonResult::storageError;
  }
  return PersonResult::saved;
}
}

bool beginPeopleStorage() {
  // formata apenas quando a montagem falha e a partição está vazia
  if (!LittleFS.begin(false, MOUNT_PATH, 10, PARTITION_LABEL)) {
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
    if (!LittleFS.format() || !LittleFS.begin(false, MOUNT_PATH, 10, PARTITION_LABEL)) {
      Serial.println("Falha ao inicializar o LittleFS na partição vazia.");
      return false;
    }
  }

  // prepara o diretório dos cadastros após montar o LittleFS
  if (!LittleFS.exists(PEOPLE_DIRECTORY) && !LittleFS.mkdir(PEOPLE_DIRECTORY)) {
    Serial.println("Falha ao criar o diretório de pessoas.");
    return false;
  }
  Serial.println("LittleFS pronto.");
  return true;
}

PeopleListResult visitPeople(const std::function<void(const Person&)>& visit) {
  const String directoryPath = String(MOUNT_PATH) + PEOPLE_DIRECTORY;
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
    if (entry->d_type == DT_DIR || !validId(entry->d_name)) {
      continue;
    }

    // lê um cadastro por vez e segue para o próximo se houver falha
    const String path = String(PEOPLE_DIRECTORY) + '/' + entry->d_name;
    File file = LittleFS.open(path);
    char name[PERSON_NAME_MAX_BYTES + 1];
    const size_t length = file.size();
    if (length == 0 || length > PERSON_NAME_MAX_BYTES ||
        file.read(reinterpret_cast<uint8_t*>(name), length) != length) {
      result = PeopleListResult::incomplete;
      continue;
    }
    file.close();

    name[length] = '\0';
    const Person person{String(entry->d_name), String(name, length)};
    if (!validName(person.name)) {
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
  char id[ID_LENGTH + 1];
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
  if (!validId(id.c_str())) {
    return PersonResult::notFound;
  }
  const String path = String(PEOPLE_DIRECTORY) + '/' + id;
  if (!LittleFS.exists(path.c_str())) {
    return PersonResult::notFound;
  }
  return savePerson(path, name);
}

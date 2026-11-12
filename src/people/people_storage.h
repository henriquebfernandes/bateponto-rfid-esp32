#pragma once

#include <Arduino.h>
#include <functional>

#include "../storage/storage_files.h"

constexpr size_t PERSON_ID_LENGTH = 16;
constexpr size_t PERSON_NAME_MAX_BYTES = 120;

struct Person {
  String id;
  String name;
};

enum class PersonResult {
  saved,
  invalidName,
  notFound,
  storageError
};

enum class PeopleListResult {
  complete,
  incomplete,
  unavailable
};

bool beginPeopleStorage();
bool isPersonId(const char* id);
StorageResult findPerson(const String& id, Person& person);
PeopleListResult visitPeople(const std::function<void(const Person&)>& visit);
PersonResult createPerson(String name);
PersonResult renamePerson(const String& id, String name);
StorageResult deletePerson(const String& id);

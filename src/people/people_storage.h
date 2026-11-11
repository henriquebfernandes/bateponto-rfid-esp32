#pragma once

#include <Arduino.h>
#include <functional>

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
PeopleListResult visitPeople(const std::function<void(const Person&)>& visit);
PersonResult createPerson(String name);
PersonResult renamePerson(const String& id, String name);

#include "card_storage.h"

#include "../people/people_storage.h"

namespace {
constexpr char CARD_DIRECTORY[] = "/cards";
constexpr char STAGING_PATH[] = "/cards/.pending";

bool isCardUid(const String& uid) {
  const size_t length = uid.length();
  return (length == 8 || length == 14 || length == 20) &&
         isHexId(uid.c_str(), length, "0123456789ABCDEF");
}

String cardPath(const String& uid) {
  return String(CARD_DIRECTORY) + '/' + uid;
}
}

bool beginCardStorage() {
  if (!ensureStorageDirectory(CARD_DIRECTORY)) {
    Serial.println("Falha ao preparar o armazenamento de cartões.");
    return false;
  }
  return true;
}

StorageResult findCardOwner(const String& uid, String& personId) {
  if (!isCardUid(uid)) {
    return StorageResult::notFound;
  }

  char owner[PERSON_ID_LENGTH + 1];
  const StorageResult result = readStoredText(cardPath(uid), owner, sizeof(owner));
  if (result != StorageResult::ok) {
    return result;
  }
  if (!isPersonId(owner)) {
    return StorageResult::error;
  }
  personId = owner;
  return StorageResult::ok;
}

CardLinkResult linkCard(const String& uid, const String& personId) {
  if (!isCardUid(uid) || !isPersonId(personId.c_str())) {
    return CardLinkResult::storageError;
  }

  // mantém o vínculo existente para qualquer pessoa que apresente o mesmo uid
  String owner;
  switch (findCardOwner(uid, owner)) {
    case StorageResult::ok:
      return CardLinkResult::alreadyLinked;
    case StorageResult::notFound:
      if (writeStoredText(cardPath(uid), STAGING_PATH, personId)) {
        return CardLinkResult::linked;
      }
      break;
    case StorageResult::error:
      break;
  }
  return CardLinkResult::storageError;
}

bool visitPersonCards(const String& personId, const std::function<void(const char*)>& visit) {
  return visitStoredFiles(CARD_DIRECTORY, [&](const char* uid) {
    String owner;
    if (findCardOwner(uid, owner) != StorageResult::ok) {
      return false;
    }
    if (owner == personId) {
      visit(uid);
    }
    return true;
  });
}

StorageResult unlinkCard(const String& uid, const String& personId) {
  String owner;
  const StorageResult result = findCardOwner(uid, owner);
  if (result != StorageResult::ok) {
    return result;
  }
  if (owner != personId) {
    return StorageResult::notFound;
  }
  return removeStoredFile(cardPath(uid));
}

bool removePersonCards(const String& personId) {
  return removeMatchingFiles(CARD_DIRECTORY, [&](const char* uid, bool& match) {
    String owner;
    if (findCardOwner(uid, owner) != StorageResult::ok) {
      return false;
    }
    match = owner == personId;
    return true;
  });
}

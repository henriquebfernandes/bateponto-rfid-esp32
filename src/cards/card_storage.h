#pragma once

#include "../storage/storage_files.h"

enum class CardLinkResult {
  linked,
  alreadyLinked,
  storageError
};

bool beginCardStorage();
StorageResult findCardOwner(const String& uid, String& personId);
CardLinkResult linkCard(const String& uid, const String& personId);
bool visitPersonCards(const String& personId, const std::function<void(const char*)>& visit);
StorageResult unlinkCard(const String& uid, const String& personId);
bool removePersonCards(const String& personId);

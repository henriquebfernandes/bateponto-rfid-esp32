#pragma once

#include <Arduino.h>

enum class CardEnrollmentStatus {
  idle,
  waiting,
  linked,
  alreadyLinked,
  expired,
  personNotFound,
  storageError
};

struct CardEnrollment {
  String personId;
  CardEnrollmentStatus status = CardEnrollmentStatus::idle;
};

bool startCardEnrollment(const String& personId);
const CardEnrollment& getCardEnrollment();
bool consumeEnrollmentCard(const String& uid);

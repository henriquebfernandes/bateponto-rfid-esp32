#include "card_enrollment.h"

#include <esp_timer.h>

#include "card_storage.h"
#include "config.h"
#include "../people/people_storage.h"

namespace {
CardEnrollment enrollment;
int64_t expiresAtUs = 0;
}

bool startCardEnrollment(const String& personId) {
  if (getCardEnrollment().status == CardEnrollmentStatus::waiting) {
    return false;
  }
  enrollment.personId = personId;
  enrollment.status = CardEnrollmentStatus::waiting;
  expiresAtUs = esp_timer_get_time() + CARD_ENROLLMENT_TIMEOUT_MS * 1000LL;
  return true;
}

const CardEnrollment& getCardEnrollment() {
  if (enrollment.status == CardEnrollmentStatus::waiting &&
      esp_timer_get_time() >= expiresAtUs) {
    enrollment.status = CardEnrollmentStatus::expired;
  }
  return enrollment;
}

bool consumeEnrollmentCard(const String& uid) {
  if (getCardEnrollment().status != CardEnrollmentStatus::waiting) {
    return false;
  }

  // encerra o cadastro na primeira leitura, inclusive se o vínculo falhar
  Person person;
  const StorageResult result = findPerson(enrollment.personId, person);
  if (result != StorageResult::ok) {
    enrollment.status = result == StorageResult::notFound
                            ? CardEnrollmentStatus::personNotFound
                            : CardEnrollmentStatus::storageError;
    return true;
  }
  switch (linkCard(uid, person.id)) {
    case CardLinkResult::linked:
      enrollment.status = CardEnrollmentStatus::linked;
      break;
    case CardLinkResult::alreadyLinked:
      enrollment.status = CardEnrollmentStatus::alreadyLinked;
      break;
    case CardLinkResult::storageError:
      enrollment.status = CardEnrollmentStatus::storageError;
      break;
  }
  return true;
}

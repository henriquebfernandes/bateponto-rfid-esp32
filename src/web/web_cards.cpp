#include "web_cards.h"

#include "../cards/card_enrollment.h"
#include "../cards/card_storage.h"
#include "../people/people_storage.h"
#include "web_auth.h"
#include "web_html.h"

namespace {
bool loadPerson(WebServer& server, Person& person) {
  switch (findPerson(server.arg("person"), person)) {
    case StorageResult::ok:
      return true;
    case StorageResult::notFound:
      server.send(404, WEB_TEXT_TYPE, "Pessoa não encontrada.");
      break;
    case StorageResult::error:
      server.send(500, WEB_TEXT_TYPE, "Não foi possível consultar a pessoa.");
      break;
  }
  return false;
}

void sendCardsPage(WebServer& server, const Person& person) {
  sendHtmlStart(server, "Cartões");
  server.sendContent(String("<p>Pessoa: ") + escapeHtml(person.name) + "</p>"
                     "<form action=\"/cards/enrollment\" method=\"post\">"
                     "<input type=\"hidden\" name=\"person\" value=\"" + person.id + "\">"
                     "<button type=\"submit\">Adicionar cartão</button></form><ul>");

  bool hasCards = false;
  const bool complete = visitPersonCards(person.id, [&](const char* uid) {
    hasCards = true;
    server.sendContent(String("<li><form action=\"/cards/unlink\" method=\"post\">") + uid +
                       " <input type=\"hidden\" name=\"person\" value=\"" + person.id + "\">"
                       "<input type=\"hidden\" name=\"uid\" value=\"" + uid + "\">"
                       "<button type=\"submit\">Desvincular</button></form></li>");
  });
  if (!complete) {
    server.sendContent("<li>Não foi possível ler todos os cartões.</li>");
  } else if (!hasCards) {
    server.sendContent("<li>Nenhum cartão vinculado.</li>");
  }
  server.sendContent("</ul><p><a href=\"/\">Voltar às pessoas</a></p>");
  sendHtmlEnd(server);
}

const char* enrollmentMessage(CardEnrollmentStatus status) {
  switch (status) {
    case CardEnrollmentStatus::idle:
      return "Selecione uma pessoa para adicionar um cartão.";
    case CardEnrollmentStatus::waiting:
      return "Aproxime o cartão do leitor.";
    case CardEnrollmentStatus::linked:
      return "Cartão vinculado.";
    case CardEnrollmentStatus::alreadyLinked:
      return "Este cartão já está vinculado.";
    case CardEnrollmentStatus::expired:
      return "O prazo de cadastro expirou.";
    case CardEnrollmentStatus::personNotFound:
      return "A pessoa selecionada não foi encontrada.";
    case CardEnrollmentStatus::storageError:
      return "Não foi possível salvar o vínculo.";
  }
  return "";
}

void sendEnrollmentPage(WebServer& server) {
  const CardEnrollment& enrollment = getCardEnrollment();
  sendHtmlStart(server, "Cadastro de cartão", enrollment.status == CardEnrollmentStatus::waiting);
  Person person;
  if (findPerson(enrollment.personId, person) == StorageResult::ok) {
    server.sendContent(String("<p>Pessoa: ") + escapeHtml(person.name) + "</p>");
  }
  server.sendContent(String("<p>") + enrollmentMessage(enrollment.status) + "</p>");
  if (!person.id.isEmpty()) {
    server.sendContent(String("<p><a href=\"/cards?person=") + person.id +
                       "\">Voltar aos cartões</a></p>");
  }
  server.sendContent("<p><a href=\"/\">Voltar às pessoas</a></p>");
  sendHtmlEnd(server);
}
}

void registerWebCards(WebServer& server) {
  server.on("/cards", HTTP_GET, [&server]() {
    Person person;
    if (requireWebSession(server) && loadPerson(server, person)) {
      sendCardsPage(server, person);
    }
  });
  server.on("/cards/enrollment", HTTP_POST, [&server]() {
    Person person;
    if (requireWebSession(server) && loadPerson(server, person)) {
      if (startCardEnrollment(person.id) || getCardEnrollment().personId == person.id) {
        redirectWeb(server, "/cards/enrollment");
      } else {
        server.send(409, WEB_TEXT_TYPE, "Já existe um cadastro em andamento para outra pessoa.");
      }
    }
  });
  server.on("/cards/enrollment", HTTP_GET, [&server]() {
    if (requireWebSession(server)) {
      sendEnrollmentPage(server);
    }
  });
  server.on("/cards/unlink", HTTP_POST, [&server]() {
    Person person;
    if (!requireWebSession(server) || !loadPerson(server, person)) {
      return;
    }
    switch (unlinkCard(server.arg("uid"), person.id)) {
      case StorageResult::ok:
        redirectWeb(server, String("/cards?person=") + person.id);
        break;
      case StorageResult::notFound:
        server.send(404, WEB_TEXT_TYPE, "Cartão não vinculado à pessoa.");
        break;
      case StorageResult::error:
        server.send(500, WEB_TEXT_TYPE, "Não foi possível desvincular o cartão.");
        break;
    }
  });
}

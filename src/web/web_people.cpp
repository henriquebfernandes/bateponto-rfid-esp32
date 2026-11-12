#include "web_people.h"

#include <WebServer.h>

#include "../people/people_storage.h"
#include "web_auth.h"
#include "web_html.h"

// delimita a página inicial incorporada ao firmware
extern const char homePageStart[] asm("_binary_data_index_html_start");
extern const char homePageEnd[] asm("_binary_data_index_html_end");

namespace {
constexpr char LIST_MARKER[] = "<!-- lista de pessoas -->";

void sendPersonRow(WebServer& server, const Person& person) {
  // vincula o formulário de renomeação ao identificador da pessoa
  String row = "<li><form action=\"/people/rename\" method=\"post\">"
               "<input type=\"hidden\" name=\"id\" value=\"";
  row += person.id;
  row += "\"><label>Nome <input name=\"name\" required value=\"";
  row += escapeHtml(person.name);
  row += "\"></label> <button type=\"submit\">Renomear</button></form>";
  row += String("<a href=\"/cards?person=") + person.id + "\">Cartões</a>";
  row += String("<form action=\"/people/delete\" method=\"post\" "
                "onsubmit=\"return confirm('Excluir esta pessoa, seus cartões e registros?')\">"
                "<input type=\"hidden\" name=\"id\" value=\"") + person.id +
         "\"><button type=\"submit\">Excluir pessoa</button></form></li>";
  server.sendContent(row);
}

void sendPeoplePage(WebServer& server) {
  // localiza o marcador que recebe a lista de pessoas
  const size_t pageSize = reinterpret_cast<uintptr_t>(homePageEnd) -
                          reinterpret_cast<uintptr_t>(homePageStart);
  const String page(homePageStart, pageSize);
  const int listStart = page.indexOf(LIST_MARKER);
  if (listStart < 0) {
    server.send(500, WEB_TEXT_TYPE, "Não foi possível listar as pessoas.");
    return;
  }

  // inicia a resposta depois de acessar o diretório de cadastros
  const auto sendPageStart = [&]() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html; charset=utf-8", "");
    server.sendContent(page.c_str(), listStart);
  };
  bool hasPeople = false;
  const PeopleListResult result = visitPeople([&](const Person& person) {
    if (!hasPeople) {
      sendPageStart();
      hasPeople = true;
    }
    sendPersonRow(server, person);
  });
  if (result == PeopleListResult::unavailable) {
    server.send(500, WEB_TEXT_TYPE, "Não foi possível listar as pessoas.");
    return;
  }

  // distingue uma lista vazia de uma leitura incompleta
  if (!hasPeople) {
    sendPageStart();
  }
  if (result == PeopleListResult::incomplete) {
    server.sendContent("<li>Parte da lista de pessoas não pôde ser lida.</li>");
  } else if (!hasPeople) {
    server.sendContent("<li>Nenhuma pessoa cadastrada.</li>");
  }
  const size_t listEnd = listStart + sizeof(LIST_MARKER) - 1;
  server.sendContent(page.c_str() + listEnd, page.length() - listEnd);
  server.sendContent("");
}

void sendSaveResult(WebServer& server, PersonResult result) {
  // volta à lista após salvar ou informa a falha
  switch (result) {
    case PersonResult::saved:
      redirectWeb(server, "/");
      break;
    case PersonResult::invalidName:
      server.send(400, WEB_TEXT_TYPE, String("Informe um nome válido de até ") +
                  PERSON_NAME_MAX_BYTES + " bytes.");
      break;
    case PersonResult::notFound:
      server.send(404, WEB_TEXT_TYPE, "Pessoa não encontrada.");
      break;
    case PersonResult::storageError:
      server.send(500, WEB_TEXT_TYPE, "Não foi possível salvar a pessoa.");
      break;
  }
}
}

void registerWebPeople(WebServer& server) {
  // exige uma sessão válida para consultar ou alterar os cadastros
  server.on("/", HTTP_GET, [&server]() {
    if (requireWebSession(server)) {
      sendPeoplePage(server);
    }
  });
  server.on("/people", HTTP_POST, [&server]() {
    if (requireWebSession(server)) {
      sendSaveResult(server, createPerson(server.arg("name")));
    }
  });
  server.on("/people/rename", HTTP_POST, [&server]() {
    if (requireWebSession(server)) {
      sendSaveResult(server, renamePerson(server.arg("id"), server.arg("name")));
    }
  });
  server.on("/people/delete", HTTP_POST, [&server]() {
    if (!requireWebSession(server)) {
      return;
    }
    switch (deletePerson(server.arg("id"))) {
      case StorageResult::ok:
        redirectWeb(server, "/");
        break;
      case StorageResult::notFound:
        server.send(404, WEB_TEXT_TYPE, "Pessoa não encontrada.");
        break;
      case StorageResult::error:
        server.send(500, WEB_TEXT_TYPE, "Não foi possível concluir a exclusão. Tente novamente.");
        break;
    }
  });
}

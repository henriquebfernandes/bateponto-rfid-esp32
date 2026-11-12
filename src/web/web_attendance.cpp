#include "web_attendance.h"

#include <algorithm>
#include <array>

#include "../attendance/attendance_storage.h"
#include "../clock/local_date_time.h"
#include "../people/people_storage.h"
#include "web_auth.h"
#include "web_html.h"

namespace {
constexpr size_t RECORDS_PER_PAGE = 20;

const char* typeLabel(AttendanceType type) {
  return type == AttendanceType::entry ? "Entrada" : "Saída";
}

const char* sourceLabel(AttendanceSource source) {
  return source == AttendanceSource::manual ? "Manual" : "Leitor RFID";
}

bool loadRecord(WebServer& server, AttendanceRecord& record) {
  switch (findAttendance(server.arg("id"), record)) {
    case StorageResult::ok:
      return true;
    case StorageResult::notFound:
      server.send(404, WEB_TEXT_TYPE, "Registro não encontrado.");
      break;
    case StorageResult::error:
      server.send(500, WEB_TEXT_TYPE, "Não foi possível ler o registro.");
      break;
  }
  return false;
}

void sendAttendanceRow(WebServer& server, const AttendanceRecord& record) {
  Person person;
  const StorageResult result = findPerson(record.personId, person);
  const String name = result == StorageResult::ok ? escapeHtml(person.name) : "Pessoa indisponível";
  server.sendContent(String("<tr><td>") + name + "</td><td>" + typeLabel(record.type) +
                     "</td><td>" + formatLocalDateTime(record.timestampMs, "%d/%m/%Y %H:%M:%S") +
                     "</td><td>" + sourceLabel(record.source) +
                     "</td><td><a href=\"/records/edit?id=" + record.id + "\">Editar</a></td></tr>");
}

void sendAttendancePage(WebServer& server) {
  AttendanceRecord before;
  if (server.hasArg("before")) {
    const StorageResult result = findAttendance(server.arg("before"), before);
    if (result == StorageResult::notFound) {
      redirectWeb(server, "/records");
      return;
    }
    if (result == StorageResult::error) {
      server.send(500, WEB_TEXT_TYPE, "Não foi possível carregar a página de registros.");
      return;
    }
  }

  // mantém em memória apenas os próximos registros e um para indicar outra página
  std::array<AttendanceRecord, RECORDS_PER_PAGE + 1> records;
  size_t count = 0;
  const bool complete = visitAttendance([&](const AttendanceRecord& record) {
    if (!before.id.isEmpty() && !isMoreRecent(before, record)) {
      return;
    }
    const auto end = records.begin() + count;
    const auto position = std::lower_bound(records.begin(), end, record, isMoreRecent);
    if (position == records.end()) {
      return;
    }
    const auto last = count == records.size() ? end - 1 : end;
    std::move_backward(position, last, last + 1);
    *position = record;
    count = std::min(count + 1, records.size());
  });
  if (!complete) {
    server.send(500, WEB_TEXT_TYPE, "Não foi possível listar os registros.");
    return;
  }

  sendHtmlStart(server, "Registros de ponto");
  server.sendContent("<p><a href=\"/records/new\">Adicionar registro</a></p>");
  if (count == 0) {
    server.sendContent("<p>Nenhum registro nesta página.</p>");
  } else {
    server.sendContent("<div class=\"table-scroll\" role=\"region\" "
                       "aria-label=\"Registros de ponto\" tabindex=\"0\">"
                       "<table><thead><tr><th scope=\"col\">Pessoa</th><th scope=\"col\">Tipo</th>"
                       "<th scope=\"col\">Data e hora</th><th scope=\"col\">Fonte</th>"
                       "<th scope=\"col\">Ações</th></tr></thead><tbody>");
    for (size_t index = 0; index < std::min(count, RECORDS_PER_PAGE); ++index) {
      sendAttendanceRow(server, records[index]);
    }
    server.sendContent("</tbody></table></div>");
  }
  if (count > RECORDS_PER_PAGE) {
    server.sendContent(String("<p><a href=\"/records?before=") + records[RECORDS_PER_PAGE - 1].id +
                       "\">Ver registros anteriores</a></p>");
  }
  if (!before.id.isEmpty()) {
    server.sendContent("<p><a href=\"/records\">Voltar aos mais recentes</a></p>");
  }
  server.sendContent("<p><a href=\"/\">Voltar às pessoas</a></p>");
  sendHtmlEnd(server);
}

void sendAttendanceForm(WebServer& server, const AttendanceRecord& record) {
  const bool editing = !record.id.isEmpty();
  sendHtmlStart(server, editing ? "Editar registro" : "Adicionar registro");
  server.sendContent(editing ? "<form action=\"/records/edit\" method=\"post\">"
                             : "<form action=\"/records/new\" method=\"post\">");
  if (editing) {
    server.sendContent(String("<input type=\"hidden\" name=\"id\" value=\"") + record.id + "\">");
  }

  server.sendContent("<label>Pessoa <select name=\"person\" required>"
                     "<option value=\"\">Selecione uma pessoa</option>");
  bool hasPeople = false;
  const PeopleListResult people = visitPeople([&](const Person& person) {
    hasPeople = true;
    server.sendContent(String("<option value=\"") + person.id + "\"" +
                       (person.id == record.personId ? " selected" : "") + ">" +
                       escapeHtml(person.name) + "</option>");
  });
  server.sendContent("</select></label>");
  if (people != PeopleListResult::complete) {
    server.sendContent("<p>Não foi possível ler toda a lista de pessoas.</p>");
  } else if (!hasPeople) {
    server.sendContent("<p>Cadastre uma pessoa para adicionar um registro.</p>");
  }

  const String dateTime = editing ? formatLocalDateTime(record.timestampMs) : "";
  server.sendContent(String("<label>Tipo <select name=\"type\"><option value=\"entry\"") +
                     (record.type == AttendanceType::entry ? " selected" : "") +
                     ">Entrada</option><option value=\"exit\"" +
                     (record.type == AttendanceType::exit ? " selected" : "") +
                     ">Saída</option></select></label>"
                     "<label>Data e hora <input type=\"datetime-local\" name=\"dateTime\" "
                     "step=\"1\" value=\"" + dateTime + "\" required></label>");
  if (editing) {
    server.sendContent(String("<p>Fonte: ") + sourceLabel(record.source) + "</p>");
  }
  server.sendContent(hasPeople ? "<button type=\"submit\">Salvar</button></form>" : "</form>");
  if (editing) {
    server.sendContent(String("<form action=\"/records/delete\" method=\"post\">"
                              "<input type=\"hidden\" name=\"id\" value=\"") + record.id +
                       "\"><button class=\"danger\" type=\"submit\">Excluir registro</button></form>");
  }
  server.sendContent("<p><a href=\"/records\">Voltar aos registros</a></p>");
  sendHtmlEnd(server);
}

void saveAttendanceForm(WebServer& server, AttendanceRecord& record) {
  const String type = server.arg("type");
  int64_t timestampMs;
  if ((type != "entry" && type != "exit") ||
      !parseLocalDateTime(server.arg("dateTime"), timestampMs)) {
    server.send(400, WEB_TEXT_TYPE, "Informe um tipo, uma data e um horário válidos.");
    return;
  }

  Person person;
  const StorageResult result = findPerson(server.arg("person"), person);
  if (result != StorageResult::ok) {
    if (result == StorageResult::notFound) {
      server.send(400, WEB_TEXT_TYPE, "Selecione uma pessoa cadastrada.");
    } else {
      server.send(500, WEB_TEXT_TYPE, "Não foi possível consultar a pessoa.");
    }
    return;
  }

  record.personId = person.id;
  record.type = type == "entry" ? AttendanceType::entry : AttendanceType::exit;
  // preserva os milissegundos do rfid se a data e a hora continuarem iguais
  if (record.timestampMs / 1000 * 1000 != timestampMs) {
    record.timestampMs = timestampMs;
  }
  const bool saved = record.id.isEmpty() ? createAttendance(record) : updateAttendance(record);
  if (saved) {
    redirectWeb(server, "/records");
  } else {
    server.send(500, WEB_TEXT_TYPE, "Não foi possível salvar o registro.");
  }
}
}

void registerWebAttendance(WebServer& server) {
  server.on("/records", HTTP_GET, [&server]() {
    if (requireWebSession(server)) {
      sendAttendancePage(server);
    }
  });
  server.on("/records/new", HTTP_GET, [&server]() {
    if (requireWebSession(server)) {
      sendAttendanceForm(server, AttendanceRecord{});
    }
  });
  server.on("/records/new", HTTP_POST, [&server]() {
    if (requireWebSession(server)) {
      AttendanceRecord record;
      saveAttendanceForm(server, record);
    }
  });
  server.on("/records/edit", HTTP_GET, [&server]() {
    AttendanceRecord record;
    if (requireWebSession(server) && loadRecord(server, record)) {
      sendAttendanceForm(server, record);
    }
  });
  server.on("/records/edit", HTTP_POST, [&server]() {
    AttendanceRecord record;
    if (requireWebSession(server) && loadRecord(server, record)) {
      saveAttendanceForm(server, record);
    }
  });
  server.on("/records/delete", HTTP_POST, [&server]() {
    if (!requireWebSession(server)) {
      return;
    }
    switch (removeAttendance(server.arg("id"))) {
      case StorageResult::ok:
        redirectWeb(server, "/records");
        break;
      case StorageResult::notFound:
        server.send(404, WEB_TEXT_TYPE, "Registro não encontrado.");
        break;
      case StorageResult::error:
        server.send(500, WEB_TEXT_TYPE, "Não foi possível excluir o registro.");
        break;
    }
  });
}

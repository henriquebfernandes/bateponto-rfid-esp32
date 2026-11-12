#include "web_server.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>

#include "config.h"
#include "web_attendance.h"
#include "web_auth.h"
#include "web_cards.h"
#include "web_people.h"

// delimita a folha de estilos incorporada ao firmware
extern const char stylesheetStart[] asm("_binary_data_styles_css_start");
extern const char stylesheetEnd[] asm("_binary_data_styles_css_end");

namespace {
constexpr uint16_t HTTP_PORT = 80;
WebServer server(HTTP_PORT);
}

bool beginWebServer() {
  // publica o nome local e anuncia o serviço HTTP
  if (!MDNS.begin(MDNS_HOSTNAME) || !MDNS.addService("http", "tcp", HTTP_PORT)) {
    Serial.println("Falha ao publicar o servidor web por mDNS.");
    return false;
  }

  // registra as rotas antes de iniciar o servidor
  server.on("/styles.css", HTTP_GET, []() {
    const size_t size = reinterpret_cast<uintptr_t>(stylesheetEnd) -
                        reinterpret_cast<uintptr_t>(stylesheetStart);
    server.send_P(200, "text/css; charset=utf-8", stylesheetStart, size);
  });
  registerWebLogin(server);
  registerWebPeople(server);
  registerWebCards(server);
  registerWebAttendance(server);
  server.begin();

  Serial.printf("Página web disponível em http://%s.local/\n", MDNS_HOSTNAME);
  return true;
}

void updateWebServer() {
  server.handleClient();
}

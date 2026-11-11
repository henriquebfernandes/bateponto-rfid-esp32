#include "web_server.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>

#include "config.h"
#include "web_auth.h"

extern const char homePageStart[] asm("_binary_data_index_html_start");
extern const char homePageEnd[] asm("_binary_data_index_html_end");

namespace {
constexpr uint16_t HTTP_PORT = 80;
WebServer server(HTTP_PORT);
}

bool beginWebServer() {
  if (!MDNS.begin(MDNS_HOSTNAME) || !MDNS.addService("http", "tcp", HTTP_PORT)) {
    Serial.println("Falha ao publicar o servidor web por mDNS.");
    return false;
  }

  registerWebLogin(server);
  server.on("/", HTTP_GET, []() {
    if (!requireWebSession(server)) {
      return;
    }
    const size_t pageSize = reinterpret_cast<uintptr_t>(homePageEnd) -
                            reinterpret_cast<uintptr_t>(homePageStart);
    server.send_P(200, "text/html; charset=utf-8", homePageStart, pageSize);
  });
  server.begin();

  Serial.printf("Página web disponível em http://%s.local/\n", MDNS_HOSTNAME);
  return true;
}

void updateWebServer() {
  server.handleClient();
}

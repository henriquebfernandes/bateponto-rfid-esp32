#include "web_auth.h"

#include <WebServer.h>
#include <esp_random.h>
#include <esp_timer.h>

#include "config.h"

extern const char loginPageStart[] asm("_binary_data_login_html_start");
extern const char loginPageEnd[] asm("_binary_data_login_html_end");

namespace {
constexpr size_t TOKEN_BYTES = 16;
constexpr char SESSION_COOKIE[] = "bateponto_session=";
char sessionToken[2 * TOKEN_BYTES + 1] = {};
int64_t sessionExpiresAtUs = 0;

static_assert(sizeof(WEB_PASSWORD) > 1, "Configure uma senha para a página web.");

void startSession(WebServer& server) {
  constexpr char HEX_DIGITS[] = "0123456789abcdef";
  uint8_t token[TOKEN_BYTES];
  esp_fill_random(token, sizeof(token));
  for (size_t i = 0; i < sizeof(token); ++i) {
    sessionToken[2 * i] = HEX_DIGITS[token[i] >> 4];
    sessionToken[2 * i + 1] = HEX_DIGITS[token[i] & 0x0F];
  }
  sessionExpiresAtUs = esp_timer_get_time() + WEB_SESSION_TIMEOUT_MS * 1000LL;

  server.sendHeader("Set-Cookie", String(SESSION_COOKIE) + sessionToken +
                                     "; Path=/; HttpOnly; SameSite=Lax");
}

bool hasValidSession(WebServer& server) {
  if (esp_timer_get_time() >= sessionExpiresAtUs) {
    return false;
  }

  const String expectedCookie = String(SESSION_COOKIE) + sessionToken;
  const String cookies = server.header("Cookie");
  const int length = cookies.length();
  for (int start = 0; start < length;) {
    int end = cookies.indexOf(';', start);
    if (end < 0) {
      end = length;
    }
    String cookie = cookies.substring(start, end);
    cookie.trim();
    if (cookie == expectedCookie) {
      return true;
    }
    start = end + 1;
  }
  return false;
}

void redirectTo(WebServer& server, const char* path) {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", path);
  server.send(303);
}

void sendLoginPage(WebServer& server, bool invalidPassword) {
  const size_t pageSize = reinterpret_cast<uintptr_t>(loginPageEnd) -
                          reinterpret_cast<uintptr_t>(loginPageStart);
  server.sendHeader("Cache-Control", "no-store");
  if (invalidPassword) {
    String page(loginPageStart, pageSize);
    page.replace("<!-- mensagem de login -->", "<p>Senha incorreta.</p>");
    server.send(401, "text/html; charset=utf-8", page);
    return;
  }
  server.send_P(200, "text/html; charset=utf-8", loginPageStart, pageSize);
}
}

void registerWebLogin(WebServer& server) {
  const char* headers[] = {"Cookie"};
  server.collectHeaders(headers, 1);

  server.on("/login", HTTP_GET, [&server]() {
    sendLoginPage(server, false);
  });
  server.on("/login", HTTP_POST, [&server]() {
    if (server.arg("password") != WEB_PASSWORD) {
      sendLoginPage(server, true);
      return;
    }
    startSession(server);
    redirectTo(server, "/");
  });
}

bool requireWebSession(WebServer& server) {
  if (!hasValidSession(server)) {
    redirectTo(server, "/login");
    return false;
  }
  server.sendHeader("Cache-Control", "no-store");
  return true;
}

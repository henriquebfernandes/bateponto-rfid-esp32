#include "web_html.h"

String escapeHtml(String value) {
  // preserva os caracteres especiais do texto nos campos e na página
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

void redirectWeb(WebServer& server, const String& path) {
  server.sendHeader("Location", path);
  server.send(303);
}

void sendHtmlStart(WebServer& server, const char* title, bool refresh) {
  String page = "<!doctype html><html lang=\"pt-BR\"><head><meta charset=\"utf-8\">"
                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  if (refresh) {
    page += "<meta http-equiv=\"refresh\" content=\"1\">";
  }
  page += String("<title>") + title + "</title></head><body><h1>" + title + "</h1>";
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent(page);
}

void sendHtmlEnd(WebServer& server) {
  server.sendContent("</body></html>");
  server.sendContent("");
}

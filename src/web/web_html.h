#pragma once

#include <WebServer.h>

constexpr char WEB_TEXT_TYPE[] = "text/plain; charset=utf-8";

String escapeHtml(String value);
void redirectWeb(WebServer& server, const String& path);
void sendHtmlStart(WebServer& server, const char* title, bool refresh = false);
void sendHtmlEnd(WebServer& server);

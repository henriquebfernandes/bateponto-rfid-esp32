#pragma once

class WebServer;

void registerWebLogin(WebServer& server);
bool requireWebSession(WebServer& server);

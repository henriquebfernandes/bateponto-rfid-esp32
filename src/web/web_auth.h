#pragma once

class WebServer;

void registerWebLogin(WebServer& server);
// redireciona para o login e retorna falso se a sessão for inválida
bool requireWebSession(WebServer& server);
